#include <Arduino.h>
#include <TMCStepper.h>
#include <avr/interrupt.h>

static const char BUILD_TAG[]="ROBOT-F6-2026-09-17-1";

// FYSETC F6 V1.4: Kommunikation mit dem ESP32-S3 ueber Serial1.
// F6 D19/RX1 empfaengt vom ESP TX/GPIO43.
// F6 D18/TX1 sendet ueber einen 5-V->3,3-V-Spannungsteiler an ESP RX/GPIO44.
static constexpr uint32_t PC_BAUD=115200;
static constexpr uint32_t ESP_BAUD=250000;
static constexpr bool MOTION_DEBUG=false;

// ============================================================================
// EINFACHE BEWEGUNGSEINSTELLUNGEN
// ============================================================================
// Bei 2 Mikroschritten benoetigen die 1,8-Grad-Motoren 100 Impulse fuer 90 Grad.
// Die TMC-Interpolation bleibt aktiv und glaettet intern auf 256 Mikroschritte.
static constexpr uint16_t STEPS_PER_90_DEGREES=100;

// Zwischenprofil zum schrittweisen Herantasten an etwa drei Sekunden reine
// Motorzeit. 2400 Grad/s entsprechen etwa 400 Motor-Umdrehungen/min.
static constexpr uint16_t START_SPEED_DEG_PER_SEC=500;
static constexpr uint16_t MAX_SPEED_DEG_PER_SEC=2400;
// 180-Grad-Zuege haben eine laengere Beschleunigungsstrecke und duerfen deshalb
// eine etwas hoehere Spitzengeschwindigkeit erreichen.
static constexpr uint16_t MAX_SPEED_180_DEG_PER_SEC=2600;

// Anzahl der Impulse zum Beschleunigen und Abbremsen.
// Groesser = weichere, laengere Rampe; kleiner = haertere Beschleunigung.
static constexpr uint16_t ACCELERATION_STEPS=8;
static constexpr uint16_t ACCELERATION_180_STEPS=14;

// Der Scramble benutzt dieses feste Tempo ohne Beschleunigungsrampe. Dieser
// Wert ist unabhaengig von START_SPEED und MAX_SPEED des Loesevorgangs.
static constexpr uint16_t SCRAMBLE_SPEED_DEG_PER_SEC=300;

// Laufstrom pro Motor in Milliampere RMS. 1700 bedeutet 1,7 A RMS.
// Dieser Wert wird beim Start per TMC-UART an alle sechs Treiber geschrieben.
static constexpr uint16_t MOTOR_RMS_CURRENT_MA=1700;

// Nicht drehende Motoren halten mit 35 % des Laufstroms ihre Cube-Seite fest.
static constexpr float MOTOR_HOLD_MULTIPLIER=0.35F;

// Mikroschritte werden ebenfalls per UART gesetzt. Der Test ergab bei 16
// Mikroschritten 800 Impulse pro 90 Grad; bei 2 sind es entsprechend 100.
static constexpr uint16_t DRIVER_MICROSTEPS=2;

// true = SpreadCycle fuer mehr Drehmoment bei schnellen Bewegungen;
// false = StealthChop fuer leiseren Lauf.
static constexpr bool USE_SPREADCYCLE=true;

static constexpr float DRIVER_R_SENSE=0.110F;
static constexpr uint8_t DRIVER_ADDRESS=0b00;

// Software-UART kann bei einer einzelnen Abfrage gelegentlich ein Byte
// verpassen. Deshalb wird ein Treiber erst nach mehreren Fehlversuchen als
// nicht erreichbar gemeldet.
static constexpr uint8_t DRIVER_UART_ATTEMPTS=5;
static constexpr uint16_t DRIVER_UART_RETRY_DELAY_MS=40;

// Getrennte, bereits getestete TMC-UART-Leitungen des F6.
static TMC2209Stepper tmcX (72,71,DRIVER_R_SENSE,DRIVER_ADDRESS);
static TMC2209Stepper tmcY (73,78,DRIVER_R_SENSE,DRIVER_ADDRESS);
static TMC2209Stepper tmcZ (75,79,DRIVER_R_SENSE,DRIVER_ADDRESS);
static TMC2209Stepper tmcE0(77,81,DRIVER_R_SENSE,DRIVER_ADDRESS);
static TMC2209Stepper tmcE1(76,80,DRIVER_R_SENSE,DRIVER_ADDRESS);
static TMC2209Stepper tmcE2(62,82,DRIVER_R_SENSE,DRIVER_ADDRESS);

