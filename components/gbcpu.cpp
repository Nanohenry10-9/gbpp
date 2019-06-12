#include "gbcpu.h"
#include <inttypes.h>

using namespace std;

#define RES 1000000000L

#define FLAG_Z 0x80
#define FLAG_N 0x40
#define FLAG_H 0x20
#define FLAG_C 0x10

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
uint16_t serialDiv;
bool wakeOnInterrupt, stop;
uint16_t timer;

void gbcpu::init(SDL_Texture* screen, SDL_Texture* e, SDL_Texture* t) {
    freq = 4194304;
    period = RES / freq;
    lastTick = 0;
    ticks = 0;
    mem.init(""); // ROM file here
    ppu.init(&mem, screen, e, t);
    apu.init(&mem);
    REG_A = REG_B = REG_C = REG_D = REG_E = REG_F = REG_H = REG_L = 0;
    REG_PC = REG_SP = 0;
    halt = 0, IME = 0;
    idleTicks = 0;
    serialDiv = 0;
    wakeOnInterrupt = stop = 0;
    timer = 0;

    printTraceIn = -1ULL;

    mem.write(0xFF50, mem.read(0xFF50) | 0x01); // skip boot ROM
    REG_PC = 0x100;
}

inline void gbcpu::setFlag(uint8_t f, bool s) {
    if (s) {
        REG_F |= f & 0xF0;
    } else {
        REG_F &= (~f & 0xF0);
    }
}

inline bool gbcpu::flagSet(uint8_t f) {
    return (REG_F & f) == f;
}

inline void gbcpu::setFlags(bool z, bool n, bool h, bool c) {
    REG_F = ((z << 7) | (n << 6) | (h << 5) | (c << 4)) & 0xF0;
}

inline void gbcpu::loadBC(uint16_t v) {
    REG_C = v & 0xFF;
    REG_B = v >> 8;
}

inline uint16_t gbcpu::getBC() {
    return (REG_B << 8) | REG_C;
}

inline void gbcpu::loadDE(uint16_t v) {
    REG_E = v & 0xFF;
    REG_D = v >> 8;
}

inline uint16_t gbcpu::getDE() {
    return (REG_D << 8) | REG_E;
}

inline void gbcpu::loadHL(uint16_t v) {
    REG_L = v & 0xFF;
    REG_H = v >> 8;
}

inline uint16_t gbcpu::getHL() {
    return (REG_H << 8) | REG_L;
}

inline void gbcpu::stackPush(uint8_t v) {
    mem.write(--REG_SP, v);
}

inline uint8_t gbcpu::stackPop() {
    return mem.read(REG_SP++);
}

void gbcpu::doInterrupts() {
    if ((mem.read(0xFF0F) & 0x01) == 0x01) { // INT 40h
        if ((mem.read(0xFFFF) & 0x01) == 0x01 && IME) {
            mem.write(0xFF0F, ~0x01 & mem.read(0xFF0F));
            IME = 0;
            stackPush(REG_PC >> 8);
            stackPush(REG_PC & 0xFF);
            REG_PC = 0x40;
        }
        if (wakeOnInterrupt) {
            wakeOnInterrupt = 0;
            halt = 0;
        }
    } else if ((mem.read(0xFF0F) & 0x02) == 0x02) { // INT 48h
        if ((mem.read(0xFFFF) & 0x02) == 0x02 && IME) {
            mem.write(0xFF0F, ~0x02 & mem.read(0xFF0F));
            IME = 0;
            stackPush(REG_PC >> 8);
            stackPush(REG_PC & 0xFF);
            REG_PC = 0x48;
        }
        if (wakeOnInterrupt) {
            wakeOnInterrupt = 0;
            halt = 0;
        }
    } else if ((mem.read(0xFF0F) & 0x04) == 0x04) { // INT 50h
        if ((mem.read(0xFFFF) & 0x04) == 0x04 && IME) {
            mem.write(0xFF0F, ~0x04 & mem.read(0xFF0F));
            IME = 0;
            stackPush(REG_PC >> 8);
            stackPush(REG_PC & 0xFF);
            REG_PC = 0x50;
        }
        if (wakeOnInterrupt) {
            wakeOnInterrupt = 0;
            halt = 0;
        }
    } else if ((mem.read(0xFF0F) & 0x08) == 0x08) { // INT 58h
        if ((mem.read(0xFFFF) & 0x08) == 0x08 && IME) {
            mem.write(0xFF0F, ~0x08 & mem.read(0xFF0F));
            IME = 0;
            stackPush(REG_PC >> 8);
            stackPush(REG_PC & 0xFF);
            REG_PC = 0x58;
        }
        if (wakeOnInterrupt) {
            wakeOnInterrupt = 0;
            halt = 0;
        }
    } else if ((mem.read(0xFF0F) & 0x10) == 0x10) { // INT 60h
        if ((mem.read(0xFFFF) & 0x10) == 0x10 && IME) {
            mem.write(0xFF0F, ~0x10 & mem.read(0xFF0F));
            IME = 0;
            stackPush(REG_PC >> 8);
            stackPush(REG_PC & 0xFF);
            REG_PC = 0x60;
        }
        if (wakeOnInterrupt) {
            wakeOnInterrupt = 0;
            halt = 0;
        }
    }
}

void gbcpu::updateTimer() {
    timer++;
    mem.write(0xFF04, (timer & 0xFF00) >> 8);
    const uint16_t timerResets[4] = {0x03FF, 0x000F, 0x001F, 0x00FF};
    if ((timer & timerResets[mem.read(0xFF07) & 0x03]) == 4 && (mem.read(0xFF07) & 0x04) == 0x04) {
        mem.write(0xFF05, mem.read(0xFF05) + 1);
        if (!mem.read(0xFF05)) {
            mem.write(0xFF0F, mem.read(0xFF0F) | 0x04); // Request INT 50h
            mem.write(0xFF05, mem.read(0xFF06));
        }
    }
}

void gbcpu::doDMA() {
    if (mem.DMAinProgress) {
        mem.write(0xFE00 + mem.DMAindex, mem.read(mem.DMAbase + mem.DMAindex));
        mem.DMAindex++;
        if (mem.DMAindex == 160) {
            mem.DMAinProgress = 0;
        }
    }
}

void gbcpu::tick() {
    serialDiv++;
    if (serialDiv == 512) {
        serialDiv = 0;
        if (mem.bitsPending > 0) {
            mem.write(0xFF01, (mem.read(0xFF01) << 1) | 1);
            mem.bitsPending--;
            if (mem.bitsPending == 0) {
                mem.write(0xFF02, mem.read(0xFF02) & ~0x80);
                mem.write(0xFF0F, mem.read(0xFF0F) | 0x08);
            }
        }
    }
    doInterrupts();
    updateTimer();
    doDMA();
    ppu.tick();
    apu.tick(timer);
    if (nextTickIME) {
        nextTickIME = 0;
        IME = 1;
    }
    if (halt) {
        return;
    }
    ticks++;
    if (idleTicks > 0) {
        idleTicks--;
        return;
    }
    process();
}

void gbcpu::dump(int n) {
    SDL_Log("Trace (%d last):", n);
    for (long i = max(int(trace.size() - n), 0); i < trace.size(); i++) {
        if (trace[i].flags == 0x01) {
            SDL_Log("...");
            continue;
        }
        SDL_Log("0x%04X: 0x%02X 0x%02X 0x%02X\tZ:%d N:%d H:%d C:%d Stack: 0x%04X A: 0x%02X B: 0x%02X C: 0x%02X D: 0x%02X E: 0x%02X H: 0x%02X L: 0x%02X", trace[i].addr, trace[i].opcode, trace[i].op1, trace[i].op2, (trace[i].flags & 0x80) == 0x80, (trace[i].flags & 0x40) == 0x40, (trace[i].flags & 0x20) == 0x20, (trace[i].flags & 0x10) == 0x10, trace[i].SP, trace[i].regs[0], trace[i].regs[1], trace[i].regs[2], trace[i].regs[3], trace[i].regs[4], trace[i].regs[5], trace[i].regs[6]);
    }

    SDL_Log("Register dump:");
    SDL_Log("A: 0x%02X   B: 0x%02X   C: 0x%02X   D: 0x%02X", REG_A, REG_B, REG_C, REG_D);
    SDL_Log("E: 0x%02X   F: 0x%02X   H: 0x%02X   L: 0x%02X", REG_E, REG_F, REG_H, REG_L);
    SDL_Log("PC: 0x%04X   SP: 0x%04X", REG_PC, REG_SP);
}

