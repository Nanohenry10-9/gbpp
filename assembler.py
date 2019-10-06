from subprocess import call
import sys

cond_jps_a16 = {}
cond_jps_a16["Z"] = 0xCA
cond_jps_a16["C"] = 0xDA
cond_jps_a16["NZ"] = 0xC2
cond_jps_a16["NC"] = 0xD2
cond_calls_a16 = {}
cond_calls_a16["Z"] = 0xCC
cond_calls_a16["C"] = 0xDC
cond_calls_a16["NZ"] = 0xC2
cond_calls_a16["NC"] = 0xD2
cond_rets = {}
cond_rets["Z"] = 0xC8
cond_rets["C"] = 0xD8
cond_rets["NZ"] = 0xC0
cond_rets["NC"] = 0xD0

def getML(asm):
    global stackOps
    global errors
    inst = asm.split(" ")[0]
    op = ""
    if len(asm.split(" ")) > 1:
        op = asm.split(" ")[1]
    r = []
    toFill = ""
    if inst == "NOP":
        r.append(0x00)
    elif inst == "STOP":
        r.append(0x10)
    elif inst == "HALT":
        r.append(0x76)
    elif inst == "DI":
        r.append(0xF3)
    elif inst == "EI":
        r.append(0xFB)
    elif inst == "RET":
        if op == "":
            r.append(0xC9)
        else:
            r.append(cond_rets[op])
    elif inst == "RETI":
        r.append(0xD9)
    elif inst == "LD":
        op1 = op.split(",")[0]
        op2 = op.split(",")[1]
        if op1 == "A":
            if op2 == "(BC)":
                r.append(0x0A)
            elif op2 == "(DE)":
                r.append(0x1A)
            elif op2 == "(HL+)":
                r.append(0x2A)
            elif op2 == "(HL-)":
                r.append(0x3A)
            elif op2 == "B":
                r.append(0x78)
            elif op2 == "C":
                r.append(0x79)
            elif op2 == "D":
                r.append(0x7A)
            elif op2 == "E":
                r.append(0x7B)
            elif op2 == "H":
                r.append(0x7C)
            elif op2 == "L":
                r.append(0x7D)
            elif op2 == "(HL)":
                r.append(0x7E)
            elif op2 == "A":
                r.append(0x7F)
            elif op2 == "(C)":
                r.append(0xF2)
            elif op2[0] == "(" and op2[-2:] == "h)":
                if len(op2) == 7:
                    r.append(0xFA)
                    r.append(int(op2[3:5], 16))
                    r.append(int(op2[1:3], 16))
            elif op2[-1] == "h":
                if len(op2) == 3:
                    r.append(0x3E)
                    r.append(int(op2[0:2], 16))
            elif op2[1:-1] in labels:
                r.append(0xFA)
                r.append(0x00)
                r.append(0x00)
                toFill = op2[1:-1]
        elif op1[0] == "(" and op1[-2:] == "h)":
            if op2 == "A":
                if len(op1) == 7:
                    r.append(0xEA)
                    r.append(int(op1[3:5], 16))
                    r.append(int(op1[1:3], 16))
        elif op1 == "HL":
            if op2[-1] == "h":
                r.append(0x21)
                r.append(int(op2[2:4], 16))
                r.append(int(op2[0:2], 16))
            elif op2 in labels:
                r.append(0x21)
                r.append(0x00)
                r.append(0x00)
                toFill = op2
        elif op1 == "BC":
            if op2[-1] == "h":
                r.append(0x01)
                r.append(int(op2[2:4], 16))
                r.append(int(op2[0:2], 16))
            elif op2 in labels:
                r.append(0x01)
                r.append(0x00)
                r.append(0x00)
                toFill = op2
        elif op1 == "DE":
            if op2[-1] == "h":
                r.append(0x11)
                r.append(int(op2[2:4], 16))
                r.append(int(op2[0:2], 16))
            elif op2 in labels:
                r.append(0x11)
                r.append(0x00)
                r.append(0x00)
                toFill = op2
        elif op1 == "SP":
            if op2[-1] == "h":
                r.append(0x31)
                r.append(int(op2[2:4], 16))
                r.append(int(op2[0:2], 16))
        elif op1 == "B":
            if op2 == "A":
                r.append(0x47)
            elif op2 == "B":
                r.append(0x40)
            elif op2 == "C":
                r.append(0x41)
            elif op2 == "D":
                r.append(0x42)
            elif op2[-1] == "h":
                if len(op2) == 3:
                    r.append(0x06)
                    r.append(int(op2[0:2], 16))
        elif op1 == "C":
            if op2 == "A":
                r.append(0x4F)
            elif op2 == "E":
                r.append(0x4B)
            elif op2[-1] == "h":
                if len(op2) == 3:
                    r.append(0x0E)
                    r.append(int(op2[0:2], 16))
        elif op1 == "D":
            if op2 == "A":
                r.append(0x57)
            elif op2[-1] == "h":
                if len(op2) == 3:
                    r.append(0x16)
                    r.append(int(op2[0:2], 16))
        elif op1 == "E":
            if op2 == "A":
                r.append(0x5F)
            if op2 == "C":
                r.append(0x59)
            elif op2[-1] == "h":
                if len(op2) == 3:
                    r.append(0x1E)
                    r.append(int(op2[0:2], 16))
        elif op1 == "H":
            if op2 == "A":
                r.append(0x67)
        elif op1 == "L":
            if op2 == "A":
                r.append(0x6F)
        elif op1 == "(HL)":
            if op2 == "A":
                r.append(0x77)
        elif op1 == "(HL+)":
            if op2 == "A":
                r.append(0x22)
        elif op1 == "(BC)":
            if op2 == "A":
                r.append(0x02)
        elif op1 == "(DE)":
            if op2 == "A":
                r.append(0x12)
        elif op1[1:-1] in labels:
            if op2 == "A":
                r.append(0xEA)
                r.append(0x00)
                r.append(0x00)
                toFill = op1[1:-1]
    elif inst == "CP":
        if op == "B":
            r.append(0xB8)
        elif op == "C":
            r.append(0xB9)
        elif op == "D":
            r.append(0xBA)
        elif op == "E":
            r.append(0xBB)
        elif op[-1] == "h":
            r.append(0xFE)
            r.append(int(op[0:2], 16))
    elif inst == "INC":
        if op == "A":
            r.append(0x3C)
        elif op == "B":
            r.append(0x04)
        elif op == "C":
            r.append(0x0C)
        elif op == "HL":
            r.append(0x23)
        elif op == "(HL)":
            r.append(0x34)
        elif op == "BC":
            r.append(0x03)
        elif op == "DE":
            r.append(0x13)
    elif inst == "DEC":
        if op == "A":
            r.append(0x3D)
        elif op == "B":
            r.append(0x05)
        elif op == "C":
            r.append(0x0D)
        elif op == "D":
            r.append(0x15)
        elif op == "E":
            r.append(0x1D)
        elif op == "DE":
            r.append(0x1B)
        elif op == "HL":
            r.append(0x2B)
        elif op == "(HL)":
            r.append(0x35)
    elif inst == "JP":
        label = ""
        if op in labels:
            label = op
        elif len(op.split(",")) > 1 and op.split(",")[1] in labels:
            label = op.split(",")[1]
        if label != "":
            toFill = label
            if len(op.split(",")) > 1 and op.split(",")[0] in cond_jps_a16:
                r.append(cond_jps_a16[op.split(",")[0]])
            else:
                r.append(0xC3)
            r.append(0)
            r.append(0)
        elif op[-1] == "h" and len(op) == 5:
            r.append(0xC3)
            r.append(int(op[2:4], 16))
            r.append(int(op[0:2], 16))
        elif len(op.split(",")) > 1 and op.split(",")[0] in cond_jps_a16 and op.split(",")[1][-1] == "h" and len(op.split(",")[1]) == 5:
            r.append(cond_jps_a16[op.split(",")[0]])
            r.append(int(op.split(",")[1][2:4], 16))
            r.append(int(op.split(",")[1][0:2], 16))
        elif op == "(HL)":
            r.append(0xE9)
    elif inst == "CALL":
        label = ""
        if op in labels:
            label = op
        elif len(op.split(",")) > 1 and op.split(",")[1] in labels:
            label = op.split(",")[1]
        if label != "":
            toFill = label
            if len(op.split(",")) > 1 and op.split(",")[0] in cond_jps_a16:
                r.append(cond_calls_a16[op.split(",")[0]])
            else:
                r.append(0xCD)
            r.append(0)
            r.append(0)
        elif op[-1] == "h" and len(op) == 5:
            r.append(0xCD)
            r.append(int(op[2:4], 16))
            r.append(int(op[0:2], 16))
    elif inst == "RRA":
        r.append(0x1F)
    elif inst == "XOR":
        if op == "A":
            r.append(0xAF)
        elif op == "D":
            r.append(0xAA)
        elif op[-1] == "h":
            if len(op) == 3:
                r.append(0xEE)
                r.append(int(op[0:2], 16))
    elif inst == "AND":
        if op == "B":
            r.append(0xA0)
        elif op[-1] == "h":
            if len(op) == 3:
                r.append(0xE6)
                r.append(int(op[0:2], 16))
    elif inst == "OR":
        if op == "B":
            r.append(0xB0)
        elif op == "D":
            r.append(0xB2)
        elif op[-1] == "h":
            if len(op) == 3:
                r.append(0xF6)
                r.append(int(op[0:2], 16))
    elif inst == "SBC":
        if op[-1] == "h":
            if len(op) == 3:
                r.append(0xDE)
                r.append(int(op[0:2], 16))
    elif inst == "ADD":
        op1 = op.split(",")[0]
        op2 = op.split(",")[1]
        if op1 == "A":
            if op2 == "B":
                r.append(0x80)
            elif op2 == "C":
                r.append(0x81)
            elif op2 == "H":
                r.append(0x84)
            elif op2[-1] == "h":
                if len(op2) == 3:
                    r.append(0xC6)
                    r.append(int(op2[0:2], 16))
        if op1 == "HL":
            if op2 == "BC":
                r.append(0x09)
            if op2 == "DE":
                r.append(0x19)
    elif inst == "SUB":
        op1 = op.split(",")[0]
        op2 = op.split(",")[1]
        if op1 == "A":
            if op2 == "B":
                r.append(0x90)
            elif op2 == "C":
                r.append(0x91)
            elif op2[-1] == "h":
                if len(op2) == 3:
                    r.append(0xD6)
                    r.append(int(op2[0:2], 16))
    elif inst == "BIT":
        op1 = op.split(",")[0]
        op2 = op.split(",")[1]
        if op1 == "0":
            if op2 == "A":
                r.append(0xCB)
                r.append(0x47)
        elif op1 == "1":
            if op2 == "A":
                r.append(0xCB)
                r.append(0x4F)
    elif inst == "RES":
        op1 = op.split(",")[0]
        op2 = op.split(",")[1]
        if op1 == "0":
            if op2 == "A":
                r.append(0xCB)
                r.append(0x87)
    elif inst == "SET":
        op1 = op.split(",")[0]
        op2 = op.split(",")[1]
        if op1 == "7":
            if op2 == "A":
                r.append(0xCB)
                r.append(0xFF)
    elif inst == "SRL":
        if op == "A":
            r.append(0xCB)
            r.append(0x3F)
    elif inst == "SLA":
        if op == "A":
            r.append(0xCB)
            r.append(0x27)
    elif inst == "PUSH":
        stackOps += 1
        if op == "AF":
            r.append(0xF5)
        elif op == "BC":
            r.append(0xC5)
        elif op == "DE":
            r.append(0xD5)
        elif op == "HL":
            r.append(0xE5)
    elif inst == "POP":
        stackOps -= 1
        if op == "AF":
            r.append(0xF1)
        elif op == "BC":
            r.append(0xC1)
        elif op == "DE":
            r.append(0xD1)
        elif op == "HL":
            r.append(0xE1)
    if len(r) == 0:
        print("Failed to assemble " + asm)
        errors += 1
    return (r, toFill)

