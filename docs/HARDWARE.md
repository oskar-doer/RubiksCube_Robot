# Hardware Components

The connections between these components are documented in the
[robot wiring instructions](../Code/robot/WIRING.md).

* 1x ESP32-S3-WROOM-1-N16R8 module from Espressif Systems
* 1x F6 V1.4 control board from FYSETC
* 6x TMC2209 V1.3 motor drivers from BIGTREETECH
  * On these modules, the R10 solder bridge on the `PDN_UART` signal had to be
    closed for UART operation. Other TMC2209 StepStick variants with `PDN_UART`
    already configured or exposed appropriately may simplify the build. Before
    purchasing, verify the pin assignment and UART connection against the
    FYSETC F6 V1.4.
* Heat sinks for the TMC2209 motor drivers
* 6x 17HE19-2004S NEMA 17 stepper motors from StepperOnline
* 1x power supply, 24 V DC / 8 A
* 1x LCD1602 display module
* 1x QYSC-S Bluetooth cube from QiYi
* 1x 10 kOhm, 3x 1 kOhm, and 1x 2 kOhm resistors (0.25 W is sufficient):
  10 kOhm/1 kOhm for the LCD contrast divider, 1 kOhm for the LCD backlight,
  and 1 kOhm/2 kOhm for the F6-to-ESP UART level divider
* Small electronic components and connection cables
* M3 threaded insert and M3 screws

## Documentation

* [ESP32-S3-WROOM-1 datasheet from Espressif](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf)
* [FYSETC F6 documentation, schematic, and board files](https://github.com/FYSETC/FYSETC-F6)
* [General TMC2209 documentation from BIGTREETECH](https://github.com/bigtreetech/docs/blob/master/docs/TMC2209.md)
* [Product information and datasheet for the StepperOnline 17HE19-2004S](https://www.omc-stepperonline.com/e-series-nema-17-bipolar-55ncm-77-88oz-in-2a-42x48mm-4-wires-w-1m-cable-connector-17he19-2004s)