// Erst auf true setzen, nachdem Zuordnung, Drehrichtung, Mikroschritte und
// Schritte pro Vierteldrehung am aufgebauten Roboter bestaetigt wurden.
static constexpr bool MOTION_CONFIGURATION_CONFIRMED=true;

struct MotorConfig {
  char face;
  uint8_t stepPin;
  uint8_t dirPin;
  uint8_t enablePin;
  uint16_t quarterSteps;
  bool clockwiseDirHigh;
  TMC2209Stepper *driver;
};

// Bestaetigte Seiten-/Steckplatzzuordnung:
// U/weiss=E1, R/rot=Y, F/gruen=E0, D/gelb=E2, L/orange=X, B/blau=Z1.
// Gemessen wurden bei 16 Mikroschritten 800 STEP-Impulse pro 90 Grad. Der
// Produktionsbetrieb nutzt 2 Mikroschritte und deshalb 100 Impulse.
// Gemessen: DIR HIGH ('+' im Test) dreht alle Motoren gegen den Uhrzeigersinn,
// wenn auf die Wellen-/Adapterseite geschaut wird. Da diese Seite zum Cube
// zeigt, entspricht das von aussen auf die Cube-Flaeche gesehen einem
// Uhrzeigersinn-Zug. Deshalb ist clockwiseDirHigh bei allen Motoren true.
static MotorConfig motors[] = {
  {'U',36,34,30,STEPS_PER_90_DEGREES,true,&tmcE1}, // weiss, oben, E1
  {'R',60,61,56,STEPS_PER_90_DEGREES,true,&tmcY }, // rot, rechts, Y
  {'F',26,28,24,STEPS_PER_90_DEGREES,true,&tmcE0}, // gruen, vorne, E0
  {'D',59,57,40,STEPS_PER_90_DEGREES,true,&tmcE2}, // gelb, unten, E2
  {'L',54,55,38,STEPS_PER_90_DEGREES,true,&tmcX }, // orange, links, X
  {'B',43,48,58,STEPS_PER_90_DEGREES,true,&tmcZ }  // blau, hinten, Z1/Z-Treiber
};

// STEP ist pro Impuls eine halbe Periode HIGH und eine halbe Periode LOW.
static constexpr uint16_t SLOW_HALF_PERIOD_US=
  (uint16_t)(45000000UL/((uint32_t)STEPS_PER_90_DEGREES*(uint32_t)START_SPEED_DEG_PER_SEC));
static constexpr uint16_t FAST_HALF_PERIOD_US=
  (uint16_t)(45000000UL/((uint32_t)STEPS_PER_90_DEGREES*(uint32_t)MAX_SPEED_DEG_PER_SEC));
static constexpr uint16_t FAST_180_HALF_PERIOD_US=
  (uint16_t)(45000000UL/((uint32_t)STEPS_PER_90_DEGREES*(uint32_t)MAX_SPEED_180_DEG_PER_SEC));
static constexpr uint16_t SCRAMBLE_HALF_PERIOD_US=
  (uint16_t)(45000000UL/((uint32_t)STEPS_PER_90_DEGREES*(uint32_t)SCRAMBLE_SPEED_DEG_PER_SEC));
static_assert(SLOW_HALF_PERIOD_US>FAST_HALF_PERIOD_US,
              "START_SPEED muss kleiner als MAX_SPEED sein");
static_assert(SLOW_HALF_PERIOD_US>FAST_180_HALF_PERIOD_US,
              "START_SPEED muss kleiner als MAX_SPEED_180 sein");
static constexpr size_t MAX_MOVES=30;
static constexpr uint8_t DRIVER_ENABLE_DELAY_MS=10;
static constexpr uint16_t EMERGENCY_POLL_US=500;
static constexpr uint16_t TIMER1_TICKS_PER_US=(uint16_t)(F_CPU/8UL/1000000UL);
static constexpr uint16_t TIMER1_MIN_LEAD_TICKS=32;
static_assert(F_CPU==16000000UL,"Das Timerprofil erwartet das F6 mit 16 MHz");