def toHex(v, d):
    alpha = "0123456789ABCDEF"
    r = ""
    for i in range(d):
        r = alpha[(v & (15 << (i * 4))) >> (i * 4)] + r
    return r + "h"

def getLowerByte(v):
    return v & 0xFF

def getUpperByte(v):
    return (v >> 8) & 0xFF

def writeROM(b):
    global writehead
    global highestWrite
    ready[writehead] = b
    highestWrite = max(highestWrite, writehead)
    writehead += 1

def setWriteHead(l):
    global writehead
    writehead = l

def getLen(arr):
    c = 0
    for p in arr:
        c += len(p[0])
    return c

asm = []
ml = {}
ml["start"] = []
ml["V-Blank-ISR"] = []
ml["LCD-STAT-ISR"] = []
ml["Timer-ISR"] = []
ml["Serial-ISR"] = []
ml["Joypad-ISR"] = []
ml["00h"] = []
ml["08h"] = []
ml["10h"] = []
ml["18h"] = []
ml["20h"] = []
ml["28h"] = []
ml["30h"] = []
ml["38h"] = []
labels = {}
s_labels = {}
s_labels["00h"] = 0x00
s_labels["08h"] = 0x08
s_labels["10h"] = 0x10
s_labels["18h"] = 0x18
s_labels["20h"] = 0x20
s_labels["28h"] = 0x28
s_labels["30h"] = 0x30
s_labels["38h"] = 0x38
s_labels["V-Blank-ISR"] = 0x40
s_labels["LCD-STAT-ISR"] = 0x48
s_labels["Timer-ISR"] = 0x50
s_labels["Serial-ISR"] = 0x58
s_labels["Joypad-ISR"] = 0x60
s_labels["start"] = 0x150
for k in s_labels:
    labels[k] = s_labels[k]
