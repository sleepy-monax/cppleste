#pragma once

#include <stddef.h>

typedef enum {
    CELESTE_P8_MUSIC,
    CELESTE_P8_SPR,
    CELESTE_P8_BTN,
    CELESTE_P8_SFX,
    CELESTE_P8_PAL,
    CELESTE_P8_PAL_RESET,
    CELESTE_P8_CIRCFILL,
    CELESTE_P8_PRINT,
    CELESTE_P8_RECTFILL,
    CELESTE_P8_LINE,
    CELESTE_P8_MGET,
    CELESTE_P8_CAMERA,
    CELESTE_P8_FGET,
    CELESTE_P8_MAP
} CELESTE_P8_CALLBACK_TYPE;

typedef int (*Celeste_P8_cb_func_t)(CELESTE_P8_CALLBACK_TYPE calltype, ...);

extern void Celeste_P8_set_call_func(Celeste_P8_cb_func_t func);

extern void Celeste_P8_set_rndseed(unsigned seed);

extern void Celeste_P8_init(void);

extern void Celeste_P8_update(void);

extern void Celeste_P8_draw(void);

extern void Celeste_P8__DEBUG(void);

size_t Celeste_P8_get_state_size(void);

void Celeste_P8_save_state(void *st);

void Celeste_P8_load_state(void const *st);