void gbcpu::add(uint8_t *r1, uint8_t r2) {
    setFlag(FLAG_H, (((*r1 & 0xF) + (r2 & 0xF)) & 0x10) == 0x10);
    setFlag(FLAG_C, (int)*r1 + (int)r2 > 0xFF);
    *r1 += r2;
    setFlag(FLAG_Z, *r1 == 0);
    setFlag(FLAG_N, 0);
}

void gbcpu::sub(uint8_t *r1, uint8_t r2) {
    setFlag(FLAG_H, (((*r1 & 0xF) + (-r2 & 0xF)) & 0x10) != 0x10);
    setFlag(FLAG_C, r2 > *r1);
    *r1 -= r2;
    setFlag(FLAG_Z, *r1 == 0);
    setFlag(FLAG_N, 1);
}

void gbcpu::adc(uint8_t *r1, uint8_t r2) {
    bool tmp = flagSet(FLAG_C);
    setFlag(FLAG_H, (((*r1 & 0xF) + (r2 & 0xF) + tmp) & 0x10) == 0x10);
    setFlag(FLAG_C, (int)*r1 + (int)r2 + (int)flagSet(FLAG_C) > 0xFF);
    *r1 += r2 + tmp;
    setFlag(FLAG_Z, *r1 == 0);
    setFlag(FLAG_N, 0);
}

void gbcpu::sbc(uint8_t *r1, uint8_t r2) {
    bool tmp = flagSet(FLAG_C);
    setFlag(FLAG_H, (((*r1 & 0xF) + ((-r2 & 0xF) + tmp)) & 0x10) != 0x10);
    setFlag(FLAG_C, (int)r2 + (int)tmp > (int)*r1);
    *r1 -= r2 + tmp;
    setFlag(FLAG_Z, *r1 == 0);
    setFlag(FLAG_N, 1);
}

void gbcpu::cp(uint8_t *r1, uint8_t r2) {
    setFlags(*r1 == r2, 1, (((*r1 & 0xF) - (r2 & 0xF)) & 0x10) != 0x10, r2 > *r1);
}

