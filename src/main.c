/* 个人作品集网站 —— 应用外壳：顶部导航 + 首页/项目/技能/联系四个页面，
   以及内嵌的国际象棋试玩页面（见 chess_view.c）。
   页面正文一律使用英文（仅本文件及其余源码的注释保留中文）；除浏览器
   URL 跳转与音频播放这两个必须调用 Web API 的桥接点（见 ui.c 的
   platform_open_url 与 sound.c）外，不包含任何手写的 JavaScript/HTML 逻辑。 */
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "ui.h"
#include "data.h"
#include "chess_view.h"

#define CANVAS_W 1200
#define CANVAS_H 820
#define NAV_HEIGHT 64
#define FONT_PATH "/fonts/msyh.ttc"

typedef enum { SCREEN_HOME, SCREEN_PROJECTS, SCREEN_SKILLS, SCREEN_CONTACT, SCREEN_CHESS } Screen;

enum {
    ACT_NAV_HOME = 1, ACT_NAV_PROJECTS, ACT_NAV_SKILLS, ACT_NAV_CONTACT,
    ACT_HOME_VIEW_PROJECTS, ACT_HOME_GITHUB,
    ACT_PROJECT_OPEN, ACT_PROJECT_OPEN_ACTION, ACT_PROJECT_PLAY_CHESS,
    ACT_CONTACT_EMAIL, ACT_CONTACT_GITHUB,
};

static SDL_Window *g_window;
static SDL_Renderer *g_renderer;
static Screen g_screen = SCREEN_HOME;
static int g_dirty = 1;
static Uint32 g_lastTicks = 0;
static int g_projectsScroll = 0;

static void dispatch_action(int action, int data) {
    if (action >= CHESS_ACTION_BASE) {
        chess_view_handle_action(action - CHESS_ACTION_BASE, data);
        return;
    }
    switch (action) {
        case ACT_NAV_HOME: g_screen = SCREEN_HOME; break;
        case ACT_NAV_PROJECTS: g_screen = SCREEN_PROJECTS; break;
        case ACT_NAV_SKILLS: g_screen = SCREEN_SKILLS; break;
        case ACT_NAV_CONTACT: g_screen = SCREEN_CONTACT; break;
        case ACT_HOME_VIEW_PROJECTS: g_screen = SCREEN_PROJECTS; break;
        case ACT_HOME_GITHUB: platform_open_url(CONTACT_GITHUB); break;
        case ACT_PROJECT_OPEN:
            if (data >= 0 && data < PROJECT_COUNT) platform_open_url(PROJECTS[data].url);
            break;
        case ACT_PROJECT_OPEN_ACTION:
            if (data >= 0 && data < PROJECT_COUNT && PROJECTS[data].actionUrl)
                platform_open_url(PROJECTS[data].actionUrl);
            break;
        case ACT_PROJECT_PLAY_CHESS:
            g_screen = SCREEN_CHESS;
            chess_view_enter();
            break;
        case ACT_CONTACT_EMAIL: platform_open_url("mailto:" CONTACT_EMAIL); break;
        case ACT_CONTACT_GITHUB: platform_open_url(CONTACT_GITHUB); break;
    }
}

static void render_nav(void) {
    ui_fill_rect(0, 0, CANVAS_W, NAV_HEIGHT, COLOR_PANEL);
    SDL_SetRenderDrawColor(g_renderer, COLOR_BORDER.r, COLOR_BORDER.g, COLOR_BORDER.b, 255);
    SDL_RenderDrawLine(g_renderer, 0, NAV_HEIGHT - 1, CANVAS_W, NAV_HEIGHT - 1);

    ui_draw_text("SUTHARSON", 24, 18, 22, COLOR_ACCENT);
    ui_draw_text("Portfolio", 176, 22, 14, COLOR_TEXT_DIM2);

    struct { const char *label; int action; Screen screen; } items[] = {
        { "Home", ACT_NAV_HOME, SCREEN_HOME },
        { "Projects", ACT_NAV_PROJECTS, SCREEN_PROJECTS },
        { "Skills", ACT_NAV_SKILLS, SCREEN_SKILLS },
        { "Contact", ACT_NAV_CONTACT, SCREEN_CONTACT },
    };
    Screen visualActive = g_screen == SCREEN_CHESS ? SCREEN_PROJECTS : g_screen;
    int btnH = 40, gap = 12, pad = 20;
    int widths[4], totalW = 0;
    for (int i = 0; i < 4; i++) {
        int tw, th;
        ui_text_size(items[i].label, 16, &tw, &th);
        widths[i] = tw + pad * 2;
        totalW += widths[i];
    }
    totalW += gap * 3;
    int x = CANVAS_W - 24 - totalW;
    for (int i = 0; i < 4; i++) {
        int active = visualActive == items[i].screen;
        SDL_Rect r = { x, (NAV_HEIGHT - btnH) / 2, widths[i], btnH };
        if (active) {
            ui_fill_rect(r.x, r.y, r.w, r.h, (SDL_Color){0x4e,0xcc,0xa3,35});
            ui_stroke_rect(r.x, r.y, r.w, r.h, 1, COLOR_ACCENT);
        }
        ui_draw_text_centered(items[i].label, r.x + r.w / 2, r.y + r.h / 2 - 9,
                               16, active ? COLOR_ACCENT : COLOR_TEXT_DIM);
        ui_hit_add(r, items[i].action, 0);
        x += widths[i] + gap;
    }
}