last_label = ""
fn = ""
out = ""
if len(sys.argv) > 1:
    for i in range(1, len(sys.argv)):
        if sys.argv[i] == "-i":
            fn = sys.argv[i + 1]
        if sys.argv[i] == "-o":
            out = sys.argv[i + 1]

if fn == "":
    while True:
        fn = input("File to assemble? ")
        try:
            with open(fn, "r") as f:
                asm = f.read().split("\n")
                break
        except FileNotFoundError:
            print("File not found")
else:
    with open(fn, "r") as f:
        asm = f.read().split("\n")

if out == "":
    out = input("File for output? ")
    
ready = [0] * 0x8000
writehead = 0
highestWrite = 0
highestVar = 0
stackOps = 0
errors = 0

variables = []

for i in range(len(asm)):
    if asm[i] != "" and asm[i][0] == "\t":
        asm[i] = asm[i][1:]

for inst in asm:
    inst = inst.strip()
    if len(inst) == 0 or inst[0] == ";":
        continue
    if inst[-1] == ":" and inst[:-1] not in s_labels:
        if inst[:-1] in labels:
            print("Warning: double label " + inst[:-1])
        labels[inst[:-1]] = -1
    elif inst.split(" ")[0] == "CONST":
        labels[inst.split(" ")[1]] = -1
    elif inst.split(" ")[0] == "VAR":
        if len(variables) == 0:
            variables.append((0xC000, inst.split(" ")[1], int(inst.split(" ")[2][:2], 16)))
            highestVar = 0xC000 + variables[-1][2]
            #print("Variable " + inst.split(" ")[1] + " at C000h with length " + toHex(int(inst.split(" ")[2][:2], 16), 2))
        else:
            newVar = (variables[-1][0] + variables[-1][2], inst.split(" ")[1], int(inst.split(" ")[2][:2], 16))
            variables.append(newVar)
            highestVar += variables[-1][2]
            #print("Variable " + newVar[1] + " at " + toHex(newVar[0], 4) + " with length " + toHex(newVar[2], 2))
