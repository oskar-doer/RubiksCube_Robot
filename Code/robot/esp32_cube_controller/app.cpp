#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <mbedtls/aes.h>
#include <LiquidCrystal.h>
#include <atomic>
#include "qiyi_protocol.h"
#include "solver_adapter.h"

// Bluetooth address of this robot's QiYi QYSC-S.
static const char CUBE_MAC[]="CC:A3:00:00:EB:66";
static const char BUILD_TAG[]="ROBOT-ESP-2026-09-17-3";
// LCD1602 in 4-bit mode: RS, E, D4, D5, D6, D7.
static LiquidCrystal lcd(1,2,40,41,42,21);
static bool lcdReady=false;
static bool lcdWiringMode=false;
// Separate hardware UART to the F6. USB Serial remains the PC monitor.
static HardwareSerial f6Serial(1);
static constexpr uint8_t F6_RX_PIN=44; // ESP pin marked RX; from F6 TX1 through divider
static constexpr uint8_t F6_TX_PIN=43; // ESP pin marked TX; directly to F6 RX1
static constexpr uint32_t F6_BAUD=250000;
static constexpr uint8_t START_BUTTON_PIN=0; // On-board BOOT button, active LOW
static BLEUUID serviceUUID("0000fff0-0000-1000-8000-00805f9b34fb");
static BLEUUID characteristicUUID("0000fff6-0000-1000-8000-00805f9b34fb");
struct Packet { uint16_t length; uint8_t bytes[256]; };
struct Job { cube_t cube; uint32_t generation; bool demo; };
struct Result { uint32_t generation,ms; bool demo,ok; int moves; char text[128]; };
enum class MotionGoal:uint8_t { SOLVE, SCRAMBLE };
static QueueHandle_t packets,jobs,results;
static std::atomic<bool> ready(false),busy(false),lost(false),overflow(false);
static BLEClient* client=nullptr;
static BLERemoteCharacteristic* characteristic=nullptr;
static BLEAdvertisedDevice* candidate=nullptr;
static uint8_t hardwareMac[6];
static bool online=false,haveState=false,haveTimestamp=false;
static cube_t current;
static uint32_t generation=0,lastTimestamp=0,helloAt=0,lastScan=0;
static int battery=-1;
static bool f6Ready=false,motionActive=false,motionPending=false,finalResultVisible=false;
static bool cubeVerificationPending=false;
static bool calibrationAwaitingState=false;
static uint32_t f6LastSeen=0,f6LastPing=0,motionStartedAt=0,nextJobId=1,activeJobId=0;
static uint32_t cubeVerificationDeadline=0,completedMotionMs=0,motionGenerationAtStart=0;
static uint32_t lastSolverMs=0;
static uint16_t motionDone=0,motionTotal=0;
static MotionGoal motionGoal=MotionGoal::SOLVE;
static char f6Line[192];
static size_t f6LineLength=0;
static bool buttonRaw=HIGH,buttonStable=HIGH;
static uint32_t buttonChangedAt=0;
static uint32_t buttonPressedAt=0;
static bool buttonLongHandled=false;
static constexpr uint32_t CALIBRATION_HOLD_MS=3000;

static void lcdLine(uint8_t row,const char* text) {
    if(!lcdReady || lcdWiringMode) return;
    char line[17];
    size_t n=strlen(text);if(n>16) n=16;
    memcpy(line,text,n);memset(line+n,' ',16-n);line[16]=0;
    lcd.setCursor(0,row);lcd.print(line);
}

static void lcdMessage(const char* first,const char* second) {
    lcdLine(0,first);lcdLine(1,second);
}

static void lcdTest() {
    lcdWiringMode=false;
    // Das LCD kann spaeter als der ESP mit Strom versorgt werden. Deshalb beim
    // manuellen Test erneut initialisieren und nicht nur neuen Text senden.
    lcd.begin(16,2);
    delay(50);
    lcd.clear();
    lcdMessage("LCD TEST OK","1234567890123456");
    Serial.println("LCD neu initialisiert; Testtext gesendet.");
}

