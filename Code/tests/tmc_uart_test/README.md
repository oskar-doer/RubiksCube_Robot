# TMC2209 UART Test for All F6 Slots

This test reads the TMC2209 in a selected driver slot on the FYSETC F6 V1.4.
A successful response confirms the UART signal path through the F6 jumper and
the closed R10 bridge on the BTT driver.

## Selecting a Slot

Set `DRIVER_SLOT` at the very top of the sketch, for example:

- `#define DRIVER_SLOT SLOT_X`
- `#define DRIVER_SLOT SLOT_Y`
- `#define DRIVER_SLOT SLOT_Z`
- `#define DRIVER_SLOT SLOT_E0`
- `#define DRIVER_SLOT SLOT_E1`
- `#define DRIVER_SLOT SLOT_E2`

The board has six TMC slots: X, Y, Z, E0, E1, and E2. The two labels Z1-MOT
and Z2-MOT refer to the two motor sockets on the same Z TMC slot. A UART test
can therefore only test the shared Z TMC.

## Requirements

- Board profile `FYSETC F6 V1.4`
- `TMCStepper` library
- JP1 on the selected F6 slot set to pins 2-3
- R10 closed on the BTT TMC2209 V1.3
- MS1/MS2 jumpers removed, resulting in UART address 0
- 24 V main power supply switched on

The enable output remains HIGH during the test. The motor is therefore not
driven.

The serial monitor and the UART connection to the TMC2209 both operate at
115200 baud.

## Expected Output on Success

```text
test_connection = 0
VERSION = 0x21
ERGEBNIS: UART OK - TMC2209 antwortet.
```