struct ParsedMove {
  uint8_t motorIndex;
  uint8_t quarterTurns;
  bool inverse;
};

// Muss vor den ersten Funktionen stehen, damit der Arduino-.ino-Preprozessor
// die automatisch erzeugten Funktionsdeklarationen korrekt anlegen kann.
struct TimerTurn {
  MotorConfig *motor;
  volatile uint8_t *stepPort;
  uint8_t stepMask;
  uint32_t pulses;
  volatile uint32_t completedPulses;
  volatile uint16_t nextEdgeTicks;
  volatile uint16_t halfPeriodTicks;
  bool scramble;
  bool halfTurn;
  volatile bool stepHigh;
  volatile bool finished;
};

static char receiveLine[192];
static size_t receiveLength=0;
static bool abortRequested=false;
static bool running=false;
static bool driversReady=false;
static char failedDriverFaces[8]="";
static uint32_t lastDriverRetry=0;
static TimerTurn timerTurns[2];
static volatile uint8_t timerTurnCount=0;
static volatile bool timerMotionActive=false;

static bool configureDriver(MotorConfig &motor) {
  TMC2209Stepper &driver=*motor.driver;
  driver.beginSerial(115200);
  driver.begin();
  delay(30);

  uint8_t connection=0xFF;
  uint8_t version=0x00;
  uint8_t usedAttempts=0;

  for(uint8_t attempt=1;attempt<=DRIVER_UART_ATTEMPTS;++attempt) {
    usedAttempts=attempt;
    connection=driver.test_connection();
    version=driver.version();
    if(connection==0 && version==0x21) break;
    delay(DRIVER_UART_RETRY_DELAY_MS);
  }

  Serial.print(F("TMC "));Serial.print(motor.face);Serial.print(F(" / UART: connection="));
  Serial.print(connection);Serial.print(F(", version=0x"));
  if(version<0x10) Serial.print('0');Serial.print(version,HEX);
  Serial.print(F(", Versuche="));Serial.println(usedAttempts);
  if(connection!=0 || version!=0x21) return false;

  // Der TMC2209 startet normalerweise mit 256 Mikroschritten. Erzwinge, dass
  // MRES aus dem UART-Register und nicht aus den Konfigurationspins kommt.
  // Ohne diese beiden Bits kann microsteps(2) wirkungslos bleiben.
  driver.pdn_disable(true);
  driver.mstep_reg_select(true);
  driver.toff(5);
  // Die BTT-Treiber kommen ab Werk mit analoger VREF-Stromskalierung. Fuer
  // einen wirksamen per UART gesetzten Strom muss die interne Referenz aktiv
  // sein; die externen 0,110-Ohm-Messwiderstaende bleiben in Benutzung.
  driver.I_scale_analog(false);
  driver.internal_Rsense(false);
  driver.rms_current(MOTOR_RMS_CURRENT_MA,MOTOR_HOLD_MULTIPLIER);
  driver.microsteps(DRIVER_MICROSTEPS);
  driver.intpol(true);
  driver.en_spreadCycle(USE_SPREADCYCLE);
  driver.pwm_autoscale(true);
  // Alle Schattenregister noch einmal gemeinsam schreiben. Das verhindert,
  // dass eine einzelne verlorene Software-UART-Uebertragung MRES auf 256
  // stehen laesst.
  driver.push();
  delay(10);

  uint16_t configuredMicrosteps=driver.microsteps();
  bool registerSelection=driver.mstep_reg_select();
  for(uint8_t attempt=1;
      attempt<DRIVER_UART_ATTEMPTS &&
      (configuredMicrosteps!=DRIVER_MICROSTEPS || !registerSelection);
      ++attempt) {
    driver.pdn_disable(true);
    driver.mstep_reg_select(true);
    driver.microsteps(DRIVER_MICROSTEPS);
    driver.intpol(true);
    delay(DRIVER_UART_RETRY_DELAY_MS);
    configuredMicrosteps=driver.microsteps();
    registerSelection=driver.mstep_reg_select();
  }

  Serial.print(F("TMC "));Serial.print(motor.face);
  Serial.print(F(" eingestellt: Strom="));Serial.print(driver.rms_current());
  Serial.print(F("mA, Mikroschritte="));Serial.print(configuredMicrosteps);
  Serial.print(F(", UART-MSTEP="));Serial.println(registerSelection ? F("JA") : F("NEIN"));
  if(configuredMicrosteps!=DRIVER_MICROSTEPS || !registerSelection) {
    Serial.print(F("FEHLER: TMC "));Serial.print(motor.face);
    Serial.println(F(" hat die Mikroschritt-Einstellung nicht uebernommen."));
    return false;
  }
  return true;
}

