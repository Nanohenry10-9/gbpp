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
SDL_Texture *screenTex, *wholeBGTex, *tilemapTex;
SDL_Renderer *mainRenderer, *debugRenderer;
SDL_Window *mainWindow, *debugWindow;
bool debugWin = 0;
LARGE_INTEGER pf, li;
uint64_t now;
uint64_t lastLog = 0;
const SDL_Rect screen = {0, 0, 160 * 3, 144 * 3};
const SDL_Rect tilemap1 = {0, 0, 8 * 8 * 2, 32 * 8 * 2};
const SDL_Rect tilemap2 = {8 * 8 * 2, 0, 8 * 8 * 2, 32 * 8};
const SDL_Rect wholeBG = {8 * 8 * 4 + 20, 0, 32 * 8, 32 * 8};
const SDL_Rect half1 = {0, 0, 8 * 8, 32 * 8};
const SDL_Rect half2 = {0, 32 * 8, 8 * 8, 32 * 8};

int main(int argv, char** args) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    mainWindow = SDL_CreateWindow("gbpp", 30, 50, 160 * 3, 144 * 3, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    mainRenderer = SDL_CreateRenderer(mainWindow, -1, SDL_RENDERER_ACCELERATED);
    screenTex = SDL_CreateTexture(mainRenderer, SDL_GetWindowPixelFormat(mainWindow), SDL_TEXTUREACCESS_STREAMING, 160, 144);
    
    cpu.init(screenTex, tilemapTex, wholeBGTex);

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
                if (key == "D") {
                    debugWin = !debugWin;
                    if (debugWin) {
                        debugWindow = SDL_CreateWindow("gbpp | debug", 160 * 4 + 50, 50, 256 * 2 + 20, 144 * 4, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
                        debugRenderer = SDL_CreateRenderer(debugWindow, -1, SDL_RENDERER_ACCELERATED);
                        tilemapTex = SDL_CreateTexture(debugRenderer, SDL_GetWindowPixelFormat(debugWindow), SDL_TEXTUREACCESS_STREAMING, 8 * 8, 48 * 8);
                        wholeBGTex = SDL_CreateTexture(debugRenderer, SDL_GetWindowPixelFormat(debugWindow), SDL_TEXTUREACCESS_STREAMING, 32 * 8, 32 * 8);
                        cpu.ppu.tm = tilemapTex;
                        cpu.ppu.bg = wholeBGTex;
                    } else {
                        SDL_DestroyWindow(debugWindow);
                    }
                } else if (key == "R") {
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
                sprintf(status, "gbpp | %07.0fHz (%03.0f%)", cpuTicks, (cpuTicks / cpu.freq) * 100);
            } else if (cpu.wakeOnInterrupt) {
                sprintf(status, "gbpp | Waiting for interrupt");
            } else if (cpu.stop) {
                sprintf(status, "gbpp | Waiting for input");
            } else {
                sprintf(status, "gbpp | Halted");
            }
            SDL_SetWindowTitle(mainWindow, status);
            cpu.ticks = 0;
            cpu.ppu.frameCount = 0;
        }

        if (SDL_GetTicks() - lastDraw >= (1000.0 / FPS)) {
            lastDraw = SDL_GetTicks();

            if ((cpu.mem.read(0xFF40) & 0x80) == 0x80) {
                SDL_RenderClear(mainRenderer);
                SDL_RenderCopy(mainRenderer, screenTex, NULL, &screen);
                SDL_RenderPresent(mainRenderer);
            }

            if (debugWin) {
                SDL_RenderClear(debugRenderer);
                cpu.ppu.renderTilemap();
                SDL_RenderCopy(debugRenderer, tilemapTex, &half1, &tilemap1);
                SDL_RenderCopy(debugRenderer, tilemapTex, &half2, &tilemap2);
                SDL_RenderCopy(debugRenderer, wholeBGTex, NULL, &wholeBG);
                SDL_RenderPresent(debugRenderer);
            }

            if (appTicks % 4 == 0) {
                SDL_PumpEvents();
            }
            appTicks++;
        }

        cpu.update(now);
    } while (!finished);

    cpu.dump(5);

    SDL_DestroyRenderer(mainRenderer);
    SDL_DestroyRenderer(debugRenderer);
    SDL_DestroyWindow(mainWindow);
    SDL_DestroyWindow(debugWindow);
    SDL_Quit();
}