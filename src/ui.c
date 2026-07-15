#include <string.h>
#include <stdio.h>
#include <math.h>
#include "ui.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MAX_FONT_SIZES 8
#define MAX_HIT_REGIONS 256

typedef struct { SDL_Rect rect; int action; int data; } HitRegion;

static SDL_Renderer *g_renderer = NULL;
static char g_fontPath[512];
static struct { int size; TTF_Font *font; } g_fonts[MAX_FONT_SIZES];
static int g_fontCount = 0;

static HitRegion g_hitRegions[MAX_HIT_REGIONS];
static int g_hitRegionCount = 0;

int ui_init(SDL_Renderer *renderer, const char *fontPath) {
    g_renderer = renderer;
    strncpy(g_fontPath, fontPath, sizeof(g_fontPath) - 1);
    g_fontPath[sizeof(g_fontPath) - 1] = 0;
    if (TTF_Init() != 0) return -1;
    return 0;
}

void ui_shutdown(void) {
    for (int i = 0; i < g_fontCount; i++) TTF_CloseFont(g_fonts[i].font);
    g_fontCount = 0;
    TTF_Quit();
}

TTF_Font *ui_font(int size) {
    for (int i = 0; i < g_fontCount; i++)
        if (g_fonts[i].size == size) return g_fonts[i].font;
    if (g_fontCount >= MAX_FONT_SIZES) return g_fontCount > 0 ? g_fonts[0].font : NULL;
    TTF_Font *font = TTF_OpenFont(g_fontPath, size);
    if (!font) return NULL;
    g_fonts[g_fontCount].size = size;
    g_fonts[g_fontCount].font = font;
    g_fontCount++;
    return font;
}

void ui_text_size(const char *utf8, int size, int *w, int *h) {
    TTF_Font *font = ui_font(size);
    if (!font || !utf8 || !utf8[0]) { *w = 0; *h = size; return; }
    TTF_SizeUTF8(font, utf8, w, h);
}

void ui_draw_text(const char *utf8, int x, int y, int size, SDL_Color color) {
    if (!utf8 || !utf8[0]) return;
    TTF_Font *font = ui_font(size);
    if (!font) return;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, utf8, color);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(g_renderer, surf);
    if (tex) {
        SDL_Rect dst = { x, y, surf->w, surf->h };
        SDL_RenderCopy(g_renderer, tex, NULL, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
}

void ui_draw_text_centered(const char *utf8, int centerX, int y, int size, SDL_Color color) {
    int w, h;
    ui_text_size(utf8, size, &w, &h);
    ui_draw_text(utf8, centerX - w / 2, y, size, color);
}

void ui_draw_multiline(const char *utf8, int x, int y, int size, SDL_Color color, int lineHeight) {
    if (!utf8) return;
    char line[512];
    const char *p = utf8;
    int cy = y;
    while (*p) {
        const char *nl = strchr(p, '\n');
        size_t len = nl ? (size_t)(nl - p) : strlen(p);
        if (len >= sizeof(line)) len = sizeof(line) - 1;
        memcpy(line, p, len);
        line[len] = 0;
        ui_draw_text(line, x, cy, size, color);
        cy += lineHeight;
        if (!nl) break;
        p = nl + 1;
    }
}

void ui_fill_rect(int x, int y, int w, int h, SDL_Color color) {
    SDL_SetRenderDrawColor(g_renderer, color.r, color.g, color.b, color.a);
    SDL_Rect r = { x, y, w, h };
    SDL_RenderFillRect(g_renderer, &r);
}

void ui_stroke_rect(int x, int y, int w, int h, int thickness, SDL_Color color) {
    SDL_SetRenderDrawColor(g_renderer, color.r, color.g, color.b, color.a);
    for (int i = 0; i < thickness; i++) {
        SDL_Rect r = { x + i, y + i, w - 2 * i, h - 2 * i };
        SDL_RenderDrawRect(g_renderer, &r);
    }
}

void ui_fill_circle(int cx, int cy, int radius, SDL_Color color) {
    SDL_SetRenderDrawColor(g_renderer, color.r, color.g, color.b, color.a);
    for (int dy = -radius; dy <= radius; dy++) {
        int dx = (int)sqrt((double)(radius * radius - dy * dy));
        SDL_RenderDrawLine(g_renderer, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

void ui_stroke_circle(int cx, int cy, int radius, SDL_Color color) {
    SDL_SetRenderDrawColor(g_renderer, color.r, color.g, color.b, color.a);
    const int segments = 64;
    for (int i = 0; i < segments; i++) {
        double a1 = (2.0 * M_PI * i) / segments;
        double a2 = (2.0 * M_PI * (i + 1)) / segments;
        SDL_RenderDrawLine(g_renderer,
            cx + (int)(radius * cos(a1)), cy + (int)(radius * sin(a1)),
            cx + (int)(radius * cos(a2)), cy + (int)(radius * sin(a2)));
    }
}

void ui_hit_begin(void) {
    g_hitRegionCount = 0;
}

void ui_hit_add(SDL_Rect rect, int action, int data) {
    if (g_hitRegionCount < MAX_HIT_REGIONS) {
        g_hitRegions[g_hitRegionCount].rect = rect;
        g_hitRegions[g_hitRegionCount].action = action;
        g_hitRegions[g_hitRegionCount].data = data;
        g_hitRegionCount++;
    }
}

int ui_point_in_rect(int x, int y, SDL_Rect r) {
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

int ui_hit_test(int mx, int my, int *action, int *data) {
    for (int i = g_hitRegionCount - 1; i >= 0; i--) {
        if (ui_point_in_rect(mx, my, g_hitRegions[i].rect)) {
            *action = g_hitRegions[i].action;
            *data = g_hitRegions[i].data;
            return 1;
        }
    }
    return 0;
}

void platform_open_url(const char *url) {
#ifdef __EMSCRIPTEN__
    EM_ASM({ window.open(UTF8ToString($0), '_blank', 'noopener'); }, url);
#else
    printf("[platform_open_url] %s\n", url);
#endif
}
