/**
 * @file  matter_bridge.cpp
 * @brief The Matter bridge. See matter_bridge.h.
 *
 * C++ because esp-matter is; kept to the project's naming, declaration and
 * single-exit rules where the language allows. All value mapping lives in
 * the pure, host-tested bridge_map.c.
 */
#include "matter_bridge.h"

extern "C"
{
#include "bridge_map.h"
#include "control.h"
#include "hvac_logic.h"
#include "settings.h"
}

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_matter.h>
#include <esp_matter_console.h>
#include <esp_matter_data_model_provider.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <app/clusters/boolean-state-server/BooleanStateCluster.h>
#include <app/clusters/fan-control-server/CodegenIntegration.h>
#include <app/server/Server.h>
#include <setup_payload/OnboardingCodesUtil.h>
#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include <platform/ESP32/OpenthreadLauncher.h>
#endif

#define LOG_TAG "matter_bridge"

#define SYNC_PERIOD_MS 1000U
#define SYNC_STACK     4096U
#define SYNC_PRIORITY  4U
#define US_PER_MS      1000

#define PERCENT_FULL 100U
#define PERCENT_MAX  100U

// One bit per reported value, for logging a failing report once.
#define FIELD_TEMP    (1UL << 0)
#define FIELD_MODE    (1UL << 1)
#define FIELD_HEAT    (1UL << 2)
#define FIELD_COOL    (1UL << 3)
#define FIELD_RUNNING (1UL << 4)
#define FIELD_FAN     (1UL << 5)
#define FIELD_EHEAT   (1UL << 6)
#define FIELD_LIMITS  (1UL << 7)
#define FIELD_FAULT   (1UL << 8)
#define FIELD_BLOWER  (1UL << 9)

using namespace esp_matter;
using namespace chip::app::Clusters;

