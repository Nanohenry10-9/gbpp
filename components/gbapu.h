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

    bool disable, prevDis;

    SDL_AudioSpec want, have;
    SDL_AudioDeviceID dev;

    void channels(bool a, bool b, bool c, bool d);
    void toggleChannel(int c);

    void init(gbmem* mem);

    void tick();
};