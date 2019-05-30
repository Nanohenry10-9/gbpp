#include <SDL.h>
#include <SDL_render.h>
#include "gbppu.h"

using namespace std;

#define LCDC 0xFF40
#define STAT 0xFF41
#define SCY  0xFF42
#define SCX  0xFF43
#define LY   0xFF44
#define LYC  0xFF45
#define DMA  0xFF46
#define BGP  0xFF47
#define OBP0 0xFF48
#define OBP1 0xFF49
#define WX   0xFF4A
#define WY   0xFF4B
#define OAM  0xFE00
#define VRAM 0x8000

#define TEST(v, b) ((v & (1 << b)) == (1 << b))

const uint8_t palette[4][3] = {{0xAC, 0xB5, 0x6B}, {0x76, 0x84, 0x48}, {0x3F, 0x50, 0x3F}, {0x24, 0x31, 0x37}};

uint16_t LX;
double frameCount;
uint32_t pixels[160 * 144];
uint8_t buffer[160 * 144];
bool debug;
uint32_t map[8 * 8 * 48 * 8];
SDL_Texture *lastFrame;
uint8_t spritesOnLine[10];
uint8_t spriteIndex;

void gbppu::init(SDL_Texture* screen) {
    LX = 0;
    frameCount = 0;
    debug = 0;
    lastFrame = screen;
    spriteIndex = 0;
}

void gbppu::render(SDL_Texture* t) {
    for (int i = 0; i < 160 * 144; i++) {
        pixels[i] = 0xFF000000 | (palette[buffer[i]][0] << 16) | (palette[buffer[i]][1] << 8) | palette[buffer[i]][2];
    }
    /*if (debug) {
        for (int i = 0; i < 160; i += 8) {
            for (int j = 0; j < 144; j++) {
                uint8_t r = (pixels[i + j * 160] & 0x00FF0000) >> 16;
                uint8_t g = (pixels[i + j * 160] & 0x0000FF00) >> 8;
                uint8_t b = pixels[i + j * 160] & 0x000000FF;
                pixels[i + j * 160] = 0xFF000000 | (uint8_t(r * 0.8) << 16) | (uint8_t(g * 0.8) << 8) | uint8_t(b * 0.8);
            }
        }
        for (int i = 0; i < 144; i += 8) {
            for (int j = 0; j < 160; j++) {
                uint8_t r = (pixels[j + i * 160] & 0x00FF0000) >> 16;
                uint8_t g = (pixels[j + i * 160] & 0x0000FF00) >> 8;
                uint8_t b = pixels[j + i * 160] & 0x000000FF;
                pixels[j + i * 160] = 0xFF000000 | (uint8_t(r * 0.8) << 16) | (uint8_t(g * 0.8) << 8) | uint8_t(b * 0.8);
            }
        }
    }*/
    SDL_UpdateTexture(t, NULL, &pixels, 160 * 4);
}

void gbppu::renderTilemap(SDL_Texture* t, gbmem* mem) {
    for (int y = 0; y < 48; y++) {
        for (int x = 0; x < 8; x++) {
            for (int ty = 0; ty < 8; ty++) {
                for (int tx = 0; tx < 8; tx++) {
                    uint8_t c = getTilePixel(mem, x + y * 8, tx, ty);
                    map[x * 8 + tx + (y * 8 + ty) * 64] = 0xFF000000 | (palette[c][0] << 16) | (palette[c][1] << 8) | palette[c][2];
                }
            }
        }
    }
    /*if (debug) {
        for (int i = 0; i < 128; i += 8) {
            for (int j = 0; j < 128; j++) {
                uint8_t r = (map[i + j * 128] & 0x00FF0000) >> 16;
                uint8_t g = (map[i + j * 128] & 0x0000FF00) >> 8;
                uint8_t b = map[i + j * 128] & 0x000000FF;
                map[i + j * 128] = 0xFF000000 | (uint8_t(r * 0.8) << 16) | (uint8_t(g * 0.8) << 8) | uint8_t(b * 0.8);
            }
        }
        for (int i = 0; i < 128; i += 8) {
            for (int j = 0; j < 128; j++) {
                uint8_t r = (map[j + i * 128] & 0x00FF0000) >> 16;
                uint8_t g = (map[j + i * 128] & 0x0000FF00) >> 8;
                uint8_t b = map[j + i * 128] & 0x000000FF;
                map[j + i * 128] = 0xFF000000 | (uint8_t(r * 0.8) << 16) | (uint8_t(g * 0.8) << 8) | uint8_t(b * 0.8);
            }
        }
    }*/
    SDL_UpdateTexture(t, NULL, &map, 64 * 4);
}

