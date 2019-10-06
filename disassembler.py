import sys

def toHex(v, d):
    alpha = "0123456789ABCDEF"
    r = ""
    for i in range(d):
        r = alpha[(v & (15 << (i * 4))) >> (i * 4)] + r
    return r + "h"

special = {
    0x2000: "ROM bank select register",
    
    0x8000: "VRAM tile data low end",
    0x97FF: "VRAM tile data high end",
    
    0x9800: "VRAM map low end",
    0x9FFF: "VRAM map high end",
    
    0xFE00: "OAM low end",
    0xFE9F: "OAM high end",
    
    0xFF00: "input register",
    0xFF01: "serial data register",
    0xFF02: "serial control register",
    
    0xFF04: "timer divider register",
    0xFF05: "timer counter register",
    0xFF06: "timer modulo register",
    0xFF07: "timer control register",

    0xFF40: "LCD control register",
    0xFF41: "LCD status register",
    0xFF42: "scroll Y register",
    0xFF43: "scroll X register",
    0xFF44: "scanline register",
    0xFF45: "scanline compare register",
    0xFF46: "DMA launch register",
    0xFF47: "BG palette register",
    0xFF48: "sprite palette 0 register",
    0xFF49: "sprite palette 1 register",
    0xFF4A: "window Y register",
    0xFF4B: "window X register",

    0xFF50: "boot ROM disable register",
    
    0xFF0F: "interrupt flag register",
    0xFFFF: "interrupt enable register"
}

def checkAddress(addr):
    if addr in special:
        return special[addr]
    if addr >= 0x8000 and addr < 0x9800:
        return "VRAM tile data"
    if addr >= 0x9800 and addr < 0xA000:
        return "VRAM map"
    if addr >= 0xFE00 and addr < 0xFEA0:
        return "OAM"
    if addr >= 0xFF10 and addr < 0xFF40:
        return "sound register"
    if addr >= 0xFF80 and addr < 0xFFFF:
        return "high RAM"
    if addr >= 0xC000 and addr < 0xE000:
        return "work RAM"
    if addr >= 0xA000 and addr < 0xC000:
        return "cartridge RAM"
    return ""

def labelSort(l): # passed to sorted() to sort labels by address in increasing order
    for p in labels:
        if p[2] == l:
            return p[0]
    return 0

