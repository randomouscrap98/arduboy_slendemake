#include <Tinyfont.h>
#include <FixedPoints.h>
#include <Arduboy2.h>
#include <ArduboyTones.h>

#include <ArduboyFX.h>      // required to access the FX external flash
#include "fx/fxdata.h"  // this file contains all references to FX data

//#define RCSMALLLOOPS

// Libs for raycasting
#include <ArduboyRaycast.h>
#include <ArduboyRaycast_Shading.h>

// Some debug junk
//#define DEBUGPAGES
//#define DEBUGMOVEMENT
#define SKIPINTRO
#define INFINITESPRINT
#define PRINTAGGRESSION
//#define SPAWNSLENDERCLOSE
//#define NOSTATICACCUM
//#define PRINTSTATIC
//#define NOFOG
//#define ARESTARTS
//#define DRAWMAP
//#define VARIABLEFPS

// Graphics / resources
#include "spritesheet.h"
#include "tilesheet.h"
#include "sounds.h"

#include "constants.h"
#include "utils.h"


enum GameState
{
    Menu,
    Intro,
    Gameplay,
    Gameover,
    Escape
};

GameState state; 

Arduboy2Base arduboy;
ArduboyTones sound(arduboy.audio.enabled);
Tinyfont tinyfont = Tinyfont(arduboy.sBuffer, Arduboy2::width(), Arduboy2::height());

uint8_t world_x;
uint8_t world_y;

uint8_t page_bitflag;
uint8_t current_pageview;  //Starts at 1, but pages are really 0 indexed
int16_t sprintmeter = SPRINTMAX;
bool holding_b = false;
uint8_t pagelocs[16] = {255};
UFixed<0,8> moveaccum = 0;
uint16_t timer1 = 0;
uint8_t bgsoundtimer = 0;
uint24_t current_bg = rotbg;

//Slenderman doesn't need to move smoothly... or so I hope (hence only integer positions)
uint8_t slenderX;
uint8_t slenderY;
uint8_t slenderLocScan = 0;
uint8_t slenderLocTotal = 0;

SFixed<13,2> staticaccum = 0;
uint8_t staticbase = 0;
uint8_t lastStatic = 0;

RcContainer<NUMSPRITES, NUMINTERNALBYTES, SCREENWIDTH, HEIGHT> raycast(tilesheet, spritesheet, spritesheet_Mask);


void setup()
{
    // Initialize the Arduboy
    arduboy.boot();
    arduboy.flashlight();
    arduboy.initRandomSeed();
    arduboy.setFrameRate(FRAMERATE);
    arduboy.initRandomSeed();
    FX::begin(FX_DATA_PAGE);    // initialise FX chip

    raycast.render.spritescaling[0] = 2.0;
    raycast.render.spritescaling[1] = 1.0;
    raycast.render.spritescaling[2] = 0.70;
    raycast.render.spritescaling[3] = 3.5;

    #ifdef SKIPINTRO
    arduboy.audio.on();
    newgame();
    #else
    state = GameState::Menu;
    #endif
}

void newgame()
{
    state = GameState::Gameplay;
    current_pageview = 0;
    page_bitflag = 0;
    moveaccum = 0;
    staticaccum = 0;
    staticbase = 0;
    lastStatic = 0;
    current_bg = rotbg;
    timer1 = 0;

    world_x = 33;
    world_y = 60;

    #ifdef SPAWNSLENDERCLOSE
    slenderX = world_x;
    slenderY = world_y - 5;
    #else
    //Way outside the realm of possibility to start
    slenderX = 255;
    slenderY = 255;
    #endif

    slenderLocScan = 0;
    slenderLocTotal = FX::readIndexedUInt8(slenderlocs_trueraw, 0);

    #ifdef NOFOG
    raycast.render.altWallShading = RcShadingType::None;
    raycast.render.shading = RcShadingType::None;
    raycast.render.spriteShading = RcShadingType::None;
    raycast.render.setLightIntensity(NORMALLIGHT);
    //raycast.render.setLightIntensity(DAYLIGHT);
    #else
    raycast.render.altWallShading = RcShadingType::Black;
    raycast.render.shading = RcShadingType::Black;
    raycast.render.spriteShading = RcShadingType::Black;
    raycast.render.setLightIntensity(NORMALLIGHT);
    #endif

    raycast.player.posX = CAGEX + 0.5;
    raycast.player.posY = CAGEY + 0.5;
    raycast.player.dirX = 0;
    raycast.player.dirY = -1;

    raycast.sprites.resetAll();

    RcSprite<NUMINTERNALBYTES> * slender = raycast.sprites.addSprite(0, 0, SLENDERSPRITE, 1, 0, &behavior_slender);

    spawnPages();

    load_region();
    load_surrounding_sprites();

    drawSidebar();
}

