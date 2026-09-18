# ESP32-S3 Cube Controller

This is the robot's production ESP32-S3 firmware. It connects to the QiYi
QYSC-S via Bluetooth, reads and verifies the cube state, calculates a solution,
displays the process on the LCD1602, and sends the move sequence to the FYSETC
F6 via UART.

## Uploading with the Arduino IDE

Keep the entire directory together and open `esp32_cube_controller.ino` in the
Arduino IDE.

Settings used:

| Setting | Value |
|---|---|
| Board package | esp32 by Espressif Systems 3.3.11 |
| Board | ESP32S3 Dev Module |
| CPU Frequency | 240 MHz |
| Flash Size | 16 MB (128 Mb) |
| PSRAM | OPI PSRAM |
| USB CDC On Boot | Enabled when using the native USB connector |
| USB Mode | Hardware CDC and JTAG |
| Partition Scheme | Custom (`partitions.csv`) |
| Serial monitor | 115200 baud |

When using the UART USB connector, `USB CDC On Boot` must be disabled. If the
upload does not start, hold BOOT, briefly press RESET, release BOOT, and upload
again.

On the first startup, min2phase generates its search tables in PSRAM and saves
them to LittleFS. This can take several minutes. Later startups load the
appropriate cache from flash memory.

## Wiring

The LCD1602 pinout, ESP-to-F6 UART connection, voltage divider, power wiring,
driver setup, and motor connections are documented centrally in
[WIRING.md](../WIRING.md).

## Operation

- Briefly press BOOT: solve a scrambled cube; scramble a solved cube with 20 moves
- Hold BOOT for three seconds: calibrate a physically solved cube as solved
- Press BOOT during a motor run: send an emergency stop to the F6

| Serial command | Function |
|---|---|
| `s` | Solve or scramble depending on the cube state |
| `c` | Calibrate a solved cube |
| `x` | Abort the motor run |
| `p` | Display the cube state and battery level |
| `t` | Test the solver with a virtual test cube |
| `l` | Display an LCD test pattern |
| `w` | Measure the LCD lines individually |
| `r` | Reestablish the Bluetooth connection |
| `h` or `?` | Display help |

The solver runs in a separate task so that Bluetooth and the display continue
to operate during the search. The cube must not be turned during the
calculation; any solution rendered outdated as a result is discarded.

## Solution Selection

The first valid solution is retained as a fallback. The ESP then continues
searching for a total of three seconds and evaluates every variant found by the
number of motor time steps:

- A normal move counts as one time step.
- Consecutive moves of opposite faces (`U D`, `R L`, `F B`, and vice versa)
  count together as one time step because the F6 executes them simultaneously.
- If the number of time steps is equal, the sequence with fewer individual moves wins.

The selected sequence may contain no more than 30 individual moves. Each answer
is replayed using an independent cube model before it is sent to the F6. The
evaluation selects the best variant found within the search window; it does not
prove a global optimum.

## Feedback and Errors

During the motor run, the LCD shows progress and elapsed time. After the run,
the ESP verifies the new Bluetooth state. Even if the solution fails, the motor
time reported by the F6 remains visible. On success, the LCD displays the motor
time and move count; the USB monitor outputs the solver, motor, and total times.

- `F6 nicht bereit` ("F6 not ready"): check the UART connection, common ground, and both baud rates.
- `TMC-UART Fehler` ("TMC UART error"): check the drivers named on the LCD and their jumpers.
- `PSRAM 0 MB`: check OPI PSRAM, the board selection, and the N16R8 module.
- No cube: wake the cube and disconnect it from other apps.
- Invalid state: calibrate the cube in the manufacturer's app or use `c` here.

## File Structure

- `esp32_cube_controller.ino`: Arduino entry point
- `app.cpp`: Bluetooth, LCD, operation, and F6 communication
- `qiyi_protocol.h`: QiYi frames, CRC, and state decoding
- `solver_adapter.cpp/.h`: min2phase integration and preparation
- `cube_model.c/.h`: independent state and solution verification
- `src/min2phase/`: adapted solver library

## Sources and Licenses

- [min2phaseCXX](https://github.com/lilborgo/min2phaseCXX): `src/min2phase/UPSTREAM-LICENSE.md`
- [QiYi protocol documentation](https://codeberg.org/Flying-Toast/qiyi_smartcube_protocol) by Simon Schwartz (MIT): `LICENSE-qiyi.txt`
- Independent cube model and integration software (GPL-3.0-or-later): `LICENSE-GPL-3.0.txt`
