#pragma once

/**** FX data header generated by fxdata-build.py tool version 1.15 ****/

using uint24_t = __uint24;

// Initialize FX hardware using  FX::begin(FX_DATA_PAGE); in the setup() function.

constexpr uint16_t FX_DATA_PAGE  = 0xffa3;
constexpr uint24_t FX_DATA_BYTES = 23748;

constexpr uint24_t staticmap_fx = 0x000000;
constexpr uint24_t staticsprites_fx = 0x001000;
constexpr uint24_t pagelocs_raw = 0x003000;
constexpr uint24_t pagelocs_offsets = 0x00306A;
constexpr uint24_t rotbg = 0x003074;
constexpr uint16_t rotbgWidth  = 500;
constexpr uint16_t rotbgHeight = 64;

constexpr uint24_t rotbg_day = 0x004018;
constexpr uint16_t rotbg_day_width  = 500;
constexpr uint16_t rotbg_day_height = 64;

constexpr uint24_t pages = 0x004FBC;
constexpr uint16_t pagesWidth  = 48;
constexpr uint16_t pagesHeight = 64;
constexpr uint8_t  pagesFrames = 8;

constexpr uint24_t soundgraphic = 0x005BC0;
constexpr uint16_t soundgraphicWidth  = 32;
constexpr uint16_t soundgraphicHeight = 32;
constexpr uint8_t  soundgraphicFrames = 2;

