#include <SDL.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <SDL_mixer.h>

#include "celeste.h"
#include "p8.h"
#include "sdl20compat.inc.c"

static void ErrLog(char *fmt, ...) {
    FILE *f = stderr;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);

    if (f != stderr && f != stdout)
        fclose(f);
}

SDL_Surface *screen = NULL;
SDL_Surface *gfx = NULL;
SDL_Surface *font = NULL;
Mix_Chunk *snd[64] = {NULL};
Mix_Music *mus[6] = {NULL};

#define PICO8_W 128
#define PICO8_H 128

static int scale = 4;
static SDL_Color const base_palette[16] = {
    {0x00, 0x00, 0x00}, {0x1d, 0x2b, 0x53}, {0x7e, 0x25, 0x53}, {0x00, 0x87, 0x51}, {0xab, 0x52, 0x36}, {0x5f, 0x57, 0x4f}, {0xc2, 0xc3, 0xc7}, {0xff, 0xf1, 0xe8}, {0xff, 0x00, 0x4d}, {0xff, 0xa3, 0x00}, {0xff, 0xec, 0x27}, {0x00, 0xe4, 0x36}, {0x29, 0xad, 0xff}, {0x83, 0x76, 0x9c}, {0xff, 0x77, 0xa8}, {0xff, 0xcc, 0xaa}
};
static SDL_Color palette[16];

static inline Uint32 getcolor(char idx) {
    SDL_Color c = palette[idx % 16];
    return SDL_MapRGB(screen->format, c.r, c.g, c.b);
}

static void ResetPalette(void) {
    memcpy(palette, base_palette, sizeof palette);
}

static char *GetDataPath(char *path, int n, char const *fname) {
    snprintf(path, n, "data/%s", fname);
    return path;
}

static Uint32 getpixel(SDL_Surface *surface, int x, int y) {
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp) {
    case 1:
        return *p;

    case 2:
        return *(Uint16 *)p;

    case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;

    case 4:
        return *(Uint32 *)p;

    default:
        return 0; /* shouldn't happen, but avoids warnings */
    }
}

void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel) {
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp) {
    case 1:
        *p = pixel;
        break;

    case 2:
        *(Uint16 *)p = pixel;
        break;

    case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
        } else {
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
        }
        break;

    case 4:
        *(Uint32 *)p = pixel;
        break;
    }
}

static void loadbmpscale(char *filename, SDL_Surface **s) {
    SDL_Surface *surf = *s;
    if (surf)
        SDL_FreeSurface(surf), surf = *s = NULL;

    char tmpath[4096];
    SDL_Surface *bmp = SDL_LoadBMP(GetDataPath(tmpath, sizeof tmpath, filename));
    if (!bmp) {
        ErrLog("error loading bmp '%s': %s\n", filename, SDL_GetError());
        return;
    }

    int w = bmp->w, h = bmp->h;

    surf =
        SDL_CreateRGBSurface(SDL_SWSURFACE, w * scale, h * scale, 8, 0, 0, 0, 0);
    assert(surf != NULL);
    unsigned char *data = (unsigned char *)surf->pixels;
    /*memcpy((_S)->format->palette->colors, base_palette, 16*sizeof(SDL_Color));*/
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            unsigned char pix = getpixel(bmp, x, y);
            for (int i = 0; i < scale; i++)
                for (int j = 0; j < scale; j++) {
                    data[x * scale + i + (y * scale + j) * w * scale] = pix;
                }
        }
    SDL_FreeSurface(bmp);
    SDL_SetColorKey(surf, SDL_SRCCOLORKEY, 0);
    // SDL_SaveBMP(_S, #_S "x.bmp");
    *s = surf;
}

#define LOGLOAD(w) printf("loading %s...", w)
#define LOGDONE() printf("done\n")

