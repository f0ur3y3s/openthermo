# openthermo — firmware project

The `idf.py` project for the Seeed XIAO ESP32-C6, Matter over Thread. Only
`main/` (`app_main()`) and `components/matter_bridge` live here; the
thermostat itself is `../components`. Building, flashing and pairing are in
the [top-level README](../README.md).

| File | What it holds |
|---|---|
| `sdkconfig.defaults` | Watchdog, brown-out, size optimisation, BLE commissioning, Matter shell. |
| `sdkconfig.defaults.esp32c6` | 4 MB flash, USB console, Thread instead of Wi-Fi. |
| `sdkconfig.defaults.debug` | The serial shell, for the debug and sim builds only. |
| `partitions.csv` | esp-matter's 4 MB layout: two 1.875 MB app slots from `0x20000`. |

## How it fits together

- **Matter writes** (mode, setpoints, fan, e-heat) become settings changes, exactly
  like a D-pad press. The control loop, its timers and the relay guard still
  decide every output; Matter never drives a relay.
- **The sync task** reports the thermostat's own state to Matter once a second:
  temperature (null on a sensor fault), mode, setpoints, running state, fan and e-heat.
- **Value mapping** (°F tenths ↔ °C hundredths, modes, fan, running state) is in
  `components/matter_bridge/bridge_map.c`, host-tested by `test/test_bridge_map`.
- **Setpoint limits** (60–80 °F) are written to all eight Thermostat limit
  attributes on every boot, because Matter keeps them in non-volatile storage.
- **Boot:** callbacks are ignored until `esp_matter::start()` returns, so the
  clusters' own start-up writes (the On/Off StartUpOnOff rule) never reach the
  thermostat; the first sync then reports every value.
- **Fixed attributes:** the limits, the deadband, the control sequence and the
  e-heat StartUpOnOff (null) are set at every boot and refused to controllers.
- **Thermostat fault:** a contact sensor endpoint that opens on a sensor fault.