static void lcdWiringTest() {
    static const uint8_t gpioPins[]={1,2,40,41,42,21};
    static const uint8_t lcdPins[]={4,6,11,12,13,14};
    lcdWiringMode=true;
    for(uint8_t pin:gpioPins) {
        pinMode(pin,OUTPUT);
        digitalWrite(pin,LOW);
    }
    Serial.println("LCD-Kabeltest startet in 3 Sekunden. Schwarze Messspitze an LCD 1 (GND).");
    Serial.println("Jeweils nur der angesagte LCD-Pin darf dabei etwa 3,3 V haben.");
    delay(3000);
    for(size_t i=0;i<sizeof(gpioPins);i++) {
        Serial.printf("10 Sekunden: LCD-Pin %u muss etwa 3,3 V haben; alle anderen Signalpins 0 V.\n",lcdPins[i]);
        digitalWrite(gpioPins[i],HIGH);
        delay(10000);
        digitalWrite(gpioPins[i],LOW);
    }
    Serial.println("LCD-Kabeltest beendet; LCD wird neu initialisiert.");
    lcdTest();
}

static void showMotionTime() {
    if(!motionActive) return;
    const uint32_t elapsed=millis()-motionStartedAt;
    char first[17],second[17];
    snprintf(first,sizeof(first),motionGoal==MotionGoal::SCRAMBLE ? "Mischt: %u/%u" : "Loest: %u/%u",motionDone,motionTotal);
    snprintf(second,sizeof(second),"Zeit: %lu.%03lus",(unsigned long)(elapsed/1000),(unsigned long)(elapsed%1000));
    lcdMessage(first,second);
}

static void showCompletedMotionTime(const char* status) {
    char second[17];
    snprintf(second,sizeof(second),"Zeit: %lu.%03lus",
             (unsigned long)(completedMotionMs/1000),(unsigned long)(completedMotionMs%1000));
    lcdMessage(status,second);
}

static void verifyFinishedCube() {
    if(!cubeVerificationPending) return;
    const bool validState=online && haveState;
    const bool solved=validState && cube_is_solved(&current);
    const bool cubeChanged=generation!=motionGenerationAtStart;
    const bool goalReached=motionGoal==MotionGoal::SOLVE ? solved : (validState && cubeChanged && !solved);
    if(!goalReached && int32_t(millis()-cubeVerificationDeadline)<0) return;

    cubeVerificationPending=false;
    char first[17],second[17];
    if(goalReached && motionGoal==MotionGoal::SOLVE) {
        const uint32_t totalMs=lastSolverMs+completedMotionMs;
        // Auf dem LCD zaehlt nur die vom F6 gemeldete reine Motorlaufzeit.
        // Solver-Berechnung und die anschliessende BLE-Pruefung gehoeren nicht dazu.
        snprintf(first,sizeof(first),"Geloest:%lu.%03lus",
                 (unsigned long)(completedMotionMs/1000),(unsigned long)(completedMotionMs%1000));
        snprintf(second,sizeof(second),"%u Zuege | OK",motionTotal);
        lcdMessage(first,second);
        Serial.println("ERGEBNIS: Cube meldet tatsaechlich einen geloesten Zustand.");
        Serial.printf("ZEITEN: Solver=%lu ms | Motoren=%lu ms | Gesamt=%lu ms | Zuege=%u\n",
                      (unsigned long)lastSolverMs,(unsigned long)completedMotionMs,
                      (unsigned long)totalMs,motionTotal);
    } else if(goalReached) {
        snprintf(first,sizeof(first),"Gemischt:%lu.%03lus",
                 (unsigned long)(completedMotionMs/1000),(unsigned long)(completedMotionMs%1000));
        snprintf(second,sizeof(second),"%u Zuege | OK",motionTotal);
        lcdMessage(first,second);
        Serial.println("ERGEBNIS: Bluetooth-Cube wurde erfolgreich gemischt.");
    } else if(!validState) {
        showCompletedMotionTime("Nicht bestaetigt");
        Serial.println("ERGEBNIS: Nicht bestaetigt, weil kein gueltiger Cube-Zustand vorliegt.");
    } else if(!cubeChanged) {
        showCompletedMotionTime("Keine Cube-Zuege");
        Serial.println("ERGEBNIS: Motoren liefen, aber der Bluetooth-Cube meldete keine einzige Drehung. War er eingesetzt?");
    } else if(motionGoal==MotionGoal::SCRAMBLE) {
        showCompletedMotionTime("Scramble Fehler");
        Serial.println("ERGEBNIS: Der Scramble endete laut Bluetooth trotzdem im geloesten Zustand.");
    } else {
        showCompletedMotionTime("Nicht geloest");
        Serial.println("ERGEBNIS: Der Cube meldet nach dem Motorlauf weiterhin einen gemischten Zustand.");
        Serial.printf("MOTOREN: %lu ms | Zuege=%u | Cube/Mechanik pruefen\n",
                      (unsigned long)completedMotionMs,motionTotal);
    }
}

