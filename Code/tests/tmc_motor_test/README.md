# Single-Motor Test for All F6 Slots

This test controls a selected TMC2209 driver channel on the FYSETC F6 V1.4.

## Selecting a Slot

Set `DRIVER_SLOT` at the very top of the sketch, for example:

- `#define DRIVER_SLOT SLOT_X`
- `#define DRIVER_SLOT SLOT_Y`
- `#define DRIVER_SLOT SLOT_Z`
- `#define DRIVER_SLOT SLOT_E0`
- `#define DRIVER_SLOT SLOT_E1`
- `#define DRIVER_SLOT SLOT_E2`

## Pin Assignment

- X: STEP 54, DIR 55, ENABLE 38
- Y: STEP 60, DIR 61, ENABLE 56
- Z: STEP 43, DIR 48, ENABLE 58
- E0: STEP 26, DIR 28, ENABLE 24
- E1: STEP 36, DIR 34, ENABLE 30
- E2: STEP 59, DIR 57, ENABLE 40
- ENABLE is active LOW

The board has six TMC slots: X, Y, Z, E0, E1, and E2. Z1-MOT and Z2-MOT are
two motor sockets connected in series to the same Z driver. They therefore
cannot be controlled separately. If only one Z motor is connected, the other
Z-MOT socket must be bridged appropriately.
- Serial interface: 115200 baud

## Operation

The driver is disabled after startup or reset. The following commands are available in the serial monitor:

- `f`: slow forward test movement
- `r`: slow reverse test movement

Each command generates 400 slow step pulses and then disables the driver again.

## Procedure for the First Test

1. Switch off and disconnect the 24 V supply.
2. Check the driver orientation, jumpers, and current limit.
3. Connect the motor without a mechanical load and use the TMC2209 with a heat sink.
4. Upload the sketch to the FYSETC F6 V1.4.
5. Open the serial monitor at 115200 baud.
6. Switch on the 24 V supply.
7. Send `f` first, followed by `r` if needed.
8. Switch off immediately if there are unusual noises, vibrations, or excessive heat.

## Safety

- Only connect or disconnect the TMC2209 and motor when the 24 V supply is switched off.
- Check the driver orientation and current limit before the first run.
- Perform the first test without a mechanical load and with a heat sink.