namespace
{

// What was last reported, so the sync task sends only changes.
typedef struct
{
    bool     b_valid; // false until the first report
    bool     b_temp_valid;
    int16_t  temp_c100;
    uint8_t  system_mode;
    int16_t  heat_c100;
    int16_t  cool_c100;
    uint16_t running;
    uint8_t  fan_mode;
    uint8_t  fan_percent;
    uint8_t  blower_percent; // PercentCurrent: the blower really running
    bool     b_eheat;
    bool     b_fault;
} bridge_reported_t;

uint16_t          g_thermostat_ep = 0U;
uint16_t          g_fan_ep        = 0U;
uint16_t          g_eheat_ep      = 0U;
uint16_t          g_fault_ep      = 0U;
bridge_reported_t g_reported      = {}; // the sync task's only
uint32_t          g_fail_logged   = 0U; // FIELD_ bits; the sync task's only
TaskHandle_t      g_h_sync        = nullptr;

// Set by a write from a controller: the sync task then reports every value
// again, so a write the thermostat snapped or ignored (65.1 F stored as 65,
// FanMode Off meaning Auto) does not leave Matter showing what was written.
std::atomic<bool> g_b_resync{ false };

// The mode e-heat was entered from, so switching e-heat off returns to it.
// Written only inside settings_update() edits, so it is serialised with
// every mode change. hvac_mode_t values fit uint8_t.
std::atomic<uint8_t> g_restore_mode{ (uint8_t)HVAC_MODE_HEAT };

// False until esp_matter::start() returns. Until then esp-matter is running
// the clusters' start-up code (the On/Off cluster's StartUpOnOff rule writes
// OnOff, which would switch e-heat off on every reboot) and our own boot-time
// attribute writes: none of it is a controller's request, and at boot the
// settings record is the only truth. The first bridge_sync() reports every
// value, overwriting whatever start-up wrote into Matter.
std::atomic<bool> g_b_live{ false };

uint64_t bridge_now_ms(void)
{
    // esp_timer counts up from 0 at boot, so the quotient is non-negative.
    return (uint64_t)(esp_timer_get_time() / US_PER_MS);
}

// One accepted Matter write, applied to the settings under their lock by
// settings_update(). The edit functions below run with that lock held, so
// they only compute: no logging, blocking or other locks.
typedef struct
{
    uint16_t                      endpoint_id;
    uint32_t                      cluster_id;
    uint32_t                      attribute_id;
    esp_matter_attr_val_t const * p_val;
    bool                          b_ours; // out: a thermostat-side attribute
} bridge_edit_t;

// Thermostat cluster writes.
void bridge_thermostat_write(uint32_t                      attribute_id,
                             esp_matter_attr_val_t const * p_val,
                             settings_t *                  p_cfg)
{
    bool    b_celsius = ((uint8_t)SETTINGS_UNITS_C == p_cfg->units); // 1 fits
    uint8_t mode      = p_cfg->mode;

    switch (attribute_id)
    {
        case Thermostat::Attributes::SystemMode::Id:
            // PRE_UPDATE already refused a mode the thermostat cannot run.
            if (bridge_map_mode_from_system(p_val->val.u8, p_cfg->mode, &mode))
            {
                p_cfg->mode = mode;
            }
            break;
        case Thermostat::Attributes::OccupiedHeatingSetpoint::Id:
            bridge_map_setpoint_write(p_val->val.i16, b_celsius, true,
                                      &p_cfg->heat_sp_f10, &p_cfg->cool_sp_f10);
            break;
        case Thermostat::Attributes::OccupiedCoolingSetpoint::Id:
            bridge_map_setpoint_write(p_val->val.i16, b_celsius, false,
                                      &p_cfg->heat_sp_f10, &p_cfg->cool_sp_f10);
            break;
        default:
            // Read-only or not ours: let Matter handle it.
            break;
    }
}

void bridge_fan_write(uint32_t                      attribute_id,
                      esp_matter_attr_val_t const * p_val, settings_t * p_cfg)
{
    if (FanControl::Attributes::FanMode::Id == attribute_id)
    {
        p_cfg->fan = bridge_map_fan_from_mode(p_val->val.u8);
    }
    else if ((FanControl::Attributes::PercentSetting::Id == attribute_id) &&
             (p_val->val.u8 <= PERCENT_MAX)) // above 100 is null
    {
        // A speed slider: any speed is Fan On, zero is back to Auto.
        p_cfg->fan = (0U == p_val->val.u8) ? (uint8_t)HVAC_FAN_AUTO
                                           : (uint8_t)HVAC_FAN_ON; // 0, 1 fit
    }
    else
    {
        // Not ours.
    }
}

// The settings_update() edit for one Matter write.
bool bridge_edit(settings_t * p_cfg, void * p_ctx)
{
    bridge_edit_t * p_edit = static_cast<bridge_edit_t *>(p_ctx);

    if ((g_thermostat_ep == p_edit->endpoint_id) &&
        (Thermostat::Id == p_edit->cluster_id))
    {
        bridge_thermostat_write(p_edit->attribute_id, p_edit->p_val, p_cfg);
        p_edit->b_ours = true;
    }
    else if ((g_fan_ep == p_edit->endpoint_id) &&
             (FanControl::Id == p_edit->cluster_id))
    {
        bridge_fan_write(p_edit->attribute_id, p_edit->p_val, p_cfg);
        p_edit->b_ours = true;
    }
    else if ((g_eheat_ep == p_edit->endpoint_id) &&
             (OnOff::Id == p_edit->cluster_id) &&
             (OnOff::Attributes::OnOff::Id == p_edit->attribute_id))
    {
        if ((uint8_t)HVAC_MODE_EHEAT != p_cfg->mode) // 4 fits
        {
            g_restore_mode.store(p_cfg->mode);
        }
        p_cfg->mode = bridge_map_mode_from_eheat(
            p_edit->p_val->val.b, p_cfg->mode, g_restore_mode.load());
        p_edit->b_ours = true;
    }
    else
    {
        // Root node, identify, descriptors: nothing for the thermostat.
    }

    return p_edit->b_ours;
}

// Attribute writes arrive here twice: before the cluster validates and
// stores them (PRE_UPDATE) and after (POST_UPDATE). Only POST_UPDATE changes
// the thermostat, so it never acts on a value Matter then refuses (a
// setpoint outside the cluster's limits, say); PRE_UPDATE only refuses
// system modes the thermostat cannot run. An accepted write is one
// settings_update(), so it cannot race a D-pad edit; the control loop takes
// it from there.
//
// esp-matter also runs this for the sync task's own reports of writable
// attributes (mode, setpoints, fan, e-heat): a report goes through the same
// write path. Those are the thermostat's own state echoing back, possibly
// already stale, so they are ignored here; so is everything before
// esp_matter::start() returns (g_b_live). Attributes the thermostat fixes
// itself (bridge_map_attr_fixed: limits, deadband, control sequence,
// StartUpOnOff) are refused, so no controller can change what it accepts.
esp_err_t bridge_attribute_cb(attribute::callback_type_t type,
                              uint16_t endpoint_id, uint32_t cluster_id,
                              uint32_t                attribute_id,
                              esp_matter_attr_val_t * p_val, void * p_priv)
{
    esp_err_t     err       = ESP_OK;
    settings_t    cfg       = {};
    uint8_t       mode      = 0U;
    bool          b_changed = false;
    bridge_edit_t edit      = {};

    (void)p_priv;

    if ((nullptr == p_val) || !g_b_live.load() ||
        ((nullptr != g_h_sync) && (xTaskGetCurrentTaskHandle() == g_h_sync)))
    {
        goto done;
    }

    if (attribute::PRE_UPDATE == type)
    {
        settings_get(&cfg);
        if (((g_thermostat_ep == endpoint_id) || (g_eheat_ep == endpoint_id)) &&
            bridge_map_attr_fixed(cluster_id, attribute_id))
        {
            err = ESP_ERR_NOT_SUPPORTED; // refuses the write
        }
        else if ((g_thermostat_ep == endpoint_id) &&
                 (Thermostat::Id == cluster_id) &&
                 (Thermostat::Attributes::SystemMode::Id == attribute_id) &&
                 !bridge_map_mode_from_system(p_val->val.u8, cfg.mode, &mode))
        {
            err = ESP_ERR_NOT_SUPPORTED; // refuses the write
        }
        else
        {
            // Accepted; acted on at POST_UPDATE.
        }
        goto done;
    }
    if (attribute::POST_UPDATE != type)
    {
        goto done;
    }

    edit.endpoint_id  = endpoint_id;
    edit.cluster_id   = cluster_id;
    edit.attribute_id = attribute_id;
    edit.p_val        = p_val;
    b_changed         = settings_update(bridge_edit, &edit, bridge_now_ms());
    if (edit.b_ours)
    {
        g_b_resync.store(true);
    }

    settings_get(&cfg);
    if ((g_thermostat_ep == endpoint_id) && (Thermostat::Id == cluster_id) &&
        ((Thermostat::Attributes::OccupiedHeatingSetpoint::Id ==
          attribute_id) ||
         (Thermostat::Attributes::OccupiedCoolingSetpoint::Id == attribute_id)))
    {
        ESP_LOGI(LOG_TAG,
                 "setpoint written: %d (0.01 C) -> heat %d cool %d (0.1 F)",
                 p_val->val.i16, cfg.heat_sp_f10, cfg.cool_sp_f10);
    }
    if (b_changed)
    {
        ESP_LOGI(LOG_TAG, "from Matter: mode %u fan %u heat %d cool %d",
                 cfg.mode, cfg.fan, cfg.heat_sp_f10, cfg.cool_sp_f10);
    }

done:
    return err;
}

esp_err_t bridge_identify_cb(identification::callback_type_t type,
                             uint16_t endpoint_id, uint8_t effect_id,
                             uint8_t effect_variant, void * p_priv)
{
    (void)effect_variant;
    (void)p_priv;
    ESP_LOGI(LOG_TAG, "identify: type %u endpoint %u effect %u", type,
             endpoint_id, effect_id);
    return ESP_OK;
}

void bridge_event_cb(ChipDeviceEvent const * p_event, intptr_t arg)
{
    (void)arg;

    switch (p_event->Type)
    {
        case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
            ESP_LOGI(LOG_TAG, "commissioning complete");
            break;
        case chip::DeviceLayer::DeviceEventType::kFailSafeTimerExpired:
            ESP_LOGW(LOG_TAG, "commissioning failed: fail-safe expired");
            break;
        case chip::DeviceLayer::DeviceEventType::kInterfaceIpAddressChanged:
            ESP_LOGI(LOG_TAG, "IP address changed");
            break;
        default:
            break;
    }
}

// Creates a setpoint limit, then sets it. The operating limits are
// non-volatile: Matter restores a value saved by older firmware over the one
// passed to create(), so the set is what makes a changed range take effect.
esp_err_t bridge_set_limit(cluster_t * p_cl, uint32_t attribute_id,
                           int16_t value,
                           attribute_t * (*p_create)(cluster_t *, int16_t))
{
    esp_err_t             err    = ESP_FAIL;
    attribute_t *         p_attr = attribute::get(p_cl, attribute_id);
    esp_matter_attr_val_t val    = esp_matter_int16(value);

    if (nullptr == p_attr)
    {
        p_attr = p_create(p_cl, value);
    }
    if (nullptr != p_attr)
    {
        err = attribute::set_val(p_attr, &val, false);
    }

    return err;
}

// Sets an existing attribute at boot (before Matter starts, so the boot gate
// keeps the write away from the thermostat). For non-volatile attributes the
// thermostat fixes itself, so a value saved by older firmware, or written by
// a controller before writes were refused, does not survive the update.
void bridge_set_fixed(cluster_t * p_cl, uint32_t attribute_id,
                      esp_matter_attr_val_t val)
{
    attribute_t * p_attr = attribute::get(p_cl, attribute_id);

    if ((nullptr == p_attr) ||
        (ESP_OK != attribute::set_val(p_attr, &val, false)))
    {
        ESP_LOGW(LOG_TAG, "boot set of 0x%lx failed",
                 (unsigned long)attribute_id);
    }
}

endpoint_t * bridge_create_thermostat(node_t * p_node)
{
    endpoint::thermostat::config_t cfg        = {};
    cluster_t *                    p_cl       = nullptr;
    endpoint_t *                   p_ep       = nullptr;
    bridge_limits_t                lim        = {};
    uint32_t                       pass       = 0U;
    unsigned                       limit_errs = 0U;

    cfg.thermostat.control_sequence_of_operation =
        BRIDGE_CONTROL_SEQUENCE_HEAT_COOL;
    cfg.thermostat.system_mode       = BRIDGE_SYSTEM_MODE_OFF;
    cfg.thermostat.local_temperature = nullable<int16_t>();
    cfg.thermostat.feature_flags =
        cluster::thermostat::feature::heating::get_id() |
        cluster::thermostat::feature::cooling::get_id() |
        cluster::thermostat::feature::auto_mode::get_id();
    cfg.thermostat.features.heating.occupied_heating_setpoint =
        bridge_map_f10_to_c100(680);
    cfg.thermostat.features.cooling.occupied_cooling_setpoint =
        bridge_map_f10_to_c100(760);
    cfg.thermostat.features.auto_mode.min_setpoint_dead_band =
        BRIDGE_DEADBAND_C10;

    p_ep =
        endpoint::thermostat::create(p_node, &cfg, ENDPOINT_FLAG_NONE, nullptr);
    if (nullptr == p_ep)
    {
        goto done;
    }

    // The cluster checks writes against both the absolute and the
    // operating limits, and uses Matter's defaults (cool 16..32 C, heat
    // 7..30 C) for any it is not given. All eight come from the thermostat's
    // own ranges, so the Home app offers exactly what it accepts.
    p_cl = cluster::get(p_ep, Thermostat::Id);
    bridge_map_limits(&lim);
    bridge_set_limit(
        p_cl, Thermostat::Attributes::AbsMinHeatSetpointLimit::Id,
        lim.min_heat_c100,
        cluster::thermostat::attribute::create_abs_min_heat_setpoint_limit);
    bridge_set_limit(
        p_cl, Thermostat::Attributes::AbsMaxHeatSetpointLimit::Id,
        lim.max_heat_c100,
        cluster::thermostat::attribute::create_abs_max_heat_setpoint_limit);
    bridge_set_limit(
        p_cl, Thermostat::Attributes::AbsMinCoolSetpointLimit::Id,
        lim.min_cool_c100,
        cluster::thermostat::attribute::create_abs_min_cool_setpoint_limit);
    bridge_set_limit(
        p_cl, Thermostat::Attributes::AbsMaxCoolSetpointLimit::Id,
        lim.max_cool_c100,
        cluster::thermostat::attribute::create_abs_max_cool_setpoint_limit);
    // The operating limits are cross-checked against each other and the
    // deadband, so a write can be refused against an old neighbour that a
    // later write in the same pass moves. Two passes settle any order; only
    // the second pass's failures are real.
    for (pass = 0U; pass < 2U; pass++)
    {
        limit_errs = 0U;
        limit_errs +=
            (ESP_OK !=
             bridge_set_limit(p_cl,
                              Thermostat::Attributes::MinHeatSetpointLimit::Id,
                              lim.min_heat_c100,
                              cluster::thermostat::attribute::
                                  create_min_heat_setpoint_limit))
                ? 1U
                : 0U;
        limit_errs +=
            (ESP_OK !=
             bridge_set_limit(p_cl,
                              Thermostat::Attributes::MaxHeatSetpointLimit::Id,
                              lim.max_heat_c100,
                              cluster::thermostat::attribute::
                                  create_max_heat_setpoint_limit))
                ? 1U
                : 0U;
        limit_errs +=
            (ESP_OK !=
             bridge_set_limit(p_cl,
                              Thermostat::Attributes::MinCoolSetpointLimit::Id,
                              lim.min_cool_c100,
                              cluster::thermostat::attribute::
                                  create_min_cool_setpoint_limit))
                ? 1U
                : 0U;
        limit_errs +=
            (ESP_OK !=
             bridge_set_limit(p_cl,
                              Thermostat::Attributes::MaxCoolSetpointLimit::Id,
                              lim.max_cool_c100,
                              cluster::thermostat::attribute::
                                  create_max_cool_setpoint_limit))
                ? 1U
                : 0U;
    }
    if (0U != limit_errs)
    {
        ESP_LOGW(LOG_TAG, "%u setpoint limit(s) could not be set", limit_errs);
    }
    (void)cluster::thermostat::attribute::create_thermostat_running_state(p_cl,
                                                                          0U);

    // Non-volatile and fixed by the thermostat: set every boot.
    bridge_set_fixed(p_cl, Thermostat::Attributes::MinSetpointDeadBand::Id,
                     esp_matter_int8(BRIDGE_DEADBAND_C10));
    bridge_set_fixed(p_cl,
                     Thermostat::Attributes::ControlSequenceOfOperation::Id,
                     esp_matter_enum8(BRIDGE_CONTROL_SEQUENCE_HEAT_COOL));

done:
    return p_ep;
}

endpoint_t * bridge_create_fan(node_t * p_node)
{
    endpoint::fan::config_t cfg  = {};
    endpoint_t *            p_ep = nullptr;

    cfg.fan_control.fan_mode          = BRIDGE_FAN_MODE_AUTO;
    cfg.fan_control.fan_mode_sequence = BRIDGE_FAN_SEQ_OFF_HIGH_AUTO;
    cfg.fan_control.percent_setting   = nullable<uint8_t>(0U);
    cfg.fan_control.percent_current   = 0U;

    p_ep = endpoint::fan::create(p_node, &cfg, ENDPOINT_FLAG_NONE, nullptr);
    if (nullptr != p_ep)
    {
        (void)cluster::fan_control::feature::fan_auto::add(
            cluster::get(p_ep, FanControl::Id));
    }

    return p_ep;
}

endpoint_t * bridge_create_eheat(node_t * p_node)
{
    endpoint::on_off_plug_in_unit::config_t cfg  = {};
    endpoint_t *                            p_ep = nullptr;

    cfg.on_off.on_off = false;
    // StartUpOnOff null: Matter leaves OnOff as it was across a reboot. Its
    // default (Off) would switch e-heat off at every boot (the boot gate
    // stops that reaching the thermostat; this stops Home seeing it).
    cfg.on_off_lighting.start_up_on_off = nullable<uint8_t>();

    p_ep = endpoint::on_off_plug_in_unit::create(p_node, &cfg,
                                                 ENDPOINT_FLAG_NONE, nullptr);
    if (nullptr != p_ep)
    {
        // Non-volatile: units that stored the old default (Off) keep it
        // unless it is set here.
        bridge_set_fixed(cluster::get(p_ep, OnOff::Id),
                         OnOff::Attributes::StartUpOnOff::Id,
                         esp_matter_nullable_enum8(nullable<uint8_t>()));
    }

    return p_ep;
}

// "Thermostat fault": a contact sensor that reads open while the thermostat
// cannot run (no valid temperature), so Apple Home can notify on it. Matter's
// BooleanState for a contact sensor is true when the contact is closed.
endpoint_t * bridge_create_fault(node_t * p_node)
{
    endpoint::contact_sensor::config_t cfg = {};

    cfg.boolean_state.state_value = true; // closed: no fault

    return endpoint::contact_sensor::create(p_node, &cfg, ENDPOINT_FLAG_NONE,
                                            nullptr);
}

// Reports one thermostat-side value. A failure is logged once per field
// until that field reports again, not every second. Returns true on success.
bool bridge_report(uint16_t endpoint_id, uint32_t cluster_id,
                   uint32_t attribute_id, esp_matter_attr_val_t val,
                   uint32_t field)
{
    bool b_ok = (ESP_OK == attribute::report(endpoint_id, cluster_id,
                                             attribute_id, &val));

    if (b_ok)
    {
        g_fail_logged &= ~field;
    }
    else if (0U == (g_fail_logged & field))
    {
        ESP_LOGW(LOG_TAG, "report %u/0x%lx/0x%lx failed", endpoint_id,
                 (unsigned long)cluster_id, (unsigned long)attribute_id);
        g_fail_logged |= field;
    }
    else
    {
        // Already logged.
    }

    return b_ok;
}

// settings_update() edit run by the sync task: notes the mode e-heat would
// return to, under the same lock as every mode change. Changes nothing.
bool bridge_note_restore(settings_t * p_cfg, void * p_ctx)
{
    (void)p_ctx;
    if ((uint8_t)HVAC_MODE_EHEAT != p_cfg->mode) // 4 fits
    {
        g_restore_mode.store(p_cfg->mode);
    }
    return false;
}

// Fan Control and Boolean State are code-driven clusters in esp-matter v1.6:
// their values are the cluster objects' own state, which an attribute
// report cannot reach, so they are set on the clusters themselves. Each
// returns true when the cluster was found (the value is then set, whether
// or not it changed).

// PercentCurrent, from the blower really running.
bool bridge_set_blower(uint8_t percent)
{
    bool                b_ok  = false;
    FanControlCluster * p_fan = nullptr;

    lock::ScopedChipStackLock stack_lock(portMAX_DELAY);
    p_fan = FanControl::FindClusterOnEndpoint(g_fan_ep);
    if (nullptr != p_fan)
    {
        (void)p_fan->SetPercentCurrent(percent); // false only if unchanged
        b_ok = true;
    }

    return b_ok;
}

// The fault contact sensor: closed (true) while the thermostat can run.
bool bridge_set_fault_contact(bool b_closed)
{
    bool                  b_ok    = false;
    BooleanStateCluster * p_state = nullptr;

    lock::ScopedChipStackLock stack_lock(portMAX_DELAY);
    // esp-matter v1.6 keeps its BooleanState clusters private and offers no
    // lookup, but registers each with the data model provider, so it is
    // found there. Its integration registers a BooleanStateCluster, so the
    // downcast is to the type actually registered.
    p_state = static_cast<BooleanStateCluster *>(
        data_model::provider::get_instance().registry().Get(
            chip::app::ConcreteClusterPath(g_fault_ep, BooleanState::Id)));
    if (nullptr != p_state)
    {
        (void)p_state->SetStateValue(b_closed); // an event only on a change
        b_ok = true;
    }

    return b_ok;
}

// Pushes the thermostat's state to Matter, sending only what changed. A
// value counts as reported only once its report succeeded, so a failed one
// is retried next second.
void bridge_sync(void)
{
    settings_t          cfg    = {};
    control_status_t    status = {};
    bridge_reported_t   now    = {};
    bridge_reported_t * p_rep  = &g_reported;
    bool                b_all  = false;
    bool                b_ok   = true;

    b_all = g_b_resync.exchange(false) || !p_rep->b_valid;
    settings_get(&cfg);
    control_get(&status);

    // E-heat set from the D-pad returns to the mode it was set from too.
    (void)settings_update(bridge_note_restore, nullptr, bridge_now_ms());

    now.b_temp_valid = status.sensor.b_valid;
    now.temp_c100    = bridge_map_f10_to_c100(status.sensor.temp_f10);
    now.system_mode  = bridge_map_system_mode(cfg.mode);
    now.heat_c100    = bridge_map_f10_to_c100(cfg.heat_sp_f10);
    now.cool_c100    = bridge_map_f10_to_c100(cfg.cool_sp_f10);
    now.running =
        bridge_map_running_state(status.applied.b_y1, status.applied.b_g,
                                 status.applied.b_o, status.applied.b_w);
    now.fan_mode       = bridge_map_fan_mode(cfg.fan);
    now.fan_percent    = ((uint8_t)HVAC_FAN_ON == cfg.fan) ? PERCENT_FULL : 0U;
    now.b_eheat        = ((uint8_t)HVAC_MODE_EHEAT == cfg.mode);
    now.blower_percent = status.applied.b_g ? PERCENT_FULL : 0U;
    now.b_fault        = !status.sensor.b_valid;

    if (b_all || (now.b_temp_valid != p_rep->b_temp_valid) ||
        (now.temp_c100 != p_rep->temp_c100))
    {
        if (bridge_report(
                g_thermostat_ep, Thermostat::Id,
                Thermostat::Attributes::LocalTemperature::Id,
                esp_matter_nullable_int16(now.b_temp_valid
                                              ? nullable<int16_t>(now.temp_c100)
                                              : nullable<int16_t>()),
                FIELD_TEMP))
        {
            p_rep->b_temp_valid = now.b_temp_valid;
            p_rep->temp_c100    = now.temp_c100;
        }
        else
        {
            b_ok = false;
        }
    }
    if (b_all || (now.system_mode != p_rep->system_mode))
    {
        if (bridge_report(g_thermostat_ep, Thermostat::Id,
                          Thermostat::Attributes::SystemMode::Id,
                          esp_matter_enum8(now.system_mode), FIELD_MODE))
        {
            p_rep->system_mode = now.system_mode;
        }
        else
        {
            b_ok = false;
        }
    }
    if (b_all || (now.heat_c100 != p_rep->heat_c100))
    {
        if (bridge_report(g_thermostat_ep, Thermostat::Id,
                          Thermostat::Attributes::OccupiedHeatingSetpoint::Id,
                          esp_matter_int16(now.heat_c100), FIELD_HEAT))
        {
            p_rep->heat_c100 = now.heat_c100;
        }
        else
        {
            b_ok = false;
        }
    }
    if (b_all || (now.cool_c100 != p_rep->cool_c100))
    {
        if (bridge_report(g_thermostat_ep, Thermostat::Id,
                          Thermostat::Attributes::OccupiedCoolingSetpoint::Id,
                          esp_matter_int16(now.cool_c100), FIELD_COOL))
        {
            p_rep->cool_c100 = now.cool_c100;
        }
        else
        {
            b_ok = false;
        }
    }
    if (b_all || (now.running != p_rep->running))
    {
        if (bridge_report(g_thermostat_ep, Thermostat::Id,
                          Thermostat::Attributes::ThermostatRunningState::Id,
                          esp_matter_bitmap16(now.running), FIELD_RUNNING))
        {
            p_rep->running = now.running;
        }
        else
        {
            b_ok = false;
        }
    }
    if (b_all || (now.fan_mode != p_rep->fan_mode) ||
        (now.fan_percent != p_rep->fan_percent))
    {
        // The speed first, then the mode: the fan cluster turns a speed of 0
        // into FanMode Off, so the mode has to land last to stay Auto.
        if (bridge_report(
                g_fan_ep, FanControl::Id,
                FanControl::Attributes::PercentSetting::Id,
                esp_matter_nullable_uint8(nullable<uint8_t>(now.fan_percent)),
                FIELD_FAN) &&
            bridge_report(g_fan_ep, FanControl::Id,
                          FanControl::Attributes::FanMode::Id,
                          esp_matter_enum8(now.fan_mode), FIELD_FAN))
        {
            p_rep->fan_mode    = now.fan_mode;
            p_rep->fan_percent = now.fan_percent;
        }
        else
        {
            b_ok = false;
        }
    }
    if (b_all || (now.blower_percent != p_rep->blower_percent))
    {
        if (bridge_set_blower(now.blower_percent))
        {
            p_rep->blower_percent = now.blower_percent;
            g_fail_logged &= ~FIELD_BLOWER;
        }
        else
        {
            if (0U == (g_fail_logged & FIELD_BLOWER))
            {
                ESP_LOGW(LOG_TAG, "fan PercentCurrent update failed");
                g_fail_logged |= FIELD_BLOWER;
            }
            b_ok = false;
        }
    }
    if (b_all || (now.b_fault != p_rep->b_fault))
    {
        if (bridge_set_fault_contact(!now.b_fault))
        {
            p_rep->b_fault = now.b_fault;
            g_fail_logged &= ~FIELD_FAULT;
        }
        else
        {
            if (0U == (g_fail_logged & FIELD_FAULT))
            {
                ESP_LOGW(LOG_TAG, "fault contact update failed");
                g_fail_logged |= FIELD_FAULT;
            }
            b_ok = false;
        }
    }
    if (b_all || (now.b_eheat != p_rep->b_eheat))
    {
        if (bridge_report(g_eheat_ep, OnOff::Id, OnOff::Attributes::OnOff::Id,
                          esp_matter_bool(now.b_eheat), FIELD_EHEAT))
        {
            p_rep->b_eheat = now.b_eheat;
        }
        else
        {
            b_ok = false;
        }
    }

    // A full pass that failed somewhere is repeated in full: a value equal
    // to the last one reported would otherwise never be retried.
    if (b_all && !b_ok)
    {
        g_b_resync.store(true);
    }
    p_rep->b_valid = true;
}

// Announces the setpoint limits to any subscriber. They are written at boot
// before Matter starts, which a controller that read them earlier (Apple
// Home, at pairing) would otherwise never hear about.
void bridge_report_limits(void)
{
    bridge_limits_t lim = {};

    bridge_map_limits(&lim);
    (void)bridge_report(g_thermostat_ep, Thermostat::Id,
                        Thermostat::Attributes::AbsMinHeatSetpointLimit::Id,
                        esp_matter_int16(lim.min_heat_c100), FIELD_LIMITS);
    (void)bridge_report(g_thermostat_ep, Thermostat::Id,
                        Thermostat::Attributes::AbsMaxHeatSetpointLimit::Id,
                        esp_matter_int16(lim.max_heat_c100), FIELD_LIMITS);
    (void)bridge_report(g_thermostat_ep, Thermostat::Id,
                        Thermostat::Attributes::AbsMinCoolSetpointLimit::Id,
                        esp_matter_int16(lim.min_cool_c100), FIELD_LIMITS);
    (void)bridge_report(g_thermostat_ep, Thermostat::Id,
                        Thermostat::Attributes::AbsMaxCoolSetpointLimit::Id,
                        esp_matter_int16(lim.max_cool_c100), FIELD_LIMITS);
    (void)bridge_report(g_thermostat_ep, Thermostat::Id,
                        Thermostat::Attributes::MinHeatSetpointLimit::Id,
                        esp_matter_int16(lim.min_heat_c100), FIELD_LIMITS);
    (void)bridge_report(g_thermostat_ep, Thermostat::Id,
                        Thermostat::Attributes::MaxHeatSetpointLimit::Id,
                        esp_matter_int16(lim.max_heat_c100), FIELD_LIMITS);
    (void)bridge_report(g_thermostat_ep, Thermostat::Id,
                        Thermostat::Attributes::MinCoolSetpointLimit::Id,
                        esp_matter_int16(lim.min_cool_c100), FIELD_LIMITS);
    (void)bridge_report(g_thermostat_ep, Thermostat::Id,
                        Thermostat::Attributes::MaxCoolSetpointLimit::Id,
                        esp_matter_int16(lim.max_cool_c100), FIELD_LIMITS);
    (void)bridge_report(g_thermostat_ep, Thermostat::Id,
                        Thermostat::Attributes::MinSetpointDeadBand::Id,
                        esp_matter_int8(BRIDGE_DEADBAND_C10), FIELD_LIMITS);
}

void bridge_sync_task(void * p_arg)
{
    (void)p_arg;

    bridge_report_limits();
    for (;;)
    {
        bridge_sync();
        vTaskDelay(pdMS_TO_TICKS(SYNC_PERIOD_MS));
    }
}

// Thread builds (the C6): OpenThread must be given its radio and storage
// before esp_matter::start(), or it asserts at start-up. The radio is the
// chip's own 802.15.4 (native); no host processor; state lives in "nvs".
// Values as esp-matter's examples. The config is copied, so a local is fine.
esp_err_t bridge_thread_config(void)
{
    esp_err_t err = ESP_OK;

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
    esp_openthread_platform_config_t cfg = {};

    cfg.radio_config.radio_mode            = RADIO_MODE_NATIVE;
    cfg.host_config.host_connection_mode   = HOST_CONNECTION_MODE_NONE;
    cfg.port_config.storage_partition_name = "nvs";
    cfg.port_config.netif_queue_size       = 10U;
    cfg.port_config.task_queue_size        = 10U;
    err = set_openthread_platform_config(&cfg);
#endif

    return err;
}

#if CONFIG_ENABLE_CHIP_SHELL
// Bench checks of what a reset does to the relay pins (docs/HARDWARE.md):
// `matter esp reboot` restarts in software, as esp_restart() does, and
// `matter esp panic` aborts, as a crash or a task-watchdog panic does. On
// the C6 both are CPU resets. Watch the relays: they must drop at once.
esp_err_t bridge_cmd_reboot(int argc, char ** argv)
{
    (void)argc;
    (void)argv;
    ESP_LOGW(LOG_TAG, "console: software restart");
    esp_restart();
    return ESP_OK; // not reached
}

esp_err_t bridge_cmd_panic(int argc, char ** argv)
{
    (void)argc;
    (void)argv;
    ESP_LOGW(LOG_TAG, "console: abort (panic)");
    abort();
    return ESP_OK; // not reached
}

void bridge_register_bench_commands(void)
{
    static console::command_t const s_cmds[] = {
        { "reboot",
         "Software restart (bench reset check). Usage: matter esp "
          "reboot.", bridge_cmd_reboot },
        { "panic",
         "Abort, as a crash would (bench reset check). Usage: matter esp "
          "panic.",  bridge_cmd_panic  },
    };

    (void)console::add_commands(s_cmds, 2U);
}
#endif

} // namespace

