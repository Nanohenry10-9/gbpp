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
        "NOP",          "LD BC,d16",    "LD (BC),A",     "INC BC",   "INC B",      "DEC B",    "LD B,d8",      "RLCA",
        "LD (a16),SP",  "ADD HL,BC",    "LD A,(BC)",     "DEC BC",   "INC C",      "DEC C",    "LD C,d8",      "RRCA",

        "STOP",         "LD DE,d16",    "LD (DE),A",     "INC DE",   "INC D",      "DEC D",    "LD D,d8",      "RLA",
        "JR r8",        "ADD HL,DE",    "LD A,(DE)",     "DEC DE",   "INC E",      "DEC E",    "LD E,d8",      "RRA",

        "JR NZ,r8",     "LD HL,d16",    "LD (HL+),A",    "INC HL",   "INC H",      "DEC H",    "LD H,d8",      "DAA",
        "JR Z,r8",      "ADD HL,HL",    "LD A,(HL+)",    "DEC HL",   "INC L",      "DEC L",    "LD L,d8",      "CPL",

        "JR NC,r8",     "LD SP,d16",    "LD (HL-),A",    "INC SP",   "INC (HL)",   "DEC (HL)", "LD (HL),d8",   "SCF",
        "JR C,r8",      "ADD HL,SP",    "LD A,(HL-)",    "DEC SP",   "INC A",      "DEC A",    "LD A,d8",      "CCF",

        "LD B,B",       "LD B,C",       "LD B,D",        "LD B,E",   "LD B,H",     "LD B,L",   "LD B,(HL)",    "LD B,A",
        "LD C,B",       "LD C,C",       "LD C,D",        "LD C,E",   "LD C,H",     "LD C,L",   "LD C,(HL)",    "LD C,A",

        "LD D,B",       "LD D,C",       "LD D,D",        "LD D,E",   "LD D,H",     "LD D,L",   "LD D,(HL)",    "LD D,A",
        "LD E,B",       "LD E,C",       "LD E,D",        "LD E,E",   "LD E,H",     "LD E,L",   "LD E,(HL)",    "LD E,A",

        "LD H,B",       "LD H,C",       "LD H,D",        "LD H,E",   "LD H,H",     "LD H,L",   "LD H,(HL)",    "LD H,A",
        "LD L,B",       "LD L,C",       "LD L,D",        "LD L,E",   "LD L,H",     "LD L,L",   "LD L,(HL)",    "LD L,A",

        "LD (HL),B",    "LD (HL),C",    "LD (HL),D",     "LD (HL),E","LD (HL),H",  "LD (HL),L","HALT",         "LD (HL),A",
        "LD A,B",       "LD A,C",       "LD A,D",        "LD A,E",   "LD A,H",     "LD A,L",   "LD A,(HL)",    "LD A,A",

        "ADD A,B",      "ADD A,C",      "ADD A,D",       "ADD A,E",  "ADD A,H",    "ADD A,L",  "ADD A,(HL)",   "ADD A,A",
        "ADC A,B",      "ADC A,C",      "ADC A,D",       "ADC A,E",  "ADC A,H",    "ADC A,L",  "ADC A,(HL)",   "ADC A,A",

        "SUB A,B",      "SUB A,C",      "SUB A,D",       "SUB A,E",  "SUB A,H",    "SUB A,L",  "SUB A,(HL)",   "SUB A,A",
        "SBC A,B",      "SBC A,C",      "SBC A,D",       "SBC A,E",  "SBC A,H",    "SBC A,L",  "SBC A,(HL)",   "SBC A,A",

        "AND A,B",      "AND A,C",      "AND A,D",       "AND A,E",  "AND A,H",    "AND A,L",  "AND A,(HL)",   "AND A,A",
        "XOR A,B",      "XOR A,C",      "XOR A,D",       "XOR A,E",  "XOR A,H",    "XOR A,L",  "XOR A,(HL)",   "XOR A,A",

        "OR A,B",       "OR A,C",       "OR A,D",        "OR A,E",   "OR A,H",     "OR A,L",   "OR A,(HL)",    "OR A,A",
        "CP A,B",       "CP A,C",       "CP A,D",        "CP A,E",   "CP A,H",     "CP A,L",   "CP A,(HL)",    "CP A,A",

        "RET NZ",       "POP BC",       "JP NZ,a16",     "JP a16",   "CALL NZ,a16","PUSH BC",   "ADD A,d8",    "RST 00h",
        "RET Z",        "RET",          "JP Z,a16",      "CB-op",    "CALL Z,a16", "CALL a16",  "ADC A,d8",    "RST 08h",

        "RET NC",       "POP DE",       "JP NC,a16",     "ILLEGAL",  "CALL NC,a16","PUSH DE",   "SUB A,d8",    "RST 10h",
        "RET C",        "RETI",         "JP C,a16",      "ILLEGAL",  "CALL C,a16", "ILLEGAL",   "SBC A,d8",    "RST 18h",

        "LDH (FFa8),A", "POP HL",       "LD (FF00h+C),A","ILLEGAL",  "ILLEGAL",    "PUSH HL",   "AND A,d8",    "RST 20h",
        "ADD SP,r8",    "JP (HL)",      "LD (a16),A",    "ILLEGAL",  "ILLEGAL",    "ILLEGAL",   "XOR A,d8",    "RST 28h",

        "LDH A,(FFa8)", "POP AF",       "LD A,(FF00h+C)","DI",       "ILLEGAL",    "PUSH AF",   "OR A,d8",     "RST 30h",
        "LD HL,SP+r8",  "LD SP,HL",     "LD A,(a16)",    "EI",       "ILLEGAL",    "ILLEGAL",   "CP A,d8",     "RST 38h"
    };

    uint8_t ROMbank, RAMbank;
    bool mode, RAMmode;
    
    uint16_t DMAbase;
    uint16_t DMAindex;
    bool DMAinProgress;

    bool keys[8];

    string serialBuffer;

    int init(string file, string br, bool* ubr);

    void tickTimer();

    string getOpString(uint16_t addr, int p);

    uint8_t* getAddr(uint16_t addr);

    void write(int addr, uint8_t data);
    uint8_t read(int addr);
    uint16_t read16(int addr);

    void updateButtons();
};