#pragma once
// Local ESP32 port: give BLE/idle tasks CPU time without changing the search.
#include <cstdint>
#ifdef ARDUINO
#include <Arduino.h>
inline uint32_t solver_millis() { return millis(); }
inline void solver_poll() {
    static uint32_t calls = 0, last = 0;
    if ((++calls & 255) == 0 && millis() - last >= 10) {
        vTaskDelay(1);
        last = millis();
    }
}
#else
#include <chrono>
inline uint32_t solver_millis() {
    using namespace std::chrono;
    return (uint32_t)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}
inline void solver_poll() {}
#endif
