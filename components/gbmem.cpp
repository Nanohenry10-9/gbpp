#include "gbmem.h"

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
uint8_t memory[0x10000];
uint8_t bootROM[0x100];
uint8_t bitsPending;

const string opcodes[256];

uint8_t ROMbank, RAMbank;
bool mode, RAMmode;

uint16_t DMAbase;
uint16_t DMAindex;
bool DMAinProgress;

uint16_t timer;

bool keys[8];

string serialBuffer;

int gbmem::init(string file, string br, bool* ubr) {
    serialBuffer = "";
    ROMbank = 1;

    if (br.size() > 0) {
        SDL_Log("Loading boot ROM...");
        ifstream DMG_ROM(br, ios::binary | ios::ate);
        if (!DMG_ROM.good()) {
            SDL_Log("%s: No such file!", br.c_str());
            SDL_Log("Boot ROM not loaded and will be skipped.");
        } else {
            DMG_ROM.seekg(0, ios::beg);
            DMG_ROM.read((char*)bootROM, 0x100);
            *ubr = 1;
        }
    } else {
        SDL_Log("Boot ROM not loaded and will be skipped.");
    }

    SDL_Log("Loading %s...", file.c_str());

    ifstream ROMfile(file, ios::binary | ios::ate);
    if (!ROMfile.good()) {
        SDL_Log("%s: No such file!", file.c_str());
        return 0;
    }
    long size = ROMfile.tellg();
    if (size % (16 * 1024) != 0) {
        SDL_Log("%s: Size must be a multiple of 16 KB (size is %li B).", file.c_str(), size);
        return 0;
    }
    ROMfile.seekg(0, ios::beg);
    for (long i = 0; i < size / 0x4000; i++) {
        ROMfile.seekg(i * 0x4000);
        ROMfile.read((char*)ROMbanks[i], 0x4000);
        banksLoaded++;
    }
    SDL_Log("Successfully loaded %i banks (%i KB).", banksLoaded, banksLoaded * 16);
    return banksLoaded;
}

void gbmem::tickTimer() {
    timer++;
    memory[0xFF04] = (timer & 0xFF00) >> 8;
    const uint16_t timerResets[4] = {0x03FF, 0x000F, 0x001F, 0x00FF};
    if ((timer & timerResets[memory[0xFF07] & 0x03]) == 4 && (memory[0xFF07] & 0x04) == 0x04) {
        memory[0xFF05]++;
        if (!memory[0xFF05]) {
            memory[0xFF0F] |= 0x04;
            memory[0xFF05] = memory[0xFF06];
        }
    }
}

string gbmem::getOpString(uint16_t addr, int p) {
    uint8_t c = read(addr);
    if (c == 0xCB) {
        char tmpBuf[64];
        sprintf(tmpBuf, "CB-%02Xh", read(addr + 1));
        string r(tmpBuf);
        while (r.size() < p) {
            r += ' ';
        }
        return r;
    }
    string r = opcodes[c];
    char t = 0;
    int l = 0, n = 0;
    for (int i = 0; i < r.size(); i++) {
        if (r[i] == 'r') {
            if (r[i + 1] == '8') {
                t = 'r';
                l = 1;
                n = i;
            }
        } else if (r[i] == 'd') {
            if (r[i + 1] == '8') {
                t = 'd';
                l = 1;
                n = i;
            } else if (r[i + 1] == '1' && r[i + 2] == '6') {
                t = 'd';
                l = 2;
                n = i;
            }
        } else if (r[i] == 'a') {
            if (r[i + 1] == '8') {
                t = 'a';
                l = 1;
                n = i;
            } else if (r[i + 1] == '1' && r[i + 2] == '6') {
                t = 'a';
                l = 2;
                n = i;
            }
        }
    }
    if (t) {
        r.erase(n, 1 + l);
        char tmpBuf[64];
        if (l == 1) {
            if (t == 'r') {
                sprintf(tmpBuf, "%d", (int8_t)read(addr + 1));
            } else {
                sprintf(tmpBuf, "%02Xh", read(addr + 1));
            }
        } else {
            sprintf(tmpBuf, "%04Xh", read16(addr + 1));
        }
        r.insert(n, tmpBuf);
    }
    while (r.size() < p) {
        r += ' ';
    }
    return r;
}

uint8_t* gbmem::getAddr(uint16_t addr) {
    if (addr >= 0xE000 && addr <= 0xFDFF) {
        addr -= 0x2000;
    }
    if (addr < 0x4000) {
        return &ROMbanks[0][addr];
    } else if (addr < 0x8000) {
        return &ROMbanks[ROMbank % banksLoaded][addr - 0x4000];
    } else if (addr >= 0xA000 && addr < 0xC000 && RAMmode) {
        return &RAMbanks[RAMbank][addr - 0xA000];
    } else {
        return &memory[addr];
    }
}

void gbmem::write(int addr, uint8_t data) {
    addr &= 0xFFFF;
    if (addr >= 0xE000 && addr <= 0xFDFF) {
        addr -= 0x2000;
    }
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
    if (addr < 0x8000 || (addr >= 0xFEA0 && addr < 0xFF00)) {
        return;
    }

    if (addr == 0xFF02) {
        if ((data & 0x80) == 0x80) {
            /*if (memory[0xFF01] != '\n') {
                serialBuffer += memory[0xFF01];
                SDL_Log("DEBUG: Received byte '%c' (%02Xh)", memory[0xFF01], memory[0xFF01]);
            } else {
                SDL_Log("Received '%s'", serialBuffer.c_str());
                serialBuffer = "";
            }*/
            bitsPending = 8;
        }
    }
    if (addr == 0xFF50 && (memory[0xFF50] & 0x01) == 0x01) {
        return;
    }
    if (addr == 0xFF46) {
        DMAbase = data << 8;
        DMAindex = 0;
        DMAinProgress = 1;
    }
    if (addr == 0xFF04) {
        timer = 0;
        data = 0;
    }

    memory[addr] = data;
}

uint8_t gbmem::read(int addr) {
    addr &= 0xFFFF;
    if (addr >= 0xE000 && addr < 0xFE00) {
        addr -= 0x2000;
    }
    if (addr < 0x100 && (memory[0xFF50] & 0x01) != 0x01) {
        return bootROM[addr];
    }
    if ((addr >= 0xFEA0 && addr <= 0xFEFF)) {
        return 0x00;
    }
    if (addr == 0xFF00) {
        updateButtons();
    }
    if (addr < 0x4000) {
        return ROMbanks[0][addr];
    } else if (addr < 0x8000) {
        return ROMbanks[ROMbank % banksLoaded][addr - 0x4000];
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
