#include <iostream>
#include <SDL.h>
#include <SDL_render.h>
#include "gbmem.h"

using namespace std;

#pragma once

class gbppu {
public:
    uint16_t LX;
    double frameCount;
    uint32_t pixels[160 * 144 * 4];
    uint8_t buffers[2][160 * 144];
    bool debug;
    uint32_t map[8 * 8 * 48 * 8];
    uint32_t bgMap[32 * 8 * 32 * 8];
    SDL_Texture *lastFrame, *tm, *bg;
    gbmem *pMem;
    uint8_t spritesOnLine[10];
    uint8_t spriteIndex;

    void init(gbmem* cMem, SDL_Texture* screen, SDL_Texture* e, SDL_Texture* t);
    
    void render();
    void renderTilemap(); // This function is very resource-heavy, as it renders two 256x256 tilemaps.

    void tick();

    void scanlineAdvance();

    uint8_t getTilePixel(uint16_t ti, uint8_t x, uint8_t y);
    uint8_t getBGMapPixel(uint16_t tx, uint16_t ty, uint8_t x, uint8_t y);
    uint8_t getSpriteTilePixel(uint8_t ti, uint8_t x, uint8_t y);

    uint16_t getBGTileAddress();
    uint16_t getBGMapAddress();
};