void changeStateClean(GameState newstate)
{
    state = newstate;
    current_pageview = 0;   //This is used in many states as a kind of secondary state (should just use a different variable...)
    timer1 = 0;             //Reset the timers everything might use. If you need a timer that extends beyond states, well...
    lastStatic = 0;         //This is also reused (though it probably shouldn't be...)
}

uint8_t numfoundpages()
{
    uint8_t count = 0;

    for(uint8_t i = 1; i != 0; i <<= 1)
        if(page_bitflag & i)
            count++;

    return count;
}

void spawnPages()
{
    #ifdef DEBUGPAGES
    //Put all the pages right in a row in front of the player
    for(uint8_t i = 0; i < NUMPAGES * 2; i += 2)
    {
        pagelocs[i] = world_x - 4 + i / 2;
        pagelocs[i + 1] = world_y - 2;
    }
    #else
    //Find nice locations to place them based on the spawn points indicated on the map 
    //(in the fx data)
    uint8_t usedlocs[NUMPAGES];
    for(uint8_t i = 0; i < NUMPAGES; i++)
    {
        //Try to pick a location that hasn't been used yet. This is the wasteful part
spawnPagesRetry:
        usedlocs[i] = rand() % NUMLOCATIONS;
        for(uint8_t j = 0; j < i; j++)
        {
            if(usedlocs[j] == usedlocs[i])
                goto spawnPagesRetry;
        }

        uint8_t offset = FX::readIndexedUInt8(pagelocs_offsets, usedlocs[i]);
        uint8_t numpagelocs = FX::readIndexedUInt8(pagelocs_raw, offset);
        
        //Now pick some random location and load it into the page location table
        FX::readDataBytes(pagelocs_raw + offset + 1 + ((rand() % numpagelocs) << 1), pagelocs + (i << 1), 2);
    }
    #endif
}

bool isSolid(uflot x, uflot y)
{
    // The location is solid if the map cell is nonzero OR if we're colliding with
    // any (solid) bounding boxes
    uint8_t tile = raycast.worldMap.getCell(x.getInteger(), y.getInteger());

    return (tile != 0) || raycast.sprites.firstColliding(x, y, RBSTATESOLID) != NULL;
}

// Perform ONLY player movement updates! No drawing!
void movement()
{
    float movement = 0;
    float rotation = 0;

    // move forward if no wall in front of you
    if (arduboy.pressed(UP_BUTTON))
        movement = MOVESPEED;
    if (arduboy.pressed(DOWN_BUTTON))
        movement = -MOVESPEED;

    if (arduboy.pressed(RIGHT_BUTTON))
        rotation = -ROTSPEED;
    if (arduboy.pressed(LEFT_BUTTON))
        rotation = ROTSPEED;

    if (arduboy.pressed(B_BUTTON) && (movement || rotation))
    {
        #ifndef INFINITESPRINT
        //We're mean and always drain sprint meter if holding B and moving
        sprintmeter -= SPRINTDRAIN;
        #endif

        if(sprintmeter < 0)
            sprintmeter = 0;

        //You're allowed to run if you're already running and there's any sprint left,
        //or if you 'just started' running and there's minimum sprint meter
        if(holding_b && sprintmeter > 0 || sprintmeter > SPRINTMIN)
        {
            rotation *= ROTMULTIPLIER;
            movement *= MOVEMULTIPLIER;
        }

        //End with us knowing if they're holding B
        holding_b = true;
    }
    else
    {
        uint16_t newsprint = sprintmeter + SPRINTRECOVER;
        sprintmeter = min(newsprint, SPRINTMAX);
        holding_b = false;
    }

    #ifdef ARESTARTS
    if(arduboy.justPressed(A_BUTTON))
    {
        newgame();
        //sound.tones(drone);
    }
    #endif

    walkingSound(movement);

    raycast.player.tryMovement(movement, rotation, &isSolid);

    checkPagePickup();

    bool reload = false;

    int8_t ofs_x = ((flot)raycast.player.posX - CAGEX).getInteger();
    int8_t ofs_y = ((flot)raycast.player.posY - CAGEY).getInteger();

    if(ofs_x) {
        world_x += ofs_x;
        raycast.player.posX -= ofs_x;
        reload = true;
    }
    if(ofs_y) {
        world_y += ofs_y;
        raycast.player.posY -= ofs_y;
        reload = true;
    }

    if(reload)
    {
        shift_sprites(-ofs_x, -ofs_y);
        load_sprites(ofs_x, ofs_y);
        load_region();
    }
}