static bool configureAllDrivers() {
  failedDriverFaces[0]='\0';
  bool success=true;
  for(MotorConfig &motor:motors) {
    if(!configureDriver(motor)) {
      success=false;
      const size_t length=strlen(failedDriverFaces);
      if(length+1<sizeof(failedDriverFaces)) {
        failedDriverFaces[length]=motor.face;
        failedDriverFaces[length+1]='\0';
      }
    }
  }
  return success;
}

static void sendDriverError(uint32_t jobId) {
  Serial1.print(F("ERR "));Serial1.print(jobId);Serial1.print(F(" DRIVER_UART_"));
  Serial1.println(failedDriverFaces[0] ? failedDriverFaces : "UNKNOWN");
}

static void setDriversEnabled(bool enabled) {
  for(const MotorConfig &motor:motors) digitalWrite(motor.enablePin,enabled ? LOW : HIGH);
}

static int8_t findMotor(char face) {
  for(uint8_t i=0;i<sizeof(motors)/sizeof(motors[0]);++i)
    if(motors[i].face==face) return (int8_t)i;
  return -1;
}

static void checkEmergencyStop() {
  while(Serial1.available()) if(Serial1.read()=='!') abortRequested=true;
  while(Serial.available()) {
    const char c=(char)Serial.read();
    if(c=='!' || c=='x' || c=='X') abortRequested=true;
  }
}

static uint16_t pulseHalfPeriodUs(uint32_t pulse,uint32_t total,bool scramble,bool halfTurn) {
  if(scramble) return SCRAMBLE_HALF_PERIOD_US;
  uint32_t ramp=halfTurn ? ACCELERATION_180_STEPS : ACCELERATION_STEPS;
  if(ramp>total/2) ramp=total/2;
  uint32_t position=pulse;
  if(total-1-pulse<position) position=total-1-pulse;
  const uint16_t fastHalfPeriod=halfTurn ? FAST_180_HALF_PERIOD_US : FAST_HALF_PERIOD_US;
  if(position>=ramp || ramp==0) return fastHalfPeriod;
  const uint32_t difference=SLOW_HALF_PERIOD_US-fastHalfPeriod;
  return (uint16_t)(SLOW_HALF_PERIOD_US-(difference*position/ramp));
}

static inline void writeTimerStep(TimerTurn &turn,bool high) {
  if(high) *turn.stepPort|=turn.stepMask;
  else *turn.stepPort&=(uint8_t)~turn.stepMask;
}

static void setupMotionTimer() {
  noInterrupts();
  TCCR1A=0;
  TCCR1B=_BV(CS11); // Timer1 frei laufend, Prescaler 8: 0,5 us pro Tick bei 16 MHz.
  TIMSK1&=(uint8_t)~_BV(OCIE1A);
  TIFR1=_BV(OCF1A);
  interrupts();
}

static void prepareTimerTurn(TimerTurn &turn,const ParsedMove &move,bool scramble) {
  turn.motor=&motors[move.motorIndex];
  turn.stepPort=portOutputRegister(digitalPinToPort(turn.motor->stepPin));
  turn.stepMask=digitalPinToBitMask(turn.motor->stepPin);
  turn.pulses=(uint32_t)turn.motor->quarterSteps*move.quarterTurns;
  turn.completedPulses=0;
  turn.nextEdgeTicks=0;
  turn.scramble=scramble;
  turn.halfTurn=move.quarterTurns==2;
  turn.halfPeriodTicks=(uint16_t)(pulseHalfPeriodUs(
    0,turn.pulses,turn.scramble,turn.halfTurn)*TIMER1_TICKS_PER_US);
  turn.stepHigh=false;
  turn.finished=false;
  writeTimerStep(turn,false);
}

