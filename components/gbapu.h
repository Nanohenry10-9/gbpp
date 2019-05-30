#include <iostream>
#include "gbmem.h"
#include <SDL.h>

using namespace std;

#pragma once

class gbapu {
public:
    uint8_t pulse12_5[8];
    uint8_t pulse25_0[8];
    uint8_t pulse50_0[8];
    uint8_t pulse75_0[8];
    SDL_AudioSpec want, have;
    SDL_AudioDeviceID dev;

    void init();

    void tick(gbmem* mem, uint16_t timer);
};