void walkingSound(float movement)
{
    moveaccum += abs(movement);

    if(moveaccum > WALKSOUNDTRIGGER)
    {
        moveaccum = 0;
        sound.tones(crunches + (rand() % CRUNCHES) * CRUNCHLEN);
    }
}

void bgSound()
{
    // Sound only after first page
    if(page_bitflag)
    {
        bgsoundtimer++;

        if(bgsoundtimer > BGSOUNDTRIGGER && !sound.playing())
        {
            sound.tones(drone);
            bgsoundtimer = 0;
        }
    }
}

void checkPagePickup()
{
    RcBounds * colliding_page = raycast.sprites.firstColliding(raycast.player.posX, raycast.player.posY, PAGEMASK);

    if(colliding_page)
    {
        RcSprite<NUMINTERNALBYTES> * page_sprite = raycast.sprites.getLinkedSprite(colliding_page);

        if(page_sprite)
        {
            sound.tones(pagepickup);
            current_pageview = page_sprite->intstate[0] + 1;
            memset(pagelocs + (current_pageview - 1) * 2, 0xFF, 2); //Don't let it show up again
            raycast.sprites.deleteLinked(page_sprite); //Immediately despawn the page
        }
    }
}

void waitPageDismiss()
{
    if(arduboy.justPressed(A_BUTTON))
    {
        page_bitflag |= (1 << (current_pageview - 1));
        current_pageview = 0;

        drawSidebar();

        //Only on page dismiss do we start the "timer"
        timer1 = arduboy.frameCount;
    }
}

void behavior_page(RcSprite<NUMINTERNALBYTES> * sprite)
{
    sprite->setHeight(4 * sin(arduboy.frameCount / 6.0));
}