# opcode to instruction/mnemonic mapping
# opcode: (bytes, "mnemonic")
opcodes = {
    0x00: (1, "NOP"),
    0x01: (3, "LD BC,{}"),
    0x02: (1, "LD (BC),A"),
    0x03: (1, "INC BC"),
    0x04: (1, "INC B"),
    0x05: (1, "DEC B"),
    0x06: (2, "LD B,{}"),
    0x07: (1, "RLCA"),
    0x08: (3, "LD ({}),SP"),
    0x09: (1, "ADD HL,BC"),
    0x0A: (1, "LD A,(BC)"),
    0x0B: (1, "DEC BC"),
    0x0C: (1, "INC C"),
    0x0D: (1, "DEC C"),
    0x0E: (2, "LD C,{}"),
    0x0F: (1, "RRCA"),
    
    0x10: (1, "STOP"),
    0x11: (3, "LD DE,{}"),
    0x12: (1, "LD (DE),A"),
    0x13: (1, "INC DE"),
    0x14: (1, "INC D"),
    0x15: (1, "DEC D"),
    0x16: (2, "LD D,{}"),
    0x17: (1, "RLA"),
    0x18: (2, "JR {}"),
    0x19: (1, "ADD HL,DE"),
    0x1A: (1, "LD A,(DE)"),
    0x1B: (1, "DEC DE"),
    0x1C: (1, "INC E"),
    0x1D: (1, "DEC E"),
    0x1E: (2, "LD E,{}"),
    0x1F: (1, "RRA"),
    
    0x20: (2, "JR NZ,{}"),
    0x21: (3, "LD HL,{}"),
    0x22: (1, "LD (HL+),A"),
    0x23: (1, "INC HL"),
    0x24: (1, "INC H"),
    0x25: (1, "DEC H"),
    0x26: (2, "LD H,{}"),
    0x27: (1, "DAA"),
    0x28: (2, "JR Z,{}"),
    0x29: (1, "ADD HL,HL"),
    0x2A: (1, "LD A,(HL+)"),
    0x2B: (1, "DEC HL"),
    0x2C: (1, "INC L"),
    0x2D: (1, "DEC L"),
    0x2E: (2, "LD L,{}"),
    0x2F: (1, "CPL"),

    0x30: (2, "JR NC,{}"),
    0x31: (3, "LD SP,{}"),
    0x32: (1, "LD (HL-),A"),
    0x34: (1, "INC (HL)"),
    0x36: (2, "LD (HL),{}"),
    0x38: (2, "JR C,{}"),
    0x3A: (1, "LD A,(HL-)"),
    0x3C: (1, "INC A"),
    0x3D: (1, "DEC A"),
    0x3E: (2, "LD A,{}"),

    0x40: (1, "LD B,B"),
    0x44: (1, "LD B,H"),
    0x46: (1, "LD B,(HL)"),
    0x47: (1, "LD B,A"),
    0x4D: (1, "LD C,L"),
    0x4F: (1, "LD C,A"),
    
    0x56: (1, "LD D,(HL)"),
    0x57: (1, "LD D,A"),
    0x5E: (1, "LD E,(HL)"),
    0x5F: (1, "LD E,A"),
    
    0x62: (1, "LD H,D"),
    0x66: (1, "LD H,(HL)"),
    0x67: (1, "LD H,A"),
    0x69: (1, "LD L,C"),
    0x6B: (1, "LD L,E"),
    0x6E: (1, "LD L,(HL)"),
    0x6F: (1, "LD L,A"),

    0x70: (1, "LD (HL),B"),
    0x71: (1, "LD (HL),C"),
    0x72: (1, "LD (HL),D"),
    0x73: (1, "LD (HL),E"),
    0x74: (1, "LD (HL),H"),
    0x75: (1, "LD (HL),L"),
    0x77: (1, "LD (HL),A"),
    0x78: (1, "LD A,B"),
    0x79: (1, "LD A,C"),
    0x7A: (1, "LD A,D"),
    0x7B: (1, "LD A,E"),
    0x7C: (1, "LD A,H"),
    0x7D: (1, "LD A,L"),
    0x7E: (1, "LD A,(HL)"),

    0x80: (1, "ADD A,B"),
    0x86: (1, "ADD A,(HL)"),
    0x87: (1, "ADD A,A"),

    0x90: (1, "SUB A,B"),
    0x9F: (1, "SBC A,A"),

    0xA7: (1, "AND A,A"),
    0xAF: (1, "XOR A,A"),

    0xB0: (1, "OR A,B"),
    0xB1: (1, "OR A,C"),
    0xB3: (1, "OR A,E"),
    0xB6: (1, "OR A,(HL)"),
    0xB7: (1, "OR A,A"),
    0xBE: (1, "CP A,(HL)"),

    0xC0: (1, "RET NZ"),
    0xC1: (1, "POP BC"),
    0xC2: (3, "JP NZ,{}"),
    0xC3: (3, "JP {}"),
    0xC4: (3, "CALL NZ,{}"),
    0xC5: (1, "PUSH BC"),
    0xC8: (1, "RET Z"),
    0xC9: (1, "RET"),
    0xCA: (3, "JP Z,{}"),
    0xCB: (2, ""),
    0xCE: (2, "ADC A,{}"),
    0xCD: (3, "CALL {}"),

    0xD0: (1, "RET NC"),
    0xD1: (1, "POP DE"),
    0xD5: (1, "PUSH DE"),
    0xD6: (2, "SUB {}"),
    0xD8: (1, "RET C"),
    0xD9: (1, "RETI"),
    0xDA: (3, "JP C,{}"),

    0xE0: (2, "LDH (FF{}),A"),
    0xE1: (1, "POP HL"),
    0xE2: (1, "LD (C),A"),
    0xE5: (1, "PUSH HL"),
    0xE6: (2, "AND {}"),
    0xE9: (1, "JP (HL)"),
    0xEA: (3, "LD ({}),A"),
    0xEF: (1, "RST 28h"),

    0xF0: (2, "LDH A,(FF{})"),
    0xF1: (1, "POP AF"),
    0xF3: (1, "DI"),
    0xF5: (1, "PUSH AF"),
    0xF6: (2, "OR {}"),
    0xF8: (2, "LD HL,SP+({})"),
    0xF9: (1, "LD SP,HL"),
    0xFA: (3, "LD A,({})"),
    0xFB: (1, "EI"),
    0xFE: (2, "CP {}"),
    0xFF: (1, "RST 38h")
}

