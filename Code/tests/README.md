# Standalone Tests

The sketches in this directory are intended for setup and troubleshooting. They
are not part of the production robot firmware and are uploaded in place of the
normal ESP or F6 program.

| Directory | Purpose |
|---|---|
| `tmc_uart_test` | Test the UART connection to a single TMC2209 |
| `tmc_motor_test` | Slowly move a single F6 motor slot |
| `esp_f6_motor_uart_test` | ESP side of the combined UART/direction test |
| `f6_esp_motor_uart_test` | F6 side of the combined UART/direction test |

Each test uses the settings from its own README or sketch. In particular, the
UART test pair operates at 115200 baud, while the production programs under
`robot/` communicate at 250000 baud.
