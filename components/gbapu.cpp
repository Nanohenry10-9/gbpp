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

#define SAMPLE_RATE 44100

#define TEST(v, b) ((v & (1 << b)) == (1 << b))

using namespace std;

gbmem *aMem;

bool disable, prevDis;

SDL_AudioSpec want, have;
SDL_AudioDeviceID dev;

uint16_t internalTimer;
int sampleIndex;

double pulse1V;
double pulse2V;
double pulse1F, pulse2F;
double pulse1P , pulse2P;
uint16_t prevP1F, prevP2F;

uint8_t waveS;
double waveF;
uint16_t prevWF;
uint8_t waveBuffer[32];

double noiseV;
double noiseF;
uint8_t noiseDiv;
uint8_t noiseB;
uint16_t prevNF;
bool noiseLevel;
uint16_t lfsr;

#define UNIT 1000000.0

static double pulseWave(int time, double f, double d) {
    if (f == 0) {
        return 0;
    }
    return double(time % int(UNIT / f)) < (d * int(UNIT / f));
}

static uint8_t getWave(int time, double f) {
    if (f == 0) {
        return 0;
    }
    return waveBuffer[int(double(time % int(UNIT / f)) / (UNIT / f) * 32)];
}

/*static void tickLFSR() {
    bool f = (lfsr & 1) ^ ((lfsr & 2) >> 1);
    lfsr >>= 1;
    lfsr |= (f << 14);
    if (noiseB == 7) {
        lfsr |= (f << 6);
    }
    noiseLevel = !(lfsr & 1);
}*/

void audioCallback(void *userData, uint8_t *rawBuffer, int bytes) {
    int &sampleIndex = *(int*)userData;
    uint16_t *buffer = (uint16_t*)rawBuffer;
    for(int i = 0; i < bytes / 2; i++, sampleIndex++) {
        double time = (double)sampleIndex / SAMPLE_RATE; // [0, 1)
        uint16_t pulse1 = 0, pulse2 = 0, wave = 0, noise = 0;
        if (pulse1V > 0 && pulse1F > 0 && pulse1P > 0) {
            pulse1 = pulseWave(time * UNIT, pulse1F, pulse1P) * pulse1V * (0xFFFF >> 4);
        }
        if (pulse2V > 0 && pulse2F > 0 && pulse2P > 0) {
            pulse2 = pulseWave(time * UNIT, pulse2F, pulse2P) * pulse2V * (0xFFFF >> 4);
        }
        if (waveS != 4 && waveF > 0) {
            wave = (getWave(time * UNIT, waveF) >> waveS) << 8;
        }
        if (noiseV > 0 && noiseF > 0) {
            /*if (int(time * UNIT) % int(UNIT / noiseF) == 0) {
                tickLFSR();
            }*/
            noise = ((rand() & 1) * 0x0FFF) * noiseV;//double(noiseLevel * (0xFFFF >> 4)) * noiseV;
        }
        buffer[i] = pulse1 + pulse2 + wave + noise;
    }
}

void gbapu::init(gbmem* mem) {
    aMem = mem;

    want.freq = SAMPLE_RATE;
    want.format = AUDIO_U16;
    want.channels = 1;
    want.samples = 2048;
    want.callback = audioCallback;
    want.userdata = &sampleIndex;

    if(SDL_OpenAudio(&want, &have) != 0) {
        SDL_Log("Failed to open audio: %s", SDL_GetError());
    }

    noiseB = 7;

    lfsr = rand() & 0xFFFF;

    SDL_PauseAudio(0);
}

const double dutyCycles[4] = {0.125, 0.25, 0.5, 0.75};
const uint8_t waveShifts[4] = {4, 0, 1, 2};
const uint8_t noiseDivisors[8] = {8, 16, 32, 48, 64, 80, 96, 112};

