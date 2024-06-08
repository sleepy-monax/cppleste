#pragma once

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    P8_MUSIC,
    P8_SPR,
    P8_BTN,
    P8_SFX,
    P8_PAL,
    P8_PAL_RESET,
    P8_CIRCFILL,
    P8_PRINT,
    P8_RECTFILL,
    P8_LINE,
    P8_MGET,
    P8_CAMERA,
    P8_FGET,
    P8_MAP
} P8;

typedef int (*P8Call)(P8 calltype, ...);

void P8bind(P8Call func);

void P8music(int track, int fade, int mask);

void P8spr(int sprite, int x, int y, int cols, int rows, bool flipx, bool flipy);

bool P8btn(int b);

void P8sfx(int id);

void P8pal(int a, int b);

void P8pal_reset();

void P8circfill(int x, int y, int r, int c);

void P8rectfill(int x, int y, int x2, int y2, int c);

void P8print(char const *str, int x, int y, int c);

void P8line(int x, int y, int x2, int y2, int c);

int P8mget(int x, int y);

bool P8fget(int t, int f);

void P8camera(int x, int y);

void P8map(int mx, int my, int tx, int ty, int mw, int mh, int mask);

// MARK: Random ----------------------------------------------------------------

void P8srand(unsigned seed);

int P8rndint(int max);

float P8rnd(float max);

// MARK: Math ------------------------------------------------------------------

static inline float P8modulo(float a, float b) {
    return fmodf(fmodf(a, b) + b, b);
}

#define P8max fmaxf

#define P8min fminf

#define P8abs fabsf

#define P8flr floorf

#define P8sin(x) -sinf(x * 6.2831853071796f)

#define P8cos(x) (-P8sin((x) + 0.25f)) // cos(x) = sin(x+pi/2)
