#include <assert.h>

enum {
    SDL_PHYSPAL = 1,
    SDL_LOGPAL = 2,
    SDL_SRCCOLORKEY = 4,
    SDL_HWPALETTE = 8,
};

static SDL_Surface *sdl2_screen = NULL;

static SDL_Texture *sdl2_screen_tex = NULL;

static SDL_Window *sdl2_window = NULL;

static SDL_Renderer *sdl2_rendr = NULL;

static SDL_Surface *SDL_SetVideoMode(int width, int height, int bpp, Uint32 flags) {
    if (!sdl2_window) {
        sdl2_window =
            SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_RESIZABLE);
        if (!sdl2_window)
            goto die;
        sdl2_rendr = SDL_CreateRenderer(sdl2_window, -1, 0);
        SDL_RenderSetLogicalSize(sdl2_rendr, width, height);
        if (!sdl2_rendr)
            goto die;
        sdl2_screen_tex =
            SDL_CreateTexture(sdl2_rendr, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, width, height);

        if (0) {
        die:
            if (sdl2_window)
                SDL_DestroyWindow(sdl2_window);
            if (sdl2_screen_tex)
                SDL_DestroyTexture(sdl2_screen_tex);
            if (sdl2_rendr)
                SDL_DestroyRenderer(sdl2_rendr);
            return NULL;
        }
    }
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    int rmask = 0xff000000, gmask = 0x00ff0000, bmask = 0x0000ff00,
        amask = 0x000000ff;
#else
    int rmask = 0x000000ff, gmask = 0x0000ff00, bmask = 0x00ff0000,
        amask = 0xff000000;
#endif
    sdl2_screen =
        SDL_CreateRGBSurface(0, width, height, 32, rmask, gmask, bmask, amask);
    assert(sdl2_screen && sdl2_screen->format->BitsPerPixel == bpp);
    return sdl2_screen;
}
static void SDL_WM_SetCaption(char const *title, char const *icon) {
    assert(sdl2_window != NULL);
    SDL_SetWindowTitle(sdl2_window, title);
    (void)icon;
}

static SDL_Surface *SDL_GetVideoSurface(void) { return sdl2_screen; }

static void SDL_Flip(SDL_Surface *screen) {
    assert(screen == sdl2_screen);
    assert(sdl2_window != NULL);
    SDL_UpdateTexture(sdl2_screen_tex, NULL, screen->pixels, screen->pitch);
    SDL_SetRenderDrawColor(sdl2_rendr, 0, 0, 0, 255);
    SDL_RenderClear(sdl2_rendr);
    SDL_RenderCopy(sdl2_rendr, sdl2_screen_tex, NULL, NULL);
    SDL_RenderPresent(sdl2_rendr);
}