bitInsts = {
    0x11: "RL C",

    0x37: "SWAP A",
    
    0x7B: "BIT 7,E",
    0x7C: "BIT 7,H"
}

disassembly = {}
raw = []

bases = []

romLen = 0x8000

labels = [ # addresses to disassemble, i.e. known places for labels
    (0x00, romLen, "0000h"),
    (0x08, romLen, "0008h"),
    (0x10, romLen, "0010h"),
    (0x18, romLen, "0018h"),
    (0x20, romLen, "0020h"),
    (0x28, romLen, "0028h"),
    (0x30, romLen, "0030h"),
    (0x38, romLen, "0038h"),

    (0x40, romLen, "V-Blank IRQ"),
    (0x48, romLen, "STAT IRQ"),
    (0x50, romLen, "Timer IRQ"),
    (0x58, romLen, "Serial IRQ"),
    (0x60, romLen, "Joypad IRQ"),

    (0x100, 0x104, "main")
]

fileIn = ""
fileOut = ""

if len(sys.argv) > 1:
    for i in range(1, len(sys.argv)):
        if sys.argv[i] == "-l": # -l [address]
            print("Disassembling additionally from " + toHex(int(sys.argv[i + 1], 16), 4))
            labels.append((int(sys.argv[i + 1], 16), romLen, toHex(int(sys.argv[i + 1], 16), 4)))
        if sys.argv[i] == "-b": # -b [start address] [end address] [base address]
            print("Rebasing addresses " + toHex(int(sys.argv[i + 1], 16), 4) + " - " + toHex(int(sys.argv[i + 2], 16), 4) + " to " + toHex(int(sys.argv[i + 3], 16), 4))
            bases.append((int(sys.argv[i + 1], 16), int(sys.argv[i + 2], 16), int(sys.argv[i + 3], 16)))
        if sys.argv[i] == "-i":
            fileIn = sys.argv[i + 1]
        if sys.argv[i] == "-o":
            fileOut = sys.argv[i + 1]

if fileIn == "":
    while True:
        fileIn = input("File to disassemble? ")
        try:
            with open(fileIn, "rb") as f:
                raw = f.read(romLen)
                break
        except FileNotFoundError:
            print("File not found")
else:
    try:
        with open(fileIn, "rb") as f:
            raw = f.read(romLen)
    except FileNotFoundError:
        print("Input file not found")

if fileOut == "":
    fileOut = input("File for output? ")

def rebaseAddress(addr):
    for b in bases:
        if addr >= b[0] and addr < b[1]:
            return (addr - b[0]) + b[2]
    return addr

addrDefs = [0xC2, 0xC3, 0xC4, 0xCA, 0xCC, 0xCD, 0xD2, 0xD4, 0xDA, 0xDC]                         # opcodes that have an address after them - used for finding sections with code
relAddrDefs = [0x18, 0x20, 0x28, 0x30, 0x38]                                                    # opcodes that have a relative address after them
noReturn = [0xC3, 0x18, 0xC9, 0xD9, 0xE9, 0xC7, 0xCF, 0xD7, 0xDF, 0xE7, 0xEF, 0xF7, 0xFF, 0x10] # opcodes that are surely not followed by code

