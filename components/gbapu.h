#include <iostream>
#include <SDL.h>
#include <cmath>
#include <windows.h>
#include "gbmem.h"

using namespace std;

#pragma once

class gbapu {
public:
    gbmem *aMem;

    SDL_AudioSpec want, have;
    SDL_AudioDeviceID dev;

    void init(gbmem* mem);

    void tick(uint16_t timer);
};