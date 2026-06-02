#include <stdlib.h>
#include <string.h>
#include "interpreter.h"

#ifdef FC_USE_SDL
#include <SDL2/SDL.h>

static double wn(Interp* I, int argc, Value* a, int i, const char* fn) {
    if (i >= argc || a[i].type != VAL_NUMBER) interp_error(I, 0, "%s expects a number argument", fn);
    return a[i].as.number;
}
static const char* ws(Interp* I, int argc, Value* a, int i, const char* fn) {
    if (i >= argc || a[i].type != VAL_STRING) interp_error(I, 0, "%s expects a string argument", fn);
    return a[i].as.str->chars;
}

static SDL_Window* g_sdl_win = NULL;
static SDL_Renderer* g_sdl_ren = NULL;
static void sdl_set_color(int color) {
    SDL_SetRenderDrawColor(g_sdl_ren, (color >> 16) & 255, (color >> 8) & 255, color & 255, 255);
}
static Value win_open(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* title = ws(I, argc, a, 0, "win.open");
    int w = (int)wn(I, argc, a, 1, "win.open");
    int h = (int)wn(I, argc, a, 2, "win.open");
    if (g_sdl_win) return NULL_VAL();
    if (SDL_Init(SDL_INIT_VIDEO) != 0) interp_error(I, 0, "win.open: SDL_Init failed: %s", SDL_GetError());
    g_sdl_win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_SHOWN);
    if (!g_sdl_win) interp_error(I, 0, "win.open: cannot create window: %s", SDL_GetError());
    g_sdl_ren = SDL_CreateRenderer(g_sdl_win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!g_sdl_ren) g_sdl_ren = SDL_CreateRenderer(g_sdl_win, -1, 0);
    if (!g_sdl_ren) interp_error(I, 0, "win.open: cannot create renderer: %s", SDL_GetError());
    return NULL_VAL();
}
static Value win_clear(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s;
    int color = (argc >= 1 && a[0].type == VAL_NUMBER) ? (int)a[0].as.number : 0;
    if (g_sdl_ren) { sdl_set_color(color); SDL_RenderClear(g_sdl_ren); }
    return NULL_VAL();
}
static Value win_pixel(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (!g_sdl_ren) interp_error(I, 0, "win.pixel: no window open");
    sdl_set_color((int)wn(I, argc, a, 2, "win.pixel"));
    SDL_RenderDrawPoint(g_sdl_ren, (int)wn(I, argc, a, 0, "win.pixel"), (int)wn(I, argc, a, 1, "win.pixel"));
    return NULL_VAL();
}
static Value win_rect(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (!g_sdl_ren) interp_error(I, 0, "win.rect: no window open");
    SDL_Rect r;
    r.x = (int)wn(I, argc, a, 0, "win.rect");
    r.y = (int)wn(I, argc, a, 1, "win.rect");
    r.w = (int)wn(I, argc, a, 2, "win.rect");
    r.h = (int)wn(I, argc, a, 3, "win.rect");
    sdl_set_color((int)wn(I, argc, a, 4, "win.rect"));
    SDL_RenderFillRect(g_sdl_ren, &r);
    return NULL_VAL();
}
static Value win_line(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (!g_sdl_ren) interp_error(I, 0, "win.line: no window open");
    sdl_set_color((int)wn(I, argc, a, 4, "win.line"));
    SDL_RenderDrawLine(g_sdl_ren, (int)wn(I, argc, a, 0, "win.line"), (int)wn(I, argc, a, 1, "win.line"),
                       (int)wn(I, argc, a, 2, "win.line"), (int)wn(I, argc, a, 3, "win.line"));
    return NULL_VAL();
}
static Value win_present(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s; (void)argc; (void)a;
    if (g_sdl_ren) SDL_RenderPresent(g_sdl_ren);
    return NULL_VAL();
}
static Value win_delay(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    SDL_Delay((Uint32)wn(I, argc, a, 0, "win.delay"));
    return NULL_VAL();
}
static Value win_poll(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s; (void)argc; (void)a;
    Value d = make_dict();
    SDL_Event e;
    if (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            dict_set_take(d.as.dict, "type", make_string_cstr("quit"));
        } else if (e.type == SDL_KEYDOWN) {
            dict_set_take(d.as.dict, "type", make_string_cstr("key"));
            dict_set_take(d.as.dict, "key", make_string_cstr(SDL_GetKeyName(e.key.keysym.sym)));
        } else if (e.type == SDL_KEYUP) {
            dict_set_take(d.as.dict, "type", make_string_cstr("keyup"));
            dict_set_take(d.as.dict, "key", make_string_cstr(SDL_GetKeyName(e.key.keysym.sym)));
        } else if (e.type == SDL_MOUSEBUTTONDOWN) {
            dict_set_take(d.as.dict, "type", make_string_cstr("mouse"));
            dict_set_take(d.as.dict, "x", NUMBER_VAL(e.button.x));
            dict_set_take(d.as.dict, "y", NUMBER_VAL(e.button.y));
            dict_set_take(d.as.dict, "button", NUMBER_VAL(e.button.button));
        } else {
            dict_set_take(d.as.dict, "type", make_string_cstr("none"));
        }
    } else {
        dict_set_take(d.as.dict, "type", make_string_cstr("none"));
    }
    return d;
}
static Value win_pressed(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* name = ws(I, argc, a, 0, "win.pressed");
    SDL_Scancode sc = SDL_GetScancodeFromName(name);
    if (sc == SDL_SCANCODE_UNKNOWN) return BOOL_VAL(0);
    const Uint8* st = SDL_GetKeyboardState(NULL);
    return BOOL_VAL(st[sc] != 0);
}
static Value win_close(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s; (void)argc; (void)a;
    if (g_sdl_ren) { SDL_DestroyRenderer(g_sdl_ren); g_sdl_ren = NULL; }
    if (g_sdl_win) { SDL_DestroyWindow(g_sdl_win); g_sdl_win = NULL; }
    SDL_Quit();
    return NULL_VAL();
}
static void aw(ObjDict* d, const char* name, NativeFn fn) {
    dict_set_take(d, name, make_native(fn, NULL_VAL(), name));
}
void register_win(Interp* I) {
    Value win = make_dict();
    aw(win.as.dict, "open", win_open);
    aw(win.as.dict, "clear", win_clear);
    aw(win.as.dict, "pixel", win_pixel);
    aw(win.as.dict, "rect", win_rect);
    aw(win.as.dict, "line", win_line);
    aw(win.as.dict, "present", win_present);
    aw(win.as.dict, "delay", win_delay);
    aw(win.as.dict, "poll", win_poll);
    aw(win.as.dict, "pressed", win_pressed);
    aw(win.as.dict, "close", win_close);
    env_define(I->globals, "win", win);
    value_release(win);
}
#else
void register_win(Interp* I) {
    (void)I;
}
#endif
