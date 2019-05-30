#include "gbmem.h"
#include <fstream>
#include <SDL.h>

#define BTN_A      0x0
#define BTN_B      0x1
#define BTN_START  0x2
#define BTN_SELECT 0x3
#define BTN_UP     0x4
#define BTN_DOWN   0x5
#define BTN_LEFT   0x6
#define BTN_RIGHT  0x7

using namespace std;

uint8_t ROMbanks[0x7F][0x4000];
uint8_t RAMbanks[0x04][0x2000], banksLoaded;
uint8_t memory[10000];
uint8_t bootROM[0x100];
uint8_t bitsPending;

uint8_t ROMbank, RAMbank;
bool mode, RAMmode;

uint16_t DMAbase;
uint16_t DMAindex;
bool DMAinProgress;

bool keys[8];

string serialBuffer;

void gbmem::init(string file) {
    banksLoaded = 0;
    serialBuffer = "";
    bitsPending = 0;
    ROMbank = 1;
    RAMbank = 0;
    mode = 0;
    RAMmode = 0;
    DMAbase = 0;
    DMAindex = 0;
    DMAinProgress = 0;

    /*ifstream DMG_ROM("DMG_ROM.bin", ios::binary | ios::ate);
    DMG_ROM.seekg(0, ios::beg);
    DMG_ROM.read((char*)bootROM, 0x100);*/

    if (file != "") {
        SDL_Log("Loading %s", file.c_str());

        ifstream ROMfile(file, ios::binary | ios::ate);
        long size = ROMfile.tellg();
        SDL_Log("File length is %d KiB, loading %d banks...", size / 1024, size / 0x4000);
        ROMfile.seekg(0, ios::beg);
        for (long i = 0; i < size / 0x4000; i++) {
            ROMfile.seekg(i * 0x4000);
            ROMfile.read((char*)ROMbanks[i], 0x4000);
            banksLoaded++;
        }
    } else {
        SDL_Log("No ROM loaded");
    }
}

void gbmem::write(int addr, uint8_t data) {
    addr &= 0xFFFF;
    if (addr < 0x1000) {
        RAMmode = (data & 0x0A) == 0x0A;
    }
    if (addr < 0x4000) {
        ROMbank = (ROMbank & 0xE0) | (data & 0x1F);
        if (ROMbank == 0x00 || ROMbank == 0x20 || ROMbank == 0x40 || ROMbank == 0x60) {
            ROMbank++;
        }
    } else if (addr < 0x6000) {
        if (mode) {
            RAMbank = data & 0x03;
        } else {
            ROMbank = (ROMbank & 0x1F) | ((data & 0x03) << 5);
            if (ROMbank == 0x00 || ROMbank == 0x20 || ROMbank == 0x40 || ROMbank == 0x60) {
                ROMbank++;
            }
        }
    } else if (addr < 0x8000) {
        mode = data != 0;
    }
    if (addr >= 0xA000 && addr < 0xC000 && RAMmode) {
        RAMbanks[RAMbank][addr - 0xA000] = data;
    }
    if (addr < 0x8000 || (addr >= 0xE000 && addr < 0xFE00) || (addr >= 0xFEA0 && addr < 0xFF00)) {
        return;
    }

    if (addr == 0xFF02) {
        if ((data & 0x80) == 0x80) {
            if (memory[0xFF01] != '\n') {
                serialBuffer += memory[0xFF01];
            } else {
                SDL_Log("Received '%s'", serialBuffer.c_str());
                serialBuffer = "";
            }
            bitsPending = 8;
        }
    }
    if (addr == 0xFF46) {
        DMAbase = data << 8;
        DMAindex = 0;
        DMAinProgress = 1;
    }

    if (addr == 0xFF04) {
        // reset whole timer!
        data = 0;
    }
    memory[addr] = data;
}

uint8_t gbmem::read(int addr) {
    addr &= 0xFFFF;
    if (addr < 0x100 && (memory[0xFF50] & 0x01) != 0x01) {
        return bootROM[addr];
    }
    if ((addr >= 0xFEA0 && addr <= 0xFEFF) && (addr >= 0xE000 && addr <= 0xFDFF)) {
        return 0xFF;
    }
    if (addr == 0xFF00) {
        updateButtons();
    }
    if (addr < 0x4000) {
        return ROMbanks[0][addr];
    } else if (addr < 0x8000) {
        return ROMbanks[ROMbank][addr - 0x4000];
    } else if (addr >= 0xA000 && addr < 0xC000 && RAMmode) {
        return RAMbanks[RAMbank][addr - 0xA000];
    } else {
        return memory[addr];
    }
}

uint16_t gbmem::read16(int addr) {
    return read(addr) | (read(addr + 1) << 8);
}

void gbmem::updateButtons() {
    // update input register and request INT 60h if necessary
    uint8_t btns = memory[0xFF00] | 0x0F;
    if ((btns & 0x10) != 0x10) { // joypad
        if (keys[BTN_DOWN]) {
            memory[0xFF0F] |= 0x10;
            btns &= ~0x08;
        }
        if (keys[BTN_UP]) {
            memory[0xFF0F] |= 0x10;
            btns &= ~0x04;
        }
        if (keys[BTN_LEFT]) {
            memory[0xFF0F] |= 0x10;
            btns &= ~0x02;
        }
        if (keys[BTN_RIGHT]) {
            memory[0xFF0F] |= 0x10;
            btns &= ~0x01;
        }
    } else if ((btns & 0x20) != 0x20) { // buttons
        if (keys[BTN_START]) {
            memory[0xFF0F] |= 0x10;
            btns &= ~0x08;
        }
        if (keys[BTN_SELECT]) {
            memory[0xFF0F] |= 0x10;
            btns &= ~0x04;
        }
        if (keys[BTN_B]) {
            memory[0xFF0F] |= 0x10;
            btns &= ~0x02;
        }
        if (keys[BTN_A]) {
            memory[0xFF0F] |= 0x10;
            btns &= ~0x01;
        }
    }
    memory[0xFF00] = btns;
}