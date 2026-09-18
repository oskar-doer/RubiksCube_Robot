# ESP-F6 UART and Motor Direction Test

Upload `esp_f6_motor_uart_test.ino` to the ESP32-S3. Upload the
`../f6_esp_motor_uart_test/f6_esp_motor_uart_test.ino` sketch to the FYSETC F6.
Both serial interfaces operate at 115200 baud.

This applies only to this matching test pair. The production sketches under
`robot/` communicate at 250000 baud.

## Arduino IDE Settings for the ESP32-S3 N16R8

| Tools menu item | Setting |
|---|---|
| Board | ESP32S3 Dev Module |
| USB Mode | Hardware CDC and JTAG |
| USB CDC On Boot | Enabled |
| Upload Mode | UART0 / Hardware CDC |
| CPU Frequency | 240 MHz |
| Flash Size | 16 MB (128 Mb) |
| PSRAM | OPI PSRAM |
| Partition Scheme | Default 4MB with spiffs |
| Upload Speed | 921600; 460800 if problems occur |
| Core Debug Level | None |
| Erase All Flash Before Sketch Upload | Disabled |
| Port | COM port of the ESP's native USB connector |

For this test, plug the USB cable into the ESP's native connector, which is
usually labeled `USB`. This keeps GPIO43/TX and GPIO44/RX available for the
UART connection to the F6. For the solver later on, select `Partition Scheme:
Custom` again because it requires the included `partitions.csv`.

First, send `p` in the ESP serial monitor without the 24 V motor supply. The
response `F6 > ESP: PONG` confirms TX, RX, and GND in both directions.

With the 24 V motor supply switched on, a command such as `u+` slowly moves the
assigned motor by 400 pulses. `u-` moves the same motor in the opposite
direction. The symbols represent the electrical DIR direction and are recorded
as clockwise or counterclockwise after the test.

Always determine the direction of rotation by looking directly at the tested
cube face from the outside. To turn it back, send the same letter with the
opposite sign.

| Command | Motor |
|---|---|
| `u+`, `u-` | U, white, E1 |
| `r+`, `r-` | R, red, Y |
| `f+`, `f-` | F, green, E0 |
| `d+`, `d-` | D, yellow, E2 |
| `l+`, `l-` | L, orange, X |
| `b+`, `b-` | B, blue, Z1 |
| `p` | UART ping without movement |
| `x` | Abort movement |

A custom pulse count is possible, for example `u+ 200`. Values from 1 to 5000
pulses are allowed. Drivers are disabled at startup and after each test.

## Measurement Result

- `u+`, `r+`, `f+`, `d+`, `l+`, and `b+`: approximately 45 degrees at 400 pulses each.
- All `+` directions: counterclockwise when viewed from the front of the motor.
- This results in 800 pulses per quarter turn.
