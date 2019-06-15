#include <SDL.h>
#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <string>
#include <windows.h>
#include "components/gbcpu.h"

using namespace std;

#define RES 1000000000L // 1 second in performace counter's unit
#define LOGRES 100000000L // 0.1 seconds in performace counter's unit

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
bool debugWin = 0, infoWin = 0;
LARGE_INTEGER pf, li;
uint64_t now;
uint64_t lastLog = 0;
const SDL_Rect screen = {0, 0, 160 * 3, 144 * 3};
const SDL_Rect tilemap1 = {0, 0, 8 * 8 * 2, 32 * 8 * 2};
const SDL_Rect tilemap2 = {8 * 8 * 2, 0, 8 * 8 * 2, 32 * 8};
const SDL_Rect wholeBG = {8 * 8 * 4 + 20, 0, 32 * 8, 32 * 8};
const SDL_Rect half1 = {0, 0, 8 * 8, 32 * 8};
const SDL_Rect half2 = {0, 32 * 8, 8 * 8, 32 * 8};
SDL_Window *infoWindow;
SDL_Surface *fontLoad;
SDL_Texture *fontMap;
SDL_Renderer *fontRenderer;
bool step = 0;

void print(SDL_Renderer *r, int x, int y, string s) {
    if (!debugWin) {
        SDL_Log("Info window not shown");
        return;
    }
    for (int i = 0; i < s.size(); i++) {
        const SDL_Rect src = {(s[i] % 32) * 8, (s[i] / 32) * 8, 8, 8};
        const SDL_Rect dst = {x + i * 16, y, 16, 16};
        SDL_RenderCopy(r, fontMap, &src, &dst);
    }
}
string toHex(int v, int d) {
    string r = "";
    for (int i = 0; i < d; i++) {
        r = "0123456789ABCDEF"[(v & (15 << (i * 4))) >> (i * 4)] + r;
    }
    return r;
}