void behavior_slender(RcSprite<NUMINTERNALBYTES> * sprite)
{
    //Calculate real location. Everything is relative to the player, hence offsetting by the cage center
    int8_t x = slenderX - world_x + CAGEX;
    int8_t y = slenderY - world_y + CAGEY;

    uint8_t numpages = numfoundpages();

    //If not possible, get him out
    if(x >= RCMAXMAPDIMENSION || y >= RCMAXMAPDIMENSION || x < 0 || y < 0)
    {
        sprite->x = 0;
        sprite->y = 0;

        //If he's not on the playing field with the player, we should find a new place to put him.
        //Though, perhaps that should be tied to some kind of aggression stat?

        if(numpages > 0)
        {
            uint8_t locdata[SLENDERLOCBYTES];
            FX::readDataBytes(slenderlocs_trueraw + 1 + (uint16_t)slenderLocScan * SLENDERLOCBYTES, locdata, SLENDERLOCBYTES);

            float dx = locdata[0] - world_x;
            float dy = locdata[1] - world_y;

            //Get the distance to this location
            float locdistance = sqrt(dx * dx + dy * dy);

            float minmax[2];
            memcpy_P(minmax, TELEPORTDISTANCE + 2 * sizeof(float) * numpages, 2 * sizeof(float));

            //The distance has to be within some range for us to use it. We also don't want slenderman teleporting
            //when he's too close to the player (or at least, for most of the page) so we prevent that too

            if(locdistance > minmax[0] && locdistance < minmax[1])
            {
                slenderX = locdata[0];
                slenderY = locdata[1];
                sound.tone(400, 50);
            }

            //Scan just one location per frame.
            slenderLocScan = (slenderLocScan + 1) % slenderLocTotal;

            if(slenderLocScan == 0)
            {
                sound.tone(200, 10);
            }
        }
    }
    else
    {
        sprite->x = x + 0.5;
        sprite->y = y + 0.5;

        float dx = (float)sprite->x - (float)raycast.player.posX;
        float dy = (float)sprite->y - (float)raycast.player.posY;

        //calc distance. We may not need sqrt, watch for it
        float distance = sqrt(dx * dx + dy * dy);

        //Calc the two angles
        float pangle = raycast.player.getAngle();
        float sangle = atan2(dy, dx); 

        //Slender is in front of the player if the difference of angles puts it on the right of the 
        //unit circle. This works because:
        //-If player is facing slender head on, angle diff = 0
        //-If slender is perfectly to left of player, angle diff = 90 degrees
        //-If slender is perfectly to right of player, angle diff = 270 degrees
        //So range is 90 - 270 degrees, which is the full right side of unit circle
        bool infront = cos(sangle - pangle) > FRONTFOCAL;

        #ifdef PRINTSTATIC
        arduboy.fillRect(105, 1, 20, 20, BLACK);
        tinyfont.setTextColor(WHITE);
        #endif

        if(infront && distance < MINSTATICDISTANCE)
        {
            SFixed<13,2> accumnow;
            memcpy_P(&accumnow, STATICACCUMS + (uint8_t)distance, sizeof(SFixed<13,2>));
            staticaccum += accumnow;

            #ifdef PRINTSTATIC
            tinyfont.setCursor(105, 6);
            tinyfont.print((float)accumnow, 2);
            #endif
        }
        else
        {
            staticaccum -= STATICDRAIN;
        }

        //Cut it off from being too ridiculous
        if(staticaccum > 255) staticaccum = 255;
        if(staticaccum < 0) staticaccum = 0;

        if(distance < MINSTATICDISTANCE)
            staticbase = pgm_read_byte(STATICBASES + (uint8_t)(distance * 2));
        else
            staticbase = 0;

        #ifdef PRINTSTATIC
        tinyfont.setCursor(105, 1);
        tinyfont.print(staticbase);
        tinyfont.setCursor(105, 11);
        tinyfont.print((float)staticaccum, 1);
        #endif

        //now that we figured out all that death crap, let's see if we should be teleporting him.
        //There are some rules based on his amount of aggression. Aggression increases the more pages
        //you have, and also accumulates the longer it takes for you to find a page. Finding a page resets
        //the accumulated aggression but the base aggression increases.
        uint8_t aggression = numpages - 1 + uint8_t((arduboy.frameCount - timer1) / AGGRESSIONTIME);
        if(aggression > 8) aggression = 8;

        #ifdef PRINTAGGRESSION
        arduboy.fillRect(105, 1, 20, 20, BLACK);
        tinyfont.setTextColor(WHITE);
        tinyfont.setCursor(105, 1);
        tinyfont.print(aggression);
        tinyfont.setCursor(105, 6);
        tinyfont.print(slenderX);
        tinyfont.print(" ");
        tinyfont.print(slenderY);
        #endif
    }

}

//Shift ALL sprites by the given amount in x and y (whole numbers only please!). Also
//erases sprites that go outside the usable area
void shift_sprites(int8_t x, int8_t y)
{
    //We allocate slenderman in slot 0, so we skip that in calculations
    for(uint8_t i = 1; i < NUMSPRITES; i++)
    {
        RcSprite<NUMINTERNALBYTES> * sp = raycast.sprites[i];
        if(sp->isActive())
        {
            sp->x += x;
            sp->y += y;

            if(sp->x < SPRITEBEGIN_X || sp->y < SPRITEBEGIN_Y || sp->x >= SPRITEEND_X + 1 || sp->y >= SPRITEEND_Y + 1)    
            {
                raycast.sprites.deleteLinked(sp);
            }
            else
            {
                RcBounds *bp = raycast.sprites.getLinkedBounds(sp);
                if (bp)
                {
                    bp->x1 += x;
                    bp->y1 += y;
                    bp->x2 += x;
                    bp->y2 += y;
                }
            }
        }
    }
}

// Load ALL sprites within the sprite view range
void load_surrounding_sprites()
{
    for(uint8_t y = SPRITEBEGIN_Y; y <= SPRITEEND_Y; y++)
    {
        for(uint8_t x = SPRITEBEGIN_X; x <= SPRITEEND_X; x++)
        {
            load_sprite(world_x - (CAGEX - x), world_y - (CAGEY - y), x, y);
        }
    }
}

