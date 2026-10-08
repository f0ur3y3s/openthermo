# Control spec

All temperatures are integer tenths of a degree, stored in °F. Display units are a separate setting. All times come from a monotonic ms clock.

## Modes and outputs

| Call | Y1 | G | O | W |
|---|---|---|---|---|
| Off, idle | 0 | 0 or fan setting | hold | 0 |
| Fan only | 0 | 1 | hold | 0 |
| Heat | 1 | 1 | 0 | 0 |
| Heat + aux | 1 | 1 | 0 | 1 |
| Cool | 1 | 1 | 1 | 0 |
| Emergency heat | 0 | 1 | 0 | 1 |

"Hold" means O keeps its last state while idle. That avoids cycling the reversing valve, and O only changes under the rule below.

## Timing and staging

- **Hysteresis.** ±0.5 °F around the setpoint.
- **Compressor minimum off.** 5 min, and the timer is armed at boot. No Y1 until it expires.
- **Compressor minimum run.** 3 min, unless a fault, mode Off or emergency heat ends it (emergency heat requires Y1 off). A change between heat and cool, by mode or in Auto, holds the running call until its 3 min are up (dropping any W), then waits out the minimum-off time before O changes.
- **O changes.** Only when Y1 has been off for at least the minimum-off time. A fault drops O with everything else at once; that restarts the minimum-off time.
- **Aux (W) in heat.** Add W when the room is at least 3 °F below setpoint, or still falling after 15 min of heat. Drop W within 1 °F of setpoint.
- **Auto mode.**
  - Requires heat setpoint + 3 °F ≤ cool setpoint. Enforce this when setpoints are written.
  - Change over between heat and cool only after 10 min idle.
- **Fan purge.** G may run on 60 s after a heat or cool call ends. Make this configurable.

## Faults

- **No valid sensor reading for 2 min:** all outputs off, show a fault on the OLED, and report the fault to Matter (null temperature, and the fault contact sensor opens).
  - A reading outside 0–120 °F counts as a failed read, whatever its CRC.
  - After a fault, the reading counts as valid again only after 60 s of unbroken good reads.
- **Task watchdog:** enabled on the control task; failing to subscribe aborts start-up.
- **Any reset:** all outputs drop at once (panic handler, `esp_restart()` shutdown handler and a bootloader hook, for the CPU-only resets that keep GPIO state), and the minimum-off timer is armed again.
- **Crash loop:** after 3 crash resets in a row (panic, watchdog, brown-out), Matter stays off and every output is held off for 60 s after boot; 15 min without a crash clears the count.
- **Out-of-range settings from NVS:** fall back to defaults and log it.

## Interfaces

- **Matter Thermostat cluster.**
  - SystemMode: Off, Heat, Cool or Auto.
  - OccupiedHeatingSetpoint and OccupiedCoolingSetpoint.
  - LocalTemperature, in 0.01 °C as the spec requires. Convert at the boundary only.
- **Fan Control cluster** for G.
- **Emergency heat.** A separate On/Off endpoint. While it is on, the thermostat forces emergency heat and ignores Heat.
- **Local UI.**
  - Up and down change the active setpoint.
  - Left and right change pages: main, mode, fan, settings, info / commission QR.
  - Centre confirms.
  - The display dims after a timeout.
