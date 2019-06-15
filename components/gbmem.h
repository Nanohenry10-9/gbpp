#include <iostream>
#include <fstream>
#include <SDL.h>

using namespace std;

#pragma once

class gbmem {
public:
    uint8_t ROMbanks[0x7F][0x4000];
    uint8_t RAMbanks[0x04][0x2000], banksLoaded;
    uint8_t memory[0x10000];
    uint8_t bootROM[0x100];
    uint8_t bitsPending;

    const string opcodes[256] = {
        "NOP",          "LD BC,d16",    "LD (BC),A",    "INC BC",   "INC B",    "DEC B",    "LD B,d8",      "RLCA",
        "LD (a16),SP",  "ADD HL,BC",    "LD A,(BC)",    "DEC BC",   "INC C",    "DEC C",    "LD C,d8",      "RRCA",

        "STOP",         "LD DE,d16",    "LD (DE),A",    "INC DE",   "INC D",    "DEC D",    "LD D,d8",      "RLA",
        "JR r8",        "ADD HL,DE",    "LD A,(DE)",    "DEC DE",   "INC E",    "DEC E",    "LD E,d8",      "RRA",

        "JR NZ,r8",     "LD HL,d16",    "LD (HL+),A",   "INC HL",   "INC H",    "DEC H",    "LD H,d8",      "DAA",
        "JR Z,r8",      "ADD HL,HL",    "LD A,(HL+)",   "DEC HL",   "INC L",    "DEC L",    "LD L,d8",      "CPL",

        "JR NC,r8",     "LD SP,d16",    "LD (HL-),A",   "INC SP",   "INC (HL)", "DEC (HL)", "LD (HL),d8",   "SCF",
        "JR C,r8",      "ADD HL,SP",    "LD A,(HL-)",   "DEC SP",   "INC A",    "DEC A",    "LD A,d8",      "CCF",

        "LD B,B",       "LD B,C",       "LD B,D",       "LD B,E",   "LD B,H",   "LD B,L",   "LD B,(HL)",    "LD B,A",
        "LD C,B",       "LD C,C",       "LD C,D",       "LD C,E",   "LD C,H",   "LD C,L",   "LD C,(HL)",    "LD C,A",

        "LD D,B",       "LD D,C",       "LD D,D",       "LD D,E",   "LD D,H",   "LD D,L",   "LD D,(HL)",    "LD D,A",
        "LD E,B",       "LD E,C",       "LD E,D",       "LD E,E",   "LD E,H",   "LD E,L",   "LD E,(HL)",    "LD E,A",

        "LD H,B",       "LD H,C",       "LD H,D",       "LD H,E",   "LD H,H",   "LD H,L",   "LD H,(HL)",    "LD H,A",
        "LD L,B",       "LD L,C",       "LD L,D",       "LD L,E",   "LD L,H",   "LD L,L",   "LD L,(HL)",    "LD L,A",

        "LD (HL),B",    "LD (HL),C",    "LD (HL),D",    "LD (HL),E","LD (HL),H","LD (HL),L","HALT",         "LD (HL),A",
        "LD A,B",       "LD A,C",       "LD A,D",       "LD A,E",   "LD A,H",   "LD A,L",   "LD A,(HL)",    "LD A,A"
    };

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