static void handleF6Line(char* line) {
    while(*line==' ') ++line;
    if(!*line) return;
    f6LastSeen=millis();
    Serial.print("F6 > ");Serial.println(line);

    if(!strcmp(line,"READY") || !strcmp(line,"PONG")) {
        f6Ready=true;
        if(online && !busy && !motionActive && !motionPending && !finalResultVisible) {
            char second[17];
            if(haveState && cube_is_solved(&current)) snprintf(second,sizeof(second),"BOOT: Mischen");
            else snprintf(second,sizeof(second),"BOOT | Bat %d%%",battery);
            lcdMessage("Cube verbunden",second);
        }
        return;
    }

    unsigned long id=0,value=0,total=0;
    if(sscanf(line,"START %lu %lu",&id,&total)==2 && id==activeJobId) {
        motionPending=false;motionActive=true;finalResultVisible=false;motionStartedAt=millis();
        cubeVerificationPending=false;motionGenerationAtStart=generation;
        motionDone=0;motionTotal=(uint16_t)total;showMotionTime();return;
    }
    if(sscanf(line,"MOVE %lu %lu %lu",&id,&value,&total)==3 && id==activeJobId) {
        motionDone=(uint16_t)value;motionTotal=(uint16_t)total;showMotionTime();return;
    }
    if(sscanf(line,"DONE %lu %lu",&id,&value)==2 && id==activeJobId) {
        motionPending=false;motionActive=false;finalResultVisible=true;motionDone=motionTotal;
        completedMotionMs=(uint32_t)value;
        cubeVerificationDeadline=millis()+2000;
        cubeVerificationPending=true;
        lcdMessage("Motoren fertig","Pruefe Cube...");return;
    }
    if(!strncmp(line,"ERR ",4)) {
        motionPending=false;motionActive=false;finalResultVisible=false;cubeVerificationPending=false;
        const char *driverFaces=strstr(line,"DRIVER_UART_");
        if(driverFaces) {
            char second[17];
            snprintf(second,sizeof(second),"Pruefen: %.7s",driverFaces+12);
            lcdMessage("TMC-UART Fehler",second);
        } else {
            lcdMessage("F6 FEHLER",line+4);
        }
        return;
    }
}

static void receiveF6() {
    while(f6Serial.available()) {
        const char c=(char)f6Serial.read();
        if(c=='\r') continue;
        if(c=='\n') {
            f6Line[f6LineLength]=0;handleF6Line(f6Line);f6LineLength=0;
        } else if(f6LineLength+1<sizeof(f6Line)) {
            f6Line[f6LineLength++]=c;
        } else {
            f6LineLength=0;
        }
    }
}

static bool sendSolutionToF6(const char* solution,int moves,MotionGoal goal) {
    if(!f6Ready || motionActive || motionPending || !solution || !*solution) return false;
    activeJobId=nextJobId++;
    motionGoal=goal;
    f6Serial.printf(goal==MotionGoal::SCRAMBLE ? "SCRAMBLE %lu %s\n" : "RUN %lu %s\n",
                    (unsigned long)activeJobId,solution);
    motionPending=true;motionDone=0;motionTotal=(uint16_t)moves;
    if(goal==MotionGoal::SCRAMBLE) lcdMessage("Scramble bereit","Warte auf F6");
    Serial.printf("%s als Auftrag %lu an F6 gesendet.\n",
                  goal==MotionGoal::SCRAMBLE ? "Scramble" : "Loesung",(unsigned long)activeJobId);
    return true;
}

static bool startScramble() {
    static constexpr uint8_t SCRAMBLE_MOVES=20;
    static const char faces[]="URFDLB";
    static const char* suffixes[]={"","'","2"};
    char scramble[96];
    size_t used=0;
    int lastFace=-1;
    for(uint8_t i=0;i<SCRAMBLE_MOVES;++i) {
        int face;
        do { face=(int)(esp_random()%6); } while(face==lastFace);
        lastFace=face;
        const char *suffix=suffixes[esp_random()%3];
        const int written=snprintf(scramble+used,sizeof(scramble)-used,"%c%s%s",
                                   faces[face],suffix,i+1<SCRAMBLE_MOVES ? " " : "");
        if(written<=0 || (size_t)written>=sizeof(scramble)-used) return false;
        used+=(size_t)written;
    }
    finalResultVisible=false;lastSolverMs=0;
    Serial.print("Scramble: ");Serial.println(scramble);
    if(sendSolutionToF6(scramble,SCRAMBLE_MOVES,MotionGoal::SCRAMBLE)) {
        lcdMessage("Scramble bereit","Warte auf F6");
        return true;
    }
    lcdMessage("Scramble Fehler","F6 nicht bereit");
    return false;
}