static void LoadData(void) {
    LOGLOAD("gfx.bmp");
    loadbmpscale("gfx.bmp", &gfx);
    LOGDONE();

    LOGLOAD("font.bmp");
    loadbmpscale("font.bmp", &font);
    LOGDONE();

    static char const sndids[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 13, 14, 15, 16, 23, 35, 37, 38, 40, 50, 51, 54, 55};
    for (int iid = 0; iid < sizeof sndids; iid++) {
        int id = sndids[iid];
        char fname[20];
        sprintf(fname, "snd%i.wav", id);
        char path[4096];
        LOGLOAD(fname);
        GetDataPath(path, sizeof path, fname);
        snd[id] = Mix_LoadWAV(path);
        if (!snd[id]) {
            ErrLog("snd%i: Mix_LoadWAV: %s\n", id, Mix_GetError());
        }
        LOGDONE();
    }
    static char const musids[] = {0, 10, 20, 30, 40};
    for (int iid = 0; iid < sizeof musids; iid++) {
        int id = musids[iid];
        char fname[20];
        sprintf(fname, "mus%i.ogg", id);
        LOGLOAD(fname);
        char path[4096];
        GetDataPath(path, sizeof path, fname);
        mus[id / 10] = Mix_LoadMUS(path);
        if (!mus[id / 10]) {
            ErrLog("mus%i: Mix_LoadMUS: %s\n", id, Mix_GetError());
        }
        LOGDONE();
    }
}
#include "../data/map.inc"

static Uint16 buttons_state = 0;

#define SDL_CHECK(r)                                                                            \
    do {                                                                                        \
        if (!(r)) {                                                                             \
            ErrLog("%s:%i, fatal error: `%s` -> %s\n", __FILE__, __LINE__, #r, SDL_GetError()); \
            exit(2);                                                                            \
        }                                                                                       \
    } while (0)

static void p8_rectfill(int x0, int y0, int x1, int y1, int col);
static void p8_print(char const *str, int x, int y, int col);

// on-screen display (for info, such as loading a state, toggling screenshake,
// toggling fullscreen, etc)
static char osd_text[200] = "";
static int osd_timer = 0;
static void OSDset(char const *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(osd_text, sizeof osd_text, fmt, ap);
    osd_text[sizeof osd_text - 1] =
        '\0'; // make sure to add NUL terminator in case of truncation
    printf("%s\n", osd_text);
    osd_timer = 30;
    va_end(ap);
}
static void OSDdraw(void) {
    if (osd_timer > 0) {
        --osd_timer;
        int const x = 4;
        int const y =
            120 + (osd_timer < 10 ? 10 - osd_timer
                                  : 0);                                // disappear by going below the screen
        p8_rectfill(x - 2, y - 2, x + 4 * strlen(osd_text), y + 6, 6); // outline
        p8_rectfill(x - 1, y - 1, x + 4 * strlen(osd_text) - 1, y + 5, 0);
        p8_print(osd_text, x, y, 7);
    }
}

static Mix_Music *current_music = NULL;
static bool enable_screenshake = 1;
static bool paused = 0;

static bool running = 1;
static void *initial_game_state = NULL;
static void *game_state = NULL;
static Mix_Music *game_state_music = NULL;
static void mainLoop(void);

int main(int argc, char **argv) {
    SDL_CHECK(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) == 0);
    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
    SDL_GameControllerAddMappingsFromRW(
        SDL_RWFromFile("gamecontrollerdb.txt", "rb"), 1
    );
    int videoflag = SDL_SWSURFACE | SDL_HWPALETTE;
    SDL_CHECK(screen = SDL_SetVideoMode(PICO8_W * scale, PICO8_H * scale, 32, videoflag));
    SDL_WM_SetCaption("Celeste", NULL);
    int mixflag = MIX_INIT_OGG;
    if (Mix_Init(mixflag) != mixflag) {
        ErrLog("Mix_Init: %s\n", Mix_GetError());
    }
    if (Mix_OpenAudio(22050, AUDIO_S16SYS, 1, 1024) < 0) {
        ErrLog("Mix_Init: %s\n", Mix_GetError());
    }
    ResetPalette();
    SDL_ShowCursor(0);

    printf("now loading...\n");

    {
        unsigned char const loading_bmp[] = {
            0x42, 0x4d, 0xca, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x82, 0x00,
            0x00, 0x00, 0x6c, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x09, 0x00,
            0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00,
            0x00, 0x00, 0x23, 0x2e, 0x00, 0x00, 0x23, 0x2e, 0x00, 0x00, 0x02, 0x00,
            0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x42, 0x47, 0x52, 0x73, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
            0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
            0x00, 0x00, 0x66, 0x3e, 0xf1, 0x24, 0xf0, 0x00, 0x00, 0x00, 0x49, 0x44,
            0x92, 0x24, 0x90, 0x00, 0x00, 0x00, 0x49, 0x3c, 0x92, 0x24, 0x90, 0x00,
            0x00, 0x00, 0x49, 0x04, 0x92, 0x24, 0x90, 0x00, 0x00, 0x00, 0x46, 0x38,
            0xf0, 0x3c, 0xf0, 0x00, 0x00, 0x00, 0x40, 0x00, 0x12, 0x00, 0x00, 0x00,
            0x00, 0x00, 0xc0, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00
        };
        unsigned int const loading_bmp_len = 202;
        SDL_RWops *rw = SDL_RWFromConstMem(loading_bmp, loading_bmp_len);
        SDL_Surface *loading = SDL_LoadBMP_RW(rw, 1);
        if (!loading)
            goto skip_load;

        SDL_Rect rc = {60, 60};
        SDL_BlitSurface(loading, NULL, screen, &rc);

        SDL_Flip(screen);
        SDL_FreeSurface(loading);
    }
skip_load:

    LoadData();

    int pico8emu(P8 call, ...);
    P8bind(pico8emu);

    // for reset
    P8srand((unsigned)(time(NULL) + SDL_GetTicks()));

    Celeste_P8_init();

    printf("ready\n");

    while (running)
        mainLoop();

    if (game_state)
        SDL_free(game_state);
    if (initial_game_state)
        SDL_free(initial_game_state);

    SDL_FreeSurface(gfx);
    SDL_FreeSurface(font);
    for (int i = 0; i < (sizeof snd) / (sizeof *snd); i++) {
        if (snd[i])
            Mix_FreeChunk(snd[i]);
    }
    for (int i = 0; i < (sizeof mus) / (sizeof *mus); i++) {
        if (mus[i])
            Mix_FreeMusic(mus[i]);
    }

    Mix_CloseAudio();
    Mix_Quit();
    SDL_Quit();
    return 0;
}

