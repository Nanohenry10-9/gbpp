#include <iostream>
#include "gbppu.h"
#include "gbmem.h"
#include "gbapu.h"
#include <deque>

using namespace std;

#pragma once

struct TraceEntry {
    uint16_t addr;
    uint8_t opcode;
    uint8_t op1, op2;
    uint8_t flags;
    uint8_t regs[7];
    uint16_t SP;
};

class gbcpu {
public:
    deque<TraceEntry> trace;
    uint64_t printTraceIn;
    long freq;
    long period;
    uint64_t lastTick;
    double ticks;
    gbppu ppu;
    gbmem mem;
    gbapu apu;
    uint8_t REG_A, REG_B, REG_C, REG_D, REG_E, REG_F, REG_H, REG_L;
    uint16_t REG_PC, REG_SP;
    bool halt, IME, nextTickIME;
    int idleTicks;
    uint16_t tickDiv;
    bool wakeOnInterrupt, stop, completeHalt, haltAfterInst;
    uint16_t timer;
    string lastInt;

    void reset();
    void init(SDL_Texture* screen, SDL_Texture* e, SDL_Texture* t, string r);

    inline void setFlag(uint8_t f, bool s);
    inline bool flagSet(uint8_t f);
    inline void setFlags(bool z, bool n, bool h, bool c);

    inline void loadBC(uint16_t v);
    inline uint16_t getBC();

    inline void loadDE(uint16_t v);
    inline uint16_t getDE();

    inline void loadHL(uint16_t v);
    inline uint16_t getHL();

    inline void stackPush(uint8_t v);
    inline uint8_t stackPop();

    void doInterrupts();
    void updateTimer();
    void doDMA();

    void tick();
    void dump(int n);

    void add(uint8_t *r1, uint8_t r2);
    void sub(uint8_t *r1, uint8_t r2);
    void adc(uint8_t *r1, uint8_t r2);
    void sbc(uint8_t *r1, uint8_t r2);
    void cp(uint8_t *r1, uint8_t r2);

    void process();
    void update(uint64_t now);
};