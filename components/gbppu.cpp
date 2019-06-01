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

#define CMAP(c, p) ((TEST(pMem->read(p), (c * 2 + 1)) << 1) | TEST(pMem->read(p), (c * 2)))

#define TEST(v, b) ((v & (1 << b)) == (1 << b))

const uint8_t palette[4][3] = {{0xAC, 0xB5, 0x6B}, {0x76, 0x84, 0x48}, {0x3F, 0x50, 0x3F}, {0x24, 0x31, 0x37}};

uint16_t LX;
double frameCount;
uint32_t pixels[160 * 144];
uint8_t buffer[160 * 144];
bool debug;
uint32_t map[8 * 8 * 48 * 8];
uint32_t bgMap[32 * 8 * 32 * 8];
SDL_Texture *lastFrame, *tm, *bg;
gbmem *pMem;
uint8_t spritesOnLine[10];
uint8_t spriteIndex;

void gbppu::init(gbmem* cMem, SDL_Texture* screen, SDL_Texture* e, SDL_Texture* t) {
    pMem = cMem;
    LX = 0;
    frameCount = 0;
    debug = 0;
    lastFrame = screen;
    spriteIndex = 0;
    tm = e;
    bg = t;
}

void gbppu::render() {
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
    SDL_UpdateTexture(lastFrame, NULL, &pixels, 160 * 4);
}

void gbppu::renderTilemap() {
    for (int y = 0; y < 48; y++) {
        for (int x = 0; x < 8; x++) {
            for (int ty = 0; ty < 8; ty++) {
                for (int tx = 0; tx < 8; tx++) {
                    uint8_t c = getTilePixel(x + y * 8, tx, ty);
                    map[x * 8 + tx + (y * 8 + ty) * 8 * 8] = 0xFF000000 | (palette[c][0] << 16) | (palette[c][1] << 8) | palette[c][2];
                }
            }
        }
    }
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 32; x++) {
            for (int ty = 0; ty < 8; ty++) {
                for (int tx = 0; tx < 8; tx++) {
                    uint8_t c = CMAP(getBGMapPixel(x, y, tx, ty), BGP);
                    bgMap[x * 8 + tx + (y * 8 + ty) * 32 * 8] = 0xFF000000 | (palette[c][0] << 16) | (palette[c][1] << 8) | palette[c][2];
                }
            }
        }
    }
    uint8_t scx = pMem->read(SCX);
    uint8_t scy = pMem->read(SCY);
    for (uint8_t x = scx; x != ((scx + 160) & 0xFF); x += 2) {
        uint8_t r = ((bgMap[x + scy * 32 * 8] & 0x00FF0000) >> 16) >> 2;
        uint8_t g = ((bgMap[x + scy * 32 * 8] & 0x0000FF00) >> 8) >> 2;
        uint8_t b = (bgMap[x + scy * 32 * 8] & 0x000000FF) >> 2;
        bgMap[x + scy * 32 * 8] = 0xFF000000 | (r << 16) || (g << 8) || b;
    }
    for (uint8_t y = scy + 2; y != ((scy + 144) & 0xFF); y += 2) {
        uint8_t r = ((bgMap[scx + y * 32 * 8] & 0x00FF0000) >> 16) >> 2;
        uint8_t g = ((bgMap[scx + y * 32 * 8] & 0x0000FF00) >> 8) >> 2;
        uint8_t b = (bgMap[scx + y * 32 * 8] & 0x000000FF) >> 2;
        bgMap[scx + y * 32 * 8] = 0xFF000000 | (r << 16) || (g << 8) || b;
    }
    for (uint8_t x = scx; x != ((scx + 160) & 0xFF); x += 2) {
        uint8_t r = ((bgMap[x + ((scy + 144) & 0xFF) * 32 * 8] & 0x00FF0000) >> 16) >> 2;
        uint8_t g = ((bgMap[x + ((scy + 144) & 0xFF) * 32 * 8] & 0x0000FF00) >> 8) >> 2;
        uint8_t b = (bgMap[x + ((scy + 144) & 0xFF) * 32 * 8] & 0x000000FF) >> 2;
        bgMap[x + ((scy + 144) & 0xFF) * 32 * 8] = 0xFF000000 | (r << 16) || (g << 8) || b;
    }
    for (uint8_t y = scy; y != ((scy + 146) & 0xFF); y += 2) {
        uint8_t r = ((bgMap[((scx + 160) & 0xFF) + y * 32 * 8] & 0x00FF0000) >> 16) >> 2;
        uint8_t g = ((bgMap[((scx + 160) & 0xFF) + y * 32 * 8] & 0x0000FF00) >> 8) >> 2;
        uint8_t b = (bgMap[((scx + 160) & 0xFF) + y * 32 * 8] & 0x000000FF) >> 2;
        bgMap[((scx + 160) & 0xFF) + y * 32 * 8] = 0xFF000000 | (r << 16) || (g << 8) || b;
    }
    SDL_UpdateTexture(tm, NULL, &map, 8 * 8 * 4);
    SDL_UpdateTexture(bg, NULL, &bgMap, 32 * 8 * 4);
}

