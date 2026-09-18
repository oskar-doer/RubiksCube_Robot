// UART-Verbindungstest fuer einen BTT TMC2209 V1.3 in einem Treibersteckplatz
// eines FYSETC F6 V1.4.
//
// Wichtig: Dieses Programm mit dem Boardprofil "FYSETC F6 V1.4"
// kompilieren. Das normale Arduino-Mega-Profil kennt Pin 82 nicht.

#include <TMCStepper.h>

// HIER DEN ZU TESTENDEN STECKPLATZ AUSWAEHLEN, zum Beispiel SLOT_E1:
#define SLOT_X 1
#define SLOT_Y 2
#define SLOT_Z 3
#define SLOT_E0 4
#define SLOT_E1 5
#define SLOT_E2 6

#ifndef DRIVER_SLOT
#define DRIVER_SLOT SLOT_Z
#endif

#if DRIVER_SLOT == SLOT_X
constexpr char SLOT_NAME[] = "X";
constexpr uint8_t ENABLE_PIN = 38;
constexpr uint8_t UART_RX_PIN = 72;
constexpr uint8_t UART_TX_PIN = 71;
#elif DRIVER_SLOT == SLOT_Y
constexpr char SLOT_NAME[] = "Y";
constexpr uint8_t ENABLE_PIN = 56;
constexpr uint8_t UART_RX_PIN = 73;
constexpr uint8_t UART_TX_PIN = 78;
#elif DRIVER_SLOT == SLOT_Z
constexpr char SLOT_NAME[] = "Z";
constexpr uint8_t ENABLE_PIN = 58;
constexpr uint8_t UART_RX_PIN = 75;
constexpr uint8_t UART_TX_PIN = 79;
#elif DRIVER_SLOT == SLOT_E0
constexpr char SLOT_NAME[] = "E0";
constexpr uint8_t ENABLE_PIN = 24;
constexpr uint8_t UART_RX_PIN = 77;
constexpr uint8_t UART_TX_PIN = 81;
#elif DRIVER_SLOT == SLOT_E1
constexpr char SLOT_NAME[] = "E1";
constexpr uint8_t ENABLE_PIN = 30;
constexpr uint8_t UART_RX_PIN = 76;  // E1_SERIAL_RX_PIN / PJ4
constexpr uint8_t UART_TX_PIN = 80;  // E1_SERIAL_TX_PIN / PD4
#elif DRIVER_SLOT == SLOT_E2
constexpr char SLOT_NAME[] = "E2";
constexpr uint8_t ENABLE_PIN = 40;
constexpr uint8_t UART_RX_PIN = 62;  // E2_SERIAL_RX_PIN / A8
constexpr uint8_t UART_TX_PIN = 82;  // E2_SERIAL_TX_PIN / PD5
#else
#error "Unbekannter DRIVER_SLOT. Erlaubt: SLOT_X, SLOT_Y, SLOT_Z, SLOT_E0, SLOT_E1, SLOT_E2."
#endif

constexpr float R_SENSE = 0.110F;
constexpr uint8_t DRIVER_ADDRESS = 0b00;  // MS1 und MS2 ohne Jumper

TMC2209Stepper driver(UART_RX_PIN, UART_TX_PIN, R_SENSE, DRIVER_ADDRESS);

void setup() {
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, HIGH);  // Ausgaenge waehrend des Tests deaktivieren

  Serial.begin(115200);     // Ausgabe zum seriellen Monitor
  driver.beginSerial(115200);
  delay(500);

  Serial.print(F("FYSETC F6 V1.4 / "));
  Serial.print(SLOT_NAME);
  Serial.println(F(" TMC2209 UART-Test"));
  Serial.println(F("Treiber bleibt deaktiviert; der Motor bewegt sich nicht."));

  driver.begin();
  delay(100);

  const uint8_t connection = driver.test_connection();
  const uint8_t version = driver.version();
  const uint8_t interfaceCounter = driver.IFCNT();

  Serial.print(F("test_connection = "));
  Serial.println(connection);
  Serial.print(F("VERSION = 0x"));
  if (version < 0x10) Serial.print('0');
  Serial.println(version, HEX);
  Serial.print(F("IFCNT = "));
  Serial.println(interfaceCounter);

  if (connection == 0 && version == 0x21) {
    Serial.println(F("ERGEBNIS: UART OK - TMC2209 antwortet."));
  } else {
    Serial.println(F("ERGEBNIS: UART NICHT BESTAETIGT."));
    Serial.println(F("Erwartet: test_connection = 0 und VERSION = 0x21."));
  }
}

void loop() {
  // Ein einmaliger Lesetest genuegt. Keine Motorbewegung.
}