/* 首页改为粗野主义（Brutalism）风格的落地页：厚重黑色描边、纯白底卡片、
   高对比色块、无圆角无渐变——这些恰好都是我们绘图原语本来就有的东西。
   页面正文本身按要求使用英文，注释仍保持中文。 */
static void render_home(SDL_Rect area) {
    const SDL_Color BLACK = { 0x0a, 0x0a, 0x0a, 255 };
    const SDL_Color WHITE = { 0xf5, 0xf5, 0xf0, 255 };
    const SDL_Color GRAY = { 0x40, 0x40, 0x40, 255 };
    int border = 8;

    int cardX = area.x + 60, cardY = area.y + 30, cardW = area.w - 120, cardH = 460;
    int pad = 44;

    /* 卡片：暗色背景上的一块纯白硬边卡片，黑色粗边框 */
    ui_fill_rect(cardX, cardY, cardW, cardH, WHITE);
    ui_stroke_rect(cardX, cardY, cardW, cardH, border, BLACK);

    int titleY = cardY + 40;
    ui_draw_text("SUTHARSON", cardX + pad, titleY, 70, BLACK);

    SDL_Rect tagBox = { cardX + pad, cardY + 130, 340, 44 };
    ui_fill_rect(tagBox.x, tagBox.y, tagBox.w, tagBox.h, COLOR_ACCENT);
    ui_stroke_rect(tagBox.x, tagBox.y, tagBox.w, tagBox.h, 4, BLACK);
    ui_draw_text_centered("SOFTWARE DEVELOPER", tagBox.x + tagBox.w / 2, tagBox.y + 13, 16, BLACK);

    int dividerY = cardY + 194;
    ui_fill_rect(cardX + pad, dividerY, cardW - pad * 2, 6, BLACK);

    const char *bio =
        "I BUILD CHESS ENGINES, AI AGENTS,\n"
        "CHATBOTS, AND FULL-STACK WEB APPS.\n"
        "\n"
        "THIS PAGE ITSELF IS WRITTEN IN C AND\n"
        "COMPILED DIRECTLY TO WEBASSEMBLY.";
    ui_draw_multiline(bio, cardX + pad, dividerY + 30, 17, BLACK, 26);

    int statY = dividerY + 30 + 5 * 26 + 20;
    int statGap = 16;
    int statW = (cardW - pad * 2 - statGap * 2) / 3;
    int statH = 90;
    struct { const char *num; const char *label; } stats[3] = {
        { "15", "PROJECTS SHIPPED" },
        { "6",  "LANGUAGES USED" },
        { "0",  "JS FRAMEWORKS HERE" },
    };
    for (int i = 0; i < 3; i++) {
        int sx = cardX + pad + i * (statW + statGap);
        ui_stroke_rect(sx, statY, statW, statH, 4, BLACK);
        ui_draw_text(stats[i].num, sx + 14, statY + 10, 32, BLACK);
        ui_draw_text(stats[i].label, sx + 14, statY + 56, 12, GRAY);
    }

    /* 卡片外的两个粗野风格按钮：实心色块 + 粗黑边框，无圆角无阴影 */
    int btnY = cardY + cardH + 30;
    SDL_Rect viewBtn = { cardX, btnY, 260, 60 };
    ui_fill_rect(viewBtn.x, viewBtn.y, viewBtn.w, viewBtn.h, COLOR_ACCENT);
    ui_stroke_rect(viewBtn.x, viewBtn.y, viewBtn.w, viewBtn.h, border, BLACK);
    ui_draw_text_centered("VIEW PROJECTS ->", viewBtn.x + viewBtn.w / 2, viewBtn.y + 20, 17, BLACK);
    ui_hit_add(viewBtn, ACT_HOME_VIEW_PROJECTS, 0);

    SDL_Rect ghBtn = { cardX + 260 + 24, btnY, 260, 60 };
    ui_fill_rect(ghBtn.x, ghBtn.y, ghBtn.w, ghBtn.h, BLACK);
    ui_stroke_rect(ghBtn.x, ghBtn.y, ghBtn.w, ghBtn.h, border, WHITE);
    ui_draw_text_centered("GITHUB ->", ghBtn.x + ghBtn.w / 2, ghBtn.y + 20, 17, WHITE);
    ui_hit_add(ghBtn, ACT_HOME_GITHUB, 0);
}

