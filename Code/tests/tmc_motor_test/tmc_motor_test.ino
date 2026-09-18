// Sicherer Einzelmotortest fuer einen Treibersteckplatz des FYSETC F6 V1.4.
// Der Treiber bleibt nach dem Einschalten deaktiviert und bewegt sich
// erst nach einem Befehl ueber den seriellen Monitor.

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
constexpr uint8_t STEP_PIN = 54;
constexpr uint8_t DIR_PIN = 55;
constexpr uint8_t ENABLE_PIN = 38;
#elif DRIVER_SLOT == SLOT_Y
constexpr char SLOT_NAME[] = "Y";
constexpr uint8_t STEP_PIN = 60;
constexpr uint8_t DIR_PIN = 61;
constexpr uint8_t ENABLE_PIN = 56;
#elif DRIVER_SLOT == SLOT_Z
constexpr char SLOT_NAME[] = "Z";
constexpr uint8_t STEP_PIN = 43;
constexpr uint8_t DIR_PIN = 48;
constexpr uint8_t ENABLE_PIN = 58;
#elif DRIVER_SLOT == SLOT_E0
constexpr char SLOT_NAME[] = "E0";
constexpr uint8_t STEP_PIN = 26;
constexpr uint8_t DIR_PIN = 28;
constexpr uint8_t ENABLE_PIN = 24;
#elif DRIVER_SLOT == SLOT_E1
constexpr char SLOT_NAME[] = "E1";
constexpr uint8_t STEP_PIN = 36;
constexpr uint8_t DIR_PIN = 34;
constexpr uint8_t ENABLE_PIN = 30;
#elif DRIVER_SLOT == SLOT_E2
constexpr char SLOT_NAME[] = "E2";
constexpr uint8_t STEP_PIN = 59;
constexpr uint8_t DIR_PIN = 57;
constexpr uint8_t ENABLE_PIN = 40;
#else
#error "Unbekannter DRIVER_SLOT. Erlaubt: SLOT_X, SLOT_Y, SLOT_Z, SLOT_E0, SLOT_E1, SLOT_E2."
#endif

constexpr unsigned int TEST_PULSES = 400;
constexpr unsigned int HALF_PERIOD_US = 1500;

void disableDriver() {
  digitalWrite(ENABLE_PIN, HIGH);  // TMC2209: ENABLE ist aktiv LOW
}

void moveTest(bool forward) {
  digitalWrite(DIR_PIN, forward ? HIGH : LOW);
  digitalWrite(ENABLE_PIN, LOW);
  delay(100);

  for (unsigned int pulse = 0; pulse < TEST_PULSES; ++pulse) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(HALF_PERIOD_US);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(HALF_PERIOD_US);
  }

  disableDriver();
  Serial.println(F("Testbewegung beendet; Treiber wieder deaktiviert."));
}

void printHelp() {
  Serial.print(F("FYSETC F6 V1.4 / "));
  Serial.print(SLOT_NAME);
  Serial.println(F(" Motortest"));
  #if DRIVER_SLOT == SLOT_Z
  Serial.println(F("Hinweis: Der Z-Treiber versorgt die Motorbuchsen Z1-MOT und Z2-MOT in Reihe."));
  Serial.println(F("Bei nur einem Z-Motor muss die andere Z-MOT-Buchse gebrueckt sein."));
  #endif
  Serial.println(F("f = langsame Testbewegung vorwaerts"));
  Serial.println(F("r = langsame Testbewegung rueckwaerts"));
  Serial.println(F("Andere Eingaben bewegen den Motor nicht."));
}

void setup() {
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);

  digitalWrite(STEP_PIN, LOW);
  digitalWrite(DIR_PIN, LOW);
  disableDriver();

  Serial.begin(115200);
  delay(300);
  printHelp();
}

void loop() {
  if (Serial.available() == 0) {
    return;
  }

  const char command = Serial.read();
  if (command == 'f' || command == 'F') {
    Serial.println(F("Vorwaertstest startet."));
    moveTest(true);
  } else if (command == 'r' || command == 'R') {
    Serial.println(F("Rueckwaertstest startet."));
    moveTest(false);
  }
}
