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

uint8_t pulse12_5[8] = {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t pulse25_0[8] = {0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t pulse50_0[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};
uint8_t pulse75_0[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00};

SDL_AudioSpec want, have;
SDL_AudioDeviceID dev;

uint8_t *bufferPos = 0;

void soundCallback(void* userdata, Uint8* stream, int len) {
    for (int i = 0; i < len; i++) {
        SDL_MixAudio(stream, bufferPos, len, SDL_MIX_MAXVOLUME);
        bufferPos++;
        if (bufferPos == &pulse50_0[8]) {
            bufferPos = &pulse50_0[0];
        }
    }
}

void gbapu::init() {
    bufferPos = &pulse50_0[0];
    SDL_memset(&want, 0, sizeof(want));
    want.freq = 8 * 200;
    want.format = AUDIO_U8;
    want.channels = 1;
    want.samples = 8;
    want.callback = soundCallback;
    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    SDL_Log("Opened device with freq=%d and samples=%d", have.freq, have.samples);
    SDL_PauseAudio(0);
    SDL_Delay(100);
    switch (SDL_GetAudioStatus()) {
        case SDL_AUDIO_PLAYING:
            SDL_Log("Audio OK");
            break;
        case SDL_AUDIO_PAUSED:
            SDL_Log("Audio paused");
            break;
        case SDL_AUDIO_STOPPED:
            SDL_Log("Audio failed");
            break;
    }
}

void gbapu::tick(gbmem* mem, uint16_t timer) {

}