// Figure out which sprites to load based on movement. May need to be changed if loading 
// diagonally causes problems
void load_sprites(int8_t ofs_x, int8_t ofs_y)
{
    //local row/column, global row/column
    uint8_t lrx, lry;
    uint8_t lcx, lcy;
    uint8_t grx, gry;
    uint8_t gcx, gcy;
    //We ALWAYS do all of a column (ofs_x), but rows will skip the first or last element if both row/column 
    //is loaded depending on direction moving.
    uint8_t skipx = 0; 

    //These are essentially constants
    lcy = SPRITEBEGIN_Y;
    gcy = world_y - SPRITEVIEW;
    lrx = SPRITEBEGIN_X;
    grx = world_x - SPRITEVIEW;

    if(ofs_x < 0) {
        lcx = SPRITEBEGIN_X;
        gcx = world_x - SPRITEVIEW;
        skipx = 0;  //Moving left, skip leftmost element on row if needed
    }
    else {
        lcx = SPRITEEND_X;
        gcx = world_x + SPRITEVIEW;
        skipx = SPRITEVIEW * 2; //Moving right, so skip rightmost on row
    }

    if(ofs_y < 0) {
        lry = SPRITEBEGIN_Y;
        gry = world_y - SPRITEVIEW;
    }
    else {
        lry = SPRITEEND_Y;
        gry = world_y + SPRITEVIEW;
    }

    //Direction doesn't really matter, there's no cache or anything
    for(uint8_t i = 0; i <= SPRITEVIEW * 2; i++)
    {
        //row and column. Only load the final column sprite if there's no row
        if(ofs_x) {
            load_sprite(gcx, gcy + i, lcx, lcy + i);
        }
        if(ofs_y && !(ofs_x && i == skipx)) { // skip one of the row loads if it intersects with the one column load
            load_sprite(grx + i, gry, lrx + i, lry);
        }
    }
}

// Load sprite at the exact location given. Will load nothing if nothing there...
void load_sprite(uint8_t x, uint8_t y, uint8_t local_x, uint8_t local_y)
{
    //Oops, trying to load outside the map.
    if(x >= staticmap_width || y >= staticmap_height || x < 0 || y < 0)
        return;
    
    //Load pages if they exist. This could become a performance issue?
    for(uint8_t i = 0; i < NUMPAGES * 2; i += 2)
    {
        if(x == pagelocs[i] && y == pagelocs[i + 1])
            load_pagesprite(local_x, local_y, i >> 1);
    }

    uint8_t buffer[staticsprites_bytes];

    FX::readDataBytes(staticsprites_fx + staticsprites_bytes * (x + y * (uint16_t)staticmap_width), buffer, staticsprites_bytes);

    //Oops, no sprite here!
    if(!buffer[0]) return;

    struct SpriteMeta meta;
    memcpy_P(&meta, SPRITEMETAS + buffer[0], sizeof(struct SpriteMeta));

    //Try to add a sprite. We figure out the scale and accompanying bounding box based on frame (later)
    RcSprite<NUMINTERNALBYTES> * sp = raycast.sprites.addSprite(
        local_x + muflot::fromInternal(buffer[1] & 0x0F), local_y + muflot::fromInternal(buffer[1] / 16), 
        buffer[0], meta.scale, meta.offset, NULL);

    if(sp && meta.bounds != 0) {
        raycast.sprites.addSpriteBounds(sp, meta.bounds, meta.solid);
    }
}

void load_pagesprite(uint8_t local_x, uint8_t local_y, uint8_t page)
{
    // Load the page
    RcSprite<NUMINTERNALBYTES> *sp = raycast.sprites.addSprite(
        local_x + 0.5, local_y + 0.5,
        PAGESPRITE, PAGESCALE, 0, behavior_page);

    if (!sp)
    {
        // Need to panic or something
    }

    sp->intstate[0] = page;
    RcBounds * bounds = raycast.sprites.addSpriteBounds(sp, 0.75, false);
    bounds->state |= PAGEMASK; // Top bit
}

