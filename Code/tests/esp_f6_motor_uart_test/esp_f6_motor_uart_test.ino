#include <Arduino.h>
#include <LiquidCrystal.h>

// USB Serial = serieller Monitor am PC.
// UART1 = Verbindung zum FYSETC F6.
static HardwareSerial f6Serial(1);
static constexpr uint32_t BAUD=115200;
static constexpr uint8_t F6_RX_PIN=44;
static constexpr uint8_t F6_TX_PIN=43;
static constexpr uint16_t DEFAULT_PULSES=400;

// Bestehende LCD-Verkabelung: RS, E, D4, D5, D6, D7.
static LiquidCrystal lcd(1,2,40,41,42,21);

static char pcLine[40];
static size_t pcLength=0;
static char f6Line[96];
static size_t f6Length=0;

static void lcdLine(uint8_t row,const char *text) {
  char output[17];
  size_t length=strlen(text);
  if(length>16) length=16;
  memcpy(output,text,length);
  memset(output+length,' ',16-length);
  output[16]='\0';
  lcd.setCursor(0,row);
  lcd.print(output);
}

static void showHelp() {
  Serial.println();
  Serial.println(F("ESP <-> F6 UART- und Motorrichtungstest"));
  Serial.println(F("p       = UART pruefen"));
  Serial.println(F("u+ / u- = oben, Weiss, E1"));
  Serial.println(F("r+ / r- = rechts, Rot, Y"));
  Serial.println(F("f+ / f- = vorne, Gruen, E0"));
  Serial.println(F("d+ / d- = unten, Gelb, E2"));
  Serial.println(F("l+ / l- = links, Orange, X"));
  Serial.println(F("b+ / b- = hinten, Blau, Z1"));
  Serial.println(F("Optional eigene Impulszahl: u+ 200"));
  Serial.println(F("x       = laufenden Test abbrechen"));
  Serial.println(F("h       = Hilfe"));
}

static bool validFace(char face) {
  return face=='U' || face=='R' || face=='F' || face=='D' || face=='L' || face=='B';
}

static void handlePcLine(char *line) {
  while(*line==' ') ++line;
  if(!*line) return;
  if((line[0]=='h' || line[0]=='H') && line[1]=='\0') {showHelp();return;}
  if((line[0]=='p' || line[0]=='P') && line[1]=='\0') {
    f6Serial.println(F("PING"));
    Serial.println(F("ESP > F6: PING"));
    lcdLine(0,"UART-Test");lcdLine(1,"Warte auf F6");
    return;
  }
  if((line[0]=='x' || line[0]=='X') && line[1]=='\0') {
    f6Serial.println(F("!"));
    Serial.println(F("ESP > F6: STOP"));
    lcdLine(0,"STOP gesendet");lcdLine(1,"Warte auf F6");
    return;
  }

  const char face=(char)toupper((unsigned char)line[0]);
  const char direction=line[1];
  if(!validFace(face) || (direction!='+' && direction!='-')) {
    Serial.println(F("Ungueltig. Beispiel: u+ oder r-"));
    return;
  }

  char *numberText=line+2;
  while(*numberText==' ') ++numberText;
  unsigned long pulses=*numberText ? strtoul(numberText,nullptr,10) : DEFAULT_PULSES;
  if(pulses<1 || pulses>5000) {
    Serial.println(F("Impulszahl muss zwischen 1 und 5000 liegen."));
    return;
  }

  f6Serial.print(F("TEST "));
  f6Serial.print(face);f6Serial.print(' ');f6Serial.print(direction);
  f6Serial.print(' ');f6Serial.println(pulses);
  Serial.print(F("ESP > F6: TEST "));
  Serial.print(face);Serial.print(' ');Serial.print(direction);
  Serial.print(' ');Serial.println(pulses);
  char display[17];
  snprintf(display,sizeof(display),"Motor %c%c",face,direction);
  lcdLine(0,display);lcdLine(1,"Warte auf F6");
}

static void handleF6Line(char *line) {
  Serial.print(F("F6 > ESP: "));Serial.println(line);
  if(!strcmp(line,"READY") || !strcmp(line,"PONG")) {
    lcdLine(0,"F6 verbunden");lcdLine(1,"Motor waehlen");
  } else if(!strncmp(line,"START ",6)) {
    lcdLine(0,"Motor dreht...");lcdLine(1,line+6);
  } else if(!strncmp(line,"DONE ",5)) {
    lcdLine(0,"Test beendet");lcdLine(1,line+5);
  } else if(!strncmp(line,"ERR ",4)) {
    lcdLine(0,"F6 FEHLER");lcdLine(1,line+4);
  }
}

void setup() {
  Serial.begin(BAUD);
  f6Serial.begin(BAUD,SERIAL_8N1,F6_RX_PIN,F6_TX_PIN);
  lcd.begin(16,2);
  lcdLine(0,"UART Motortest");lcdLine(1,"Startet...");
  delay(1200);
  showHelp();
  f6Serial.println(F("PING"));
}

void loop() {
  while(Serial.available()) {
    const char c=(char)Serial.read();
    if(c=='\r') continue;
    if(c=='\n') {
      pcLine[pcLength]='\0';handlePcLine(pcLine);pcLength=0;
    } else if(pcLength+1<sizeof(pcLine)) {
      pcLine[pcLength++]=c;
    } else {
      pcLength=0;Serial.println(F("Eingabe zu lang."));
    }
  }

  while(f6Serial.available()) {
    const char c=(char)f6Serial.read();
    if(c=='\r') continue;
    if(c=='\n') {
      f6Line[f6Length]='\0';handleF6Line(f6Line);f6Length=0;
    } else if(f6Length+1<sizeof(f6Line)) {
      f6Line[f6Length++]=c;
    } else {
      f6Length=0;Serial.println(F("F6-Antwort zu lang."));
    }
  }
}