for i in range(len(asm)):
    if asm[i].split(" ")[0] in ["CONST", "VAR"]:
        continue
    for v in variables:
        asm[i] = toHex(v[0], 4).join(asm[i].split(v[1]))
for inst in asm:
    inst = inst.strip()
    if len(inst) == 0 or inst[0] == ";" or inst.split(" ")[0] == "VAR":
        continue
    elif inst.split(" ")[0] == "CONST":
        labels[inst.split(" ")[1]] = s_labels[last_label] + getLen(ml[last_label])
        d = inst.split(" ")[2].split(",")
        tmp = []
        for b in d:
            if len(b) == 3 and b[2] == "h":
                tmp.append(int(b[:2], 16))
            else:
                print("Invalid hex literal " + b)
                errers += 1
        ml[last_label].append((tmp, ""))
    elif inst[:-1] in labels:
        if inst[:-1] in s_labels:
            last_label = inst[:-1]
        else:
            labels[inst[:-1]] = s_labels[last_label] + getLen(ml[last_label])
    else:
        try:
            m = getML(inst)
            ml[last_label].append(m)
        except Exception as e:
            print(inst + " caused an error (" + str(e) + ")")
            errers += 1
for l in s_labels:
    setWriteHead(s_labels[l])
    for p in ml[l]:
        if p[1] != "":
            writeROM(p[0][0])
            writeROM(getLowerByte(labels[p[1]]))
            writeROM(getUpperByte(labels[p[1]]))
            continue
        for b in p[0]:
            writeROM(b)
print("=" * 5 + "> " + str(errors) + " error(s)")
print("Done assembling, code and data (excluding header) use " + str(highestWrite // 1024) + "KiB of ROM.")
print("Global variables use " + str(highestVar - 0xC000) + "B of WRAM.")
if stackOps > 0:
    print("WARNING: more PUSHes than POPs! Return addresses might be effed up.")
elif stackOps < 0:
    print("WARNING: more POPs than PUSHes! Return addresses might be effed up.")
else:
    print("INFO: Same number of PUSHes and POPs.")
print("Adding header...")
logo = [0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 
        0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 
        0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E]
for i in range(len(logo)):
    ready[0x104 + i] = logo[i]
checksum = 0
for i in range(0x134, 0x14C + 1):
    checksum = checksum - ready[i] - 1
checksum &= 0xFF
ready[0x14D] = checksum
ready[0x100] = 0xC3
ready[0x101] = getLowerByte(s_labels["start"])
ready[0x102] = getUpperByte(s_labels["start"])
print("Writing to file...")
with open(out, "wb") as file:
    file.write(bytearray(ready))
