#include <Arduino.h>

static constexpr uint32_t BAUD=115200;
static constexpr uint16_t HALF_PERIOD_US=1000;
static constexpr uint16_t MAX_PULSES=5000;

struct MotorConfig {
  char face;
  const char *name;
  uint8_t stepPin;
  uint8_t dirPin;
  uint8_t enablePin;
};

// Bestaetigte Flaechenzuordnung am FYSETC F6 V1.4.
static const MotorConfig motors[] = {
  {'U',"E1",36,34,30}, // oben, Weiss
  {'R',"Y", 60,61,56}, // rechts, Rot
  {'F',"E0",26,28,24}, // vorne, Gruen
  {'D',"E2",59,57,40}, // unten, Gelb
  {'L',"X", 54,55,38}, // links, Orange
  {'B',"Z1",43,48,58}  // hinten, Blau
};

static char receiveLine[80];
static size_t receiveLength=0;
static bool abortRequested=false;
static bool running=false;

static const MotorConfig *findMotor(char face) {
  for(const MotorConfig &motor:motors) if(motor.face==face) return &motor;
  return nullptr;
}

static void disableAll() {
  for(const MotorConfig &motor:motors) digitalWrite(motor.enablePin,HIGH);
}

static void checkStop() {
  while(Serial1.available()) if(Serial1.read()=='!') abortRequested=true;
  while(Serial.available()) {
    const char c=(char)Serial.read();
    if(c=='!' || c=='x' || c=='X') abortRequested=true;
  }
}

static void runTest(const MotorConfig &motor,char direction,uint16_t pulses) {
  if(running) {Serial1.println(F("ERR BUSY"));return;}
  running=true;abortRequested=false;
  disableAll();
  digitalWrite(motor.dirPin,direction=='+' ? HIGH : LOW);
  digitalWrite(motor.enablePin,LOW);
  delay(100);

  Serial1.print(F("START "));Serial1.print(motor.face);Serial1.print(direction);
  Serial1.print(' ');Serial1.print(motor.name);Serial1.print(' ');Serial1.println(pulses);
  Serial.print(F("Test: "));Serial.print(motor.face);Serial.print(direction);
  Serial.print(F(" / Slot "));Serial.print(motor.name);Serial.print(F(" / "));Serial.print(pulses);Serial.println(F(" Impulse"));

  uint16_t completed=0;
  for(;completed<pulses;++completed) {
    checkStop();
    if(abortRequested) break;
    digitalWrite(motor.stepPin,HIGH);delayMicroseconds(HALF_PERIOD_US);
    digitalWrite(motor.stepPin,LOW); delayMicroseconds(HALF_PERIOD_US);
  }

  digitalWrite(motor.enablePin,HIGH);
  running=false;
  if(abortRequested) {
    Serial1.print(F("ERR ABORTED "));Serial1.println(completed);
  } else {
    Serial1.print(F("DONE "));Serial1.print(motor.face);Serial1.print(direction);
    Serial1.print(' ');Serial1.println(completed);
  }
}

static void handleLine(char *line) {
  while(*line==' ') ++line;
  if(!strcmp(line,"PING")) {Serial1.println(F("PONG"));return;}
  if(!strcmp(line,"!")) {abortRequested=true;return;}
  if(strncmp(line,"TEST ",5)) {Serial1.println(F("ERR BAD_COMMAND"));return;}

  char face=0,direction=0,extra=0;
  unsigned int pulses=0;
  if(sscanf(line+5," %c %c %u %c",&face,&direction,&pulses,&extra)!=3) {
    Serial1.println(F("ERR BAD_TEST"));return;
  }
  const MotorConfig *motor=findMotor(face);
  if(!motor || (direction!='+' && direction!='-') || pulses<1 || pulses>MAX_PULSES) {
    Serial1.println(F("ERR BAD_VALUES"));return;
  }
  runTest(*motor,direction,(uint16_t)pulses);
}

void setup() {
  for(const MotorConfig &motor:motors) {
    pinMode(motor.stepPin,OUTPUT);digitalWrite(motor.stepPin,LOW);
    pinMode(motor.dirPin,OUTPUT); digitalWrite(motor.dirPin,LOW);
    pinMode(motor.enablePin,OUTPUT);digitalWrite(motor.enablePin,HIGH);
  }
  Serial.begin(BAUD);
  Serial1.begin(BAUD);
  delay(300);
  Serial.println(F("F6 ESP-UART Motorrichtungstest; Treiber sind deaktiviert."));
  Serial1.println(F("READY"));
}

void loop() {
  while(Serial1.available()) {
    const char c=(char)Serial1.read();
    if(c=='!') {abortRequested=true;continue;}
    if(c=='\r') continue;
    if(c=='\n') {
      receiveLine[receiveLength]='\0';handleLine(receiveLine);receiveLength=0;
    } else if(receiveLength+1<sizeof(receiveLine)) {
      receiveLine[receiveLength++]=c;
    } else {
      receiveLength=0;Serial1.println(F("ERR LINE_TOO_LONG"));
    }
  }
}