// Reload the map for the local region. We'll see if this is too slow...
void load_region()
{
    int16_t left = world_x - CAGEX;
    int16_t top = world_y - CAGEY;
    int16_t right = CAGEX + world_x;
    int16_t bottom = CAGEY + world_y;

    uint8_t world_begin_x, world_begin_y;
    uint8_t map_begin_x, map_begin_y;
    uint8_t map_end_x = LOADWIDTH - 1 - (right >= staticmap_width ? 1 + right - staticmap_width : 0);
    uint8_t map_end_y = LOADHEIGHT - 1 - (bottom >= staticmap_height ? 1 + bottom - staticmap_height : 0);

    if(left < 0) { 
        world_begin_x = 0; 
        map_begin_x = -left; 
    }
    else { 
        world_begin_x = left; 
        map_begin_x = 0; 
    }
    if(top < 0) { 
        world_begin_y = 0; 
        map_begin_y = -top; 
    }
    else { 
        world_begin_y = top; 
        map_begin_y = 0; 
    }

    uint8_t writelen = 1 + map_end_x - map_begin_x;
    for(uint8_t y = map_begin_y; y <= map_end_y; y++) {
        FX::readDataBytes(staticmap_fx + (world_begin_y++ * (uint16_t)staticmap_width + world_begin_x), raycast.worldMap.map + (y * (uint16_t)RCMAXMAPDIMENSION + map_begin_x), writelen);
    }
}


void drawRotBg()
{
    //The offset into the over-wide bg in fx 
    uint16_t offset = (2 * M_PI - raycast.player.getAngle()) * ROTBGSCALE;
    
    //We simply copy the buffer into the screen. That's all
    for(uint8_t y = 0; y < HEIGHT / 8; y++)
        FX::readDataBytes(current_bg + offset + y * rotbgWidth, arduboy.sBuffer + y * WIDTH, SCREENWIDTH);
}

void drawSidebar()
{
    fastClear(&arduboy, raycast.render.VIEWWIDTH, 0, WIDTH,HEIGHT);
    arduboy.drawFastVLine(raycast.render.VIEWWIDTH + 1, 0, HEIGHT, WHITE);
    constexpr uint8_t PAGEWIDTH = 6;
    constexpr uint8_t PAGEHEIGHT = 8;
    constexpr uint8_t PAGESPACE = 3;

    for(uint8_t i = 0; i < NUMPAGES; i++)
    {
        uint8_t x = raycast.render.VIEWWIDTH + 7 + (PAGESPACE + PAGEWIDTH) * (i & 1);
        uint8_t y = 7 + (PAGEHEIGHT + PAGESPACE) * (i / 2);

        if(page_bitflag & (1 << i)) {
            arduboy.fillRect(x, y, PAGEWIDTH, PAGEHEIGHT, WHITE);
        }
        else {
            arduboy.drawRect(x, y, PAGEWIDTH, PAGEHEIGHT, WHITE);
        }
    }
}

void drawPage(uint8_t page)
{
    FX::drawBitmap(26, 0, pages, page, dbmNormal);
}

void drawSprintMeter()
{
    arduboy.drawFastHLine(raycast.render.VIEWWIDTH + 7, HEIGHT - 4, 15, BLACK);
    arduboy.drawFastHLine(raycast.render.VIEWWIDTH + 7, HEIGHT - 4, 15 * sprintmeter / SPRINTMAX, WHITE);
}


void drawStatic(uint8_t amount) //uint16_t amount)
{
    //Prevent drawing static MOST of the time (it's somewhat expensive)
    if(!amount) return;

    //Only change static slowly, at a rate dependent on framerate. Changing 
    //AT the framerate is hard to look at, even for me. Even as is, it's probably
    //still too flashy for a lot of people
    if((arduboy.frameCount % STATICFREQUENCY) == 0)
        lastStatic = rand() % amount;

    shadeScreen<WHITE>(&arduboy, (float)lastStatic / 256, 0, 0, SCREENWIDTH, HEIGHT);
}

void doMenu()
{
    if(arduboy.justPressed(LEFT_BUTTON) || arduboy.justPressed(RIGHT_BUTTON))
        arduboy.audio.toggle();
    
    bool audio = arduboy.audio.enabled();
    constexpr uint8_t x1 = 21;
    constexpr uint8_t x2 = 74;
    constexpr uint8_t y = 12;
    constexpr uint8_t pad = 2;

    arduboy.fillRect(x1 - pad, y - pad, 32 + 2 * pad, 32 + 2 * pad, audio ? WHITE : BLACK);
    arduboy.fillRect(x2 - pad, y - pad, 32 + 2 * pad, 32 + 2 * pad, audio ? BLACK : WHITE);

    FX::drawBitmap(x1, y, soundgraphic, 0, dbmInvert); 
    FX::drawBitmap(x2, y, soundgraphic, 1, dbmInvert); 

    tinyfont.setTextColor(WHITE);
    tinyfont.setCursor(26, 54);
    tinyfont.print(F("EPILEPSY WARNING"));

    if(timer1)
    {
        shadeScreen<BLACK>(&arduboy, 1 - (timer1 - arduboy.frameCount) / (float)STDFADE, 0, 0, WIDTH, HEIGHT);

        if(arduboy.frameCount >= timer1)
            state = GameState::Intro;
    }
    else if(arduboy.justPressed(A_BUTTON))
    {
        arduboy.audio.saveOnOff();
        timer1 = arduboy.frameCount + STDFADE;
    }
}