extern "C" void matter_bridge_factory_reset(void)
{
    ESP_LOGW(LOG_TAG, "factory reset: forgetting the pairing, then restart");
    if (ESP_OK != esp_matter::factory_reset())
    {
        ESP_LOGE(LOG_TAG, "factory reset failed");
    }
}

extern "C" bool matter_bridge_pairing(char * p_qr, size_t qr_len,
                                      char * p_manual, size_t manual_len,
                                      bool * p_b_paired)
{
    bool b_ok = false;

    if ((nullptr == p_qr) || (nullptr == p_manual) || (nullptr == p_b_paired) ||
        (qr_len < 2U) || (manual_len < 2U) || !g_b_live.load())
    {
        goto done;
    }

    {
        lock::ScopedChipStackLock stack_lock(portMAX_DELAY);
        // One byte kept for the terminator the spans do not write.
        chip::MutableCharSpan qr(p_qr, qr_len - 1U);
        chip::MutableCharSpan manual(p_manual, manual_len - 1U);
        chip::RendezvousInformationFlags const flags(
            chip::RendezvousInformationFlag::kBLE);

        if ((CHIP_NO_ERROR == GetQRCode(qr, flags)) &&
            (CHIP_NO_ERROR == GetManualPairingCode(manual, flags)))
        {
            p_qr[qr.size()]         = '\0';
            p_manual[manual.size()] = '\0';
            *p_b_paired =
                (0U <
                 chip::Server::GetInstance().GetFabricTable().FabricCount());
            b_ok = true;
        }
    }

done:
    return b_ok;
}