int main(int argv, char* args[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    mainWindow = SDL_CreateWindow("gbpp", 30, 50, 160 * 3, 144 * 3, SDL_WINDOW_OPENGL);
    mainRenderer = SDL_CreateRenderer(mainWindow, -1, SDL_RENDERER_ACCELERATED);
    screenTex = SDL_CreateTexture(mainRenderer, SDL_GetWindowPixelFormat(mainWindow), SDL_TEXTUREACCESS_STREAMING, 160, 144);
    
    bool noSound = 0;
    string game = "";
    for (int i = 1; i < argv; i++) {
        if (strcmp(args[i], "--no-sound") == 0) {
            noSound = 1;
        } else if (strcmp(args[i], "--rom") == 0) {
            game = args[++i];
        }
    }
    if (game == "") {
        SDL_Log("ROM name missing! Pass one with the --rom option.");
        return 1;
    }
    cpu.init(screenTex, tilemapTex, wholeBGTex, game);
    cpu.apu.disable = noSound;

    QueryPerformanceFrequency(&pf);
    sysFreq = double(pf.QuadPart);

    do {
        QueryPerformanceCounter(&li);
        now = (li.QuadPart / sysFreq) * RES;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    if (event.window.windowID == SDL_GetWindowID(mainWindow)) {
                        finished = 1;
                        break;
                    } else if (event.window.windowID == SDL_GetWindowID(debugWindow)) {
                        debugWin = 0;
                        SDL_DestroyWindow(debugWindow);
                    } else if (event.window.windowID == SDL_GetWindowID(infoWindow)) {
                        infoWin = 0;
                        SDL_DestroyWindow(infoWindow);
                    }
                }
                break;
            } else if (event.type == SDL_KEYDOWN) {
                string key = SDL_GetKeyName(event.key.keysym.sym);
                bool p = 0;
                if (key == "D") {
                    debugWin = !debugWin;
                    if (debugWin) {
                        debugWindow = SDL_CreateWindow("gbpp | Tilemap & registers", 60 + 160 * 3, 50, 256 * 2 + 20, 32 * 8 * 2, SDL_WINDOW_OPENGL);
                        debugRenderer = SDL_CreateRenderer(debugWindow, -1, SDL_RENDERER_ACCELERATED);
                        tilemapTex = SDL_CreateTexture(debugRenderer, SDL_GetWindowPixelFormat(debugWindow), SDL_TEXTUREACCESS_STREAMING, 8 * 8, 48 * 8);
                        wholeBGTex = SDL_CreateTexture(debugRenderer, SDL_GetWindowPixelFormat(debugWindow), SDL_TEXTUREACCESS_STREAMING, 32 * 8, 32 * 8);
                        cpu.ppu.tm = tilemapTex;
                        cpu.ppu.bg = wholeBGTex;
                        fontLoad = SDL_LoadBMP("font.bmp");
                        fontMap = SDL_CreateTextureFromSurface(debugRenderer, fontLoad);
                        SDL_FreeSurface(fontLoad);
                    } else {
                        SDL_DestroyWindow(debugWindow);
                        SDL_DestroyRenderer(debugRenderer);
                    }
                } else if (key == "I") {
                    infoWin = !infoWin;
                    if (infoWin) {
                        infoWindow = SDL_CreateWindow("gbpp | Debug", 60 + 160 * 3, 50, 600, 600, SDL_WINDOW_OPENGL);
                        fontRenderer = SDL_CreateRenderer(infoWindow, -1, SDL_RENDERER_ACCELERATED);
                    } else {
                        SDL_DestroyWindow(infoWindow);
                        SDL_DestroyRenderer(fontRenderer);
                    }
                } else if (key == "R") {
                    cpu.reset();
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
                } else if (key == "M") {
                    step = !step;
                    cpu.completeHalt = 0;
                } else if (key == "Space" && step) {
                    cpu.completeHalt = 0;
                    cpu.haltAfterInst = 1;
                }
                if (p && cpu.stop) {
                    cpu.stop = 0;
                    cpu.halt = 0;
                }
            } else if (event.type == SDL_KEYUP) {
                string key = SDL_GetKeyName(event.key.keysym.sym);
                if (key == "Escape") {
                    finished = 1;
                    break;
                } else if (key == "A") {
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
                sprintf(status, "gbpp | %07.0fHz (%03.0f%%)", cpuTicks, (cpuTicks / cpu.freq) * 100);
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

            if (debugWin && appTicks % 2 == 0) {
                SDL_SetRenderDrawColor(debugRenderer, 0, 0, 0, 255);
                SDL_RenderClear(debugRenderer);
                cpu.ppu.renderTilemap();
                SDL_RenderCopy(debugRenderer, tilemapTex, &half1, &tilemap1);
                SDL_RenderCopy(debugRenderer, tilemapTex, &half2, &tilemap2);
                SDL_RenderCopy(debugRenderer, wholeBGTex, NULL, &wholeBG);

                ostringstream tmp;
                tmp << "PC: " << toHex(cpu.REG_PC, 4) << "h";
                print(debugRenderer, 150, 300, tmp.str());
                tmp.str("");
                tmp << "SP: " + toHex(cpu.REG_SP, 4) + "h";
                print(debugRenderer, 150, 300 + 16, tmp.str());
                tmp.str("");
                tmp << "AF: " << toHex(cpu.REG_A, 2) << toHex(cpu.REG_F, 2) << "h";
                print(debugRenderer, 150, 300 + 16 * 2, tmp.str());
                tmp.str("");
                tmp << "BC: " + toHex(cpu.REG_B, 2) << toHex(cpu.REG_C, 2) << "h";
                print(debugRenderer, 150, 300 + 16 * 3, tmp.str());
                tmp.str("");
                tmp << "DE: " << toHex(cpu.REG_D, 2) << toHex(cpu.REG_E, 2) << "h";
                print(debugRenderer, 150, 300 + 16 * 4, tmp.str());

                tmp.str("");
                tmp << (char)26 << " " << cpu.mem.opcodes[cpu.mem.read(cpu.REG_PC)];
                print(debugRenderer, 150 + 16 * 10, 300, tmp.str());

                tmp.str("");
                tmp << "HL: " << toHex(cpu.REG_H, 2) << toHex(cpu.REG_L, 2) << "h";
                print(debugRenderer, 150, 300 + 16 * 5, tmp.str());

                tmp.str("");
                tmp << "Last INT: " << cpu.lastInt;
                print(debugRenderer, 150, 300 + 16 * 7, tmp.str());
                tmp.str("");
                tmp << "ROM bank: " << toHex(cpu.mem.ROMbank, 2) << "h";
                print(debugRenderer, 150, 300 + 16 * 8, tmp.str());

                SDL_RenderPresent(debugRenderer);
            }

            if (infoWin) {
                SDL_SetRenderDrawColor(fontRenderer, 0, 0, 0, 255);
                SDL_RenderClear(fontRenderer);

                SDL_RenderPresent(fontRenderer);
            }

            if (appTicks % 4 == 0) {
                SDL_PumpEvents();
            }
            appTicks++;
        }

        if (!step || cpu.haltAfterInst) {
            cpu.update(now);
        }
    } while (!finished);

    cpu.dump(5);

    SDL_DestroyRenderer(mainRenderer);
    SDL_DestroyRenderer(debugRenderer);
    SDL_DestroyRenderer(fontRenderer);
    SDL_DestroyWindow(mainWindow);
    SDL_DestroyWindow(debugWindow);
    SDL_DestroyWindow(infoWindow);
    SDL_Quit();

    return 0;
}