ISR(TIMER1_COMPA_vect) {
  const uint16_t now=TCNT1;
  for(uint8_t i=0;i<timerTurnCount;++i) {
    TimerTurn &turn=timerTurns[i];
    if(turn.finished || (int16_t)(now-turn.nextEdgeTicks)<0) continue;

    if(!turn.stepHigh) {
      writeTimerStep(turn,true);
      turn.stepHigh=true;
      turn.nextEdgeTicks+=(uint16_t)turn.halfPeriodTicks;
    } else {
      writeTimerStep(turn,false);
      turn.stepHigh=false;
      if(++turn.completedPulses>=turn.pulses) {
        turn.finished=true;
      } else {
        turn.halfPeriodTicks=(uint16_t)(pulseHalfPeriodUs(
          turn.completedPulses,turn.pulses,turn.scramble,turn.halfTurn)*TIMER1_TICKS_PER_US);
        turn.nextEdgeTicks+=(uint16_t)turn.halfPeriodTicks;
      }
    }
  }

  const uint16_t scheduleNow=TCNT1;
  uint16_t nearestDelta=0x7fff;
  bool pending=false;
  for(uint8_t i=0;i<timerTurnCount;++i) {
    TimerTurn &turn=timerTurns[i];
    if(turn.finished) continue;
    int16_t delta=(int16_t)(turn.nextEdgeTicks-scheduleNow);
    if(delta<(int16_t)TIMER1_MIN_LEAD_TICKS) {
      turn.nextEdgeTicks=(uint16_t)(scheduleNow+TIMER1_MIN_LEAD_TICKS);
      delta=TIMER1_MIN_LEAD_TICKS;
    }
    if((uint16_t)delta<nearestDelta) nearestDelta=(uint16_t)delta;
    pending=true;
  }

  if(pending) {
    OCR1A=(uint16_t)(scheduleNow+nearestDelta);
  } else {
    TIMSK1&=(uint8_t)~_BV(OCIE1A);
    timerMotionActive=false;
  }
}

static void stopTimerMotion() {
  noInterrupts();
  TIMSK1&=(uint8_t)~_BV(OCIE1A);
  for(uint8_t i=0;i<timerTurnCount;++i) {
    writeTimerStep(timerTurns[i],false);
    timerTurns[i].stepHigh=false;
    timerTurns[i].finished=true;
  }
  timerMotionActive=false;
  interrupts();
}

static bool areOppositeFaces(const ParsedMove &first,const ParsedMove &second) {
  const char a=motors[first.motorIndex].face;
  const char b=motors[second.motorIndex].face;
  return (a=='U' && b=='D') || (a=='D' && b=='U') ||
         (a=='R' && b=='L') || (a=='L' && b=='R') ||
         (a=='F' && b=='B') || (a=='B' && b=='F');
}

static bool runTimerMotion(const ParsedMove &first,const ParsedMove *second,bool scramble) {
  MotorConfig &firstMotor=motors[first.motorIndex];
  digitalWrite(firstMotor.dirPin,
               (first.inverse ? !firstMotor.clockwiseDirHigh : firstMotor.clockwiseDirHigh) ? HIGH : LOW);
  if(second) {
    MotorConfig &secondMotor=motors[second->motorIndex];
    digitalWrite(secondMotor.dirPin,
                 (second->inverse ? !secondMotor.clockwiseDirHigh : secondMotor.clockwiseDirHigh) ? HIGH : LOW);
  }
  delayMicroseconds(20);

  noInterrupts();
  timerTurnCount=second ? 2 : 1;
  prepareTimerTurn(timerTurns[0],first,scramble);
  if(second) prepareTimerTurn(timerTurns[1],*second,scramble);
  const uint16_t startsAt=(uint16_t)(TCNT1+TIMER1_MIN_LEAD_TICKS);
  for(uint8_t i=0;i<timerTurnCount;++i) timerTurns[i].nextEdgeTicks=startsAt;
  timerMotionActive=true;
  OCR1A=startsAt;
  TIFR1=_BV(OCF1A);
  TIMSK1|=_BV(OCIE1A);
  interrupts();

  uint32_t lastEmergencyPoll=micros();
  while(timerMotionActive) {
    const uint32_t now=micros();
    if(now-lastEmergencyPoll>=EMERGENCY_POLL_US) {
      lastEmergencyPoll=now;
      checkEmergencyStop();
      if(abortRequested) {
        stopTimerMotion();
        return false;
      }
    }
  }
  return true;
}