static void render_project_card(SDL_Rect r, const Project *p) {
    ui_fill_rect(r.x, r.y, r.w, r.h, COLOR_PANEL2);
    ui_stroke_rect(r.x, r.y, r.w, r.h, 1, COLOR_BORDER);
    ui_draw_text(p->title, r.x + 16, r.y + 14, 17, COLOR_TEXT);
    ui_draw_multiline(p->desc, r.x + 16, r.y + 46, 13, COLOR_TEXT_DIM, 20);
    ui_draw_text(p->highlight, r.x + 16, r.y + 90, 12, COLOR_ACCENT);
    ui_draw_text(p->tech, r.x + 16, r.y + r.h - 54, 12, COLOR_TEXT_DIM2);

    int idx = (int)(p - PROJECTS);
    int btnY = r.y + r.h - 38, btnH = 28, gap = 8;
    int btnW = (r.w - 32 - gap) / 2;

    /* 左：View Source，永远指向仓库根目录 */
    SDL_Rect srcBtn = { r.x + 16, btnY, btnW, btnH };
    ui_fill_rect(srcBtn.x, srcBtn.y, srcBtn.w, srcBtn.h, COLOR_PANEL);
    ui_stroke_rect(srcBtn.x, srcBtn.y, srcBtn.w, srcBtn.h, 1, COLOR_BORDER2);
    ui_draw_text_centered("View Source", srcBtn.x + srcBtn.w / 2, srcBtn.y + 6, 13, COLOR_TEXT_DIM);
    ui_hit_add(srcBtn, ACT_PROJECT_OPEN, idx);

    /* 右：第二个动作——国际象棋是站内试玩，其余打开 actionUrl（真实演示/
       发布页，没有的话是仓库 README，绝不编造链接） */
    SDL_Rect actBtn = { srcBtn.x + btnW + gap, btnY, btnW, btnH };
    ui_fill_rect(actBtn.x, actBtn.y, actBtn.w, actBtn.h, COLOR_ACCENT);
    ui_stroke_rect(actBtn.x, actBtn.y, actBtn.w, actBtn.h, 1, COLOR_ACCENT);
    ui_draw_text_centered(p->actionLabel, actBtn.x + actBtn.w / 2, actBtn.y + 6, 13, (SDL_Color){15,31,26,255});
    ui_hit_add(actBtn, p->isChess ? ACT_PROJECT_PLAY_CHESS : ACT_PROJECT_OPEN_ACTION, idx);
}

static void render_projects(SDL_Rect area) {
    ui_draw_text("Projects", area.x + 20, area.y + 20, 26, COLOR_TEXT);
    ui_draw_text("Selected completed projects from github.com/sutharson-k", area.x + 20, area.y + 56, 14, COLOR_TEXT_DIM2);

    int cols = 3;
    int gap = 20;
    int cardW = (area.w - 40 - gap * (cols - 1)) / cols;
    int cardH = 196;
    int rowH = cardH + gap;
    int rows = (PROJECT_COUNT + cols - 1) / cols;
    int totalH = rows * rowH + 40;
    int maxScroll = totalH - (area.h - 90);
    if (maxScroll < 0) maxScroll = 0;
    if (g_projectsScroll > maxScroll) g_projectsScroll = maxScroll;
    if (g_projectsScroll < 0) g_projectsScroll = 0;

    SDL_Rect clip = { area.x, area.y + 90, area.w, area.h - 90 };
    SDL_RenderSetClipRect(g_renderer, &clip);
    for (int i = 0; i < PROJECT_COUNT; i++) {
        int col = i % cols, row = i / cols;
        SDL_Rect r = {
            area.x + 20 + col * (cardW + gap),
            area.y + 90 + row * rowH - g_projectsScroll,
            cardW, cardH
        };
        render_project_card(r, &PROJECTS[i]);
    }
    SDL_RenderSetClipRect(g_renderer, NULL);
}