/* These inputs aren't sent to the game. */
enum {
    PSEUDO_BTN_SAVE_STATE = 6,
    PSEUDO_BTN_LOAD_STATE = 7,
    PSEUDO_BTN_EXIT = 8,
    PSEUDO_BTN_PAUSE = 9,
};

static Uint16 const stick_deadzone = 32767 / 2; // about half

static void mainLoop(void) {
    Uint8 const *kbstate = SDL_GetKeyboardState(NULL);

    static int reset_input_timer = 0;
    // hold F9 (select+start+y) to reset
    if (initial_game_state != NULL && kbstate[SDL_SCANCODE_F9]) {
        reset_input_timer++;
        if (reset_input_timer >= 30) {
            reset_input_timer = 0;
            // reset
            OSDset("reset");
            paused = 0;
            Mix_HaltChannel(-1);
            Mix_HaltMusic();
            Celeste_P8_init();
        }
    } else
        reset_input_timer = 0;

    Uint16 prev_buttons_state = buttons_state;
    buttons_state = 0;

    SDL_GameControllerUpdate();

    if (!((prev_buttons_state >> PSEUDO_BTN_PAUSE) & 1) &&
        (buttons_state >> PSEUDO_BTN_PAUSE) & 1) {
        goto toggle_pause;
    }

    if (!((prev_buttons_state >> PSEUDO_BTN_EXIT) & 1) &&
        (buttons_state >> PSEUDO_BTN_EXIT) & 1) {
        goto press_exit;
    }

    SDL_Event ev;
    while (SDL_PollEvent(&ev))
        switch (ev.type) {
        case SDL_QUIT:
            running = 0;
            break;

        case SDL_KEYDOWN: {
            if (ev.key.repeat)
                break;                                           // no key repeat
            if (ev.key.keysym.scancode == SDL_SCANCODE_ESCAPE) { // do pause
            toggle_pause:
                if (paused)
                    Mix_Resume(-1), Mix_ResumeMusic();
                else
                    Mix_Pause(-1), Mix_PauseMusic();
                paused = !paused;
                break;
            } else if (ev.key.keysym.scancode == SDL_SCANCODE_DELETE) { // exit
            press_exit:
                running = 0;
                break;
            } else if (ev.key.keysym.scancode == SDL_SCANCODE_F11 &&
                       !(kbstate[SDL_SCANCODE_LSHIFT] || kbstate[SDL_SCANCODE_ESCAPE])) {
                screen = SDL_GetVideoSurface();
                break;
            } else if (ev.key.keysym.scancode == SDL_SCANCODE_5) {
                Celeste_P8__DEBUG();
                break;
            } else if ( // toggle screenshake (e / L+R)
                ev.key.keysym.scancode == SDL_SCANCODE_E
            ) {
                enable_screenshake = !enable_screenshake;
                OSDset("screenshake: %s", enable_screenshake ? "on" : "off");
            }
            break;
        }
        }

    if (kbstate[SDL_SCANCODE_LEFT])
        buttons_state |= (1 << 0);
    if (kbstate[SDL_SCANCODE_RIGHT])
        buttons_state |= (1 << 1);
    if (kbstate[SDL_SCANCODE_UP])
        buttons_state |= (1 << 2);
    if (kbstate[SDL_SCANCODE_DOWN])
        buttons_state |= (1 << 3);
    if (kbstate[SDL_SCANCODE_Z] || kbstate[SDL_SCANCODE_C] || kbstate[SDL_SCANCODE_N] ||
        kbstate[SDL_SCANCODE_A])
        buttons_state |= (1 << 4);
    if (kbstate[SDL_SCANCODE_X] || kbstate[SDL_SCANCODE_V] || kbstate[SDL_SCANCODE_M] ||
        kbstate[SDL_SCANCODE_B])
        buttons_state |= (1 << 5);

    if (paused) {
        int const x0 = PICO8_W / 2 - 3 * 4, y0 = 8;

        p8_rectfill(x0 - 1, y0 - 1, 6 * 4 + x0 + 1, 6 + y0 + 1, 6);
        p8_rectfill(x0, y0, 6 * 4 + x0, 6 + y0, 0);
        p8_print("paused", x0 + 1, y0 + 1, 7);
    } else {
        Celeste_P8_update();
        Celeste_P8_draw();
    }
    OSDdraw();

    /*for (int i = 0 ; i < 16;i++) {
            SDL_Rect rc = {i*8*scale, 0, 8*scale, 4*scale};
            SDL_FillRect(screen, &rc, i);
    }*/

    SDL_Flip(screen);

    static int t = 0;
    static unsigned frame_start = 0;
    unsigned frame_end = SDL_GetTicks();
    unsigned frame_time = frame_end - frame_start;
    unsigned target_millis;
    // frame timing for 30fps is 33.333... ms, but we only have integer
    // granularity so alternate between 33 and 34 ms, like [33,33,34,33,33,34,...]
    // which averages out to 33.333...
    if (t < 2)
        target_millis = 33;
    else
        target_millis = 34;

    if (++t == 3)
        t = 0;

    if (frame_time < target_millis) {
        SDL_Delay(target_millis - frame_time);
    }
    frame_start = SDL_GetTicks();
}