static void worker(void*) {
    uint32_t start=millis();
    ready=prepareSolver();
    Serial.printf("Solver-Vorbereitung: %lu ms; %s\n",(unsigned long)(millis()-start),ready ? "BEREIT" : "FEHLER: PSRAM pruefen");
    Job job;
    for(;;) {
        if(xQueueReceive(jobs,&job,portMAX_DELAY)!=pdTRUE) continue;
        Result result={};result.generation=job.generation;result.demo=job.demo;
        uint8_t reported=0;
        start=millis();
        std::string solution=solveCube(job.cube,reported);
        result.ms=millis()-start;
        result.ok=verifySolution(job.cube,solution,result.moves) && result.moves==reported;
        snprintf(result.text,sizeof(result.text),"%s",solution.c_str());
        xQueueSend(results,&result,portMAX_DELAY);
    }
}

static bool parseMac(const char* text,uint8_t* mac) {
    unsigned v[6];char extra;
    if(sscanf(text,"%2x:%2x:%2x:%2x:%2x:%2x%c",&v[0],&v[1],&v[2],&v[3],&v[4],&v[5],&extra)!=6) return false;
    for(int i=0;i<6;i++) mac[i]=v[i];
    return true;
}

class ScanCallbacks:public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice device) override {
        String name=device.getName();
        String address=device.getAddress().toString();
        bool match=strlen(CUBE_MAC) ? address.equalsIgnoreCase(CUBE_MAC) :
                   name.startsWith("QY-QYSC");
        if(!match || candidate) return;
        candidate=new BLEAdvertisedDevice(device);
        parseMac(address.c_str(),hardwareMac);
        // Manufacturer company 0x0504, followed by reversed hardware address.
        String m=device.getManufacturerData();
        if(!strlen(CUBE_MAC) && m.length()>=8 && uint8_t(m[0])==4 && uint8_t(m[1])==5)
            for(int i=0;i<6;i++) hardwareMac[i]=uint8_t(m[7-i]);
        if(strlen(CUBE_MAC)) parseMac(CUBE_MAC,hardwareMac);
        Serial.printf("QiYi gefunden: %s, BLE %s\n",name.c_str(),address.c_str());
        BLEDevice::getScan()->stop();
    }
};
class ClientCallbacks:public BLEClientCallbacks {
    void onDisconnect(BLEClient*) override { lost=true; }
};
static ScanCallbacks scanCallbacks;
static ClientCallbacks clientCallbacks;

static void notify(BLERemoteCharacteristic*,uint8_t* bytes,size_t n,bool) {
    if(n<16 || n>256 || n%16) {overflow=true;return;}
    Packet packet;packet.length=n;memcpy(packet.bytes,bytes,n);
    if(xQueueSend(packets,&packet,0)!=pdTRUE) overflow=true;
}

static bool sendPayload(const uint8_t* payload,size_t size) {
    if(!online || !client->isConnected() || !characteristic) return false;
    uint8_t plain[256],encrypted[256];
    size_t n=qiyi::frame(plain,payload,size);
    if(!n) return false;
    mbedtls_aes_context aes;mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes,qiyi::key,128);
    for(size_t i=0;i<n;i+=16) mbedtls_aes_crypt_ecb(&aes,MBEDTLS_AES_ENCRYPT,plain+i,encrypted+i);
    mbedtls_aes_free(&aes);
    // QiYi QYSC-S normally exposes WRITE_NR (write without response).
    // Request a response only on variants which support regular WRITE alone.
    bool withResponse=characteristic->canWrite() && !characteristic->canWriteNoResponse();
    characteristic->writeValue(encrypted,n,withResponse);
    return client->isConnected();
}

static void invalidate();

