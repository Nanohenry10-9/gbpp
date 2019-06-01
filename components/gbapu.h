#include <iostream>
#include "gbmem.h"
#include <SDL.h>
#include <cmath>

using namespace std;

#pragma once

class gbapu {
public:
    gbmem *aMem;

    SDL_AudioSpec want, have;
    SDL_AudioDeviceID dev;

    void init(gbmem* cMem);

    void tick(uint16_t timer);
};