static int gettileflag(int, int);
static void p8_line(int, int, int, int, unsigned char);

// lots of code from
// https://github.com/SDL-mirror/SDL/blob/bc59d0d4a2c814900a506d097a381077b9310509/src/video/SDL_surface.c#L625
// coordinates should be scaled already
static inline void Xblit(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect, int color, int flipx, int flipy) {
    assert(src && dst && !src->locked && !dst->locked);
    assert(dst->format->BitsPerPixel == 32 && src->format->BitsPerPixel == 8);
    SDL_Rect fulldst;
    /* If the destination rectangle is NULL, use the entire dest surface */
    if (!dstrect)
        dstrect = (fulldst = (SDL_Rect){0, 0, dst->w, dst->h}, &fulldst);

    int srcx, srcy, w, h;

    /* clip the source rectangle to the source surface */
    if (srcrect) {
        int maxw, maxh;

        srcx = srcrect->x;
        w = srcrect->w;
        if (srcx < 0) {
            w += srcx;
            dstrect->x -= srcx;
            srcx = 0;
        }
        maxw = src->w - srcx;
        if (maxw < w)
            w = maxw;

        srcy = srcrect->y;
        h = srcrect->h;
        if (srcy < 0) {
            h += srcy;
            dstrect->y -= srcy;
            srcy = 0;
        }
        maxh = src->h - srcy;
        if (maxh < h)
            h = maxh;

    } else {
        srcx = srcy = 0;
        w = src->w;
        h = src->h;
    }

    /* clip the destination rectangle against the clip rectangle */
    {
        SDL_Rect *clip = &dst->clip_rect;
        int dx, dy;

        dx = clip->x - dstrect->x;
        if (dx > 0) {
            w -= dx;
            dstrect->x += dx;
            srcx += dx;
        }
        dx = dstrect->x + w - clip->x - clip->w;
        if (dx > 0)
            w -= dx;

        dy = clip->y - dstrect->y;
        if (dy > 0) {
            h -= dy;
            dstrect->y += dy;
            srcy += dy;
        }
        dy = dstrect->y + h - clip->y - clip->h;
        if (dy > 0)
            h -= dy;
    }

    if (w && h) {
        unsigned char *srcpix = (unsigned char *)src->pixels;
        int srcpitch = src->pitch;
        Uint32 *dstpix = (Uint32 *)dst->pixels;
#define _blitter(dp, xflip)                                                                                         \
    do                                                                                                              \
        for (int y = 0; y < h; y++)                                                                                 \
            for (int x = 0; x < w; x++) {                                                                           \
                unsigned char p =                                                                                   \
                    srcpix[!xflip ? srcx + x + (srcy + y) * srcpitch : srcx + (w - x - 1) + (srcy + y) * srcpitch]; \
                if (p)                                                                                              \
                    dstpix[dstrect->x + x + (dstrect->y + y) * dst->w] = getcolor(dp);                              \
            }                                                                                                       \
    while (0)
        if (color && flipx)
            _blitter(color, 1);
        else if (!color && flipx)
            _blitter(p, 1);
        else if (color && !flipx)
            _blitter(color, 0);
        else if (!color && !flipx)
            _blitter(p, 0);
#undef _blitter
    }
}