static void calibrateCubeAsSolved() {
    if(busy || motionActive || motionPending) {
        Serial.println("Kalibrierung nicht moeglich: Roboter ist beschaeftigt.");
        lcdMessage("Nicht kalibriert","Roboter arbeitet");
        return;
    }
    if(!online || !client || !client->isConnected() || !characteristic) {
        Serial.println("Kalibrierung nicht moeglich: Cube ist nicht verbunden.");
        lcdMessage("Nicht kalibriert","Cube fehlt");
        return;
    }

    // QiYi CubeSync (Opcode 4): Zeitstempel, 27 gepackte Farbnibbles und
    // zwei reservierte Bytes. Nur senden, wenn der echte Cube geloest ist.
    uint8_t payload[34]={0};
    payload[0]=4;
    const uint32_t timestamp=haveTimestamp ? lastTimestamp+1 : millis();
    payload[1]=(uint8_t)(timestamp>>24);
    payload[2]=(uint8_t)(timestamp>>16);
    payload[3]=(uint8_t)(timestamp>>8);
    payload[4]=(uint8_t)timestamp;
    static const uint8_t solvedColors[6]={3,1,4,2,0,5}; // U,R,F,D,L,B
    for(uint8_t i=0;i<54;++i) {
        const uint8_t color=solvedColors[i/9];
        if(i&1) payload[5+i/2]|=(uint8_t)(color<<4);
        else payload[5+i/2]=color;
    }

    if(!sendPayload(payload,sizeof(payload))) {
        Serial.println("Kalibrierbefehl konnte nicht gesendet werden.");
        lcdMessage("Kalibrierfehler","Senden fehlte");
        return;
    }

    // Anschliessend einen frischen Zustand vom Cube anfordern (Opcode 5).
    delay(100);
    const uint8_t freshStateRequest[5]={5,5,5,5,5};
    sendPayload(freshStateRequest,sizeof(freshStateRequest));
    invalidate();
    helloAt=millis();
    calibrationAwaitingState=true;
    finalResultVisible=false;
    Serial.println("Kalibrierung gesendet. Physischer Cube muss jetzt geloest sein.");
    lcdMessage("Kalibriere Cube","Nicht drehen");
}

static void invalidate() { haveState=false;haveTimestamp=false;++generation; }
static void disconnectCube() {
    online=false;characteristic=nullptr;invalidate();
    calibrationAwaitingState=false;
    finalResultVisible=false;
    if(client && client->isConnected()) client->disconnect();
    xQueueReset(packets);
    if(!motionActive && !motionPending) lcdMessage("Cube getrennt","Suche erneut...");
}

static void connectCube() {
    Serial.println("Suche QiYi (4 s). Cube drehen; Smart Player/csTimer trennen.");
    if(!busy && !motionActive && !motionPending) lcdMessage("Suche QiYi...","Cube drehen");
    BLEDevice::getScan()->start(4,false);
    if(!candidate) { BLEDevice::getScan()->clearResults();return; }
    lost=false;
    bool success=client->connect(candidate->getAddress(),candidate->getAddressType(),6000);
    delete candidate;candidate=nullptr;
    BLEDevice::getScan()->clearResults();
    if(!success) {Serial.println("BLE-Verbindung fehlgeschlagen.");return;}
    client->setMTU(247);
    auto service=client->getService(serviceUUID);
    characteristic=service ? service->getCharacteristic(characteristicUUID) : nullptr;
    if(!characteristic || !characteristic->canNotify() ||
       (!characteristic->canWrite() && !characteristic->canWriteNoResponse())) {
        Serial.println("QiYi-Service/Notify/Write fehlt.");disconnectCube();return;
    }
    xQueueReset(packets);invalidate();online=true;
    characteristic->registerForNotify(notify);
    if(!busy && !motionActive && !motionPending) lcdMessage("Cube verbunden","Warte Zustand");
    uint8_t hello[17]={0,0x6b,1,0,0,0x22,6,0,2,8,0};
    for(int i=0;i<6;i++) hello[11+i]=hardwareMac[5-i];
    Serial.printf("Handshake mit Hardware-MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
        hardwareMac[0],hardwareMac[1],hardwareMac[2],hardwareMac[3],hardwareMac[4],hardwareMac[5]);
    sendPayload(hello,sizeof(hello));helloAt=millis();
}