extern "C" void matter_bridge_start(void)
{
    node::config_t node_cfg = {};
    node_t *       p_node   = nullptr;
    endpoint_t *   p_thermo = nullptr;
    endpoint_t *   p_fan    = nullptr;
    endpoint_t *   p_eheat  = nullptr;
    endpoint_t *   p_fault  = nullptr;
    esp_err_t      err      = ESP_FAIL;

    p_node = node::create(&node_cfg, bridge_attribute_cb, bridge_identify_cb);
    if (nullptr == p_node)
    {
        ESP_LOGE(LOG_TAG, "node create failed: running without Matter");
        goto done;
    }

    p_thermo = bridge_create_thermostat(p_node);
    p_fan    = bridge_create_fan(p_node);
    p_eheat  = bridge_create_eheat(p_node);
    p_fault  = bridge_create_fault(p_node);
    if ((nullptr == p_thermo) || (nullptr == p_fan) || (nullptr == p_eheat) ||
        (nullptr == p_fault))
    {
        ESP_LOGE(LOG_TAG, "endpoint create failed: running without Matter");
        goto done;
    }
    g_thermostat_ep = endpoint::get_id(p_thermo);
    g_fan_ep        = endpoint::get_id(p_fan);
    g_eheat_ep      = endpoint::get_id(p_eheat);
    g_fault_ep      = endpoint::get_id(p_fault);
    ESP_LOGI(LOG_TAG, "endpoints: thermostat %u, fan %u, e-heat %u, fault %u",
             g_thermostat_ep, g_fan_ep, g_eheat_ep, g_fault_ep);

    err = bridge_thread_config();
    if (ESP_OK != err)
    {
        ESP_LOGE(LOG_TAG, "Thread config failed (0x%x): running without Matter",
                 err);
        goto done;
    }

    err = esp_matter::start(bridge_event_cb);
    if (ESP_OK != err)
    {
        ESP_LOGE(LOG_TAG, "start failed (0x%x): running without Matter", err);
        goto done;
    }
    g_b_live.store(true); // start-up done: writes from here on are requests

    {
        lock::ScopedChipStackLock stack_lock(portMAX_DELAY);
        PrintOnboardingCodes(chip::RendezvousInformationFlags(
            chip::RendezvousInformationFlag::kBLE));
    }

#if CONFIG_ENABLE_CHIP_SHELL
    // Serial shell on the console: `matter esp factoryreset` forgets the
    // pairing (thermostat settings stay).
    (void)console::diagnostics_register_commands();
    (void)console::factoryreset_register_commands();
    // `matter esp attribute get <endpoint> <cluster> <attribute>`, e.g.
    // `matter esp attribute get 0x1 0x201 0x18` for the cool setpoint maximum.
    (void)console::attribute_register_commands();
    bridge_register_bench_commands();
    (void)console::init();
#endif

    if (pdPASS != xTaskCreate(bridge_sync_task, "matter_sync", SYNC_STACK,
                              nullptr, SYNC_PRIORITY, &g_h_sync))
    {
        ESP_LOGE(LOG_TAG, "sync task create failed");
    }

done:
    return;
}