static bool turnMotor(const ParsedMove &move,bool scramble) {
  const uint32_t turnStartedAt=micros();
  const bool success=runTimerMotion(move,nullptr,scramble);
  if(MOTION_DEBUG && success) {
    Serial.print(F("Drehzeit "));Serial.print(motors[move.motorIndex].face);
    if(move.quarterTurns==2) Serial.print('2');
    else if(move.inverse) Serial.print('\'');
    Serial.print(F(": "));Serial.print((micros()-turnStartedAt)/1000UL);Serial.println(F(" ms"));
  }
  return success;
}

static bool turnMotorPair(const ParsedMove &first,const ParsedMove &second,bool scramble) {
  const uint32_t turnStartedAt=micros();
  const bool success=runTimerMotion(first,&second,scramble);
  if(MOTION_DEBUG && success) {
    Serial.print(F("Parallele Drehzeit "));Serial.print(motors[first.motorIndex].face);
    Serial.print('+');Serial.print(motors[second.motorIndex].face);Serial.print(F(": "));
    Serial.print((micros()-turnStartedAt)/1000UL);Serial.println(F(" ms"));
  }
  return success;
}

static bool parseMoves(char *text,ParsedMove *parsed,size_t &count) {
  count=0;
  char *save=nullptr;
  for(char *token=strtok_r(text," ",&save);token;token=strtok_r(nullptr," ",&save)) {
    if(count>=MAX_MOVES) return false;
    const int8_t index=findMotor(token[0]);
    if(index<0) return false;
    ParsedMove move={(uint8_t)index,1,false};
    if(token[1]=='\0') {}
    else if(token[1]=='\'' && token[2]=='\0') move.inverse=true;
    else if(token[1]=='2' && token[2]=='\0') move.quarterTurns=2;
    else return false;
    parsed[count++]=move;
  }
  return count>0;
}

static void executeRun(uint32_t jobId,char *moveText,bool scramble) {
  if(!MOTION_CONFIGURATION_CONFIRMED) {
    Serial1.print(F("ERR "));Serial1.print(jobId);Serial1.println(F(" CONFIG_REQUIRED"));
    return;
  }
  if(!driversReady) {
    sendDriverError(jobId);
    return;
  }
  if(running) {
    Serial1.print(F("ERR "));Serial1.print(jobId);Serial1.println(F(" BUSY"));
    return;
  }

  ParsedMove parsed[MAX_MOVES];
  size_t moveCount=0;
  if(!parseMoves(moveText,parsed,moveCount)) {
    Serial1.print(F("ERR "));Serial1.print(jobId);Serial1.println(F(" BAD_MOVES"));
    return;
  }

  running=true;abortRequested=false;
  setDriversEnabled(true);delay(DRIVER_ENABLE_DELAY_MS);
  Serial1.print(F("START "));Serial1.print(jobId);Serial1.print(' ');Serial1.println(moveCount);
  const uint32_t startedAt=millis();
  if(MOTION_DEBUG) {
    if(scramble) {
      Serial.print(F("Feste Scramble-Geschwindigkeit: "));
      Serial.print(SCRAMBLE_SPEED_DEG_PER_SEC);Serial.println(F(" Grad/s"));
    } else {
      Serial.print(F("Loese-Geschwindigkeit bis: "));
      Serial.print(MAX_SPEED_DEG_PER_SEC);Serial.print('/');
      Serial.print(MAX_SPEED_180_DEG_PER_SEC);Serial.println(F(" Grad/s (90/180)"));
    }
  }

  size_t completed=0;
  while(completed<moveCount) {
    const bool parallel=completed+1<moveCount && areOppositeFaces(parsed[completed],parsed[completed+1]);
    const bool success=parallel ? turnMotorPair(parsed[completed],parsed[completed+1],scramble)
                                : turnMotor(parsed[completed],scramble);
    if(!success) break;
    completed+=parallel ? 2 : 1;
    Serial1.print(F("MOVE "));Serial1.print(jobId);Serial1.print(' ');
    Serial1.print(completed);Serial1.print(' ');Serial1.println(moveCount);
  }

  setDriversEnabled(false);running=false;
  if(abortRequested) {
    Serial1.print(F("ERR "));Serial1.print(jobId);Serial1.println(F(" ABORTED"));
  } else {
    Serial1.print(F("DONE "));Serial1.print(jobId);Serial1.print(' ');Serial1.println(millis()-startedAt);
  }
}

