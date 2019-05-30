#include <SDL.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <string>
#include <windows.h>
#include "components/gbcpu.h"

using namespace std;

#define RES 1000000000L
#define LOGRES 100000000L

gbcpu cpu;
int lastDraw = 0;
double sysFreq = 0.0;
int FPS = 60;
long qpfStart = 0, appTicks = 0;
bool finished = 0;
SDL_Event event;
SDL_Texture *screenTex, *tilemapTex;
SDL_Renderer *renderer;
SDL_Window *window;
LARGE_INTEGER pf, li;
uint64_t now;
uint64_t lastLog = 0;
const SDL_Rect screen = {0, 0, 160 * 4, 144 * 4};
const SDL_Rect tilemap1 = {160 * 4 + 20, 0, 8 * 8 * 2, 32 * 8 * 2};
const SDL_Rect tilemap2 = {160 * 4 + 8 * 8 * 2 + 20, 0, 8 * 8 * 2, 32 * 8};
const SDL_Rect half1 = {0, 0, 8 * 8, 32 * 8};
const SDL_Rect half2 = {0, 32 * 8, 8 * 8, 32 * 8};

int main(int argv, char** args) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    window = SDL_CreateWindow("gbpp", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 160 * 4 + 20 + 256 * 2, 144 * 4, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    screenTex = SDL_CreateTexture(renderer, SDL_GetWindowPixelFormat(window), SDL_TEXTUREACCESS_STREAMING, 160, 144);
    tilemapTex = SDL_CreateTexture(renderer, SDL_GetWindowPixelFormat(window), SDL_TEXTUREACCESS_STREAMING, 8 * 8, 48 * 8);

    cpu.init(screenTex);

    QueryPerformanceFrequency(&pf);
    sysFreq = double(pf.QuadPart);

    do {
        QueryPerformanceCounter(&li);
        now = (li.QuadPart / sysFreq) * RES;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                finished = 1;
                break;
            } else if (event.type == SDL_KEYDOWN) {
                string key = SDL_GetKeyName(event.key.keysym.sym);
                bool p = 0;
                if (key == "R") {
                    cpu.mem.ROMbank = 0;
                    cpu.REG_PC = 0;
                } else if (key == "A") {
                    cpu.mem.keys[0] = 1;
                    p = 1;
                } else if (key == "S") {
                    cpu.mem.keys[1] = 1;
                    p = 1;
                } else if (key == "Return") {
                    cpu.mem.keys[2] = 1;
                    p = 1;
                } else if (key == "Backspace") {
                    cpu.mem.keys[3] = 1;
                    p = 1;
                } else if (key == "Up") {
                    cpu.mem.keys[4] = 1;
                    p = 1;
                } else if (key == "Down") {
                    cpu.mem.keys[5] = 1;
                    p = 1;
                } else if (key == "Left") {
                    cpu.mem.keys[6] = 1;
                    p = 1;
                } else if (key == "Right") {
                    cpu.mem.keys[7] = 1;
                    p = 1;
                }
                if (p && cpu.stop) {
                    cpu.stop = 0;
                    cpu.halt = 0;
                }
            } else if (event.type == SDL_KEYUP) {
                string key = SDL_GetKeyName(event.key.keysym.sym);
                if (key == "A") {
                    cpu.mem.keys[0] = 0;
                } else if (key == "S") {
                    cpu.mem.keys[1] = 0;
                } else if (key == "Return") {
                    cpu.mem.keys[2] = 0;
                } else if (key == "Backspace") {
                    cpu.mem.keys[3] = 0;
                } else if (key == "Up") {
                    cpu.mem.keys[4] = 0;
                } else if (key == "Down") {
                    cpu.mem.keys[5] = 0;
                } else if (key == "Left") {
                    cpu.mem.keys[6] = 0;
                } else if (key == "Right") {
                    cpu.mem.keys[7] = 0;
                }
            }
        }

        if (now - lastLog > LOGRES) {
            double elapsed = now - lastLog;
            lastLog = now;
            double cpuTicks = cpu.ticks * (elapsed / LOGRES) * (RES / LOGRES);
            char status[256];
            if (!cpu.halt) {
                sprintf(status, "gbpp | %07.0fHz (%03.0f%) - PC: 0x%04X", cpuTicks, (cpuTicks / cpu.freq) * 100, cpu.REG_PC);
            } else if (cpu.wakeOnInterrupt) {
                sprintf(status, "gbpp | Waiting for interrupt");
            } else if (cpu.stop) {
                sprintf(status, "gbpp | Waiting for button press");
            } else {
                sprintf(status, "gbpp | HALTED");
            }
            SDL_SetWindowTitle(window, status);
            cpu.ticks = 0;
            cpu.ppu.frameCount = 0;
        }

        if (SDL_GetTicks() - lastDraw >= (1000.0 / FPS)) {
            lastDraw = SDL_GetTicks();
            SDL_RenderClear(renderer);

            if ((cpu.mem.read(0xFF40) & 0x80) == 0x80) {
                SDL_RenderCopy(renderer, screenTex, NULL, &screen); // screen
            }

            if (appTicks % FPS == 0) {
                cpu.ppu.renderTilemap(tilemapTex, &cpu.mem); // tilemap
            }
            SDL_RenderCopy(renderer, tilemapTex, &half1, &tilemap1);
            SDL_RenderCopy(renderer, tilemapTex, &half2, &tilemap2);

            SDL_RenderPresent(renderer);
            appTicks++;
        }

        cpu.update(now);
    } while (!finished);

    cpu.dump(5);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}