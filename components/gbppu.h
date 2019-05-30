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
    SDL_Texture *lastFrame;
    uint8_t spritesOnLine[10];
    uint8_t spriteIndex;

    void init(SDL_Texture* screen);

    SDL_Texture* getFrame();
    void render(SDL_Texture* t);
    void renderTilemap(SDL_Texture* t, gbmem* mem);

    void tick(gbmem* mem);

    void scanlineAdvance(gbmem* mem);

    uint8_t getTilePixel(gbmem* mem, uint16_t ti, uint8_t x, uint8_t y);
    uint8_t getBGMapPixel(gbmem* mem, uint16_t tx, uint16_t ty, uint8_t x, uint8_t y);
    uint8_t getSpriteTilePixel(gbmem* mem, uint8_t ti, uint8_t x, uint8_t y);

    uint16_t getBGTileAddress(gbmem* mem);
    uint16_t getBGMapAddress(gbmem* mem);
};