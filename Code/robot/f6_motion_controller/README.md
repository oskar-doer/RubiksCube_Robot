# F6 Motion Controller

Arduino board profile: `FYSETC F6 V1.4`, library: `TMCStepper`, serial
monitor: 115200 baud. The UART connection to the ESP on `Serial1` operates at
250000 baud.

The controller UART, TMC2209 setup, and motor connections are documented in
[WIRING.md](../WIRING.md).

The controller understands solutions consisting of `U R F D L B` and the
suffixes `'` and `2`. Example from the ESP:

```text
RUN 1 U R' F2 D
```

Responses:

```text
READY
PONG
START 1 4
MOVE 1 1 4
DONE 1 1234
ERR 1 FEHLERNAME
```

`!` aborts a movement. You can also enter `x` in the F6's USB serial monitor
for this purpose.

Timer1 generates the STEP edges at absolute times. The motion profile uses 2
microsteps with TMC interpolation, separate ramps for 90- and 180-degree moves,
and continues to execute moves of opposite faces in parallel.

The six entries in `motors[]`, their directions of rotation, and
`MOTION_CONFIGURATION_CONFIRMED=true` correspond to the verified setup. They
only need to be changed if the motors or slots are rearranged.