void gbppu::tick(gbmem* mem) {
    scanlineAdvance(mem);
    /*if (LX == 0 && mem->read(LY) < ) {

    }*/
    if (LX >= 80 && LX < 240) {
        uint8_t col = ((LX - 80) + mem->read(SCX)) & 7;
        uint8_t row = (mem->read(LY) + mem->read(SCY)) & 7;
        uint16_t pixelPalette = BGP;
        uint8_t c = getBGMapPixel(mem, ((LX - 80) + mem->read(SCX)) / 8, (mem->read(LY) + mem->read(SCY)) / 8, col, row);
        for (int i = 0; i < 40; i++) {
            uint16_t x = mem->read(OAM + i * 4 + 1) - 8;
            uint16_t y = mem->read(OAM + i * 4) - 16;
            if ((LX - 80) >= x && mem->read(LY) >= y && (LX - 80) < x + 8 && mem->read(LY) < y + 8) {
                uint8_t spriteX = (LX - 80) - x;
                uint8_t spriteY = mem->read(LY) - y;
                uint8_t tileIndex = mem->read(OAM + i * 4 + 2);
                uint8_t attr = mem->read(OAM + i * 4 + 3);
                if (TEST(attr, 7) && c != 0) {
                    continue;
                }
                if (TEST(attr, 5)) {
                    spriteX = 7 - spriteX;
                }
                if (TEST(attr, 6)) {
                    spriteY = 7 - spriteY;
                }
                uint8_t tmp = getSpriteTilePixel(mem, tileIndex, spriteX, spriteY);
                if (tmp != 0) {
                    uint16_t pixelPalette = TEST(attr, 7)? OBP1 : OBP0;
                    c = tmp;
                }
            }
        }
        switch (c) {
            case 0:
                c = (TEST(mem->read(BGP), 1) << 1) | TEST(mem->read(BGP), 0);
                break;
            case 1:
                c = (TEST(mem->read(BGP), 3) << 1) | TEST(mem->read(BGP), 2);
                break;
            case 2:
                c = (TEST(mem->read(BGP), 5) << 1) | TEST(mem->read(BGP), 4);
                break;
            case 3:
                c = (TEST(mem->read(BGP), 7) << 1) | TEST(mem->read(BGP), 6);
                break;
        }
        buffer[(LX - 80) + mem->read(LY) * 160] = c;
    }
}

void gbppu::scanlineAdvance(gbmem* mem) {
    LX++;
    if (mem->read(LY) < 144) {
        if (LX == 0) {
            mem->write(STAT, (mem->read(STAT) & 0xFC) | 0x02);
            if (TEST(mem->read(STAT), 5)) {
                mem->write(0xFF0F, mem->read(0xFF0F) | 0x02);
            }
        } else if (LX == 80) {
            mem->write(STAT, (mem->read(STAT) & 0xFC) | 0x03);
        } else if (LX == 248) {
            mem->write(STAT, mem->read(STAT) & 0xFC);
            if (TEST(mem->read(STAT), 3)) {
                mem->write(0xFF0F, mem->read(0xFF0F) | 0x02);
            }
        }
    } else if (mem->read(LY) == 144) {
        mem->write(0xFF0F, mem->read(0xFF0F) | 0x01);
        mem->write(STAT, (mem->read(STAT) & 0xFC) | 0x01);
        if (TEST(mem->read(STAT), 4)) {
            mem->write(0xFF0F, mem->read(0xFF0F) | 0x02);
        }
    }
    if (LX == 456) {
        LX = 0;
        mem->write(LY, mem->read(LY) + 1);
        if (mem->read(LY) == mem->read(LYC)) {
            mem->write(STAT, mem->read(STAT) | 0x04);
            if (TEST(mem->read(STAT), 6)) {
                mem->write(0xFF0F, mem->read(0xFF0F) | 0x02);
            }
        } else {
            mem->write(STAT, mem->read(STAT) & ~0x04);
        }
        if (mem->read(LY) == 154) {
            mem->write(LY, 0);
            render(lastFrame);
            frameCount++;
        }
    }
}

uint8_t gbppu::getBGMapPixel(gbmem* mem, uint16_t tx, uint16_t ty, uint8_t x, uint8_t y) {
    tx &= 0x1F;
    ty &= 0x1F;
    uint16_t tilemap = getBGMapAddress(mem);
    uint16_t tiles = getBGTileAddress(mem);
    uint8_t tileIndex = mem->read(tilemap + tx + (ty * 32));
    if (tiles == 0x8800) {
        tileIndex += 0x80;
    }
    uint16_t tile = tiles + tileIndex * 16;
    uint8_t data[8][8];
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            data[i][j] = (TEST(mem->read(tile + (i * 2) + 1), 7 - j) << 1) | TEST(mem->read(tile + (i * 2)), 7 - j);
        }
    }
    return data[y][x];
}

uint8_t gbppu::getTilePixel(gbmem* mem, uint16_t ti, uint8_t x, uint8_t y) {
    int tile = 0x8000 + ti * 16;
    uint8_t data[8][8];
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            data[i][j] = (TEST(mem->read(tile + (i * 2) + 1), 7 - j) << 1) | TEST(mem->read(tile + (i * 2)), 7 - j);
        }
    }
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            switch (data[i][j]) {
                case 0:
                    data[i][j] = (TEST(mem->read(BGP), 1) << 1) | TEST(mem->read(BGP), 0);
                    break;
                case 1:
                    data[i][j] = (TEST(mem->read(BGP), 3) << 1) | TEST(mem->read(BGP), 2);
                    break;
                case 2:
                    data[i][j] = (TEST(mem->read(BGP), 5) << 1) | TEST(mem->read(BGP), 4);
                    break;
                case 3:
                    data[i][j] = (TEST(mem->read(BGP), 7) << 1) | TEST(mem->read(BGP), 6);
                    break;
            }
        }
    }
    return data[y][x];
}

uint8_t gbppu::getSpriteTilePixel(gbmem* mem, uint8_t ti, uint8_t x, uint8_t y) {
    int tile = 0x8000 + ti * 16;
    uint8_t data[8][8];
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            data[i][j] = (TEST(mem->read(tile + (i * 2) + 1), 7 - j) << 1) | TEST(mem->read(tile + (i * 2)), 7 - j);
        }
    }
    return data[y][x];
}

uint16_t gbppu::getBGTileAddress(gbmem* mem) {
    if ((mem->read(LCDC) & 0x10) == 0x10) {
        return 0x8000;
    }
    return 0x8800;
}

uint16_t gbppu::getBGMapAddress(gbmem* mem) {
    if ((mem->read(LCDC) & 0x08) == 0x08) {
        return 0x9C00;
    }
    return 0x9800;
}