static void render_skills(SDL_Rect area) {
    ui_draw_text("Skills", area.x + 20, area.y + 20, 26, COLOR_TEXT);
    ui_draw_text("Technologies used and practiced across these projects", area.x + 20, area.y + 56, 14, COLOR_TEXT_DIM2);

    int y = area.y + 110;
    for (int i = 0; i < SKILL_GROUP_COUNT; i++) {
        SDL_Rect panel = { area.x + 20, y, area.w - 40, 100 };
        ui_fill_rect(panel.x, panel.y, panel.w, panel.h, COLOR_PANEL2);
        ui_stroke_rect(panel.x, panel.y, panel.w, panel.h, 1, COLOR_BORDER);
        ui_draw_text(SKILL_GROUPS[i].category, panel.x + 20, panel.y + 16, 16, COLOR_ACCENT);
        ui_draw_multiline(SKILL_GROUPS[i].items, panel.x + 20, panel.y + 48, 15, COLOR_TEXT, 26);
        y += 100 + 16;
    }
}

static void render_contact(SDL_Rect area) {
    int cx = area.x + area.w / 2;
    ui_draw_text_centered("Contact", cx, area.y + 80, 26, COLOR_TEXT);
    ui_draw_text_centered("Feel free to reach out through either of these", cx, area.y + 118, 14, COLOR_TEXT_DIM2);

    SDL_Rect emailBtn = { cx - 160, area.y + 180, 320, 52 };
    ui_fill_rect(emailBtn.x, emailBtn.y, emailBtn.w, emailBtn.h, COLOR_PANEL2);
    ui_stroke_rect(emailBtn.x, emailBtn.y, emailBtn.w, emailBtn.h, 1, COLOR_BORDER2);
    ui_draw_text_centered("Email: " CONTACT_EMAIL, emailBtn.x + emailBtn.w / 2, emailBtn.y + 17, 15, COLOR_TEXT);
    ui_hit_add(emailBtn, ACT_CONTACT_EMAIL, 0);

    SDL_Rect ghBtn = { cx - 160, area.y + 246, 320, 52 };
    ui_fill_rect(ghBtn.x, ghBtn.y, ghBtn.w, ghBtn.h, COLOR_ACCENT);
    ui_draw_text_centered("GitHub: github.com/sutharson-k", ghBtn.x + ghBtn.w / 2, ghBtn.y + 17, 15, (SDL_Color){15,31,26,255});
    ui_hit_add(ghBtn, ACT_CONTACT_GITHUB, 0);
}

static void render_frame(void) {
    ui_hit_begin();
    SDL_SetRenderDrawColor(g_renderer, COLOR_BG.r, COLOR_BG.g, COLOR_BG.b, 255);
    SDL_RenderClear(g_renderer);

    render_nav();
    SDL_Rect content = { 0, NAV_HEIGHT, CANVAS_W, CANVAS_H - NAV_HEIGHT };
    switch (g_screen) {
        case SCREEN_HOME: render_home(content); break;
        case SCREEN_PROJECTS: render_projects(content); break;
        case SCREEN_SKILLS: render_skills(content); break;
        case SCREEN_CONTACT: render_contact(content); break;
        case SCREEN_CHESS: chess_view_render(content); break;
    }

    SDL_RenderPresent(g_renderer);
}

static void main_loop(void) {
    SDL_Event e;
    int clicked = 0, mx = 0, my = 0, wheelY = 0;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            clicked = 1; mx = e.button.x; my = e.button.y; g_dirty = 1;
        } else if (e.type == SDL_MOUSEWHEEL) {
            wheelY = e.wheel.y; g_dirty = 1;
        } else if (e.type == SDL_MOUSEMOTION || e.type == SDL_WINDOWEVENT) {
            g_dirty = 1;
        }
    }

    Uint32 now = SDL_GetTicks();
    int dt = (int)(now - g_lastTicks);
    g_lastTicks = now;
    if (g_screen == SCREEN_CHESS) chess_view_tick(dt);

    if (clicked) {
        int action, data;
        if (ui_hit_test(mx, my, &action, &data)) dispatch_action(action, data);
    }
    if (g_screen == SCREEN_PROJECTS && wheelY) {
        g_projectsScroll -= wheelY * 40;
    }

    if (g_dirty || g_screen == SCREEN_CHESS) {
        render_frame();
        g_dirty = 0;
    }
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    srand((unsigned int)time(NULL));

    SDL_Init(SDL_INIT_VIDEO);
    g_window = SDL_CreateWindow("Sutharson - Portfolio", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                 CANVAS_W, CANVAS_H, SDL_WINDOW_SHOWN);
    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);

    ui_init(g_renderer, FONT_PATH);
    chess_view_init();

    g_lastTicks = SDL_GetTicks();
    render_frame();
    g_dirty = 0;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, 1);
#else
    int running = 1;
    while (running) {
        main_loop();
        SDL_Delay(16);
    }
    ui_shutdown();
    SDL_DestroyRenderer(g_renderer);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
#endif
    return 0;
}
