#ifndef CHESS_VIEW_H
#define CHESS_VIEW_H

#include <SDL2/SDL.h>

/* chess_view 在 main.c 的点击分发中占用的动作号区间：
   main.c 命中测试时若 action >= CHESS_ACTION_BASE，
   则将 (action - CHESS_ACTION_BASE) 与 data 转交给 chess_view_handle_action。 */
#define CHESS_ACTION_BASE 5000

enum {
    CV_ACT_SET_COLOR_WHITE = 0,
    CV_ACT_SET_COLOR_BLACK,
    CV_ACT_SET_DIFFICULTY,   /* data: 2=简单 3=中等 4=困难 */
    CV_ACT_START_GAME,
    CV_ACT_BOARD_SQUARE,     /* data: r*8+c（棋盘逻辑坐标） */
    CV_ACT_UNDO,
    CV_ACT_RESIGN,
    CV_ACT_DRAW_OFFER,
    CV_ACT_NEW_GAME,
    CV_ACT_PROMOTION_CHOICE, /* data: 0=后 1=车 2=象 3=马 */
    CV_ACT_PLAY_AGAIN,
};

void chess_view_init(void);
/* 每次从"项目"页进入试玩时调用，回到开局设置界面 */
void chess_view_enter(void);
void chess_view_render(SDL_Rect area);
void chess_view_handle_action(int action, int data);
/* 每帧调用，推进电脑走棋的思考延时 */
void chess_view_tick(int dtMs);

#endif
