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