bool doTimedEvent(uint16_t time)
{
    if(arduboy.frameCount > timer1 + time)
    {
        timer1 = arduboy.frameCount;
        current_pageview++;
        return true;
    }

    return false;
}

void doIntro()
{
    //This is the first time we're calling this, get out
    if(current_pageview == 0)
    {
        timer1 = arduboy.frameCount;
        current_pageview = 1;
        return;
    }

    //Now for the rest, we use timer in the opposite way, it's actually the start
    //of the intro and we do things based on how far we are from it
    uint16_t introt = arduboy.frameCount - timer1;

    constexpr uint16_t text1black = FRAMERATE * 1.5;
    constexpr uint16_t text1show = FRAMERATE * 1.5;
    constexpr uint16_t text2show = FRAMERATE * 2.5;
    constexpr uint16_t text2black = FRAMERATE * 3.5;

    constexpr uint16_t text1end = text1black + text1show + STDFADE * 2;
    constexpr uint16_t text2end = text1end + text2show + STDFADE * 2 + text2black;
    constexpr uint16_t text1fadef = text1black + STDFADE + text1show;
    constexpr uint16_t text2fadef = text1end + STDFADE + text2show;

    if(introt < text1black)
    {
        // Do nothing for now
    } 
    else if(introt < text1end)
    {
        tinyfont.setCursor(26, 29);
        tinyfont.print(F("haloopdy - 2023"));

        if(introt < text1black + STDFADE) //fade in
            shadeScreen<BLACK>(&arduboy, 1 - (introt - text1black) / (float)STDFADE, 0, 0, WIDTH, HEIGHT);
        else if(introt > text1fadef) //fade out
            shadeScreen<BLACK>(&arduboy, (introt - text1fadef) / (float)STDFADE, 0, 0, WIDTH, HEIGHT);
    }
    else if(introt < text2end)
    {
        tinyfont.setCursor(42, 20);
        tinyfont.print(F("Based on:"));
        tinyfont.setCursor(5, 26);
        tinyfont.print(F("Slender: The Eight Pages"));
        tinyfont.setCursor(10, 38);
        tinyfont.print(F("by Parsec Productions"));

        if(introt < text1end + STDFADE) //fade in
            shadeScreen<BLACK>(&arduboy, 1 - (introt - text1end) / (float)STDFADE, 0, 0, WIDTH, HEIGHT);
        else if(introt > text2fadef) //fade out
            shadeScreen<BLACK>(&arduboy, min(1.0f, (introt - text2fadef) / (float)STDFADE), 0, 0, WIDTH, HEIGHT);
    }
    else
    {
        newgame();
        return;
    }

    constexpr uint16_t walkend = text2fadef + STDFADE / 1.5;

    if(introt < walkend)
    {
        walkingSound(MOVESPEED);
    }
    else if(introt > walkend + FRAMERATE / 2 && current_pageview == 1)
    {
        current_pageview = 2;
        sound.tones(climbfence);
    }
}

void doEscape()
{
    // So for "style", I guess I might leave it stuck in the escape text
    if(current_pageview == 0)
    {
        timer1 = arduboy.frameCount + FRAMERATE * 1;
        current_pageview = 1;
    }
    else if(arduboy.frameCount > timer1)
    {
        tinyfont.setCursor(38, 30);
        tinyfont.setTextColor(BLACK);
        tinyfont.print(F("YOU ESCAPED"));
    }
}

