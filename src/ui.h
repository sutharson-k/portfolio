#ifndef UI_H
#define UI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

/* ===== 配色（深色主题，与国际象棋项目保持一致） ===== */
#define COLOR_BG        (SDL_Color){0x16,0x16,0x25,255}
#define COLOR_PANEL     (SDL_Color){0x20,0x20,0x35,255}
#define COLOR_PANEL2    (SDL_Color){0x1e,0x1e,0x35,255}
#define COLOR_BORDER    (SDL_Color){0x33,0x33,0x50,255}
#define COLOR_BORDER2   (SDL_Color){0x40,0x40,0x65,255}
#define COLOR_ACCENT    (SDL_Color){0x4e,0xcc,0xa3,255}
#define COLOR_TEXT      (SDL_Color){0xe0,0xe0,0xe0,255}
#define COLOR_TEXT_DIM  (SDL_Color){0xaa,0xaa,0xaa,255}
#define COLOR_TEXT_DIM2 (SDL_Color){0x88,0x88,0x88,255}
#define COLOR_DANGER    (SDL_Color){0xd2,0x32,0x32,255}

int ui_init(SDL_Renderer *renderer, const char *fontPath);
void ui_shutdown(void);

TTF_Font *ui_font(int size);

/* 立即模式文字绘制：整帧只在输入变化时重绘（见 main.c 的脏标记），
   因此这里不做纹理缓存，直接渲染即可，逻辑更简单也更不容易出错 */
void ui_draw_text(const char *utf8, int x, int y, int size, SDL_Color color);
void ui_draw_text_centered(const char *utf8, int centerX, int y, int size, SDL_Color color);
/* 按 '\n' 拆行绘制多行文字，行距为 lineHeight */
void ui_draw_multiline(const char *utf8, int x, int y, int size, SDL_Color color, int lineHeight);
void ui_text_size(const char *utf8, int size, int *w, int *h);

void ui_fill_rect(int x, int y, int w, int h, SDL_Color color);
void ui_stroke_rect(int x, int y, int w, int h, int thickness, SDL_Color color);
void ui_fill_circle(int cx, int cy, int radius, SDL_Color color);
void ui_stroke_circle(int cx, int cy, int radius, SDL_Color color);

/* ===== 即时模式点击区域系统 ===== */
void ui_hit_begin(void);
/* action/data 由调用方定义并在 ui_hit_test 命中后使用 */
void ui_hit_add(SDL_Rect rect, int action, int data);
/* 返回是否命中；命中时写出 action/data */
int ui_hit_test(int mx, int my, int *action, int *data);

int ui_point_in_rect(int x, int y, SDL_Rect r);

/* 通过 Emscripten 桥接在新标签页打开链接（唯一必要的 JS 交互点） */
void platform_open_url(const char *url);

#endif
