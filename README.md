# gbpp (Game Boy Plus Plus)

A Game Boy emulator for Windows written in C++, using SDL2 for the graphics, input and audio.

(Despite its name, it in no way attempts to be better than other emulators out there, it's just a fun little project.)

## Source code and building

`gbpp.cpp` contains the main application logic and loop, `components/gbcpu.cpp` the CPU (Central Processing Unit) and timer emulation, `components/gbapu.cpp` the APU (Audio Processing Unit) and sound emulation, `components/gbppu.cpp` the PPU (Picture Processing Unit) emulation and `components/gbmem.cpp` the RAM (Random-Access Memory), ROM (Read-Only Memory) and MBC1 (Memory Bank Controller 1) emulation.

Requires `SDL2` to be installed and `SDL2.dll` to be in the same directory as `gbpp.cpp`. It can then be compiled for Windows using MinGW with
```
g++ gbpp.cpp components\*.cpp -I"path/to/SDL-includes" -L"path/to/SDL-libraries" -w -Wl,-subsystem,windows -lmingw32 -lSDL2main -lSDL2 -o gbpp -O2 -std=c++0x
```

## Accuracy

Emulation speed is accurate but some instructions are still incorrectly implemented and there is no sound yet.

It passes some of blargg's instruction test ROMs but the `cpu_instrs` test crashes on test 03. It can run Super Mario Land, BGB's test ROM and some other games (mostly) fine, apart from a few minor graphical bugs and hangs.

The emulated CPU runs most of the time at around 4MiHz on an Intel Core i7-8550U at 1.80GHz.

## User interface

The UI consists of two windows, one for the emulated Game Boy's screen and one for debug. The one for debug can be toggled on and off with **D**, however it is recommended to keep the debug window closed because it renders to whole tilemap and screen 30 times a second, which causes massive slow-downs. The main window's size is 640x576 pixels.

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

The emulator does not do sound yet, though support is on its way (if only SDL audio would work on Windows the way it is supposed to :unamused:).

## Things to add in the future

* Fix the CPU, which has some opcodes slightly broken at the moment
* Window support (essentially a second movable background layer)
* Proper background/window/sprite rendering priority support
* Sound support, though I need to find an API for that (sound generation on Windows is quite bad apparently)
* More debug options, such as an on-the-fly RAM reader/writer

## Assembler and disassembler

Additionally there are two Python 3 scripts in the repository, one for assembling (`assembler.py`) and one for disassembling (`disassembler.py`). They both ask for source and destination files, and the disassembler accepts flags from the command line. Documentation on these two scripts will be added soon.

### Disassembler flags

TODO.
