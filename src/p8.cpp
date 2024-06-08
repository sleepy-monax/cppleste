#include "p8.h"

static P8Call _p8call = NULL;

void P8bind(P8Call func) {
    _p8call = func;
}

void P8music(int track, int fade, int mask) {
    _p8call(P8_MUSIC, track, fade, mask);
}

void P8spr(int sprite, int x, int y, int cols, int rows, bool flipx, bool flipy) {
    _p8call(P8_SPR, sprite, x, y, cols, rows, flipx, flipy);
}

bool P8btn(int b) {
    return _p8call(P8_BTN, b);
}

void P8sfx(int id) {
    _p8call(P8_SFX, id);
}

void P8pal(int a, int b) {
    _p8call(P8_PAL, a, b);
}

void P8pal_reset() {
    _p8call(P8_PAL_RESET);
}

void P8circfill(int x, int y, int r, int c) {
    _p8call(P8_CIRCFILL, x, y, r, c);
}

void P8rectfill(int x, int y, int x2, int y2, int c) {
    _p8call(P8_RECTFILL, x, y, x2, y2, c);
}

void P8print(char const *str, int x, int y, int c) {
    _p8call(P8_PRINT, str, x, y, c);
}

void P8line(int x, int y, int x2, int y2, int c) {
    _p8call(P8_LINE, x, y, x2, y2, c);
}

int P8mget(int x, int y) {
    return _p8call(P8_MGET, x, y);
}

bool P8fget(int t, int f) {
    return _p8call(P8_FGET, t, f);
}

void P8camera(int x, int y) {
    _p8call(P8_CAMERA, x, y);
}

void P8map(int mx, int my, int tx, int ty, int mw, int mh, int mask) {
    _p8call(P8_MAP, mx, my, tx, ty, mw, mh, mask);
}

static unsigned rnd_seed_lo = 0, rnd_seed_hi = 1;

void P8srand(unsigned seed) {
    if (seed == 0) {
        rnd_seed_hi = 0x60009755;
        seed = 0xdeadbeef;
    } else {
        rnd_seed_hi = seed ^ 0xbead29ba;
    }
    for (int i = 0x20; i > 0; i--) {
        rnd_seed_hi = ((rnd_seed_hi << 16) | (rnd_seed_hi >> 16)) + seed;
        seed += rnd_seed_hi;
    }
    rnd_seed_lo = seed;
}

int P8rndint(int max) { // decomp'd pico-8
    if (!max)
        return 0;
    rnd_seed_hi = ((rnd_seed_hi << 16) | (rnd_seed_hi >> 16)) + rnd_seed_lo;
    rnd_seed_lo += rnd_seed_hi;
    return rnd_seed_hi % (unsigned)max;
};

float P8rnd(float max) {
    int n = P8rndint(max * (1 << 16));
    return (float)n / (1 << 16);
}
