# gbpp (Game Boy Plus Plus)

A Game Boy emulator written in C++, using SDL2 for the graphics, controls and audio.

(Despite its name, it in no way attempts to be better than other emulators out there, it's just a fun project.)

## Source code and building

`gbpp.cpp` contains the main application logic and loop, `components/gbcpu.cpp` the CPU and timer emulation, `components/gbapu.cpp` the APU and sound emulation, `components/gbppu.cpp` the PPU emulation and `components/gbmem.cpp` the RAM, ROM and MBC1 emulation.

Requires `SDL2` to be installed and `SDL2.dll` to be in the same directory as `gbpp.cpp`. It can then be compiled for Windows using MinGW with
```
g++ gbpp.cpp components\*.cpp -I"path/to/SDL-includes" -L"path/to/SDL-libraries" -w -Wl,-subsystem,windows -lmingw32 -lSDL2main -lSDL2 -o gbpp -O2 -std=c++0x
```

## Accuracy

Well, far from perfect.

It passes some of blargg's instruction test ROMs but the `cpu_instrs` test crashes on test 02. It can run Super Mario Land, BGB's test ROM and some other games (mostly) fine, apart from a few minor graphical bugs and oddities.

It runs most of the time at around 70% - 100% of full speed on an Intel Core i7-8550U at 1.80GHz, which is not ideal but fine for now.

## User interface

The UI has currently the emulated Game Boy's screen on the left and the whole background, window and sprite tile data on the right. The window is 1172x576 pixels, so if that does not fit your screen, please edit the window size in `gbpp.cpp` on line 35.

## Controls

The controls are mapped to the following keys:

| Keyboard key    | Game Boy button |
| --------------- | ----------------|
| A               | A               |
| S               | B               |
| Arrow keys      | D-pad           |
| Return          | Start           |
| Backspace       | Select          |

In addition, **R** resets the emulator to boot state.

Right now these are hardcoded and cannot be configured (except by manually editing the source code).

## Audio

The emulator does not do sound yet, though support is on its way (if only SDL audio would co-operate).

## Assembler and disassembler

Additionally there are two Python 3 scripts in the repository, one for assembling (`assembler.py`) and one for disassembling (`disassembler.py`). They both ask for source and destination files, and the disassembler accepts flags from the command line. Documentation on these two scripts will be provided soon.