void doGameOver()
{
    constexpr uint16_t flashingtime = FRAMERATE * 1.5;
    constexpr uint16_t reprisetime = FRAMERATE * 1.5;
    constexpr uint16_t flashingfrequency = 1;

    //Just for ease, every frame of game over will be white
    shadeScreen<WHITE>(&arduboy, 1.0f, 0, 0, WIDTH, HEIGHT);

    if(current_pageview == 0)
    {
        timer1 = arduboy.frameCount;
        current_pageview = 1;
    }
    else if(current_pageview == 1)
    {
        //For the first x time, we do a random flash for some amount
        if((arduboy.frameCount % flashingfrequency) == 0)
            lastStatic = rand() % 3;
        
        if(lastStatic == 0 || arduboy.frameCount == timer1 + flashingtime)
        {
            //Display a (hopefully flash) of slenderman. The frame is dependent on how far you are through
            //the frame.
            uint8_t frame = min(3, 4 * float(arduboy.frameCount - timer1) / flashingtime);
            FX::drawBitmap(0, 0, gameover, frame, dbmNormal);
            sound.tones(screeching);
        }

        doTimedEvent(flashingtime);
    }
    else if(current_pageview == 2)
    {
        doTimedEvent(reprisetime);
    }
    else if(current_pageview == 3)
    {
        tinyfont.setCursor(42, 30);
        tinyfont.setTextColor(BLACK);
        tinyfont.print(F("TRY AGAIN"));

        if(arduboy.justPressed(A_BUTTON))
        {
            timer1 = arduboy.frameCount;
            current_pageview++;
        }
    }
    else if(current_pageview == 4)
    {
        shadeScreen<BLACK>(&arduboy, min(1.0f, float(arduboy.frameCount - timer1) / STDFADE), 0, 0, WIDTH, HEIGHT);
        doTimedEvent(STDFADE);
    }
    else if(current_pageview == 5)
    {
        shadeScreen<BLACK>(&arduboy, 1.0f, 0, 0, WIDTH, HEIGHT);
        if(doTimedEvent(STDFADE))
            newgame();
    }
}



void loop()
{
    if(!arduboy.nextFrame()) return;

    arduboy.pollButtons();

    if(state == GameState::Menu)
    {
        doMenu();
    }
    else if(state == GameState::Intro)
    {
        doIntro();
    }
    else if(state == GameState::Escape)
    {
        doEscape();
    }
    else if(state == GameState::Gameover)
    {
        doGameOver();
    }
    else
    {
        // Process player movement + interaction
        if(current_pageview)
            waitPageDismiss();
        else
            movement();

        // Draw the correct background for the area.
        drawRotBg();
        drawSprintMeter();

        raycast.runIteration(&arduboy);

        if(current_pageview)
        {
            //"Reset" accumulated static when viewing a page. This is to give the player some respite
            staticaccum = 0;
            drawPage(current_pageview - 1);
        }

        #ifdef DRAWMAP
        raycast.worldMap.drawMap(&arduboy, 105, 0);
        #endif

        if(page_bitflag == 255)
        {
            //these really only need to be set once but whatever
            slenderX = 255;
            slenderY = 255;

            timer1++;
            constexpr uint16_t initialfade = FRAMERATE * 6;
            constexpr uint16_t secondaryfade = FRAMERATE * 10 + initialfade;
            constexpr uint16_t finalcut = secondaryfade + FRAMERATE * 2.5f;

            if(timer1 < initialfade)
            {
                shadeScreen<BLACK>(&arduboy, min(1.0f, timer1 / (FRAMERATE * 4.5f)), 0, 0, WIDTH, HEIGHT);
                if(timer1 > FRAMERATE * 4.5)
                    moveaccum = 0; //Disable sound? kind of a hack
            }
            else if(timer1 == initialfade)
            {
                raycast.render.altWallShading = RcShadingType::None;
                raycast.render.shading = RcShadingType::White;
                raycast.render.spriteShading = RcShadingType::White;
                raycast.render.setLightIntensity(DAYLIGHT);
                current_bg = rotbg_day;
            }
            else if(timer1 > secondaryfade)
            {
                shadeScreen<WHITE>(&arduboy, min(1.0f, (timer1 - secondaryfade) / (FRAMERATE * 2.5f)), 0, 0, WIDTH, HEIGHT);

                if(timer1 == finalcut)
                    changeStateClean(GameState::Escape);
            }
        }
        else
        {
            //These are things that happen when you haven't won yet
            bgSound();
            uint8_t s = (uint8_t)min(255, staticaccum + (SFixed<13,2>)staticbase); 
            if(s == 255)
                changeStateClean(GameState::Gameover);
            drawStatic(s); //This kind of only works if you call it every frame
        }
    }

    FX::display(false);
}
