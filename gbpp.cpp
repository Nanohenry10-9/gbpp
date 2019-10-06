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
SDL_Surface *fontLoad;
SDL_Texture *fontMap;
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
void print(SDL_Renderer *r, int x, int y, string s, int fsize) {
    if (!debugWin) {
        SDL_Log("Info window not shown");
        return;
    }
    for (int i = 0; i < s.size(); i++) {
        const SDL_Rect src = {(s[i] % 32) * 8, (s[i] / 32) * 8, 8, 8};
        const SDL_Rect dst = {x + i * fsize, y, fsize, fsize};
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

void printHelp() {
    SDL_Log("+-----------------+");
    SDL_Log("|  GBPP emulator  |");
    SDL_Log("+-----------------+");
    SDL_Log("A DMG C++ emulator using SDL2.");
    SDL_Log("By nanohenry");
    SDL_Log(" ");
    SDL_Log("Usage:");
    SDL_Log("gbpp | gbpp -h | gbpp --help            Print this message");
    SDL_Log("gbpp -r [path] | --rom [path]           Load the emulated memory with a ROM from file at [path]");
    SDL_Log("gbpp -s | --no-sound                    Start the emulator muted");
    SDL_Log("gbpp -b [addr] | --breakpoint [addr]    Add breakpoint to [addr]");
    SDL_Log("gbpp -t [path] | --bootrom [path]       Enable the bootstrap ROM and load it from [path]");
}

int main(int argv, char* args[]) {
    bool noSound = 0;
    string bootROM = "";
    string game = "";
    if (argv < 2) {
        printHelp();
        return 1;
    } else {
        for (int i = 1; i < argv; i++) {
            if (strcmp(args[i], "--no-sound") == 0 || strcmp(args[i], "-s") == 0) {
                noSound = 1;
            } else if (strcmp(args[i], "--rom") == 0 || strcmp(args[i], "-r") == 0) {
                game = args[++i];
            } else if (strcmp(args[i], "--breakpoint") == 0 || strcmp(args[i], "-b") == 0) {
                uint16_t b = stoul(args[++i], nullptr, 16);
                cpu.breakpoints.insert(b);
                SDL_Log("Breaking on 0x%04X", b);
            } else if (strcmp(args[i], "--bootrom") == 0 || strcmp(args[i], "-t") == 0) {
                bootROM = args[++i];
            } else if (strcmp(args[i], "-h") == 0 || strcmp(args[i], "--help") == 0) {
                printHelp();
                return 0;
            }
        }
        if (game == "") {
            SDL_Log("ROM name missing! Pass one with the --rom option (i.e. 'gbpp --rom [path/to/rom].gb').");
            return 1;
        }
    }
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    mainWindow = SDL_CreateWindow("gbpp", 30, 50, 160 * 3, 144 * 3, SDL_WINDOW_OPENGL);
    mainRenderer = SDL_CreateRenderer(mainWindow, -1, SDL_RENDERER_ACCELERATED);
    screenTex = SDL_CreateTexture(mainRenderer, SDL_GetWindowPixelFormat(mainWindow), SDL_TEXTUREACCESS_STREAMING, 160, 144);
    
    cpu.init(screenTex, tilemapTex, wholeBGTex, game, bootROM);
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
                        SDL_DestroyTexture(tilemapTex);
                        SDL_DestroyTexture(wholeBGTex);
                        cpu.ppu.tm = NULL;
                        cpu.ppu.bg = NULL;
                        SDL_DestroyTexture(fontMap);
                        SDL_DestroyWindow(debugWindow);
                        SDL_DestroyRenderer(debugRenderer);
                    }
                }
                break;
            } else if (event.type == SDL_KEYDOWN) {
                string key = SDL_GetKeyName(event.key.keysym.sym);
                bool p = 0;
                if (key == "D") {
                    debugWin = !debugWin;
                    if (debugWin) {
                        debugWindow = SDL_CreateWindow("gbpp | Debugger", 60 + 160 * 3, 50, 256 * 2 + 20 + 200, 32 * 8 * 2, SDL_WINDOW_OPENGL);
                        debugRenderer = SDL_CreateRenderer(debugWindow, -1, SDL_RENDERER_ACCELERATED);
                        tilemapTex = SDL_CreateTexture(debugRenderer, SDL_GetWindowPixelFormat(debugWindow), SDL_TEXTUREACCESS_STREAMING, 8 * 8, 48 * 8);
                        wholeBGTex = SDL_CreateTexture(debugRenderer, SDL_GetWindowPixelFormat(debugWindow), SDL_TEXTUREACCESS_STREAMING, 32 * 8, 32 * 8);
                        cpu.ppu.tm = tilemapTex;
                        cpu.ppu.bg = wholeBGTex;
                        fontLoad = SDL_LoadBMP("font.bmp");
                        fontMap = SDL_CreateTextureFromSurface(debugRenderer, fontLoad);
                        SDL_FreeSurface(fontLoad);
                    } else {
                        SDL_DestroyTexture(tilemapTex);
                        SDL_DestroyTexture(wholeBGTex);
                        cpu.ppu.tm = NULL;
                        cpu.ppu.bg = NULL;
                        SDL_DestroyTexture(fontMap);
                        SDL_DestroyWindow(debugWindow);
                        SDL_DestroyRenderer(debugRenderer);
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
                    cpu.ppu.drawScanline = step;
                    cpu.ppu.render();
                    cpu.completeHalt = 0;
                    SDL_Log("Step by step %s", step? "ON" : "OFF");
                } else if (key == "Space" && step) {
                    cpu.ppu.render();
                    cpu.completeHalt = 0;
                    cpu.haltAfterInst = 1;
                } else if (key == "Space" && cpu.breakpoint) {
                    cpu.ppu.render();
                    cpu.breakpoint = 0;
                    cpu.bypassAddress = cpu.REG_PC;
                } else if (key == "1") {
                    cpu.apu.toggleChannel(0);
                } else if (key == "2") {
                    cpu.apu.toggleChannel(1);
                } else if (key == "3") {
                    cpu.apu.toggleChannel(2);
                } else if (key == "4") {
                    cpu.apu.toggleChannel(3);
                } else if (key == "W") {
                    SDL_Log(" ");
                    SDL_Log("APU wave data (32 4-bit samples):");
                    uint8_t tmp[32];
                    string buff[16];
                    for (int i = 0; i < 16; i++) {
                        buff[i] = "";
                        for (int j = 0; j < 64; j++) {
                            buff[i] += ' ';
                        }
                        tmp[i * 2] = (cpu.mem.read(0xFF30 + i) & 0b11110000) >> 4;
                        tmp[i * 2 + 1] = cpu.mem.read(0xFF30 + i) & 0b1111;
                    }
                    for (int i = 0; i < 32; i++) {
                        for (int j = 0; j < 16; j++) {
                            if (tmp[i] == 15 - j) {
                                buff[j][i * 2] = '#';
                                buff[j][i * 2 + 1] = '#';
                            } else if (tmp[i] > 15 - j) {
                                buff[j][i * 2] = '|';
                                buff[j][i * 2 + 1] = '|';
                            }
                        }
                    }
                    for (int i = 0; i < 16; i++) {
                        SDL_Log("%02X %s", 15 - i, buff[i].c_str());
                    }
                    string x = "   ";
                    const string chrset = "0123456789ABCDEF";
                    for (int i = 0; i < 32; i += 2) {
                        x += chrset[(i & 0xF0) >> 4];
                        x += chrset[i & 0x0F];
                        x += ' ';
                        x += ' ';
                    }
                    SDL_Log("%s", x.c_str());
                    x = "     ";
                    for (int i = 1; i < 32; i += 2) {
                        x += chrset[(i & 0xF0) >> 4];
                        x += chrset[i & 0x0F];
                        x += ' ';
                        x += ' ';
                    }
                    SDL_Log("%s", x.c_str());
                    SDL_Log(" ");
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
            char status[128];
            if (!cpu.completeHalt && !cpu.breakpoint && !step) {
                sprintf(status, "gbpp | %07.0fHz (%03.0f%%)", cpuTicks, (cpuTicks / cpu.freq) * 100);
            } else if (cpu.breakpoint) {
                sprintf(status, "gbpp | Breakpoint");
            } else if (step) {
                sprintf(status, "gbpp | Instruction-by-instruction mode");
            } else {
                sprintf(status, "gbpp | Halted");
            }
            SDL_SetWindowTitle(mainWindow, status);

            cpu.ticks = 0;
            cpu.ppu.frameCount = 0;
        }

        if (SDL_GetTicks() - lastDraw >= (1000.0 / FPS)) {
            lastDraw = SDL_GetTicks();

            SDL_RenderClear(mainRenderer);
            SDL_RenderCopy(mainRenderer, screenTex, NULL, &screen);
            SDL_RenderPresent(mainRenderer);

            if (debugWin && appTicks % 2 == 0) {
                SDL_SetRenderDrawColor(debugRenderer, 0, 0, 0, 255);
                SDL_RenderClear(debugRenderer);
                cpu.ppu.renderTilemap();
                SDL_RenderCopy(debugRenderer, tilemapTex, &half1, &tilemap1);
                SDL_RenderCopy(debugRenderer, tilemapTex, &half2, &tilemap2);
                SDL_RenderCopy(debugRenderer, wholeBGTex, NULL, &wholeBG);

                char tmpBuf[256];

                sprintf(tmpBuf, "PC: %04Xh", cpu.REG_PC);
                print(debugRenderer, 150, 300, tmpBuf);
                sprintf(tmpBuf, "SP: %04Xh", cpu.REG_SP);
                print(debugRenderer, 150, 300 + 16, tmpBuf);
                sprintf(tmpBuf, "AF: %02X%02Xh Zero:     %d", cpu.REG_A, cpu.REG_F, (cpu.REG_F & 0x80) != 0);
                print(debugRenderer, 150, 300 + 16 * 2, tmpBuf);
                sprintf(tmpBuf, "BC: %02X%02Xh Subtract: %d", cpu.REG_B, cpu.REG_C, (cpu.REG_F & 0x40) != 0);
                print(debugRenderer, 150, 300 + 16 * 3, tmpBuf);
                sprintf(tmpBuf, "DE: %02X%02Xh H-carry:  %d", cpu.REG_D, cpu.REG_E, (cpu.REG_F & 0x20) != 0);
                print(debugRenderer, 150, 300 + 16 * 4, tmpBuf);
                sprintf(tmpBuf, "HL: %02X%02Xh Carry:    %d", cpu.REG_H, cpu.REG_L, (cpu.REG_F & 0x10) != 0);
                print(debugRenderer, 150, 300 + 16 * 5, tmpBuf);
                sprintf(tmpBuf, "Last INT: %s", cpu.lastInt.c_str());
                print(debugRenderer, 150, 300 + 16 * 7, tmpBuf);
                sprintf(tmpBuf, "ROM bank: %02Xh", cpu.mem.ROMbank);
                print(debugRenderer, 150, 300 + 16 * 8, tmpBuf);
                sprintf(tmpBuf, "Scanline X: %d", cpu.ppu.LX);
                print(debugRenderer, 150, 300 + 16 * 9, tmpBuf);
                sprintf(tmpBuf, "Scanline Y: %02Xh", cpu.mem.read(0xFF44));
                print(debugRenderer, 150, 300 + 16 * 10, tmpBuf);
                sprintf(tmpBuf, "%c %s", 26, cpu.mem.getOpString(cpu.REG_PC, 0).c_str());
                print(debugRenderer, 150 + 16 * 10, 300, tmpBuf);

                for (int i = -30; i <= 30; i++) { // draw dissassembly from PC - 30 to PC + 30
                    sprintf(tmpBuf, "%04X %s", cpu.REG_PC + i, cpu.mem.getOpString(cpu.REG_PC + i, 0).c_str());
                    print(debugRenderer, 550, 32 * 8 - 4 + i * 8, tmpBuf, 8);
                    if (i == 0) {
                        sprintf(tmpBuf, "%c", 26);
                        print(debugRenderer, 550 - 8, 32 * 8 - 4 + i * 8, tmpBuf, 8);
                    }
                }

                SDL_RenderPresent(debugRenderer);
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
    SDL_DestroyWindow(mainWindow);
    SDL_DestroyWindow(debugWindow);
    SDL_Quit();

    return 0;
}