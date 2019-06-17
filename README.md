# gbpp (Game Boy Plus Plus)

A DMG Game Boy emulator for Windows written in C++, using SDL2 for the graphics, input and audio.

(Despite its name, it in no way attempts to be better than other emulators out there, it's just a fun little project.)

## Source code and building

`gbpp.cpp` contains the main application logic and loop, `components/gbcpu.cpp` the CPU (Central Processing Unit) and timer emulation, `components/gbapu.cpp` the APU (Audio Processing Unit) and sound emulation, `components/gbppu.cpp` the PPU (Picture Processing Unit) emulation and `components/gbmem.cpp` the RAM (Random-Access Memory), ROM (Read-Only Memory) and MBC1 (Memory Bank Controller 1) emulation.

Requires `SDL2` to be installed and `SDL2.dll` to be in the same directory as `gbpp.cpp`. It can then be compiled for Windows using MinGW with
```
g++ gbpp.cpp components\*.cpp -I"path/to/includes" -L"path/to/libraries" -lmingw32 -lSDL2main -lSDL2 -o gbpp -O3 -W -Wall -Wno-sign-compare -std=c++11
```

gbpp can be run from the command line, and it accepts flags. The flag `--no-sound` will disable sound and `--rom [path]` will load the emulated ROM with the file at `[path]`. All other flags/options will be ignored.

## Accuracy

Emulation speed is accurate but some instructions are still incorrectly implemented and audio is a bit here-and-there.

It passes some of blargg's instruction test ROMs but the `cpu_instrs` test crashes on test 03. It can run Super Mario Land, _Is that a demo in your pocket?_ and some other games or demos (mostly) fine, apart from a few bugs.

The emulated CPU runs most of the time at around 4MiHz on an Intel Core i7-8550U at 1.80GHz.

## User interface

gbpp's UI consists of two windows, one for the emulated Game Boy's screen and one for debug. The main window's size is 480x432 pixels, which is three times the resolution of the Game Boy's screen.

In the debug window, gbpp renders the whole background (32x32 8x8 tiles, palette mapped) and the tilemap from addresses `8000h` to `9800h`. It also shows value for each register, the last interrupt that was serviced and the current selected ROM bank.

## Controls

The controls are mapped to the following keys:

| Keyboard key    | Game Boy button |
| --------------- | --------------- |
| A               | A               |
| S               | B               |
| Arrow keys      | D-pad           |
| Return          | Start           |
| Backspace       | Select          |

In addition, **R** resets the emulator to boot state (`PC = 0` with the first ROM bank selected) and **D** toggles the debug window.

Right now these keys are hardcoded and cannot be configured (except by manually editing the source code).

## Audio

While the emulator does output sound, it is not very accurate. There are occasional pops and clicks and it does not support noise frequency control or pulse 1's frequency sweep. However, Super Mario Land's and Tetris' soundtracks play mostly fine apart from shorter note durations.

## Things to add in the future

* Fix the CPU, which has some opcodes slightly broken at the moment
* Fix & finish the APU, as the sound sometimes pops and clicks
* Window support (essentially a second movable background layer)
* Proper background/window/sprite rendering priority support
* More debug options, such as an on-the-fly RAM reader/writer

## Assembler and disassembler

Additionally there are two Python 3 scripts in the repository, one for assembling (`assembler.py`) and one for disassembling (`disassembler.py`). They both ask for source and destination files, and the disassembler accepts flags from the command line. 

### Disassembler flags

The disassembler can be told to disassemble instructions from addresses it would not diasassemble from otherwise. This can be done with the `-l [address]` flag, where `[address]` is the address to disassemble from.

If there are instructions in the ROM that are loaded elsewhere but have absolute addresses (such as in blargg's tests), the disassembler can be instructed to "rebase" those addresses with `-b [start address] [end address] [base address]`, where the instructions to be rebased are from `[start address]` to `[end address]` (exclusive) and the new base address is `[base address]`. For example, if the ROM has instructions from `C000h` to `D000h` and they are meant to use addresses from `4000h` to `5000h`, you would pass the disassembler `-b C000h D000h 4000h`.