static void printState() {
    if(!haveState) {
        Serial.println("Noch kein gueltiger Cube-Zustand.");
        if(!busy && !motionActive && !motionPending && !finalResultVisible) lcdMessage("Kein Zustand","Cube drehen");
        return;
    }
    char faces[55];for(int i=0;i<54;i++) faces[i]="URFDLB"[current.f[i]];faces[54]=0;
    Serial.printf("Zustand URFDLB: %s\nBatterie: %d %% | %s\n",faces,battery,
                  cube_is_solved(&current) ? "GELOEST" : "gemischt");
    if(!busy && !motionActive && !motionPending && !finalResultVisible) {
        char second[17];
        if(cube_is_solved(&current)) snprintf(second,sizeof(second),"BOOT: Mischen");
        else if(f6Ready) snprintf(second,sizeof(second),"BOOT | Bat %d%%",battery);
        else snprintf(second,sizeof(second),"F6 fehlt | %d%%",battery);
        lcdMessage("Cube verbunden",second);
    }
}

static void receivePackets() {
    Packet packet;
    while(xQueueReceive(packets,&packet,0)==pdTRUE) {
        if(!online || lost) continue;
        uint8_t plain[256];
        mbedtls_aes_context aes;mbedtls_aes_init(&aes);
        mbedtls_aes_setkey_dec(&aes,qiyi::key,128);
        for(size_t i=0;i<packet.length;i+=16)
            mbedtls_aes_crypt_ecb(&aes,MBEDTLS_AES_DECRYPT,packet.bytes+i,plain+i);
        mbedtls_aes_free(&aes);
        if(!qiyi::valid(plain,packet.length)) {
            Serial.println("Ungueltiges Bluetooth-Paket (Laenge/CRC/AES).");invalidate();continue;
        }
        if(plain[2]<2 || plain[2]>5) continue;
        // Hello- und Bewegungsereignisse bestaetigen. Sync/Fresh-State
        // benoetigen laut QiYi-Protokoll keine Bestaetigung.
        if(plain[2]==2 || plain[2]==3) sendPayload(plain+2,5);
        cube_t state;
        if(!qiyi::state(plain,packet.length,state)) {
            Serial.println("QiYi-Zustandsformat ungueltig.");invalidate();continue;
        }
        uint32_t ts=uint32_t(plain[3])<<24|uint32_t(plain[4])<<16|uint32_t(plain[5])<<8|plain[6];
        if(haveTimestamp && int32_t(ts-lastTimestamp)<=0) continue;
        lastTimestamp=ts;haveTimestamp=true;
        int validation=cube_validate(&state,nullptr);
        if(validation!=CUBE_OK) {
            Serial.printf("Cube-Zustand physikalisch ungueltig: Fehler %d.\n",validation);
            haveState=false;++generation;continue;
        }
        if(!haveState || memcmp(&state,&current,sizeof(state))) ++generation;
        current=state;haveState=true;battery=plain[35];
        if(plain[2]==3 && plain[34]>=1 && plain[34]<=12) {
            const char* names[]={"L'","L","R'","R","D'","D","U'","U","F'","F","B'","B"};
            Serial.printf("Drehung: %s\n",names[plain[34]-1]);
        }
        const bool calibrationResponse=calibrationAwaitingState;
        calibrationAwaitingState=false;
        printState();
        if(calibrationResponse) {
            if(cube_is_solved(&current)) {
                Serial.println("Kalibrierung bestaetigt: Cube meldet GELOEST.");
                lcdMessage("Kalibrierung OK","Cube geloest");
            } else {
                Serial.println("Kalibrierung nicht bestaetigt: Cube meldet weiterhin gemischt.");
                lcdMessage("Kalibrierfehler","Erneut versuchen");
            }
        }
    }
}

static void requestSolve(bool demo) {
    if(!ready) {Serial.println("Solver noch nicht bereit. PSRAM/Vorbereitung beachten.");lcdMessage("Solver startet","Bitte warten");return;}
    if(busy) {Serial.println("Eine Berechnung laeuft bereits.");lcdMessage("Berechnung laeuft","Bitte warten");return;}
    if(!demo && cubeVerificationPending) {Serial.println("Der letzte Motorlauf wird noch mit dem Cube-Zustand verglichen.");lcdMessage("Pruefe Cube...","Bitte warten");return;}
    if(!demo && (motionActive || motionPending)) {Serial.println("Der F6 bearbeitet bereits einen Auftrag.");lcdMessage("Motoren laufen","Bitte warten");return;}
    if(!demo && !f6Ready) {Serial.println("F6 ist nicht bereit.");lcdMessage("F6 nicht bereit","UART pruefen");return;}
    if(!demo && (!online || !haveState)) {Serial.println("Kein verbundener Cube mit gueltigem Zustand.");lcdMessage("Cube nicht bereit","Cube drehen");return;}
    if(!demo && cube_is_solved(&current)) {
        Serial.println("Cube ist geloest: BOOT startet den Scramble-Modus.");
        startScramble();
        return;
    }
    if(!demo) finalResultVisible=false;
    Job job={};job.demo=demo;job.generation=generation;
    if(demo) {
        cube_reset(&job.cube);
        Serial.print("Synthetischer Test, Scramble: ");
        int last=-1;
        for(int i=0;i<25;i++) {
            int f;do {f=esp_random()%6;} while(f==last);last=f;
            uint8_t m=cube_mv(f,1+esp_random()%3);cube_move(&job.cube,m);
            Serial.printf("%s ",cube_move_name(m));
        }
        Serial.println();
    } else job.cube=current;
    lastSolverMs=0;busy=true;
    if(xQueueSend(jobs,&job,0)!=pdTRUE) {busy=false;return;}
    Serial.println("min2phase sucht 3 Sekunden nach der schnellsten parallelen Loesung. Cube ruhig halten.");
    lcdMessage(demo ? "Test: Suche 3s" : "Suche 3s...","Cube ruhig");
}

