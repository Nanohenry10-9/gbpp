#include <iostream>

using namespace std;

#pragma once

class gbmem {
public:
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

    void init(string file);

    void write(int addr, uint8_t data);
    uint8_t read(int addr);
    uint16_t read16(int addr);

    void updateButtons();
};