static void p8_rectfill(int x0, int y0, int x1, int y1, int col) {
    int w = (x1 - x0 + 1) * scale;
    int h = (y1 - y0 + 1) * scale;
    if (w > 0 && h > 0) {
        SDL_Rect rc = {x0 * scale, y0 * scale, w, h};
        SDL_FillRect(screen, &rc, getcolor(col));
    }
}

static void p8_print(char const *str, int x, int y, int col) {
    for (char c = *str; c; c = *(++str)) {
        c &= 0x7F;
        SDL_Rect srcrc = {8 * (c % 16), 8 * (c / 16)};
        srcrc.x *= scale;
        srcrc.y *= scale;
        srcrc.w = srcrc.h = 8 * scale;

        SDL_Rect dstrc = {x * scale, y * scale, scale, scale};
        Xblit(font, &srcrc, screen, &dstrc, col, 0, 0);
        x += 4;
    }
}

int pico8emu(P8 call, ...) {
    static int camera_x = 0, camera_y = 0;
    if (!enable_screenshake) {
        camera_x = camera_y = 0;
    }

    va_list args;
    int ret = 0;
    va_start(args, call);

#define INT_ARG() va_arg(args, int)
#define BOOL_ARG() (bool)va_arg(args, int)
#define RET_INT(_i) \
    do {            \
        ret = (_i); \
        goto end;   \
    } while (0)
#define RETbool(_b) RET_INT(!!(_b))

    switch (call) {
    case P8_MUSIC: { // music(idx,fade,mask)
        int index = INT_ARG();
        int fade = INT_ARG();
        int mask = INT_ARG();

        (void)mask; // we do not care about this since sdl mixer keeps sounds and
                    // music separate

        if (index == -1) { // stop playing
            Mix_FadeOutMusic(fade);
            current_music = NULL;
        } else if (mus[index / 10]) {
            Mix_Music *musi = mus[index / 10];
            current_music = musi;
            Mix_FadeInMusic(musi, -1, fade);
        }
    } break;
    case P8_SPR: { // spr(sprite,x,y,cols,rows,flipx,flipy)
        int sprite = INT_ARG();
        int x = INT_ARG();
        int y = INT_ARG();
        int cols = INT_ARG();
        int rows = INT_ARG();
        int flipx = BOOL_ARG();
        int flipy = BOOL_ARG();

        (void)cols;
        (void)rows;

        assert(rows == 1 && cols == 1);

        if (sprite >= 0) {
            SDL_Rect srcrc = {8 * (sprite % 16), 8 * (sprite / 16)};
            srcrc.x *= scale;
            srcrc.y *= scale;
            srcrc.w = srcrc.h = scale * 8;
            SDL_Rect dstrc = {(x - camera_x) * scale, (y - camera_y) * scale, scale, scale};
            Xblit(gfx, &srcrc, screen, &dstrc, 0, flipx, flipy);
        }
    } break;
    case P8_BTN: { // btn(b)
        int b = INT_ARG();
        assert(b >= 0 && b <= 5);
        RETbool(buttons_state & (1 << b));
    } break;
    case P8_SFX: { // sfx(id)
        int id = INT_ARG();

        if (id < (sizeof snd) / (sizeof *snd) && snd[id])
            Mix_PlayChannel(-1, snd[id], 0);
    } break;
    case P8_PAL: { // pal(a,b)
        int a = INT_ARG();
        int b = INT_ARG();
        if (a >= 0 && a < 16 && b >= 0 && b < 16) {
            // swap palette colors
            palette[a] = base_palette[b];
        }
    } break;
    case P8_PAL_RESET: { // pal()
        ResetPalette();
    } break;
    case P8_CIRCFILL: { // circfill(x,y,r,col)
        int cx = INT_ARG() - camera_x;
        int cy = INT_ARG() - camera_y;
        int r = INT_ARG();
        int col = INT_ARG();

        int realcolor = getcolor(col);

        int f = 1 - r;     // used to track the progress of the drawn circle (since
                           // its semi-recursive)
        int ddFx = 1;      // step x
        int ddFy = -2 * r; // step y
        int x = 0;
        int y = r;

        // this algorithm doesn't account for the diameters
        // so we have to set them manually
        p8_line(cx, cy - y, cx, cy + r, col);
        p8_line(cx + r, cy, cx - r, cy, col);

        while (x < y) {
            if (f >= 0) {
                y--;
                ddFy += 2;
                f += ddFy;
            }
            x++;
            ddFx += 2;
            f += ddFx;

            // build our current arc
            p8_line(cx + x, cy + y, cx - x, cy + y, col);
            p8_line(cx + x, cy - y, cx - x, cy - y, col);
            p8_line(cx + y, cy + x, cx - y, cy + x, col);
            p8_line(cx + y, cy - x, cx - y, cy - x, col);
        }
    } break;
    case P8_PRINT: { // print(str,x,y,col)
        char const *str = va_arg(args, char const *);
        int x = INT_ARG() - camera_x;
        int y = INT_ARG() - camera_y;
        int col = INT_ARG() % 16;

        p8_print(str, x, y, col);
    } break;
    case P8_RECTFILL: { // rectfill(x0,y0,x1,y1,col)
        int x0 = INT_ARG() - camera_x;
        int y0 = INT_ARG() - camera_y;
        int x1 = INT_ARG() - camera_x;
        int y1 = INT_ARG() - camera_y;
        int col = INT_ARG();

        p8_rectfill(x0, y0, x1, y1, col);
    } break;
    case P8_LINE: { // line(x0,y0,x1,y1,col)
        int x0 = INT_ARG() - camera_x;
        int y0 = INT_ARG() - camera_y;
        int x1 = INT_ARG() - camera_x;
        int y1 = INT_ARG() - camera_y;
        int col = INT_ARG();

        p8_line(x0, y0, x1, y1, col);
    } break;
    case P8_MGET: { // mget(tx,ty)
        int tx = INT_ARG();
        int ty = INT_ARG();

        RET_INT(tilemap_data[tx + ty * 128]);
    } break;
    case P8_CAMERA: { // camera(x,y)
        if (enable_screenshake) {
            camera_x = INT_ARG();
            camera_y = INT_ARG();
        }
    } break;
    case P8_FGET: { // fget(tile,flag)
        int tile = INT_ARG();
        int flag = INT_ARG();

        RET_INT(gettileflag(tile, flag));
    } break;
    case P8_MAP: { // map(mx,my,tx,ty,mw,mh,mask)
        int mx = INT_ARG(), my = INT_ARG();
        int tx = INT_ARG(), ty = INT_ARG();
        int mw = INT_ARG(), mh = INT_ARG();
        int mask = INT_ARG();

        for (int x = 0; x < mw; x++) {
            for (int y = 0; y < mh; y++) {
                int tile = tilemap_data[x + mx + (y + my) * 128];
                // hack
                if (mask == 0 || (mask == 4 && tile_flags[tile] == 4) ||
                    gettileflag(tile, mask != 4 ? mask - 1 : mask)) {
                    // al_draw_bitmap(sprites[tile], tx+x*8 - camera_x, ty+y*8 - camera_y,
                    // 0);
                    SDL_Rect srcrc = {8 * (tile % 16), 8 * (tile / 16)};
                    srcrc.x *= scale;
                    srcrc.y *= scale;
                    srcrc.w = srcrc.h = scale * 8;
                    SDL_Rect dstrc = {(tx + x * 8 - camera_x) * scale, (ty + y * 8 - camera_y) * scale, scale * 8, scale * 8};

                    if (0) {
                        srcrc.x = srcrc.y = 0;
                        srcrc.w = srcrc.h = 8;
                        dstrc.x = x * 8, dstrc.y = y * 8;
                        dstrc.w = dstrc.h = 8;
                    }

                    // SDL_BlitSurface(gfx, &srcrc, screen, &dstrc);
                    Xblit(gfx, &srcrc, screen, &dstrc, 0, 0, 0);
                }
            }
        }
    } break;
    }

end:
    va_end(args);
    return ret;
}