static void handleLine(char *line) {
  while(*line==' ') ++line;
  if(!strcmp(line,"PING")) {
    if(driversReady) Serial1.println(F("PONG"));
    else sendDriverError(0);
    return;
  }
  if(!strcmp(line,"!")) { abortRequested=true;return; }
  bool scramble=false;
  char *idText=nullptr;
  if(!strncmp(line,"RUN ",4)) idText=line+4;
  else if(!strncmp(line,"SCRAMBLE ",9)) {scramble=true;idText=line+9;}
  else {Serial1.println(F("ERR 0 BAD_COMMAND"));return;}

  char *separator=strchr(idText,' ');
  if(!separator) { Serial1.println(F("ERR 0 BAD_RUN"));return; }
  *separator='\0';
  const uint32_t jobId=strtoul(idText,nullptr,10);
  if(jobId==0) { Serial1.println(F("ERR 0 BAD_ID"));return; }
  executeRun(jobId,separator+1,scramble);
}

void setup() {
  for(const MotorConfig &motor:motors) {
    pinMode(motor.stepPin,OUTPUT);digitalWrite(motor.stepPin,LOW);
    pinMode(motor.dirPin,OUTPUT);digitalWrite(motor.dirPin,LOW);
    pinMode(motor.enablePin,OUTPUT);digitalWrite(motor.enablePin,HIGH);
  }
  setupMotionTimer();
  Serial.begin(PC_BAUD);
  Serial1.begin(ESP_BAUD);
  delay(300);
  Serial.println(F("F6 Rubiks-Cube Bewegungscontroller"));
  Serial.print(F("Programmversion: "));Serial.println(BUILD_TAG);
  Serial.print(F("Bewegungsprofil: "));Serial.print(STEPS_PER_90_DEGREES);
  Serial.print(F(" Schritte/90 Grad, "));Serial.print(START_SPEED_DEG_PER_SEC);
  Serial.print('-');Serial.print(MAX_SPEED_DEG_PER_SEC);Serial.print('/');
  Serial.print(MAX_SPEED_180_DEG_PER_SEC);
  Serial.print(F(" Grad/s (Start/90/180), "));Serial.print(MOTOR_RMS_CURRENT_MA);Serial.println(F(" mA RMS"));
  Serial.println(MOTION_CONFIGURATION_CONFIRMED ? F("Motorkonfiguration freigegeben.") : F("Motorkonfiguration noch gesperrt."));
  driversReady=configureAllDrivers();
  lastDriverRetry=millis();
  Serial.print(F("TMC-Konfiguration: "));Serial.println(driversReady ? F("ALLE BEREIT") : F("FEHLER"));
  if(driversReady) Serial1.println(F("READY"));
  else sendDriverError(0);
}

void loop() {
  if(!driversReady && millis()-lastDriverRetry>=3000) {
    lastDriverRetry=millis();
    Serial.println(F("TMC-UART wird erneut geprueft..."));
    driversReady=configureAllDrivers();
    if(driversReady) {
      Serial.println(F("TMC-Konfiguration: ALLE BEREIT"));
      Serial1.println(F("READY"));
    } else {
      Serial.print(F("TMC-UART fehlt bei: "));Serial.println(failedDriverFaces);
    }
  }
  while(Serial1.available()) {
    const char c=(char)Serial1.read();
    if(c=='!') { abortRequested=true;continue; }
    if(c=='\r') continue;
    if(c=='\n') {
      receiveLine[receiveLength]='\0';
      handleLine(receiveLine);
      receiveLength=0;
    } else if(receiveLength+1<sizeof(receiveLine)) {
      receiveLine[receiveLength++]=c;
    } else {
      receiveLength=0;
      Serial1.println(F("ERR 0 LINE_TOO_LONG"));
    }
  }
}