void gbppu::tick() {
    scanlineAdvance();
    if (LX == 0 && (pMem->read(STAT) & 0x03) == 0x02) {
        spriteIndex = 0;
        for (int si = 0; si < 40; si++) {
            uint8_t y = pMem->read(OAM + si * 4) - 16;
            if (pMem->read(LY) >= y && pMem->read(LY) < y + 8) {
                spritesOnLine[spriteIndex++] = si;
            }
        }
    }
    if ((pMem->read(STAT) & 0x03) == 0x03) {
        uint8_t c = 0;
        uint16_t pixelPalette = BGP;
        /*if (TEST(pMem->read(LCDC), 5) && (LX - 80) >= pMem->read(WX) - 7 && pMem->read(LY) >= pMem->read(WY)) {
            c = 3;
        } else {*/
        uint8_t col = ((LX - 80) + pMem->read(SCX)) & 7;
        uint8_t row = (pMem->read(LY) + pMem->read(SCY)) & 7;
        c = getBGMapPixel(((LX - 80) + pMem->read(SCX)) / 8, (pMem->read(LY) + pMem->read(SCY)) / 8, col, row);
        //}
        for (int i = 0; i < spriteIndex; i++) {
            int16_t x = pMem->read(OAM + spritesOnLine[i] * 4 + 1) - 8;
            int16_t y = pMem->read(OAM + spritesOnLine[i] * 4) - 16;
            if ((LX - 80) >= x && pMem->read(LY) >= y && (LX - 80) < x + 8 && pMem->read(LY) < y + 8) {
                uint8_t spriteX = (LX - 80) - x;
                uint8_t spriteY = pMem->read(LY) - y;
                uint8_t tileIndex = pMem->read(OAM + spritesOnLine[i] * 4 + 2);
                uint8_t attr = pMem->read(OAM + spritesOnLine[i] * 4 + 3);
                if (TEST(attr, 7) && c != 0) {
                    continue;
                }
                if (TEST(attr, 5)) {
                    spriteX = 7 - spriteX;
                }
                if (TEST(attr, 6)) {
                    spriteY = 7 - spriteY;
                }
                uint8_t tmp = getSpriteTilePixel(tileIndex, spriteX, spriteY);
                if (tmp != 0) {
                    uint16_t pixelPalette = TEST(attr, 7)? OBP1 : OBP0;
                    c = tmp;
                }
            }
        }
        buffer[(LX - 80) + pMem->read(LY) * 160] = CMAP(c, pixelPalette);
    }
}

void gbppu::scanlineAdvance() {
    LX++;
    if (pMem->read(LY) < 144) {
        if (LX == 80) {
            pMem->write(STAT, (pMem->read(STAT) & 0xFC) | 0x03);
        } else if (LX == 248) {
            pMem->write(STAT, pMem->read(STAT) & 0xFC);
            if (TEST(pMem->read(STAT), 3)) {
                pMem->write(0xFF0F, pMem->read(0xFF0F) | 0x02);
            }
        }
    } else if (pMem->read(LY) == 144) {
        pMem->write(0xFF0F, pMem->read(0xFF0F) | 0x01);
        pMem->write(STAT, (pMem->read(STAT) & 0xFC) | 0x01);
        if (TEST(pMem->read(STAT), 4)) {
            pMem->write(0xFF0F, pMem->read(0xFF0F) | 0x02);
        }
    }
    if (LX == 456) {
        LX = 0;
        pMem->write(STAT, (pMem->read(STAT) & 0xFC) | 0x02);
        if (TEST(pMem->read(STAT), 5)) {
            pMem->write(0xFF0F, pMem->read(0xFF0F) | 0x02);
        }
        pMem->write(LY, pMem->read(LY) + 1);
        if (pMem->read(LY) == pMem->read(LYC)) {
            pMem->write(STAT, pMem->read(STAT) | 0x04);
            if (TEST(pMem->read(STAT), 6)) {
                pMem->write(0xFF0F, pMem->read(0xFF0F) | 0x02);
            }
        } else {
            pMem->write(STAT, pMem->read(STAT) & ~0x04);
        }
        if (pMem->read(LY) == 154) {
            pMem->write(LY, 0);
            render();
            frameCount++;
        }
    }
}

uint8_t gbppu::getBGMapPixel(uint16_t tx, uint16_t ty, uint8_t x, uint8_t y) {
    tx &= 0x1F;
    ty &= 0x1F;
    uint16_t tilemap = getBGMapAddress();
    uint16_t tiles = getBGTileAddress();
    uint8_t tileIndex = pMem->read(tilemap + tx + (ty * 32));
    if (tiles == 0x8800) {
        tileIndex += 0x80;
    }
    uint16_t tile = tiles + tileIndex * 16;
    uint8_t data[8][8];
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            data[i][j] = (TEST(pMem->read(tile + (i * 2) + 1), 7 - j) << 1) | TEST(pMem->read(tile + (i * 2)), 7 - j);
        }
    }
    return data[y][x];
}

uint8_t gbppu::getTilePixel(uint16_t ti, uint8_t x, uint8_t y) {
    int tile = 0x8000 + ti * 16;
    uint8_t data[8][8];
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            data[i][j] = (TEST(pMem->read(tile + (i * 2) + 1), 7 - j) << 1) | TEST(pMem->read(tile + (i * 2)), 7 - j);
        }
    }
    return CMAP(data[y][x], BGP);
}

uint8_t gbppu::getSpriteTilePixel(uint8_t ti, uint8_t x, uint8_t y) {
    int tile = 0x8000 + ti * 16;
    uint8_t data[8][8];
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            data[i][j] = (TEST(pMem->read(tile + (i * 2) + 1), 7 - j) << 1) | TEST(pMem->read(tile + (i * 2)), 7 - j);
        }
    }
    return data[y][x];
}

uint16_t gbppu::getBGTileAddress() {
    if ((pMem->read(LCDC) & 0x10) == 0x10) {
        return 0x8000;
    }
    return 0x8800;
}

uint16_t gbppu::getBGMapAddress() {
    if ((pMem->read(LCDC) & 0x08) == 0x08) {
        return 0x9C00;
    }
    return 0x9800;
}