static int gettileflag(int tile, int flag) {
    return tile < sizeof(tile_flags) / sizeof(*tile_flags) &&
           (tile_flags[tile] & (1 << flag)) != 0;
}

// coordinates should NOT be scaled before calling this
static void p8_line(int x0, int y0, int x1, int y1, unsigned char color) {
#define CLAMP(v, min, max) v = v < min ? min : v >= max ? max - 1 \
                                                        : v;
    CLAMP(x0, 0, screen->w);
    CLAMP(y0, 0, screen->h);
    CLAMP(x1, 0, screen->w);
    CLAMP(y1, 0, screen->h);

    Uint32 realcolor = getcolor(color);

#undef CLAMP
#define PLOT(x, y)                                          \
    do {                                                    \
        SDL_Rect rc = {x * scale, y * scale, scale, scale}; \
        SDL_FillRect(screen, &rc, realcolor);               \
    } while (0)
    int sx, sy, dx, dy, err, e2;
    dx = abs(x1 - x0);
    dy = abs(y1 - y0);
    if (!dx && !dy)
        return;

    if (x0 < x1)
        sx = 1;
    else
        sx = -1;
    if (y0 < y1)
        sy = 1;
    else
        sy = -1;
    err = dx - dy;
    if (!dy && !dx)
        return;
    else if (!dx) { // vertical line
        for (int y = y0; y != y1; y += sy)
            PLOT(x0, y);
    } else if (!dy) { // horizontal line
        for (int x = x0; x != x1; x += sx)
            PLOT(x, y0);
    }
    while (x0 != x1 || y0 != y1) {
        PLOT(x0, y0);
        e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
#undef PLOT
}

// vim: ts=2 sw=2 noexpandtab
