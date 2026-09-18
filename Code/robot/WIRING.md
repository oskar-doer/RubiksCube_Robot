# Wiring Instructions

This document contains the complete wiring for the production robot. The test
sketches under `../tests/` may use different baud rates, but they use the same
physical ESP32-S3, F6, driver, and motor connections described here.

## Safety

- Switch off and disconnect the 24 V supply before installing or removing a
  TMC2209 driver or motor cable.
- Check the polarity of every supply connection before switching on power.
- Never connect the F6's 24 V motor supply to the ESP32-S3 or LCD.
- Connect the ESP32-S3 and LCD only to the F6's regulated 5 V output, never to
  its 24 V input.
- Fit a heat sink to every TMC2209 and check each driver's orientation before
  powering the F6.
- The F6 and ESP32-S3 must share GND.
- The F6 TX1 signal is 5 V. Connect it to the ESP32-S3 only through the voltage
  divider shown below.

## Power

1. Connect the 24 V DC / 8 A supply to the main power input of the FYSETC F6,
   observing the marked positive and negative terminals.
2. Connect a regulated `5V` output on the F6 directly to the ESP32-S3's
   `5V`/`5Vin` input. Connect F6 GND directly to ESP32-S3 GND.
3. Connect the same F6 `5V` rail directly to LCD pin 2 (`VDD`) and the F6 GND
   rail directly to LCD pin 1 (`VSS`). For the contrast voltage, connect a
   10 kOhm resistor from F6 `5V` to LCD pin 3 (`VO`) and a 1 kOhm resistor from
   LCD pin 3 (`VO`) to F6 GND. The LCD is not powered through the ESP.
4. Connect LCD pin 15 (`A`/`LED+`) to F6 `5V` through a 1 kOhm resistor and
   connect LCD pin 16 (`K`/`LED-`) directly to F6 GND.
5. The UART connection uses this same common GND.

The ESP32-S3 and LCD are two parallel loads on the F6's 5 V rail: both receive
their 5 V directly from the F6. Do not feed either device from the F6's 24 V
input. When connecting USB to the ESP32-S3 for programming, avoid connecting
two 5 V sources unless the specific ESP32-S3 development board explicitly
supports it.

Do not switch on the 24 V supply until the drivers, motor cables, controller
UART, and common ground have all been checked.

## ESP32-S3 to LCD1602

The LCD is connected directly in 4-bit mode. These GPIO numbers match the
`LiquidCrystal` configuration in `esp32_cube_controller/app.cpp`.

| LCD1602 pin | Function | Connection |
|---|---|---|
| 1 | VSS | F6 GND |
| 2 | VDD | F6 5V |
| 3 | VO, contrast | Junction of a 10 kOhm / 1 kOhm voltage divider (see below) |
| 4 | RS | GPIO1 |
| 5 | RW | GND |
| 6 | E | GPIO2 |
| 7-10 | D0-D3 | not connected |
| 11 | D4 | GPIO40 |
| 12 | D5 | GPIO41 |
| 13 | D6 | GPIO42 |
| 14 | D7 | GPIO21 |
| 15 | A / LED+ | F6 5V through a 1 kOhm resistor |
| 16 | K / LED- | F6 GND |

Wire the two fixed contrast resistors as a voltage divider:

```text
F6 5V ---- R1: 10 kOhm ----+---- LCD pin 3 (VO)
                            |
                         R2: 1 kOhm
                            |
F6 GND ---------------------+
```

R1 is the upper resistor between `5V` and `VO`; R2 is the lower resistor
between `VO` and GND. With a 5 V supply, this produces approximately 0.45 V at
LCD pin 3 (`5 V * 1 kOhm / (10 kOhm + 1 kOhm)`), which provides a suitable
fixed contrast voltage for a typical LCD1602. Both resistors are ordinary
low-power types; 0.25 W is more than sufficient.

The 1 kOhm resistor between F6 `5V` and LCD pin 15 limits the backlight current.

## ESP32-S3 to FYSETC F6

| ESP32-S3 | FYSETC F6 V1.4 |
|---|---|
| 5V/5Vin | 5V |
| TX / GPIO43 | D19 / RX1 |
| RX / GPIO44 | D18 / TX1 through the voltage divider below |
| GND | GND |

ESP TX supplies 3.3 V and can be connected directly to F6 RX1. F6 TX1 supplies
5 V and must be reduced before it reaches the ESP RX input:

```text
F6 D18/TX1 -- 1 kOhm --+-- ESP RX/GPIO44
                       |
                     2 kOhm
                       |
                      GND
```

The junction between the 1 kOhm and 2 kOhm resistors connects to ESP GPIO44.
The lower end of the 2 kOhm resistor connects to the same common GND as both
controllers.

The production firmware communicates at 250000 baud. The USB serial monitors
remain at 115200 baud. After changing the controller baud rate, upload matching
firmware to both controllers.

## TMC2209 Drivers on the F6

Install one TMC2209 in each of the `X`, `Y`, `Z`, `E0`, `E1`, and `E2` driver
slots. The production firmware uses the F6's onboard STEP, DIR, ENABLE, and UART
connections, so no separate signal wires are required between the F6 and these
six drivers.

For the BIGTREETECH TMC2209 V1.3 modules used in this build:

- close the `R10` solder bridge on each module for UART operation;
- set `JP1` for each F6 driver slot to pins 2-3;
- remove the `MS1` and `MS2` jumpers so the UART address is 0;
- verify the module orientation against the F6 pin labels before applying
  power; and
- install a heat sink on every driver.

At startup, the F6 checks all six UART connections. The robot remains disabled
if a driver does not respond with `test_connection = 0` and `VERSION = 0x21`.

## Motors to the F6

Connect each motor to the output belonging to its assigned driver slot:

| Cube face | Color/position | F6 driver slot | F6 motor output |
|---|---|---|---|
| U | white, top | E1 | E1-MOT |
| R | red, right | Y | Y-MOT |
| F | green, front | E0 | E0-MOT |
| D | yellow, bottom | E2 | E2-MOT |
| L | orange, left | X | X-MOT |
| B | blue, back | Z | Z1-MOT |

`Z1-MOT` and `Z2-MOT` are two motor sockets connected in series to the same Z
driver; they are not independent axes. This build uses `Z1-MOT` for the B motor.
If only one Z socket is occupied, bridge the unused Z motor socket appropriately
for the series circuit.

Keep each motor's two coil pairs together in the four-pin connector. If a cable
has been repinned or replaced, verify the coil pairs with the motor datasheet or
an ohmmeter before connecting it.

## Final Check Before Power-Up

- The 24 V polarity at the F6 is correct.
- Every driver is oriented correctly, has its required UART configuration, and
  has a heat sink.
- All six motor cables are in the outputs listed above.
- ESP `5V`/`5Vin` and LCD `VDD` are both connected directly to F6 `5V`.
- The LCD is not powered from the ESP32-S3.
- LCD pin 3 is connected to the junction of the fixed contrast divider: 10 kOhm
  to F6 `5V` and 1 kOhm to F6 GND.
- LCD pin 15 (`A`/`LED+`) is connected to F6 `5V` through a 1 kOhm resistor,
  and LCD pin 16 (`K`/`LED-`) is connected directly to F6 GND.
- LCD pins 7-10 are left unconnected and LCD RW is tied to GND.
- ESP GPIO43 goes directly to F6 D19/RX1.
- F6 D18/TX1 reaches ESP GPIO44 only through the 1 kOhm/2 kOhm divider.
- ESP GND, LCD GND, voltage-divider GND, and F6 GND are common.