void gbcpu::process() {
    uint8_t opcode = mem.read(REG_PC++);
    uint8_t cycles = 0;

    if (trace.size() > 0 && opcode == 0x00) {
        if (trace[trace.size() - 1].flags == REG_F) {
            TraceEntry tmp;
            tmp.opcode = opcode;
            tmp.op1 = mem.read(REG_PC);
            tmp.flags = 0x01;
            trace.push_back(tmp);
        }
    } else {
        TraceEntry tmp;
        tmp.addr = REG_PC - 1;
        tmp.opcode = opcode;
        tmp.op1 = mem.read(REG_PC);
        tmp.op2 = mem.read(REG_PC + 1);
        tmp.flags = REG_F;
        tmp.regs[0] = REG_A;
        tmp.regs[1] = REG_B;
        tmp.regs[2] = REG_C;
        tmp.regs[3] = REG_D;
        tmp.regs[4] = REG_E;
        tmp.regs[5] = REG_H;
        tmp.regs[6] = REG_L;
        tmp.SP = REG_SP;
        trace.push_back(tmp);
        if (printTraceIn != -1ULL) {
            printTraceIn--;
            if (printTraceIn == 0) {
                dump(300);
                halt = true;
            }
        }
    }
    if (trace.size() > 1000) {
        trace.pop_front();
    }

    switch (opcode) {
        case 0x00: // NOP
            cycles = 1;
            break;
        case 0x01: // LD BC,d16
            loadBC(mem.read16(REG_PC++));
            REG_PC++;
            cycles = 3;
            break;
        case 0x02: // LD (BC),A
            mem.write(getBC(), REG_A);
            cycles = 2;
            break;
        case 0x03: // INC BC
            REG_C++;
            if (REG_C == 0x00) {
                REG_B++;
            }
            cycles = 2;
            break;
        case 0x04: // INC B
            setFlag(FLAG_H, (((REG_B & 0xF) + 1) & 0x10) == 0x10);
            REG_B++;
            setFlag(FLAG_Z, REG_B == 0);
            setFlag(FLAG_N, 0);
            cycles = 1;
            break;
        case 0x05: // DEC B
            setFlag(FLAG_H, (((REG_B & 0xF) - 1) & 0x10) == 0x10);
            REG_B--;
            setFlag(FLAG_Z, REG_B == 0);
            setFlag(FLAG_N, 1);
            cycles = 1;
            break;
        case 0x06: // LD B,d8
            REG_B = mem.read(REG_PC++);
            cycles = 2;
            break;
        case 0x07: // RLCA
            setFlags((REG_A << 1) == 0, 0, 0, (REG_A & 0x80) == 0x80);
            REG_A <<= 1;
            cycles = 1;
            break;
        case 0x08: // LD (a16),SP
            mem.write(mem.read(REG_PC++), REG_SP >> 8);
            mem.write(mem.read(REG_PC++), REG_SP & 0xFF);
            cycles = 5;
            break;
        case 0x09: // ADD HL,BC
            {
                setFlag(FLAG_H, (((getHL() & 0xFFF) + (getBC() & 0xFFF)) & 0x1000) == 0x1000);
                uint32_t tmp = getHL() + getBC();
                loadHL((uint16_t)tmp);
                setFlag(FLAG_N, 0);
                setFlag(FLAG_C, (tmp & 0x10000) == 0x10000);
            }
            cycles = 2;
            break;
        case 0x0A: // LD A,(BC)
            REG_A = mem.read(getBC());
            cycles = 2;
            break;
        case 0x0B: // DEC BC
            REG_C--;
            if (REG_C == 0xFF) {
                REG_B--;
            }
            cycles = 2;
            break;
        case 0x0C: // INC C
            setFlag(FLAG_H, (((REG_C & 0xF) + 1) & 0x10) == 0x10);
            REG_C++;
            setFlag(FLAG_Z, REG_C == 0);
            setFlag(FLAG_N, 0);
            cycles = 1;
            break;
        case 0x0D: // DEC C
            setFlag(FLAG_H, (((REG_C & 0xF) - 1) & 0x10) == 0x10);
            REG_C--;
            setFlag(FLAG_Z, REG_C == 0);
            setFlag(FLAG_N, 1);
            cycles = 1;
            break;
        case 0x0E: // LD C,d8
            REG_C = mem.read(REG_PC++);
            cycles = 2;
            break;
        case 0x0F: // RRCA
            setFlags((REG_A >> 1) == 0, 0, 0, (REG_A & 0x01) == 0x01);
            REG_A >>= 1;
            REG_A |= flagSet(FLAG_C) << 7;
            cycles = 1;
            break;
        case 0x10: // STOP
            stop = 1;
            halt = 1;
            cycles = 1;
            break;
        case 0x11: // LD DE,d16
            loadDE(mem.read16(REG_PC++));
            REG_PC++;
            cycles = 3;
            break;
        case 0x12: // LD (DE),A
            mem.write(getDE(), REG_A);
            cycles = 2;
            break;
        case 0x13: // INC DE
            REG_E++;
            if (REG_E == 0x00) {
                REG_D++;
            }
            cycles = 2;
            break;
        case 0x14: // INC D
            setFlag(FLAG_H, (((REG_D & 0xF) + 1) & 0x10) == 0x10);
            REG_D++;
            setFlag(FLAG_Z, REG_D == 0);
            setFlag(FLAG_N, 0);
            cycles = 1;
            break;
        case 0x15: // DEC D
            setFlag(FLAG_H, (((REG_D & 0xF) - 1) & 0x10) == 0x10);
            REG_D--;
            setFlag(FLAG_Z, REG_D == 0);
            setFlag(FLAG_N, 1);
            cycles = 1;
            break;
        case 0x16: // LD D,d8
            REG_D = mem.read(REG_PC++);
            cycles = 2;
            break;
        case 0x17: // RLA
            {
                bool tmp = flagSet(FLAG_C);
                setFlag(FLAG_C, (REG_A & 0x80) == 0x80);
                REG_A <<= 1;
                REG_A |= tmp;
            }
            setFlag(FLAG_Z, REG_A == 0);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            cycles = 1;
            break;
        case 0x18: // JR r8
            REG_PC += (int8_t)mem.read(REG_PC);
            REG_PC++;
            cycles = 3;
            break;
        case 0x19: // ADD HL,DE
            {
                setFlag(FLAG_H, (((getHL() & 0xFFF) + (getDE() & 0xFFF)) & 0x1000) == 0x1000);
                uint32_t tmp = getHL() + getDE();
                loadHL((uint16_t)tmp);
                setFlag(FLAG_N, 0);
                setFlag(FLAG_C, (tmp & 0x10000) == 0x10000);
            }
            cycles = 2;
            break;
        case 0x1A: // LD A,(DE)
            REG_A = mem.read(getDE());
            cycles = 2;
            break;
        case 0x1B: // DEC DE
            REG_E--;
            if (REG_E == 0xFF) {
                REG_D--;
            }
            cycles = 2;
            break;
        case 0x1C: // INC E
            setFlag(FLAG_H, (((REG_E & 0xF) + 1) & 0x10) == 0x10);
            REG_E++;
            setFlag(FLAG_Z, REG_E == 0);
            setFlag(FLAG_N, 0);
            cycles = 1;
            break;
        case 0x1D: // DEC E
            setFlag(FLAG_H, (((REG_E & 0xF) - 1) & 0x10) == 0x10);
            REG_E--;
            setFlag(FLAG_Z, REG_E == 0);
            setFlag(FLAG_N, 1);
            cycles = 1;
            break;
        case 0x1E: // LD E,d8
            REG_E = mem.read(REG_PC++);
            cycles = 2;
            break;
        case 0x1F: // RRA
            {
                bool tmp = flagSet(FLAG_C);
                setFlag(FLAG_C, (REG_A & 0x01) == 0x01);
                REG_A >>= 1;
                REG_A |= tmp << 7;
            }
            setFlag(FLAG_Z, REG_A == 0);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            cycles = 1;
            break;
        case 0x20: // JR NZ,r8
            if (!flagSet(FLAG_Z)) {
                REG_PC += (int8_t)mem.read(REG_PC);
                REG_PC++;
                cycles = 3;
            } else {
                REG_PC++;
                cycles = 2;
            }
            break;
        case 0x21: // LD HL,d16
            loadHL(mem.read16(REG_PC++));
            REG_PC++;
            cycles = 3;
            break;
        case 0x22: // LD (HL+),A (increments HL in the case below)
            mem.write(getHL(), REG_A);
        case 0x23: // INC HL
            REG_L++;
            if (REG_L == 0x00) {
                REG_H++;
            }
            cycles = 2;
            break;
        case 0x24: // INC H
            setFlag(FLAG_H, (((REG_H & 0xF) + 1) & 0x10) == 0x10);
            REG_H++;
            setFlag(FLAG_Z, REG_H == 0);
            setFlag(FLAG_N, 0);
            cycles = 1;
            break;
        case 0x25: // DEC H
            setFlag(FLAG_H, (((REG_H & 0xF) - 1) & 0x10) == 0x10);
            REG_H--;
            setFlag(FLAG_Z, REG_H == 0);
            setFlag(FLAG_N, 1);
            cycles = 1;
            break;
        case 0x26: // LD H,d8
            REG_H = mem.read(REG_PC++);
            cycles = 2;
            break;
        case 0x27: // DAA
            if (!flagSet(FLAG_N)) {
                if (flagSet(FLAG_C) || REG_A > 0x99) {
                    REG_A += 0x60;
                    setFlag(FLAG_C, 1);
                }
                if (flagSet(FLAG_H) || (REG_A & 0x0F) > 0x09) {
                    REG_A += 0x06;
                }
            } else {
                if (flagSet(FLAG_C)) {
                    REG_A -= 0x60;
                }
                if (flagSet(FLAG_H)) {
                    REG_A -= 0x06;
                }
            }
            setFlag(FLAG_Z, REG_A == 0);
            setFlag(FLAG_H, 0);
            cycles = 1;
            break;
        case 0x28: // JR Z,r8
            if (flagSet(FLAG_Z)) {
                REG_PC += (int8_t)mem.read(REG_PC);
                REG_PC++;
                cycles = 3;
            } else {
                REG_PC++;
                cycles = 2;
            }
            break;
        case 0x29: // ADD HL,HL
            {
                setFlag(FLAG_H, (((getHL() & 0xFFF) + (getHL() & 0xFFF)) & 0x1000) == 0x1000);
                uint32_t tmp = getHL() + getHL();
                loadHL((uint16_t)tmp);
                setFlag(FLAG_N, 0);
                setFlag(FLAG_C, (tmp & 0x10000) == 0x10000);
            }
            cycles = 2;
            break;
        case 0x2A: // LD A,(HL+)
            REG_A = mem.read(getHL());
            REG_L++;
            if (REG_L == 0x00) {
                REG_H++;
            }
            cycles = 2;
            break;
        case 0x2B: // DEC HL
            REG_L--;
            if (REG_L == 0xFF) {
                REG_H--;
            }
            cycles = 2;
            break;
        case 0x2C: // INC L
            setFlag(FLAG_H, (((REG_L & 0xF) + 1) & 0x10) == 0x10);
            REG_L++;
            setFlag(FLAG_Z, REG_L == 0);
            setFlag(FLAG_N, 0);
            cycles = 1;
            break;
        case 0x2D: // DEC L
            setFlag(FLAG_H, (((REG_L & 0xF) - 1) & 0x10) == 0x10);
            REG_L--;
            setFlag(FLAG_Z, REG_L == 0);
            setFlag(FLAG_N, 1);
            cycles = 1;
            break;
        case 0x2E: // LD L,d8
            REG_L = mem.read(REG_PC++);
            cycles = 2;
            break;
        case 0x2F: // CPL
            REG_A = ~REG_A;
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, 1);
            cycles = 1;
            break;
        case 0x30: // JR NC,r8
            if (!flagSet(FLAG_C)) {
                REG_PC += (int8_t)mem.read(REG_PC);
                REG_PC++;
                cycles = 3;
            } else {
                REG_PC++;
                cycles = 2;
            }
            break;
        case 0x31: // LD SP,d16
            REG_SP = mem.read16(REG_PC++);
            REG_PC++;
            cycles = 3;
            break;
        case 0x32: // LD (HL-),A
            mem.write(getHL(), REG_A);
            REG_L--;
            if (REG_L == 0xFF) {
                REG_H--;
            }
            cycles = 2;
            break;
        case 0x33: // INC SP
            setFlag(FLAG_H, (((REG_SP & 0xFF) + 1) & 0x100) == 0x100);
            REG_SP++;
            setFlag(FLAG_Z, REG_SP == 0);
            setFlag(FLAG_N, 0);
            cycles = 2;
            break;
        case 0x34: // INC (HL)
            setFlag(FLAG_H, (((mem.read(getHL()) & 0xF) + 1) & 0x10) == 0x10);
            mem.write(getHL(), mem.read(getHL()) + 1);
            setFlag(FLAG_Z, mem.read(getHL()) == 0);
            setFlag(FLAG_N, 1);
            cycles = 3;
            break;
        case 0x35: // DEC (HL)
            setFlag(FLAG_H, (((mem.read(getHL()) & 0xF) - 1) & 0x10) == 0x10);
            mem.write(getHL(), mem.read(getHL()) - 1);
            setFlag(FLAG_Z, mem.read(getHL()) == 0);
            setFlag(FLAG_N, 1);
            cycles = 3;
            break;
        case 0x36: // LD (HL),d8
            mem.write(getHL(), mem.read(REG_PC++));
            cycles = 3;
            break;
        case 0x37:
            setFlag(FLAG_C, 1);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            break;
        case 0x38: // JR C,r8
            if (flagSet(FLAG_C)) {
                REG_PC += (int8_t)mem.read(REG_PC);
                REG_PC++;
                cycles = 3;
            } else {
                REG_PC++;
                cycles = 2;
            }
            break;
        case 0x39: // ADD HL,SP
            {
                setFlag(FLAG_H, (((getHL() & 0xFF) + (REG_SP & 0xFF)) & 0x100) == 0x100);
                uint32_t tmp = getHL() + REG_SP;
                loadHL((uint16_t)tmp);
                setFlag(FLAG_N, 0);
                setFlag(FLAG_C, (tmp & 0x10000) == 0x10000);
            }
            cycles = 2;
            break;
        case 0x3A: // LD A,(HL-)
            REG_A = mem.read(getHL());
            REG_L--;
            if (REG_L == 0xFF) {
                REG_H--;
            }
            cycles = 2;
            break;
        case 0x3C: // INC A
            setFlag(FLAG_H, (((REG_A & 0xF) + 1) & 0x10) == 0x10);
            REG_A++;
            setFlag(FLAG_Z, REG_A == 0);
            setFlag(FLAG_N, 0);
            cycles = 1;
            break;
        case 0x3D: // DEC A
            setFlag(FLAG_H, (((REG_A & 0xF) - 1) & 0x10) == 0x10);
            REG_A--;
            setFlag(FLAG_Z, REG_A == 0);
            setFlag(FLAG_N, 1);
            cycles = 1;
            break;
        case 0x3E: // LD A,d8
            REG_A = mem.read(REG_PC++);
            cycles = 2;
            break;
        case 0x3F:
            setFlag(FLAG_C, !flagSet(FLAG_C));
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            break;
        case 0x40: // LD B,B
        case 0x49: // LD C,C
        case 0x52: // LD D,D
        case 0x5B: // LD E,E
        case 0x64: // LD H,H
        case 0x6D: // LD L,L
        case 0x7F: // LD A,A
            cycles = 1;
            break;
        case 0x44: // LD B,H
            REG_B = REG_H;
            cycles = 1;
            break;
        case 0x46: // LD B,(HL)
            REG_B = mem.read(getHL());
            cycles = 2;
            break;
        case 0x4D: // LD C,L
            REG_C = REG_L;
            cycles = 1;
            break;
        case 0x4E: // LD C,(HL)
            REG_C = mem.read(getHL());
            cycles = 2;
            break;
        case 0x4F: // LD C,A
            REG_C = REG_A;
            cycles = 1;
            break;
        case 0x47: // LD B,A
            REG_B = REG_A;
            cycles = 1;
            break;
        case 0x4C: // LD C,H
            REG_C = REG_H;
            cycles = 1;
            break;
        case 0x54: // LD D,H
            REG_D = REG_H;
            cycles = 1;
            break;
        case 0x56: // LD D,(HL)
            REG_D = mem.read(getHL());
            cycles = 2;
            break;
        case 0x57: // LD D,A
            REG_D = REG_A;
            cycles = 1;
            break;
        case 0x5E: // LD E,(HL)
            REG_E = mem.read(getHL());
            cycles = 2;
            break;
        case 0x5C: // LD E,H
            REG_E = REG_H;
            cycles = 1;
            break;
        case 0x5D: // LD E,L
            REG_E = REG_L;
            cycles = 1;
            break;
        case 0x5F: // LD E,A
            REG_E = REG_A;
            cycles = 1;
            break;
        case 0x60: // LD H,B
            REG_H = REG_B;
            cycles = 1;
            break;
        case 0x62: // LD H,D
            REG_H = REG_D;
            cycles = 1;
            break;
        case 0x66: // LD H,(HL)
            REG_H = mem.read(getHL());
            cycles = 2;
            break;
        case 0x67: // LD H,A
            REG_H = REG_A;
            cycles = 1;
            break;
        case 0x69: // LD L,C
            REG_L = REG_C;
            cycles = 1;
            break;
        case 0x6B: // LD L,E
            REG_L = REG_E;
            cycles = 1;
            break;
        case 0x6C: // LD L,H
            REG_L = REG_H;
            cycles = 1;
            break;
        case 0x6E: // LD L,(HL)
            REG_L = mem.read(getHL());
            cycles = 2;
            break;
        case 0x6F: // LD L,A
            REG_L = REG_A;
            cycles = 1;
            break;
        case 0x70: // LD (HL),B
            mem.write(getHL(), REG_B);
            cycles = 2;
            break;
        case 0x71: // LD (HL),C
            mem.write(getHL(), REG_C);
            cycles = 2;
            break;
        case 0x72: // LD (HL),D
            mem.write(getHL(), REG_D);
            cycles = 2;
            break;
        case 0x73: // LD (HL),E
            mem.write(getHL(), REG_E);
            cycles = 2;
            break;
        case 0x76: // HALT
            halt = 1;
            wakeOnInterrupt = 1;
            cycles = 1;
            break;
        case 0x77: // LD (HL),A
            mem.write(getHL(), REG_A);
            cycles = 2;
            break;
        case 0x78: // LD A,B
            REG_A = REG_B;
            cycles = 1;
            break;
        case 0x79: // LD A,C
            REG_A = REG_C;
            cycles = 1;
            break;
        case 0x7A: // LD A,D
            REG_A = REG_D;
            cycles = 1;
            break;
        case 0x7B: // LD A,E
            REG_A = REG_E;
            cycles = 1;
            break;
        case 0x7C: // LD A,H
            REG_A = REG_H;
            cycles = 1;
            break;
        case 0x7D: // LD A,L
            REG_A = REG_L;
            cycles = 1;
            break;
        case 0x7E: // LD A,(HL)
            REG_A = mem.read(getHL());
            cycles = 2;
            break;
        case 0x80: // ADD A,B
            add(&REG_A, REG_B);
            cycles = 1;
            break;
        case 0x81: // ADD A,C
            add(&REG_A, REG_C);
            cycles = 1;
            break;
        case 0x82: // ADD A,D
            add(&REG_A, REG_D);
            cycles = 1;
            break;
        case 0x83: // ADD A,E
            add(&REG_A, REG_E);
            cycles = 1;
            break;
        case 0x84: // ADD A,H
            add(&REG_A, REG_H);
            cycles = 1;
            break;
        case 0x85: // ADD A,L
            add(&REG_A, REG_L);
            cycles = 1;
            break;
        case 0x86: // ADD A,(HL)
            add(&REG_A, mem.read(getHL()));
            cycles = 2;
            break;
        case 0x87: // ADD A,A
            add(&REG_A, REG_A);
            cycles = 1;
            break;
        case 0x88: // ADC A,B
            adc(&REG_A, REG_B);
            cycles = 1;
            break;
        case 0x89: // ADC A,C
            adc(&REG_A, REG_C);
            cycles = 1;
            break;
        case 0x8A: // ADC A,D
            adc(&REG_A, REG_D);
            cycles = 1;
            break;
        case 0x8B: // ADC A,E
            adc(&REG_A, REG_E);
            cycles = 1;
            break;
        case 0x8C: // ADC A,H
            adc(&REG_A, REG_H);
            cycles = 1;
            break;
        case 0x8D: // ADC A,L
            adc(&REG_A, REG_L);
            cycles = 1;
            break;
        case 0x8E: // ADC A,(HL)
            adc(&REG_A, mem.read(getHL()));
            cycles = 2;
            break;
        case 0x8F: // ADC A,A
            adc(&REG_A, REG_A);
            cycles = 2;
            break;
        case 0x90: // SUB A,B
            sub(&REG_A, REG_B);
            cycles = 1;
            break;
        case 0x91: // SUB A,C
            sub(&REG_A, REG_C);
            cycles = 1;
            break;
        case 0x92: // SUB A,D
            sub(&REG_A, REG_D);
            cycles = 1;
            break;
        case 0x93: // SUB A,E
            sub(&REG_A, REG_E);
            cycles = 1;
            break;
        case 0x94: // SUB A,H
            sub(&REG_A, REG_H);
            cycles = 1;
            break;
        case 0x95: // SUB A,L
            sub(&REG_A, REG_L);
            cycles = 1;
            break;
        case 0x96: // SUB A,(HL)
            sub(&REG_A, mem.read(getHL()));
            cycles = 2;
            break;
        case 0x97: // SUB A,A
            sub(&REG_A, REG_A);
            cycles = 1;
            break;
        case 0x98: // SBC A,B
            sbc(&REG_A, REG_B);
            cycles = 1;
            break;
        case 0x99: // SBC A,C
            sbc(&REG_A, REG_C);
            cycles = 1;
            break;
        case 0x9A: // SBC A,D
            sbc(&REG_A, REG_D);
            cycles = 1;
            break;
        case 0x9B: // SBC A,E
            sbc(&REG_A, REG_E);
            cycles = 1;
            break;
        case 0x9C: // SBC A,H
            sbc(&REG_A, REG_H);
            cycles = 1;
            break;
        case 0x9D: // SBC A,L
            sbc(&REG_A, REG_L);
            cycles = 1;
            break;
        case 0x9E: // SBC A,(HL)
            sbc(&REG_A, mem.read(getHL()));
            cycles = 2;
            break;
        case 0x9F: // SBC A,A
            sbc(&REG_A, REG_A);
            cycles = 2;
            break;
        case 0xA0: // AND B
            REG_A &= REG_B;
            setFlags(REG_A == 0, 0, 1, 0);
            cycles = 1;
            break;
        case 0xA1: // AND C
            REG_A &= REG_C;
            setFlags(REG_A == 0, 0, 1, 0);
            cycles = 1;
            break;
        case 0xA2: // AND D
            REG_A &= REG_D;
            setFlags(REG_A == 0, 0, 1, 0);
            cycles = 1;
            break;
        case 0xA3: // AND E
            REG_A &= REG_E;
            setFlags(REG_A == 0, 0, 1, 0);
            cycles = 1;
            break;
        case 0xA4: // AND H
            REG_A &= REG_H;
            setFlags(REG_A == 0, 0, 1, 0);
            cycles = 1;
            break;
        case 0xA5: // AND L
            REG_A &= REG_L;
            setFlags(REG_A == 0, 0, 1, 0);
            cycles = 1;
            break;
        case 0xA6: // AND (HL)
            REG_A &= mem.read(getHL());
            setFlags(REG_A == 0, 0, 1, 0);
            cycles = 1;
            break;
        case 0xA7: // AND A
            REG_A &= REG_A;
            setFlags(REG_A == 0, 0, 1, 0);
            cycles = 1;
            break;
        case 0xA8: // XOR B
            REG_A ^= REG_B;
            setFlags(REG_A == 0, 0, 0, 0);
            cycles = 1;
            break;
        case 0xA9: // XOR C
            REG_A ^= REG_C;
            setFlags(REG_A == 0, 0, 0, 0);
            cycles = 1;
            break;
        case 0xAA: // XOR D
            REG_A ^= REG_D;
            setFlags(REG_A == 0, 0, 0, 0);
            cycles = 1;
            break;
        case 0xAB: // XOR E
            REG_A ^= REG_E;
            setFlags(REG_A == 0, 0, 0, 0);
            cycles = 1;
            break;
        case 0xAC: // XOR H
            REG_A ^= REG_H;
            setFlags(REG_A == 0, 0, 0, 0);
            cycles = 1;
            break;
        case 0xAD: // XOR L
            REG_A ^= REG_L;
            setFlags(REG_A == 0, 0, 0, 0);
            cycles = 1;
            break;
        case 0xAE: // XOR (HL)
            REG_A ^= mem.read(getHL());
            setFlags(REG_A == 0, 0, 0, 0);
            cycles = 2;
            break;
        case 0xAF: // XOR A
            REG_A ^= REG_A;
            setFlags(REG_A == 0, 0, 0, 0);
            cycles = 1;
            break;
        case 0xB0: // OR B
            REG_A |= REG_B;
            setFlags(REG_A == 0, 0, 0, 0);
            cycles = 1;
            break;
        case 0xB1: // OR C
            REG_A |= REG_C;
            setFlags(REG_A == 0, 0, 0, 0);
            cycles = 1;
            break;
        case 0xB2: // OR D
            REG_A |= REG_D;
            setFlags(REG_A == 0, 0, 0, 0);
            cycles = 1;
            break;
        case 0xB3: // OR E
            REG_A |= REG_E;
            setFlags(REG_A == 0, 0, 0, 0);
            cycles = 1;
            break;
        case 0xB4: // OR H
            REG_A |= REG_H;
            setFlags(REG_A == 0, 0, 0, 0);
            cycles = 1;
            break;
        case 0xB5: // OR L
            REG_A |= REG_L;
            setFlags(REG_A == 0, 0, 0, 0);
            cycles = 1;
            break;
        case 0xB6: // OR (HL)
            REG_A |= mem.read(getHL());
            setFlags(REG_A == 0, 0, 0, 0);
            cycles = 2;
            break;
        case 0xB7: // OR A
            REG_A |= REG_A;
            setFlags(REG_A == 0, 0, 0, 0);
            cycles = 1;
            break;
        case 0xB8: // CP B
            cp(&REG_A, REG_B);
            cycles = 1;
            break;
        case 0xB9: // CP C
            cp(&REG_A, REG_C);
            cycles = 1;
            break;
        case 0xBA: // CP D
            cp(&REG_A, REG_D);
            cycles = 1;
            break;
        case 0xBB: // CP E
            cp(&REG_A, REG_E);
            cycles = 1;
            break;
        case 0xBC: // CP H
            cp(&REG_A, REG_H);
            cycles = 1;
            break;
        case 0xBD: // CP L
            cp(&REG_A, REG_L);
            cycles = 1;
            break;
        case 0xBE: // CP (HL)
            cp(&REG_A, mem.read(getHL()));
            cycles = 2;
            break;
        case 0xBF: // CP A
            cp(&REG_A, REG_A);
            cycles = 1;
            break;
        case 0xC0: // RET NZ
            if (!flagSet(FLAG_Z)) {
                REG_PC = stackPop() | (stackPop() << 8);
                cycles = 5;
            } else {
                cycles = 2;
            }
            break;
        case 0xC1: // POP BC
            REG_C = stackPop();
            REG_B = stackPop();
            cycles = 3;
            break;
        case 0xC2: // JP NZ,a16
            if (!flagSet(FLAG_Z)) {
                REG_PC = mem.read16(REG_PC);
                cycles = 4;
            } else {
                REG_PC += 2;
                cycles = 3;
            }
            break;
        case 0xC3: // JP a16
            REG_PC = mem.read16(REG_PC++);
            cycles = 4;
            break;
        case 0xC4: // CALL NZ,a16
            REG_PC += 2;
            if (!flagSet(FLAG_Z)) {
                stackPush(REG_PC >> 8);
                stackPush(REG_PC & 0xFF);
                REG_PC = mem.read16(REG_PC - 2);
                cycles = 6;
            } else {
                cycles = 2;
            }
            break;
        case 0xC5: // PUSH BC
            stackPush(REG_B);
            stackPush(REG_C);
            cycles = 4;
            break;
        case 0xC6: // ADD A,d8
            setFlag(FLAG_H, (((REG_A & 0xF) + (mem.read(REG_PC) & 0xF)) & 0x10) == 0x10);
            setFlag(FLAG_C, (int)REG_A + (int)mem.read(REG_PC) > 0xFF);
            REG_A += mem.read(REG_PC++);
            setFlag(FLAG_Z, REG_A == 0);
            setFlag(FLAG_N, 0);
            cycles = 2;
            break;
        case 0xC7: // RST 00h
            stackPush(REG_PC >> 8);
            stackPush(REG_PC & 0xFF);
            REG_PC = 0x00;
            cycles = 4;
            break;
        case 0xC8: // RET Z
            if (flagSet(FLAG_Z)) {
                REG_PC = stackPop() | (stackPop() << 8);
                cycles = 5;
            } else {
                cycles = 2;
            }
            break;
        case 0xC9: // RET
            REG_PC = stackPop() | (stackPop() << 8);
            cycles = 4;
            break;
        case 0xCA: // JP Z,a16
            if (flagSet(FLAG_Z)) {
                REG_PC = mem.read16(REG_PC);
                cycles = 4;
            } else {
                REG_PC += 2;
                cycles = 3;
            }
            break;
        case 0xCC: // CALL Z,a16
            REG_PC += 2;
            if (flagSet(FLAG_Z)) {
                stackPush(REG_PC >> 8);
                stackPush(REG_PC & 0xFF);
                REG_PC = mem.read16(REG_PC - 2);
                cycles = 6;
            } else {
                cycles = 2;
            }
            break;
        case 0xCD: // CALL a16
            REG_PC += 2;
            stackPush(REG_PC >> 8);
            stackPush(REG_PC & 0xFF);
            REG_PC = mem.read16(REG_PC - 2);
            cycles = 6;
            break;
        case 0xCE: // ADC A,d8
            setFlag(FLAG_H, (((REG_A & 0xF) + (mem.read(REG_PC) & 0xF) + flagSet(FLAG_C)) & 0x10) == 0x10);
            setFlag(FLAG_C, (int)REG_A + (int)mem.read(REG_PC) + (int)flagSet(FLAG_C) > 0xFF);
            REG_A += mem.read(REG_PC++) + flagSet(FLAG_C);
            setFlag(FLAG_Z, REG_A == 0);
            setFlag(FLAG_N, 0);
            cycles = 2;
            break;
        case 0xCF: // RST 08h
            stackPush(REG_PC >> 8);
            stackPush(REG_PC & 0xFF);
            REG_PC = 0x08;
            cycles = 4;
            break;
        case 0xD0: // RET NC
            if (!flagSet(FLAG_C)) {
                REG_PC = stackPop() | (stackPop() << 8);
                cycles = 5;
            } else {
                cycles = 2;
            }
            break;
        case 0xD1: // POP DE
            REG_E = stackPop();
            REG_D = stackPop();
            cycles = 3;
            break;
        case 0xD2: // JP NC,a16
            if (!flagSet(FLAG_C)) {
                REG_PC = mem.read16(REG_PC);
                cycles = 4;
            } else {
                REG_PC += 2;
                cycles = 3;
            }
            break;
        case 0xD4: // CALL NC,a16
            REG_PC += 2;
            if (!flagSet(FLAG_C)) {
                stackPush(REG_PC >> 8);
                stackPush(REG_PC & 0xFF);
                REG_PC = mem.read16(REG_PC - 2);
                cycles = 6;
            } else {
                cycles = 2;
            }
            break;
        case 0xD5: // PUSH DE
            stackPush(REG_D);
            stackPush(REG_E);
            cycles = 4;
            break;
        case 0xD6: // SUB A,d8
            setFlag(FLAG_C, mem.read(REG_PC) > REG_A);
            setFlag(FLAG_H, (((REG_A & 0xF) - (mem.read(REG_PC) & 0xF)) & 0x10) == 0x10);
            REG_A -= mem.read(REG_PC++);
            setFlag(FLAG_Z, REG_A == 0);
            setFlag(FLAG_N, 1);
            cycles = 2;
            break;
        case 0xD7: // RST 10h
            stackPush(REG_PC >> 8);
            stackPush(REG_PC & 0xFF);
            REG_PC = 0x10;
            cycles = 4;
            break;
        case 0xD8: // RET C
            if (flagSet(FLAG_C)) {
                REG_PC = stackPop() | (stackPop() << 8);
                cycles = 5;
            } else {
                cycles = 2;
            }
            break;
        case 0xD9: // RETI
            REG_PC = stackPop() | (stackPop() << 8);
            nextTickIME = 1;
            cycles = 4;
            break;
        case 0xDC: // CALL C,a16
            REG_PC += 2;
            if (flagSet(FLAG_C)) {
                stackPush(REG_PC >> 8);
                stackPush(REG_PC & 0xFF);
                REG_PC = mem.read16(REG_PC - 2);
                cycles = 6;
            } else {
                cycles = 2;
            }
            break;
        case 0xDE: // SBC A,d8
            setFlag(FLAG_H, (((REG_A & 0xF) - ((mem.read(REG_PC) & 0xF) + flagSet(FLAG_C))) & 0x10) == 0x10);
            setFlag(FLAG_C, (int)mem.read(REG_PC) + (int)flagSet(FLAG_C) > (int)REG_A);
            REG_A -= mem.read(REG_PC++) + flagSet(FLAG_C);
            setFlag(FLAG_Z, REG_A == 0);
            setFlag(FLAG_N, 1);
            cycles = 2;
            break;
        case 0xDF: // RST 18h
            halt = true;
            stackPush(REG_PC >> 8);
            stackPush(REG_PC & 0xFF);
            REG_PC = 0x18;
            cycles = 4;
            break;
        case 0xE0: // LD (FF00h+a8),A
            mem.write(0xFF00 + mem.read(REG_PC++), REG_A);
            cycles = 3;
            break;
        case 0xE1: // POP HL
            REG_L = stackPop();
            REG_H = stackPop();
            cycles = 3;
            break;
        case 0xE2: // LD (FF00h+C),A
            mem.write(0xFF00 + REG_C, REG_A);
            cycles = 2;
            break;
        case 0xE5: // PUSH HL
            stackPush(REG_H);
            stackPush(REG_L);
            cycles = 4;
            break;
        case 0xE6: // AND d8
            REG_A &= mem.read(REG_PC++);
            setFlags(REG_A == 0, 0, 1, 0);
            cycles = 2;
            break;
        case 0xE7: // RST 20h
            stackPush(REG_PC >> 8);
            stackPush(REG_PC & 0xFF);
            REG_PC = 0x20;
            cycles = 4;
            break;
        case 0xE8: // ADD SP,r8
            {
                setFlag(FLAG_H, (((REG_SP & 0xFF) + mem.read(REG_PC)) & 0x100) == 0x100);
                uint32_t tmp = REG_SP + mem.read(REG_PC++);
                REG_SP = tmp;
                setFlag(FLAG_N, 0);
                setFlag(FLAG_Z, 0);
                setFlag(FLAG_C, (tmp & 0x10000) == 0x10000);
            }
            cycles = 4;
            break;
        case 0xE9: // JP (HL)
            REG_PC = getHL();
            cycles = 1;
            break;
        case 0xEA: // LD (a16),A
            mem.write(mem.read16(REG_PC++), REG_A);
            REG_PC++;
            cycles = 4;
            break;
        case 0xEE: // XOR d8
            REG_A ^= mem.read(REG_PC++);
            setFlags(REG_A == 0, 0, 0, 0);
            cycles = 2;
            break;
        case 0xEF: // RST 28h
            stackPush(REG_PC >> 8);
            stackPush(REG_PC & 0xFF);
            REG_PC = 0x28;
            cycles = 4;
            break;
        case 0xF0: // LD A,(FF00h+a8)
            REG_A = mem.read(0xFF00 + mem.read(REG_PC++));
            cycles = 3;
            break;
        case 0xF1: // POP AF
            REG_F = stackPop() & 0xF0;
            REG_A = stackPop();
            cycles = 3;
            break;
        case 0xF2: // LD A,(FF00h+C)
            REG_A = mem.read(0xFF00 + REG_C);
            cycles = 2;
            break;
        case 0xF3: // DI
            IME = 0;
            cycles = 1;
            break;
        case 0xF5: // PUSH AF
            stackPush(REG_A);
            stackPush(REG_F & 0xF0);
            cycles = 4;
            break;
        case 0xF6: // OR d8
            REG_A |= mem.read(REG_PC++);
            setFlags(REG_A == 0, 0, 0, 0);
            cycles = 2;
            break;
        case 0xF7: // RST 30h
            stackPush(REG_PC >> 8);
            stackPush(REG_PC & 0xFF);
            REG_PC = 0x30;
            cycles = 4;
            break;
        case 0xF8: // LD HL,SP+r8
            setFlags(0, 0, (REG_SP + (int8_t)mem.read(REG_PC)) & 0x1FF == 0x1FF, (REG_SP + (int8_t)mem.read(REG_PC)) & 0x10000 == 0x10000);
            loadHL(REG_SP + (int8_t)mem.read(REG_PC++));
            cycles = 3;
            break;
        case 0xF9: // LD SP,HL
            REG_SP = getHL();
            cycles = 2;
            break;
        case 0xFA: // LD A,(a16)
            REG_A = mem.read(mem.read16(REG_PC++));
            REG_PC++;
            cycles = 4;
            break;
        case 0xFB: // EI
            nextTickIME = 1;
            cycles = 1;
            break;
        case 0xFE: // CP d8
            setFlags(REG_A == mem.read(REG_PC), 1, (((REG_A & 0xF) - (mem.read(REG_PC) & 0xF)) & 0x10) == 0x10, mem.read(REG_PC) > REG_A);
            REG_PC++;
            cycles = 2;
            break;
        case 0xFF: // RST 38h
            stackPush(REG_PC >> 8);
            stackPush(REG_PC & 0xFF);
            REG_PC = 0x38;
            cycles = 4;
            break;
        case 0xCB: // CB-prefix
            opcode = mem.read(REG_PC++);
            cycles = 2;
            if ((opcode & 0xF) == 0xE || (opcode & 0xF) == 0x6) { // 16-bit
                cycles = 4;
            }
            switch (opcode) {
                case 0x00: // RLC B
                    setFlags((REG_B << 1) == 0, 0, 0, (REG_B & 0x80) == 0x80);
                    REG_B <<= 1;
                    break;
                case 0x01: // RLC C
                    setFlags((REG_C << 1) == 0, 0, 0, (REG_C & 0x80) == 0x80);
                    REG_C <<= 1;
                    break;
                case 0x02: // RLC D
                    setFlags((REG_D << 1) == 0, 0, 0, (REG_D & 0x80) == 0x80);
                    REG_D <<= 1;
                    break;
                case 0x03: // RLC E
                    setFlags((REG_E << 1) == 0, 0, 0, (REG_E & 0x80) == 0x80);
                    REG_E <<= 1;
                    break;
                case 0x04: // RLC H
                    setFlags((REG_H << 1) == 0, 0, 0, (REG_H & 0x80) == 0x80);
                    REG_H <<= 1;
                    break;
                case 0x05: // RLC L
                    setFlags((REG_L << 1) == 0, 0, 0, (REG_L & 0x80) == 0x80);
                    REG_L <<= 1;
                    break;
                case 0x06: // RLC (HL)
                    setFlags((mem.read(getHL()) << 1) == 0, 0, 0, (mem.read(getHL()) & 0x80) == 0x80);
                    mem.write(getHL(), mem.read(getHL()) << 1);
                    break;
                case 0x07: // RLC A
                    setFlags((REG_A << 1) == 0, 0, 0, (REG_A & 0x80) == 0x80);
                    REG_A <<= 1;
                    break;
                case 0x08: // RRC B
                    setFlags((REG_B >> 1) == 0, 0, 0, (REG_B & 0x01) == 0x01);
                    REG_B >>= 1;
                    break;
                case 0x09: // RRC C
                    setFlags((REG_C >> 1) == 0, 0, 0, (REG_C & 0x01) == 0x01);
                    REG_C >>= 1;
                    break;
                case 0x0A: // RRC D
                    setFlags((REG_D >> 1) == 0, 0, 0, (REG_D & 0x01) == 0x01);
                    REG_D >>= 1;
                    break;
                case 0x0B: // RRC E
                    setFlags((REG_E >> 1) == 0, 0, 0, (REG_E & 0x01) == 0x01);
                    REG_E >>= 1;
                    break;
                case 0x0C: // RRC H
                    setFlags((REG_H >> 1) == 0, 0, 0, (REG_H & 0x01) == 0x01);
                    REG_H >>= 1;
                    break;
                case 0x0D: // RRC L
                    setFlags((REG_L >> 1) == 0, 0, 0, (REG_L & 0x01) == 0x01);
                    REG_L >>= 1;
                    break;
                case 0x0E: // RRC (HL)
                    setFlags((mem.read(getHL()) >> 1) == 0, 0, 0, (mem.read(getHL()) & 0x01) == 0x01);
                    mem.write(getHL(), mem.read(getHL()) >> 1);
                    break;
                case 0x0F: // RRC A
                    setFlags((REG_A >> 1) == 0, 0, 0, (REG_A & 0x01) == 0x01);
                    REG_A >>= 1;
                    break;
                case 0x10: // RL B
                    {
                        bool tmp = flagSet(FLAG_C);
                        setFlag(FLAG_C, (REG_B & 0x80) == 0x80);
                        REG_B <<= 1;
                        REG_B |= tmp;
                    }
                    setFlag(FLAG_Z, REG_B == 0);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 0);
                    break;
                case 0x11: // RL C
                    {
                        bool tmp = flagSet(FLAG_C);
                        setFlag(FLAG_C, (REG_C & 0x80) == 0x80);
                        REG_C <<= 1;
                        REG_C |= tmp;
                    }
                    setFlag(FLAG_Z, REG_C == 0);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 0);
                    break;
                case 0x12: // RL D
                    {
                        bool tmp = flagSet(FLAG_C);
                        setFlag(FLAG_C, (REG_D & 0x80) == 0x80);
                        REG_D <<= 1;
                        REG_D |= tmp;
                    }
                    setFlag(FLAG_Z, REG_D == 0);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 0);
                    break;
                case 0x16: // RL (HL)
                    {
                        bool tmp = flagSet(FLAG_C);
                        setFlag(FLAG_C, (mem.read(getHL()) & 0x80) == 0x80);
                        mem.write(getHL(), (mem.read(getHL()) << 1) | tmp);
                    }
                    setFlag(FLAG_Z, mem.read(getHL()) == 0);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 0);
                    break;
                case 0x18: // RR B
                    {
                        bool tmp = flagSet(FLAG_C);
                        setFlag(FLAG_C, (REG_B & 0x01) == 0x01);
                        REG_B >>= 1;
                        REG_B |= tmp << 7;
                    }
                    setFlag(FLAG_Z, REG_B == 0);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 0);
                    break;
                case 0x19: // RR C
                    {
                        bool tmp = flagSet(FLAG_C);
                        setFlag(FLAG_C, (REG_C & 0x01) == 0x01);
                        REG_C >>= 1;
                        REG_C |= tmp << 7;
                    }
                    setFlag(FLAG_Z, REG_C == 0);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 0);
                    break;
                case 0x1A: // RR D
                    {
                        bool tmp = flagSet(FLAG_C);
                        setFlag(FLAG_C, (REG_D & 0x01) == 0x01);
                        REG_D >>= 1;
                        REG_D |= tmp << 7;
                    }
                    setFlag(FLAG_Z, REG_D == 0);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 0);
                    break;
                case 0x1B: // RR E
                    {
                        bool tmp = flagSet(FLAG_C);
                        setFlag(FLAG_C, (REG_E & 0x01) == 0x01);
                        REG_E >>= 1;
                        REG_E |= tmp << 7;
                    }
                    setFlag(FLAG_Z, REG_E == 0);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 0);
                    break;
                case 0x23: // SLA E
                    setFlags((REG_E << 1) == 0, 0, 0, (REG_E & 0x80) == 0x80);
                    REG_E <<= 1;
                    break;
                case 0x27: // SLA A
                    setFlags((REG_A << 1) == 0, 0, 0, (REG_A & 0x80) == 0x80);
                    REG_A <<= 1;
                    break;
                case 0x28: // SRA B
                    setFlags((REG_B >> 1) == 0, 0, 0, 0);
                    REG_B >>= 1;
                    break;
                case 0x2F: // SRA A
                    setFlags((REG_A >> 1) == 0, 0, 0, 0);
                    REG_A >>= 1;
                    break;
                case 0x30: // SWAP B
                    REG_B = ((REG_B & 0x0F) << 4) | ((REG_B & 0xF0) >> 4);
                    setFlags(REG_B == 0, 0, 0, 0);
                    break;
                case 0x31: // SWAP C
                    REG_C = ((REG_C & 0x0F) << 4) | ((REG_C & 0xF0) >> 4);
                    setFlags(REG_C == 0, 0, 0, 0);
                    break;
                case 0x33: // SWAP E
                    REG_E = ((REG_E & 0x0F) << 4) | ((REG_E & 0xF0) >> 4);
                    setFlags(REG_E == 0, 0, 0, 0);
                    break;
                case 0x37: // SWAP A
                    REG_A = ((REG_A & 0x0F) << 4) | ((REG_A & 0xF0) >> 4);
                    setFlags(REG_A == 0, 0, 0, 0);
                    break;
                case 0x38: // SRL B
                    setFlags((REG_B >> 1) == 0, 0, 0, (REG_B & 0x01) == 0x01);
                    REG_B >>= 1;
                    break;
                case 0x3F: // SRL A
                    setFlags((REG_A >> 1) == 0, 0, 0, (REG_A & 0x01) == 0x01);
                    REG_A >>= 1;
                    break;
                case 0x40: // BIT 0,B
                    setFlag(FLAG_Z, (REG_B & 0x01) != 0x01);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x42: // BIT 0,D
                    setFlag(FLAG_Z, (REG_D & 0x01) != 0x01);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x45: // BIT 0,L
                    setFlag(FLAG_Z, (REG_L & 0x01) != 0x01);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x46: // BIT 0,(HL)
                    setFlag(FLAG_Z, (mem.read(getHL()) & 0x01) != 0x01);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x47: // BIT 0,A
                    setFlag(FLAG_Z, (REG_A & 0x01) != 0x01);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x48: // BIT 1,B
                    setFlag(FLAG_Z, (REG_B & 0x02) != 0x02);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x4E: // BIT 1,(HL)
                    setFlag(FLAG_Z, (mem.read(getHL()) & 0x02) != 0x02);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x4F: // BIT 1,A
                    setFlag(FLAG_Z, (REG_A & 0x02) != 0x02);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x50: // BIT 2,B
                    setFlag(FLAG_Z, (REG_B & 0x04) != 0x04);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x53: // BIT 2,E
                    setFlag(FLAG_Z, (REG_E & 0x04) != 0x04);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x56: // BIT 2,(HL)
                    setFlag(FLAG_Z, (mem.read(getHL()) & 0x04) != 0x04);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x57: // BIT 2,A
                    setFlag(FLAG_Z, (REG_A & 0x04) != 0x04);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x58: // BIT 3,B
                    setFlag(FLAG_Z, (REG_B & 0x08) != 0x08);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x5B: // BIT 3,E
                    setFlag(FLAG_Z, (REG_E & 0x08) != 0x08);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x5F: // BIT 3,A
                    setFlag(FLAG_Z, (REG_A & 0x08) != 0x08);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x60: // BIT 4,B
                    setFlag(FLAG_Z, (REG_B & 0x10) != 0x10);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x61: // BIT 4,C
                    setFlag(FLAG_Z, (REG_C & 0x10) != 0x10);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x65: // BIT 4,L
                    setFlag(FLAG_Z, (REG_L & 0x10) != 0x10);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x67: // BIT 4,A
                    setFlag(FLAG_Z, (REG_A & 0x10) != 0x10);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x68: // BIT 5,B
                    setFlag(FLAG_Z, (REG_B & 0x20) != 0x20);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x69: // BIT 5,C
                    setFlag(FLAG_Z, (REG_C & 0x20) != 0x20);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x6F: // BIT 5,A
                    setFlag(FLAG_Z, (REG_A & 0x20) != 0x20);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x70: // BIT 6,B
                    setFlag(FLAG_Z, (REG_B & 0x40) != 0x40);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x77: // BIT 6,A
                    setFlag(FLAG_Z, (REG_A & 0x40) != 0x40);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x78: // BIT 7,B
                    setFlag(FLAG_Z, (REG_B & 0x80) != 0x80);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x7C: // BIT 7,H
                    setFlag(FLAG_Z, (REG_H & 0x80) != 0x80);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x7E: // BIT 7,(HL)
                    setFlag(FLAG_Z, (mem.read(getHL()) & 0x80) != 0x80);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x7F: // BIT 7,A
                    setFlag(FLAG_Z, (REG_A & 0x80) != 0x80);
                    setFlag(FLAG_N, 0);
                    setFlag(FLAG_H, 1);
                    break;
                case 0x83: // RES 0,E
                    REG_E &= ~0x01;
                    break;
                case 0x86: // RES 0,(HL)
                    mem.write(getHL(), mem.read(getHL()) & ~0x01);
                    break;
                case 0x87: // RES 0,A
                    REG_A &= ~0x01;
                    break;
                case 0x8F: // RES 1,A
                    REG_A &= ~0x02;
                    break;
                case 0x95: // RES 2,L
                    REG_L &= ~0x04;
                    break;
                case 0xA7: // RES 4,A
                    REG_A &= ~0x10;
                    break;
                case 0xAE: // RES 5,(HL)
                    mem.write(getHL(), mem.read(getHL()) & ~0x20);
                    break;
                case 0xBE: // RES 7,(HL)
                    mem.write(getHL(), mem.read(getHL()) & ~0x80);
                    break;
                case 0xC5: // SET 0,L
                    REG_L |= 0x01;
                    break;
                case 0xC6: // SET 0,(HL)
                    mem.write(getHL(), mem.read(getHL()) | 0x01);
                    break;
                case 0xC7: // SET 0,A
                    REG_A |= 0x01;
                    break;
                case 0xCF: // SET 1,A
                    REG_A |= 0x02;
                    break;
                case 0xDF: // SET 3,A
                    REG_A |= 0x08;
                    break;
                case 0xEE: // SET 5,(HL)
                    mem.write(getHL(), mem.read(getHL()) | 0x20);
                    break;
                case 0xFE: // SET 7,(HL)
                    mem.write(getHL(), mem.read(getHL()) | 0x80);
                    break;
                default:
                    halt = true;
                    SDL_Log("Invalid CB-prefixed instruction 0x%02X at 0x%04X in bank #%d; CPU halted.", opcode, REG_PC - 2, mem.ROMbank);
                    break;
            }
            break;
        case 0xD3:
        case 0xDB:
        case 0xDD:
        case 0xE3:
        case 0xE4:
        case 0xEB:
        case 0xEC:
        case 0xED:
        case 0xF4:
        case 0xFC:
        case 0xFD:
            SDL_Log("Illegal opcode 0x%02X", opcode);
            break;
        default:
            halt = true;
            SDL_Log("Invalid instruction 0x%02X at 0x%04X in bank #%d; CPU halted.", opcode, REG_PC - 1, mem.ROMbank);
            break;
    }

    idleTicks += cycles * 4;
}

void gbcpu::update(uint64_t now) {
    uint64_t elapsed = now - lastTick;
    if (lastTick == 0) {
        elapsed = period;
    }
    lastTick = now;
    uint64_t loops = 0;
    while (elapsed > period && loops++ < 100000) {
        tick();
        elapsed -= period;
    }
}