void gbapu::tick(uint16_t timer) {
    if (disable && !prevDis) {
        SDL_PauseAudio(1);
        SDL_Log("Audio stopped");
    } else if (!disable && prevDis) {
        SDL_PauseAudio(0);
        SDL_Log("Audio started");
    }
    prevDis = disable;

    internalTimer = timer;

    if (TEST(aMem->read(NR14), 7)) {
        aMem->write(NR52, aMem->read(NR52) | 0x01);
        aMem->write(NR14, aMem->read(NR14) & ~0x80);
        pulse1V = ((aMem->read(NR12) & 0b11110000) >> 4) / 15.0;
        if ((aMem->read(NR11) & 0b00111111) == 0) {
            aMem->write(NR11, aMem->read(NR11) | 0b00111111);
        }
    }
    pulse1P = dutyCycles[(aMem->read(NR11) & 0xC0) >> 6];
    int rawFreq1 = aMem->read(NR13) | ((aMem->read(NR14) & 0b0111) << 8);
    if (prevP1F != rawFreq1) {
        pulse1F = 131072.0 / (2048 - rawFreq1);
    }
    prevP1F = rawFreq1;
    if (TEST(aMem->read(NR14), 6) && internalTimer % 16384 == 0) { // duration counter
        if ((aMem->read(NR11) & 0b00111111) > 0) {
            aMem->write(NR11, (aMem->read(NR11) & 0b11000000) | ((aMem->read(NR11) & 0b00111111) - 1));
        } else {
            pulse1V = 0;
            aMem->write(NR52, aMem->read(NR52) & ~0x01);
        }
    }
    if (internalTimer == 0 && (aMem->read(NR12) & 0b0111) != 0) { // volume sweep
        double newVol = pulse1V + ((aMem->read(NR12) & 0b1000)? (1.0 / 15.0) : -(1.0 / 15.0));
        if (newVol >= 0 && newVol <= 1) {
            pulse1V = newVol;
        }
    }

    if (TEST(aMem->read(NR24), 7)) {
        aMem->write(NR52, aMem->read(NR52) | 0x02);
        aMem->write(NR24, aMem->read(NR24) & ~0x80);
        pulse2V = ((aMem->read(NR22) & 0b11110000) >> 4) / 15.0;
        if ((aMem->read(NR21) & 0b00111111) == 0) {
            aMem->write(NR21, aMem->read(NR21) | 0b00111111);
        }
    }
    pulse2P = dutyCycles[(aMem->read(NR21) & 0xC0) >> 6];
    int rawFreq2 = aMem->read(NR23) | ((aMem->read(NR24) & 0b0111) << 8);
    if (prevP2F != rawFreq2) {
        pulse2F = 131072.0 / (2048 - rawFreq2);
    }
    prevP2F = rawFreq2;
    if (TEST(aMem->read(NR24), 6) && internalTimer % 16384 == 0) {
        if ((aMem->read(NR21) & 0b00111111) > 0) {
            aMem->write(NR21, (aMem->read(NR21) & 0b11000000) | ((aMem->read(NR21) & 0b00111111) - 1));
        } else {
            pulse2V = 0;
            aMem->write(NR52, aMem->read(NR52) & ~0x02);
        }
    }
    if (internalTimer == 0 && (aMem->read(NR22) & 0b0111) != 0) {
        double newVol = pulse2V + ((aMem->read(NR22) & 0b1000)? (1.0 / 15.0) : -(1.0 / 15.0));
        if (newVol >= 0 && newVol <= 1) {
            pulse2V = newVol;
        }
    }

    if (!TEST(aMem->read(NR30), 7)) {
        waveS = 4;
    } else {
        waveS = waveShifts[(aMem->read(NR32) & 0b01100000) >> 5];
    }
    if (waveS != 4) {
        int rawFreq3 = aMem->read(NR33) | ((aMem->read(NR34) & 0b0111) << 8);
        if (prevWF != rawFreq3) {
            waveF = 65536.0 / (2048 - rawFreq3);
        }
        prevWF = rawFreq3;
    }
    int index = internalTimer & 15;
    waveBuffer[index * 2] = (aMem->read(WAVE + index) & 0b11110000) >> 4;
    waveBuffer[index * 2 + 1] = aMem->read(WAVE + index) & 0b1111;

    if (TEST(aMem->read(NR44), 7)) {
        aMem->write(NR52, aMem->read(NR52) | 0x08);
        aMem->write(NR44, aMem->read(NR44) & ~0x80);
        noiseV = ((aMem->read(NR42) & 0b11110000) >> 4) / 15.0;
        if ((aMem->read(NR41) & 0b00111111) == 0) {
            aMem->write(NR41, aMem->read(NR41) | 0b00111111);
        }
    }
    uint8_t rawFreq4 = aMem->read(NR43);
    if (prevNF != rawFreq4) {
        noiseF = 524288.0 / max(double(rawFreq4 & 0b0111), 0.5) / pow(((rawFreq4 & 0b11110000) >> 4) + 1, 2);
        noiseDiv = (rawFreq4 & 0b11110000) >> 4;
    }
    prevNF = rawFreq4;
    if (TEST(aMem->read(NR44), 6) && internalTimer % 16384 == 0) {
        if ((aMem->read(NR41) & 0b00111111) > 0) {
            aMem->write(NR41, (aMem->read(NR41) & 0b11000000) | ((aMem->read(NR41) & 0b00111111) - 1));
        } else {
            noiseV = 0;
            aMem->write(NR52, aMem->read(NR52) & ~0x08);
        }
    }
    if (internalTimer == 0 && (aMem->read(NR42) & 0b0111) != 0) {
        double newVol = noiseV + ((aMem->read(NR42) & 0b1000)? (1.0 / 15.0) : -(1.0 / 15.0));
        if (newVol >= 0 && newVol <= 1) {
            noiseV = newVol;
        }
    }
    /*if (internalTimer % int(4194304.0 / noiseF) == 0) {
        tickLFSR();
    }*/
}