static void pollStartButton() {
    const bool raw=digitalRead(START_BUTTON_PIN);
    if(raw!=buttonRaw) {buttonRaw=raw;buttonChangedAt=millis();}
    if(raw!=buttonStable && millis()-buttonChangedAt>=40) {
        buttonStable=raw;
        if(buttonStable==LOW) {
            buttonPressedAt=millis();
            buttonLongHandled=false;
            if(motionActive || motionPending) {
                Serial.println("BOOT-Taster: Motorstopp angefordert.");
                f6Serial.print("!\n");
                lcdMessage("STOP gesendet","Warte auf F6");
                buttonLongHandled=true;
            }
        } else if(!buttonLongHandled) {
            Serial.println("BOOT-Taster kurz: Loesen oder Mischen angefordert.");
            requestSolve(false);
        }
    }
    if(buttonStable==LOW && !buttonLongHandled &&
       millis()-buttonPressedAt>=CALIBRATION_HOLD_MS) {
        buttonLongHandled=true;
        Serial.println("BOOT-Taster 3 Sekunden: Cube als geloest kalibrieren.");
        calibrateCubeAsSolved();
    }
}

static void help() {
    Serial.printf("Programmversion: %s\n",BUILD_TAG);
    Serial.println("\nBOOT kurz = gemischten Cube loesen / geloesten Cube mischen\nBOOT 3 Sekunden = physischen geloesten Cube kalibrieren\nBOOT waehrend Motorlauf = sofort stoppen\nBefehle (115200 Baud):\n s = je nach Zustand loesen oder mischen\n c = physischen geloesten Cube als GELOEST kalibrieren\n x = Motorlauf abbrechen\n p = Zustand anzeigen\n t = Solver ohne Motoren testen\n l = LCD-Test anzeigen\n w = LCD-Verkabelung messen\n r = Cube neu verbinden\n h = Hilfe");
}
void appSetup() {
    Serial.begin(115200);delay(1500);
    f6Serial.begin(F6_BAUD,SERIAL_8N1,F6_RX_PIN,F6_TX_PIN);
    pinMode(START_BUTTON_PIN,INPUT_PULLUP);
    buttonRaw=buttonStable=digitalRead(START_BUTTON_PIN);
    lcd.begin(16,2);lcdReady=true;lcdTest();delay(1500);
    lcdMessage("Cube-Roboter","Startet...");
    Serial.println("\nRubiks-Roboter / ESP32-S3 N16R8");
    Serial.printf("Programmversion: %s\n",BUILD_TAG);
    Serial.printf("Flash %u MB | PSRAM %u MB | CPU %u MHz\n",ESP.getFlashChipSize()/1048576,ESP.getPsramSize()/1048576,ESP.getCpuFreqMHz());
    packets=xQueueCreate(16,sizeof(Packet));jobs=xQueueCreate(1,sizeof(Job));results=xQueueCreate(1,sizeof(Result));
    if(!packets || !jobs || !results) {Serial.println("FEHLER: RAM fuer Warteschlangen fehlt.");while(true) delay(1000);}
    BLEDevice::init("Rubiks-Roboter");BLEDevice::setMTU(247);
    BLEDevice::getScan()->setAdvertisedDeviceCallbacks(&scanCallbacks);
    BLEDevice::getScan()->setActiveScan(true);
    client=BLEDevice::createClient();client->setClientCallbacks(&clientCallbacks);
    if(xTaskCreatePinnedToCore(worker,"solver",32768,nullptr,1,nullptr,1)!=pdPASS)
        Serial.println("FEHLER: Solver-Task konnte nicht starten.");
    help();lastScan=millis()-5000;
}
void appLoop() {
    receiveF6();
    pollStartButton();
    if(lost.exchange(false)) {Serial.println("Cube getrennt; alter Zustand verworfen.");disconnectCube();}
    if(overflow.exchange(false)) {
        Serial.println("BLE-Paketverlust oder unpassende MTU; Verbindung wird erneuert.");disconnectCube();
    }
    receivePackets();
    verifyFinishedCube();
    Result result;
    if(xQueueReceive(results,&result,0)==pdTRUE) {
        busy=false;
        if(!result.demo) lastSolverMs=result.ms;
        bool stale=!result.demo && (!online || !haveState || result.generation!=generation);
        Serial.printf("\n%s: Rechenzeit %lu ms | Zuege %d | Pruefung %s\n",result.demo ? "SYNTHETISCHER TEST" : "CUBE-LOESUNG",
            (unsigned long)result.ms,result.moves,result.ok ? "OK" : "FEHLER");
        if(stale)
            Serial.println("VERALTET: Cube wurde waehrend der Suche veraendert/getrennt. Erneut s senden.");
        else if(result.ok) Serial.printf("Loesung: %s\n",result.moves ? result.text : "bereits geloest");
        if(!result.ok) Serial.printf("Solver-Antwort: %s (7=Zuglimit, 8=Suchlimit, 9=Tabellen fehlen).\n",result.text);
        char first[17],second[17];
        if(stale) {
            lcdMessage("Loesung veraltet","Erneut s senden");
        } else if(!result.ok) {
            snprintf(second,sizeof(second),"Antwort: %.7s",result.text);
            lcdMessage("Solver FEHLER",second);
        } else if(result.demo) {
            snprintf(first,sizeof(first),"Test: %d Zuege",result.moves);
            snprintf(second,sizeof(second),"Rechenzeit:%lums",(unsigned long)result.ms);
            lcdMessage(first,second);
        } else if(result.moves==0) {
            lcdMessage("Cube verbunden","Schon geloest");
        } else if(!sendSolutionToF6(result.text,result.moves,MotionGoal::SOLVE)) {
            lcdMessage("Start fehlte",f6Ready ? "F6 beschaeftigt" : "F6 nicht bereit");
            Serial.println("Loesung nicht ausgefuehrt: F6 ist nicht bereit oder bereits beschaeftigt.");
        }
    }
    while(Serial.available()) {
        char command=Serial.read();
        if(command!='\r' && command!='\n') Serial.printf("Befehl empfangen: '%c'\n",command);
        if(command=='s') requestSolve(false);
        else if(command=='c' || command=='C') calibrateCubeAsSolved();
        else if(command=='t') requestSolve(true);
        else if(command=='p') printState();
        else if(command=='x') {f6Serial.print("!\n");lcdMessage("STOP gesendet","Warte auf F6");}
        else if(command=='r') {disconnectCube();lastScan=millis()-5000;}
        else if(command=='l') lcdTest();
        else if(command=='w') lcdWiringTest();
        else if(command=='h' || command=='?') help();
    }
    if(online && !haveState && millis()-helloAt>10000) {
        Serial.println("Kein gueltiger Zustand nach Handshake. MAC/Modell pruefen; neuer Versuch.");
        disconnectCube();
    }
    if(!motionActive && !motionPending && millis()-f6LastPing>=2000) {
        f6Serial.print("PING\n");f6LastPing=millis();
    }
    if(f6Ready && !motionActive && !motionPending && millis()-f6LastSeen>6000) {
        f6Ready=false;
        Serial.println("F6 antwortet nicht mehr.");
    }
    static uint32_t lastClockUpdate=0;
    if(motionActive && millis()-lastClockUpdate>=100) {
        lastClockUpdate=millis();showMotionTime();
    }
    if(!online && !motionActive && !motionPending && millis()-lastScan>=5000) {connectCube();lastScan=millis();}
    static uint32_t progress=0;
    if((busy || !ready) && millis()-progress>5000) {
        progress=millis();Serial.println(busy ? "Loesungssuche laeuft ..." : "Solver-Vorbereitung laeuft; bei PSRAM-Fehler Einstellungen pruefen.");
    }
    delay(5);
}
