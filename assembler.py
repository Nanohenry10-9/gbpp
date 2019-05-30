cond_jps_a16 = {}
cond_jps_a16["Z"] = 0xCA
cond_jps_a16["C"] = 0xDA
cond_jps_a16["NZ"] = 0xC2
cond_jps_a16["NC"] = 0xD2

def getML(asm):
    inst = asm.split(" ")[0]
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
        r.append(0xC9)
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
        elif op1 == "B":
            if op2 == "A":
                r.append(0x47)
            elif op2[-1] == "h":
                if len(op2) == 3:
                    r.append(0x06)
                    r.append(int(op2[0:2], 16))
        elif op1 == "(HL)":
            if op2 == "A":
                r.append(0x77)
        elif op1[1:-1] in labels:
            if op2 == "A":
                r.append(0xEA)
                r.append(0x00)
                r.append(0x00)
                toFill = op1[1:-1]
    elif inst == "CP":
        if op[-1] == "h":
            r.append(0xFE)
            r.append(int(op[0:2], 16))
    elif inst == "INC":
        if op == "A":
            r.append(0x3C)
        elif op == "HL":
            r.append(0x23)
        elif op == "DE":
            r.append(0x13)
    elif inst == "DEC":
        if op == "A":
            r.append(0x3D)
        if op == "B":
            r.append(0x05)
        elif op == "DE":
            r.append(0x1B)
        elif op == "HL":
            r.append(0x2B)
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
        elif op == "(HL)":
            r.append(0xE9)
    elif inst == "RRA":
        r.append(0x1F)
    elif inst == "XOR":
        if op == "A":
            r.append(0xAF)
        elif op[-1] == "h":
            if len(op) == 3:
                r.append(0xEE)
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
    if len(r) == 0:
        print("Failed to assemble " + asm)
    return (r, toFill)

def getLowerByte(v):
    return v & 0xFF

def getUpperByte(v):
    return (v >> 8) & 0xFF

def writeROM(b):
    global writehead
    ready[writehead] = b
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
fn = input("Assembly source? ")
out = input("Output file? ")
ready = [0] * 0x8000
writehead = 0

try:
    with open(fn, "r") as file:
        asm = file.read().split("\n")
    for inst in asm:
        inst = inst.strip()
        if len(inst) == 0 or inst[0] == ";":
            continue
        if inst[-1] == ":" and inst[:-1] not in s_labels:
            labels[inst[:-1]] = -1
        elif inst.split(" ")[0] == "DATA":
            labels[inst.split(" ")[1]] = -1
    for inst in asm:
        inst = inst.strip()
        if len(inst) == 0 or inst[0] == ";":
            continue
        elif inst.split(" ")[0] == "DATA":
            labels[inst.split(" ")[1]] = s_labels[last_label] + getLen(ml[last_label])
            d = inst.split(" ")[2].split(",")
            tmp = []
            for b in d:
                if len(b) == 3 and b[2] == "h":
                    tmp.append(int(b[:2], 16))
                else:
                    print("Invalid hex literal " + b)
            ml[last_label].append((tmp, ""))
        elif inst[:-1] in labels:
            if inst[:-1] in s_labels:
                last_label = inst[:-1]
            else:
                labels[inst[:-1]] = s_labels[last_label] + getLen(ml[last_label])
        else:
            m = getML(inst)
            ml[last_label].append(m)
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
    input("Press return to continue...")
except FileNotFoundError as e:
    print("No such file (" + str(e) + ")")
    input("Press enter to exit...")
except PermissionError as e:
    print("Invalid file (" + str(e) + ")")
    input("Press enter to exit...")
