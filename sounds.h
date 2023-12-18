#pragma once

#include <ArduboyTones.h>

constexpr uint8_t CRUNCHLEN = 7;
constexpr uint8_t CRUNCHES = 4;

const uint16_t crunches[] PROGMEM = {
  50,30, 100,20, 200,10,
  TONES_END,
  70,30, 140,20, 250,7,
  TONES_END,
  60,30, 120,20, 150,10,
  TONES_END,
  40,20, 90,20, 220,5,
  TONES_END,
};

const uint16_t pagepickup[] PROGMEM = {
    50,20, 150,20, 250,10, 350,10,
    TONES_END
};

const uint16_t climbfence[] PROGMEM = {
    2000,10, 2500,5, 0,500, 
    2010,10, 2510,5, 0,200, 
    2000,10, 2500,5, 0,300, 
    2000,10, 2520,5, 0,700, 
    100,10, 90,10, 80,10, 0,50,
    30,20, 50,20, 0,100,
    TONES_END
};

const uint16_t drone[] PROGMEM = {
    30,20, 29,20, 28,20, 27,20, 26,20, 25,20, 24,20, 23,20,
    TONES_END
};

const uint16_t screeching[] PROGMEM = {
  //50,10, 30,5, 1000,5, 100,10, 4000,5,
  50,10, 200,10, 600,10, 4000,5,
  //50,10, 200,10, 600,10,
  TONES_END
};