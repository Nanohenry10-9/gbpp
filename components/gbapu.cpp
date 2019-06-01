#include "gbapu.h"

#define NR10 0xFF10
#define NR11 0xFF11
#define NR12 0xFF12
#define NR13 0xFF13
#define NR14 0xFF14

#define NR21 0xFF16
#define NR22 0xFF17
#define NR23 0xFF18
#define NR24 0xFF19

#define NR30 0xFF1A
#define NR31 0xFF1B
#define NR32 0xFF1C
#define NR33 0xFF1D
#define NR34 0xFF1E

#define NR41 0xFF20
#define NR42 0xFF21
#define NR43 0xFF22
#define NR44 0xFF23

#define NR50 0xFF24
#define NR51 0xFF25
#define NR52 0xFF26

#define WAVE 0xFF30

using namespace std;

gbmem *aMem;

SDL_AudioSpec want, have;
SDL_AudioDeviceID dev;

uint16_t internalTimer = 0;

void gbapu::init(gbmem* cMem) {
    aMem = cMem;

    // nothing here yet :(
}

void gbapu::tick(uint16_t timer) {
    internalTimer = timer;
}