i = 0       # current index of label in list
while True: # iterate over addresses to disassemble
    if i >= len(labels): # stop if at end
        break
    head = labels[i][0] # get starting address of label
    if head >= len(raw): # if label is beyond end of ROM
        i += 1           # take the next one
        continue         # and repeat previous steps
    if labels[i][2] not in disassembly: # make sure label has entry in dictionary 
        disassembly[labels[i][2]] = ""
    else:                               # if it already has one, it is already disassembled
        i += 1                          # so continue to next label
        continue
    cur = labels[i][2] # the name of the currently disassembled label
    while True:
        if head >= len(raw) or head >= labels[i][1]: # if end of label or ROM found
            break                                    # stop and move onto next label
        while True:
            dt = False
            for j in range(len(labels)):
                if i == j:
                    continue
                if head >= labels[j][0] and head <= labels[j][0] + 1:
                    disassembly.pop(labels[j][2], None)
                    del labels[j]
                    dt = True
                    for x in range(len(labels)):
                        if labels[x][2] == cur:
                            i = x
                    break
            if not dt:
                break
        if raw[head] not in opcodes:
            print("Could not disassemble " + toHex(raw[head], 2))
            labels[i] = (labels[i][0], head, labels[i][2])
            disassembly[labels[i][2]] += "    ; " + toHex(raw[head], 2) + "?\n"
            break
        if raw[head] in addrDefs or raw[head] in relAddrDefs:
            jp = rebaseAddress(raw[head + 1] | (raw[head + 2] << 8))
            f = True
            if raw[head] in relAddrDefs:
                offset = raw[head + 1]
                if offset & 0x80:
                    offset -= 0x100
                jp = head + 2 + offset
            for l in labels:
                if l[0] == jp:
                    f = False
                    break
            if f and (jp > head or jp < labels[i][0]):
                labels.append((jp, len(raw), toHex(jp, 4)))
        if raw[head] in noReturn:
            labels[i] = (labels[i][0], head + opcodes[raw[head]][0], labels[i][2])
        op = opcodes[raw[head]][1] # get opcode assembly mnemonic
        com = ""
        if raw[head] == 0xCB:
            if raw[head + 1] in bitInsts:
                op = bitInsts[raw[head + 1]]
            else:
                print("Could not disassemble CB" + toHex(raw[head + 1], 2))
                disassembly[labels[i][2]] += "    ; CB" + toHex(raw[head + 1], 2) + "?\n"
                labels[i] = (labels[i][0], head, labels[i][2])
                break
        if opcodes[raw[head]][0] == 2:
            if op[:2] == "JR" or raw[head] in [0xF8, 0xE8]:
                offset = raw[head + 1]
                if offset & 0x80:
                    offset -= 0x100
                if raw[head] in [0xF8, 0xE8]:
                    op = op.format(offset)
                else:
                    op = op.format(toHex(head + 2 + offset, 4))
            else:
                op = op.format(toHex(raw[head + 1], 2))
                if op[:3] == "LDH":
                    com = checkAddress(0xFF00 + raw[head + 1])
        elif opcodes[raw[head]][0] == 3:
            addr = rebaseAddress(raw[head + 1] | (raw[head + 2] << 8))
            op = op.format(toHex(addr, 4))
            com = checkAddress(addr)
        tb = (16 - len(op)) * " "
        op += tb + "; " + toHex(head, 4) + "    " + toHex(raw[head], 2) + " "
        for j in range(1, opcodes[raw[head]][0]):
           op += toHex(raw[head + j], 2) + " "
        for j in range(opcodes[raw[head]][0], 4):
           op += "    "
        disassembly[labels[i][2]] += "    " + op + "   " + com + "\n" # add dissassembled instruction under label
        head += opcodes[raw[head]][0]
    i += 1

def inLabel(a):
    for l in labels:
        if a >= l[0] and a < l[1]:
            return l
    return -1

def char(c):
    if c >= 0x20 and c < 0x7F:
        return chr(c)
    return "."

i = 0
while i < len(raw):
    l = inLabel(i) # check if data added to label
    if l != -1:    # if not part of label (i.e. if not disassembled/code)
        i = l[1]   # go to end of label
        continue   # repeat last three lines to be sure
    if i < len(raw):
        startAddr = i
        length = 0
        b = ""
        while i < len(raw) and inLabel(i) == -1:
            r = ""
            t = ""
            j = 0
            s = False
            for j in range(16):
                l = inLabel(i + j)
                if l != -1 or i + j >= len(raw):
                    j -= 1
                    s = True
                    break
                r += toHex(raw[i + j], 2) + " "
                t += char(raw[i + j])
                length += 1
            b += "    .db " + r + ((68 - len(r)) * " ") + "; " + toHex(i, 4) + " - " + toHex(i + j, 4) + " " + t + "\n"
            if s:
                break
            i += 16
        labels.append((startAddr, startAddr + length, toHex(startAddr, 4)))
        disassembly[toHex(startAddr, 4)] = b
    i += 1

finished = ""
for l in sorted(disassembly.keys(), key = labelSort):
    c = ""
    if l == "0104h":
        c = " ; cartridge header"
    elif l == "main":
        c = " ; code entry point"
    finished += l + ":" + c + "\n" + disassembly[l] + "\n"

with open(fileOut, "w") as f:
    f.write(finished)
input("Press return to continue...")

