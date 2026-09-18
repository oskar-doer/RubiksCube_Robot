#include "solver_adapter.h"
#include "src/min2phase/min2phase.h"
#include "src/min2phase/Search.h"
#include <limits>
#include <new>
#include <cstring>
#ifdef ARDUINO
#include <Arduino.h>
#include <LittleFS.h>
#include <esp_heap_caps.h>
#endif

namespace min2phase {
void init() { info::init(); coords::init(); }
std::string solve(const std::string& f,int8_t d,int32_t max,int32_t min,int8_t v,uint8_t* m) {
    return Search().solve(f,d,max,min,v,m);
}
}

#ifdef ARDUINO
static uint32_t checksum(const uint8_t* data,size_t n) {
    uint32_t h=2166136261u;
    for(size_t i=0;i<n;i++) h=(h^data[i])*16777619u;
    return h;
}
#endif

bool prepareSolver() {
    using namespace min2phase;
    if(coords::isInit()) return true;
    const size_t bytes=sizeof(coords::coords_t);
#ifdef ARDUINO
    if(!psramFound()) return false;
    void* memory=heap_caps_malloc(bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if(!memory) return false;
    coords::coords=new(memory) coords::coords_t();
    Serial.printf("Suchtabellen: %u Bytes im PSRAM.\n",unsigned(bytes));
    // Dedicated test filesystem. Its cache is specific to this firmware build.
    const uint32_t magic=0x4d325032;
    const uint32_t build=checksum((const uint8_t*)(__DATE__ " " __TIME__),sizeof(__DATE__ " " __TIME__));
    bool fs=LittleFS.begin(true);
    uint32_t header[4]={0};
    if(fs) {
        File f=LittleFS.open("/min2phase.bin","r");
        if(f && f.size()==bytes+sizeof(header) && f.read((uint8_t*)header,sizeof(header))==sizeof(header)
           && header[0]==magic && header[1]==bytes && header[2]==build
           && f.read((uint8_t*)coords::coords,bytes)==bytes
           && checksum((uint8_t*)coords::coords,bytes)==header[3] && coords::isInit()) {
            info::init();
            Serial.println("Suchtabellen aus Flash geladen.");
            return true;
        }
        // Reset possibly partial or invalid cached data before generation.
        new(memory) coords::coords_t();
    }
    Serial.println("Erzeuge Suchtabellen einmalig. Das kann mehrere Minuten dauern.");
    min2phase::init();
    if(fs) {
        header[0]=magic;header[1]=bytes;header[2]=build;
        header[3]=checksum((uint8_t*)coords::coords,bytes);
        File f=LittleFS.open("/min2phase.bin","w");
        bool saved=f && f.write((uint8_t*)header,sizeof(header))==sizeof(header)
                    && f.write((uint8_t*)coords::coords,bytes)==bytes;
        Serial.println(saved ? "Suchtabellen gespeichert." : "Cache nicht gespeichert; Solver ist trotzdem bereit.");
    }
#else
    coords::coords=new coords::coords_t();
    min2phase::init();
#endif
    return coords::isInit();
}

std::string solveCube(const cube_t& cube,uint8_t& moves,uint32_t searchTimeMs) {
    if(cube_validate(&cube,nullptr)!=CUBE_OK) return "INVALID_STATE";
    if(cube_is_solved(&cube)) { moves=0;return ""; }
    std::string facelets;
    for(uint8_t f:cube.f) facelets+="URFDLB"[f];
    // Die erste gueltige Loesung bleibt als Rueckfall erhalten. Danach zaehlen
    // direkt benachbarte Gegenseiten als ein paralleler Motor-Zeitschritt.
    if(searchTimeMs==0) return min2phase::solve(facelets,30,1000000,0,0,&moves);
    const int32_t probes=std::numeric_limits<int32_t>::max();
    return min2phase::Search().solve(facelets,30,probes,probes,0,&moves,searchTimeMs);
}

bool verifySolution(const cube_t& cube,const std::string& solution,int& count) {
    static const char faces[]="URFDLB";
    cube_t test=cube;count=0;
    size_t i=0;
    while(i<solution.size()) {
        if(solution[i]==' ') { ++i;continue; }
        const char* f=strchr(faces,solution[i++]);
        if(!f || !*f) return false;
        int turns=1;
        if(i<solution.size() && solution[i]=='2') {turns=2;++i;}
        else if(i<solution.size() && solution[i]=='\'') {turns=3;++i;}
        if(i<solution.size() && solution[i]!=' ') return false;
        cube_move(&test,cube_mv(f-faces,turns));
        ++count;
    }
    return cube_is_solved(&test);
}
