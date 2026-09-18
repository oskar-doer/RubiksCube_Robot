# Rubik's Cube Robot

This repository separates the production robot firmware from standalone
hardware tests:

```text
robot/
  esp32_cube_controller/   Production ESP32-S3 firmware
  f6_motion_controller/    Production FYSETC F6 firmware
tests/                     Individual tests for setup and troubleshooting
```

For normal operation, upload these two sketches:

- `robot/esp32_cube_controller/esp32_cube_controller.ino`
- `robot/f6_motion_controller/f6_motion_controller.ino`

The production UART connection between the ESP and F6 uses 250000 baud. The
USB monitors use 115200 baud. Sketches under `tests/` are independent diagnostic
programs and can therefore use different settings, particularly 115200 baud
between the test devices.

The complete production wiring is documented in
[`robot/WIRING.md`](robot/WIRING.md). Operation and the currently configured
motion values are documented in [`robot/README.md`](robot/README.md). Details
about the two controllers are provided in their respective subdirectories.
