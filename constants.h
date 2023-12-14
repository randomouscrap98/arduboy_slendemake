#pragma once

constexpr uint8_t staticmap_width = 64;
constexpr uint8_t staticmap_height = 64;
constexpr uint8_t staticsprites_bytes = 2;

constexpr uint8_t FRAMERATE = 25; //Too much overdraw?
constexpr float MOVESPEED = 1.25f / FRAMERATE;
constexpr float ROTSPEED = 1.5f / FRAMERATE;
constexpr float MOVEMULTIPLIER = 2.25;
constexpr float ROTMULTIPLIER = 2.25;

constexpr float NORMALLIGHT = 2.5;
constexpr float SPRINTLIGHT = 1.0;

constexpr uint8_t SCREENWIDTH = 100;
constexpr float ROTBGSCALE = (rotbgWidth - SCREENWIDTH) / (2 * M_PI);

struct SpriteMeta {
    int8_t offset;
    uint8_t scale;
    float bounds;
};

constexpr SpriteMeta SPRITEMETAS[] PROGMEM = {
    { 0, 0, 0 },
    { -9, 0, 1.0 },
    { -12, 3, 2.5 },
    { -3, 1, 0.5 },
    { 7, 2, 0.0 },
    { -9, 0, 1.0 }
};


// Since we're using this number so many times in template types, might 
// as well make it a constant.
constexpr uint8_t NUMINTERNALBYTES = 1;
constexpr uint8_t NUMSPRITES = 32;

// Some stuff for external map loading
constexpr uint8_t CAGEX = 7;
constexpr uint8_t CAGEY = 7;
constexpr uint8_t LOADWIDTH = 15;
constexpr uint8_t LOADHEIGHT = 15;

constexpr uint8_t SPRITEVIEW = 6;

//Sprites outside this radius get banished, sprites that enter this radius
//get loaded
constexpr uint8_t SPRITEBEGIN_X = CAGEX - SPRITEVIEW;
constexpr uint8_t SPRITEEND_X = CAGEX + SPRITEVIEW;
constexpr uint8_t SPRITEBEGIN_Y = CAGEY - SPRITEVIEW;
constexpr uint8_t SPRITEEND_Y = CAGEY + SPRITEVIEW;

constexpr int16_t SPRINTMIN_SECS = 1;
constexpr int16_t SPRINT_SECS = 5;
constexpr int16_t SPRINTREC_SECS = 15;
constexpr int16_t SPRINTMAX = FRAMERATE * SPRINTMIN_SECS * SPRINT_SECS * SPRINTREC_SECS; //Realistically, this should just be the lcm of all the literals below
constexpr int16_t SPRINTMIN = SPRINTMAX / (FRAMERATE * SPRINTMIN_SECS); //If you let go of sprint, you won't be able to sprint again until there's this much sprint available
constexpr int16_t SPRINTDRAIN = SPRINTMAX / (FRAMERATE * SPRINT_SECS); 
constexpr int16_t SPRINTRECOVER = SPRINTMAX / (FRAMERATE * SPRINTREC_SECS); 
//constexpr uint8_t SPRITEGC_PERFRAME = 3;  // How many sprites to loop through per frame for garbage collect
