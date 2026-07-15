/* 国际象棋试玩视图：把 chess.c / ai.c 引擎接入到本站的即时模式 UI 系统中。
   移植自原项目 public/game.js 的控制器逻辑，但改为逐帧渲染 + 动作分发。 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess_view.h"
#include "chess.h"
#include "ai.h"
#include "ui.h"
#include "sound.h"

typedef struct { char white[16]; char black[16]; } MoveRow;

typedef struct {
    Chess game;
    ChessAI ai;
    char playerColor;
    int difficulty;
    int subScreen;         /* 0=开局设置 1=对局中 */

    int selected;
    int selR, selC;
    Move validMoves[MAX_MOVES];
    int validMoveCount;

    Move lastMove;
    int hasLastMove;

    int isComputerTurn;
    int computerMoveTimer;

    int gameOver;
    char resultTitle[64];
    char resultMessage[160];

    Move pendingPromotion;
    int hasPendingPromotion;

    MoveRow moveRows[300];
    int moveRowCount;

    char statusOverride[80];
    int statusOverrideTimer;
} ChessViewState;

static ChessViewState cv;

static double frand(void) { return rand() / (double)RAND_MAX; }

static void to_display(int r, int c, int flip, int *dr, int *dc) {
    *dr = flip ? 7 - r : r;
    *dc = flip ? 7 - c : c;
}

static char piece_glyph(char piece) {
    switch (piece) {
        case 'k': return 'K';
        case 'q': return 'Q';
        case 'r': return 'R';
        case 'b': return 'B';
        case 'n': return 'N';
        case 'p': return 'P';
        default: return '?';
    }
}

static void render_piece_chip(int cx, int cy, int radius, char piece, char color) {
    SDL_Color fill = color == 'w' ? (SDL_Color){0xf0,0xf0,0xe8,255} : (SDL_Color){0x2a,0x2a,0x2e,255};
    SDL_Color border = color == 'w' ? (SDL_Color){0x50,0x50,0x50,255} : (SDL_Color){0xc8,0xc8,0xc8,255};
    SDL_Color text = color == 'w' ? (SDL_Color){0x20,0x20,0x20,255} : (SDL_Color){0xe8,0xe8,0xe8,255};
    ui_fill_circle(cx, cy, radius, fill);
    ui_stroke_circle(cx, cy, radius, border);
    char buf[2] = { piece_glyph(piece), 0 };
    int size = radius > 10 ? radius + 2 : 12;
    int w, h;
    ui_text_size(buf, size, &w, &h);
    ui_draw_text(buf, cx - w / 2, cy - h / 2, size, text);
}

static int is_promotion_row(int tr) {
    return cv.playerColor == 'w' ? tr == 0 : tr == 7;
}

static void append_notation(char color, const char *notation) {
    if (color == 'w') {
        if (cv.moveRowCount >= 300) return;
        strncpy(cv.moveRows[cv.moveRowCount].white, notation, 15);
        cv.moveRows[cv.moveRowCount].white[15] = 0;
        cv.moveRows[cv.moveRowCount].black[0] = 0;
        cv.moveRowCount++;
    } else {
        if (cv.moveRowCount > 0 && cv.moveRows[cv.moveRowCount - 1].black[0] == 0 &&
            cv.moveRows[cv.moveRowCount - 1].white[0] != 0) {
            strncpy(cv.moveRows[cv.moveRowCount - 1].black, notation, 15);
            cv.moveRows[cv.moveRowCount - 1].black[15] = 0;
        } else {
            if (cv.moveRowCount >= 300) return;
            cv.moveRows[cv.moveRowCount].white[0] = 0;
            strncpy(cv.moveRows[cv.moveRowCount].black, notation, 15);
            cv.moveRows[cv.moveRowCount].black[15] = 0;
            cv.moveRowCount++;
        }
    }
}

static void play_move_sound(const Move *m) {
    if (m->castle) sound_play(SOUND_CASTLE);
    else if (m->capture) sound_play(SOUND_CAPTURE);
    else sound_play(SOUND_MOVE);
    if (chess_in_check(&cv.game.state, cv.game.state.turn)) sound_play(SOUND_CHECK);
}

static int check_game_end(void) {
    char color = cv.game.state.turn;
    if (chess_is_checkmate(&cv.game.state, color)) {
        cv.gameOver = 1;
        snprintf(cv.resultTitle, sizeof(cv.resultTitle), "%s获胜！", color == 'w' ? "黑方" : "白方");
        strncpy(cv.resultMessage, "将死", sizeof(cv.resultMessage) - 1);
        sound_play(SOUND_GAMEOVER);
        return 1;
    }
    if (chess_is_stalemate(&cv.game.state, color)) {
        cv.gameOver = 1;
        strncpy(cv.resultTitle, "和棋！", sizeof(cv.resultTitle) - 1);
        strncpy(cv.resultMessage, "逼和（无子可动）", sizeof(cv.resultMessage) - 1);
        sound_play(SOUND_GAMEOVER);
        return 1;
    }
    if (chess_is_draw_full(&cv.game)) {
        cv.gameOver = 1;
        strncpy(cv.resultTitle, "和棋！", sizeof(cv.resultTitle) - 1);
        strncpy(cv.resultMessage,
                cv.game.state.halfMoves >= 100 ? "五十步规则" : "子力不足 / 三次重复局面",
                sizeof(cv.resultMessage) - 1);
        sound_play(SOUND_GAMEOVER);
        return 1;
    }
    return 0;
}

static void execute_move(Move move) {
    ChessState before = cv.game.state;
    char notation[16];
    chess_to_algebraic(&before, &move, notation);
    chess_apply_move(&cv.game, move);
    cv.lastMove = move;
    cv.hasLastMove = 1;
    cv.selected = 0;
    cv.validMoveCount = 0;
    append_notation(move.color, notation);
    play_move_sound(&move);
    if (check_game_end()) return;
    if (cv.game.state.turn != cv.playerColor) {
        cv.isComputerTurn = 1;
        cv.computerMoveTimer = 300;
    }
}

static void do_computer_move(void) {
    Move move;
    if (!ai_get_best_move(&cv.ai, &cv.game, &move)) {
        cv.isComputerTurn = 0;
        check_game_end();
        return;
    }
    ChessState before = cv.game.state;
    char notation[16];
    chess_to_algebraic(&before, &move, notation);
    chess_apply_move(&cv.game, move);
    cv.lastMove = move;
    cv.hasLastMove = 1;
    cv.isComputerTurn = 0;
    append_notation(move.color, notation);
    play_move_sound(&move);
    check_game_end();
}

static void on_square_click(int r, int c) {
    if (cv.gameOver || cv.isComputerTurn || cv.hasPendingPromotion) return;

    int foundIdx = -1;
    if (cv.selected) {
        for (int i = 0; i < cv.validMoveCount; i++) {
            if (cv.validMoves[i].tr == r && cv.validMoves[i].tc == c) { foundIdx = i; break; }
        }
    }
    if (foundIdx >= 0) {
        Move mv = cv.validMoves[foundIdx];
        if (mv.piece == 'p' && is_promotion_row(mv.tr)) {
            cv.pendingPromotion = mv;
            cv.hasPendingPromotion = 1;
            return;
        }
        execute_move(mv);
        return;
    }

    char sqPiece = cv.game.state.piece[r][c];
    char sqColor = cv.game.state.color[r][c];
    if (sqPiece != 0 && sqColor == cv.playerColor) {
        cv.selected = 1; cv.selR = r; cv.selC = c;
        cv.validMoveCount = chess_get_legal_moves(&cv.game.state, r, c, cv.validMoves);
        return;
    }
    cv.selected = 0;
    cv.validMoveCount = 0;
}

static void complete_promotion(char piece) {
    if (!cv.hasPendingPromotion) return;
    Move mv = cv.pendingPromotion;
    mv.promotion = piece;
    cv.hasPendingPromotion = 0;
    execute_move(mv);
}

static void do_undo(void) {
    if (cv.gameOver || cv.isComputerTurn || cv.game.historyCount < 2) return;
    if (cv.playerColor == 'w') {
        chess_undo(&cv.game);
        chess_undo(&cv.game);
    } else {
        chess_undo(&cv.game);
        if (cv.game.historyCount > 0 && cv.game.state.turn != cv.playerColor) {
            chess_undo(&cv.game);
        }
    }
    cv.selected = 0;
    cv.validMoveCount = 0;
    cv.hasLastMove = 0;
    cv.isComputerTurn = 0;
    if (cv.moveRowCount > 0) {
        MoveRow *last = &cv.moveRows[cv.moveRowCount - 1];
        if (last->black[0]) last->black[0] = 0;
        else cv.moveRowCount--;
    }
}

static void do_resign(void) {
    if (cv.gameOver) return;
    cv.gameOver = 1;
    snprintf(cv.resultTitle, sizeof(cv.resultTitle), "%s获胜！", cv.playerColor == 'w' ? "黑方" : "白方");
    strncpy(cv.resultMessage, "你认输了", sizeof(cv.resultMessage) - 1);
    sound_play(SOUND_GAMEOVER);
}

static void do_draw_offer(void) {
    if (cv.gameOver || cv.isComputerTurn) return;
    int score = chess_material_score(&cv.game.state);
    if (score < 0) score = -score;
    int hasQueen = 0;
    for (int r = 0; r < 8 && !hasQueen; r++)
        for (int c = 0; c < 8; c++)
            if (cv.game.state.piece[r][c] == 'q') { hasQueen = 1; break; }
    double drawChance = hasQueen ? (score < 200 ? 0.5 : 0.1) : (score < 100 ? 0.7 : 0.2);
    if (frand() < drawChance) {
        cv.gameOver = 1;
        strncpy(cv.resultTitle, "和棋！", sizeof(cv.resultTitle) - 1);
        strncpy(cv.resultMessage, "电脑接受了求和", sizeof(cv.resultMessage) - 1);
        sound_play(SOUND_GAMEOVER);
    } else {
        strncpy(cv.statusOverride, "电脑拒绝了求和", sizeof(cv.statusOverride) - 1);
        cv.statusOverrideTimer = 2000;
    }
}

static void start_game(void) {
    sound_init();
    ai_init(&cv.ai, cv.difficulty);
    chess_reset(&cv.game);
    cv.selected = 0;
    cv.validMoveCount = 0;
    cv.hasLastMove = 0;
    cv.isComputerTurn = 0;
    cv.gameOver = 0;
    cv.moveRowCount = 0;
    cv.hasPendingPromotion = 0;
    cv.statusOverrideTimer = 0;
    cv.subScreen = 1;
    if (cv.playerColor == 'b') {
        cv.isComputerTurn = 1;
        cv.computerMoveTimer = 500;
    }
}

void chess_view_init(void) {
    memset(&cv, 0, sizeof(cv));
    cv.playerColor = 'w';
    cv.difficulty = 3;
    cv.subScreen = 0;
}

void chess_view_enter(void) {
    cv.subScreen = 0;
}

void chess_view_tick(int dtMs) {
    if (cv.statusOverrideTimer > 0) {
        cv.statusOverrideTimer -= dtMs;
    }
    if (cv.subScreen == 1 && cv.isComputerTurn && !cv.gameOver) {
        cv.computerMoveTimer -= dtMs;
        if (cv.computerMoveTimer <= 0) do_computer_move();
    }
}

void chess_view_handle_action(int action, int data) {
    switch (action) {
        case CV_ACT_SET_COLOR_WHITE: cv.playerColor = 'w'; break;
        case CV_ACT_SET_COLOR_BLACK: cv.playerColor = 'b'; break;
        case CV_ACT_SET_DIFFICULTY: cv.difficulty = data; break;
        case CV_ACT_START_GAME: start_game(); break;
        case CV_ACT_BOARD_SQUARE: on_square_click(data / 8, data % 8); break;
        case CV_ACT_UNDO: do_undo(); break;
        case CV_ACT_RESIGN: do_resign(); break;
        case CV_ACT_DRAW_OFFER: do_draw_offer(); break;
        case CV_ACT_NEW_GAME: cv.subScreen = 0; break;
        case CV_ACT_PROMOTION_CHOICE: {
            static const char pieces[4] = {'q','r','b','n'};
            if (data >= 0 && data < 4) complete_promotion(pieces[data]);
            break;
        }
        case CV_ACT_PLAY_AGAIN: cv.subScreen = 0; break;
    }
}

/* ================= 渲染 ================= */

static void button(SDL_Rect r, const char *label, int active, int enabled, int action, int data) {
    SDL_Color bg = active ? (SDL_Color){0x4e,0xcc,0xa3,40} : COLOR_PANEL2;
    SDL_Color border = active ? COLOR_ACCENT : COLOR_BORDER2;
    ui_fill_rect(r.x, r.y, r.w, r.h, enabled ? bg : COLOR_PANEL2);
    ui_stroke_rect(r.x, r.y, r.w, r.h, 1, enabled ? border : COLOR_BORDER);
    SDL_Color text = !enabled ? COLOR_TEXT_DIM2 : (active ? COLOR_ACCENT : COLOR_TEXT_DIM);
    ui_draw_text_centered(label, r.x + r.w / 2, r.y + r.h / 2 - 9, 16, text);
    if (enabled) ui_hit_add(r, CHESS_ACTION_BASE + action, data);
}

static void render_start(SDL_Rect area) {
    int panelW = 420, panelH = 360;
    int px = area.x + (area.w - panelW) / 2;
    int py = area.y + 30;
    ui_fill_rect(px, py, panelW, panelH, COLOR_PANEL);
    ui_stroke_rect(px, py, panelW, panelH, 1, COLOR_BORDER);

    ui_draw_text_centered("国际象棋对战", px + panelW / 2, py + 24, 24, COLOR_TEXT);
    ui_draw_text_centered("与内置 AI 对弈，无需联网", px + panelW / 2, py + 58, 14, COLOR_TEXT_DIM);

    ui_draw_text("选择执子颜色", px + 40, py + 100, 13, COLOR_TEXT_DIM2);
    button((SDL_Rect){px + 40, py + 124, 150, 40}, "白方", cv.playerColor == 'w', 1, CV_ACT_SET_COLOR_WHITE, 0);
    button((SDL_Rect){px + 210, py + 124, 150, 40}, "黑方", cv.playerColor == 'b', 1, CV_ACT_SET_COLOR_BLACK, 0);

    ui_draw_text("选择难度", px + 40, py + 184, 13, COLOR_TEXT_DIM2);
    button((SDL_Rect){px + 40, py + 208, 108, 40}, "简单", cv.difficulty == 2, 1, CV_ACT_SET_DIFFICULTY, 2);
    button((SDL_Rect){px + 156, py + 208, 108, 40}, "中等", cv.difficulty == 3, 1, CV_ACT_SET_DIFFICULTY, 3);
    button((SDL_Rect){px + 272, py + 208, 108, 40}, "困难", cv.difficulty == 4, 1, CV_ACT_SET_DIFFICULTY, 4);

    SDL_Rect startBtn = { px + 40, py + 280, panelW - 80, 50 };
    ui_fill_rect(startBtn.x, startBtn.y, startBtn.w, startBtn.h, COLOR_ACCENT);
    ui_draw_text_centered("开始对局", startBtn.x + startBtn.w / 2, startBtn.y + 15, 18, (SDL_Color){15,31,26,255});
    ui_hit_add(startBtn, CHESS_ACTION_BASE + CV_ACT_START_GAME, 0);
}

static void render_bar(int x, int y, int w, int h, int isSelf, const char *capturedLetters, int capturedCount, char capturedColor) {
    ui_fill_rect(x, y, w, h, COLOR_PANEL2);
    ui_stroke_rect(x, y, w, h, 1, COLOR_BORDER);
    render_piece_chip(x + 20, y + h / 2, 12, 'k', isSelf ? cv.playerColor : (cv.playerColor == 'w' ? 'b' : 'w'));
    ui_draw_text(isSelf ? "我" : "电脑", x + 40, y + h / 2 - 8, 14, COLOR_TEXT_DIM);
    int cx = x + w - 16;
    for (int i = 0; i < capturedCount && i < 12; i++) {
        render_piece_chip(cx, y + h / 2, 8, capturedLetters[i], capturedColor);
        cx -= 18;
    }
}

static void render_playing(SDL_Rect area) {
    int flip = cv.playerColor == 'b';
    int cell = 52;
    int boardSize = cell * 8;
    int boardX = area.x + 36;
    int boardY = area.y + 84;

    ui_draw_text("国际象棋对战", area.x + 36, area.y + 16, 20, COLOR_TEXT);

    render_bar(boardX, boardY - 40, boardSize, 34, 0,
               cv.game.state.capturedW, cv.game.state.capturedWCount, 'b');
    render_bar(boardX, boardY + boardSize + 6, boardSize, 34, 1,
               cv.game.state.capturedB, cv.game.state.capturedBCount, 'w');

    for (int gr = 0; gr < 8; gr++) {
        for (int gc = 0; gc < 8; gc++) {
            int lr, lc;
            to_display(gr, gc, flip, &lr, &lc);
            int sx = boardX + gc * cell, sy = boardY + gr * cell;
            SDL_Color base = (gr + gc) % 2 == 0 ? (SDL_Color){0xeb,0xec,0xd0,255} : (SDL_Color){0x77,0x95,0x56,255};
            ui_fill_rect(sx, sy, cell, cell, base);

            if (cv.selected && cv.selR == lr && cv.selC == lc) {
                ui_fill_rect(sx, sy, cell, cell, (SDL_Color){0xf5,0xe6,0x63,180});
            }
            if (cv.hasLastMove &&
                ((cv.lastMove.fr == lr && cv.lastMove.fc == lc) || (cv.lastMove.tr == lr && cv.lastMove.tc == lc))) {
                ui_stroke_rect(sx + 1, sy + 1, cell - 2, cell - 2, 2, (SDL_Color){0xaa,0x96,0x28,160});
            }

            char piece = cv.game.state.piece[lr][lc];
            if (piece != 0) render_piece_chip(sx + cell / 2, sy + cell / 2, cell / 2 - 6, piece, cv.game.state.color[lr][lc]);

            int isValidDest = 0, isCapture = 0;
            for (int i = 0; i < cv.validMoveCount; i++) {
                if (cv.validMoves[i].tr == lr && cv.validMoves[i].tc == lc) {
                    isValidDest = 1;
                    isCapture = piece != 0;
                    break;
                }
            }
            if (isValidDest) {
                if (isCapture) ui_stroke_circle(sx + cell / 2, sy + cell / 2, cell / 2 - 4, (SDL_Color){0xd2,0x32,0x32,200});
                else ui_fill_circle(sx + cell / 2, sy + cell / 2, cell / 6, (SDL_Color){0,0,0,70});
            }

            if (piece == 'k' && cv.game.state.color[lr][lc] == cv.game.state.turn && chess_in_check(&cv.game.state, cv.game.state.turn)) {
                ui_stroke_circle(sx + cell / 2, sy + cell / 2, cell / 2 - 3, (SDL_Color){0xd2,0x32,0x32,220});
            }

            SDL_Rect sqRect = { sx, sy, cell, cell };
            ui_hit_add(sqRect, CHESS_ACTION_BASE + CV_ACT_BOARD_SQUARE, lr * 8 + lc);
        }
    }
    ui_stroke_rect(boardX - 2, boardY - 2, boardSize + 4, boardSize + 4, 2, COLOR_BORDER2);

    /* 着法记录面板 */
    int panelX = boardX + boardSize + 16;
    int panelY = boardY - 40;
    int panelW = area.w - (panelX - area.x) - 20;
    if (panelW < 140) panelW = 140;
    if (panelW > 220) panelW = 220;
    int panelH = boardSize + 80;
    ui_fill_rect(panelX, panelY, panelW, panelH, COLOR_PANEL2);
    ui_stroke_rect(panelX, panelY, panelW, panelH, 1, COLOR_BORDER);
    ui_draw_text("着法记录", panelX + 10, panelY + 8, 13, COLOR_TEXT_DIM2);
    int rowH = 20;
    int maxRows = (panelH - 36) / rowH;
    int startRow = cv.moveRowCount > maxRows ? cv.moveRowCount - maxRows : 0;
    int ry = panelY + 34;
    char numbuf[8];
    for (int i = startRow; i < cv.moveRowCount; i++) {
        snprintf(numbuf, sizeof(numbuf), "%d.", i + 1);
        ui_draw_text(numbuf, panelX + 8, ry, 13, COLOR_TEXT_DIM2);
        ui_draw_text(cv.moveRows[i].white, panelX + 34, ry, 13, COLOR_TEXT);
        ui_draw_text(cv.moveRows[i].black, panelX + (panelW / 2) + 6, ry, 13, COLOR_TEXT_DIM);
        ry += rowH;
    }

    /* 状态与控制按钮 */
    int ctrlY = boardY + boardSize + 6 + 34 + 14;
    const char *status;
    char statusBuf[80];
    if (cv.statusOverrideTimer > 0) {
        status = cv.statusOverride;
    } else {
        int inCheck = chess_in_check(&cv.game.state, cv.game.state.turn);
        snprintf(statusBuf, sizeof(statusBuf), "%s走棋%s", cv.game.state.turn == 'w' ? "白方" : "黑方", inCheck ? " — 将军！" : "");
        status = statusBuf;
    }
    ui_draw_text(status, boardX, ctrlY + 10, 15, COLOR_ACCENT);

    int btnW = 84, btnH = 32, gap = 8;
    int bx = boardX + boardSize - (btnW * 4 + gap * 3);
    int canUndo = !cv.gameOver && !cv.isComputerTurn && cv.game.historyCount >= 2;
    int canAct = !cv.gameOver && !cv.isComputerTurn;
    button((SDL_Rect){bx, ctrlY, btnW, btnH}, "求和", 0, canAct, CV_ACT_DRAW_OFFER, 0);
    button((SDL_Rect){bx + (btnW + gap), ctrlY, btnW, btnH}, "悔棋", 0, canUndo, CV_ACT_UNDO, 0);
    button((SDL_Rect){bx + (btnW + gap) * 2, ctrlY, btnW, btnH}, "认输", 0, !cv.gameOver, CV_ACT_RESIGN, 0);
    button((SDL_Rect){bx + (btnW + gap) * 3, ctrlY, btnW, btnH}, "新对局", 0, 1, CV_ACT_NEW_GAME, 0);

    if (cv.hasPendingPromotion) {
        ui_fill_rect(area.x, area.y, area.w, area.h, (SDL_Color){0,0,0,160});
        int mw = 320, mh = 160;
        int mx = area.x + (area.w - mw) / 2, my = area.y + (area.h - mh) / 2;
        ui_fill_rect(mx, my, mw, mh, COLOR_PANEL);
        ui_stroke_rect(mx, my, mw, mh, 1, COLOR_BORDER2);
        ui_draw_text_centered("选择升变棋子", mx + mw / 2, my + 16, 16, COLOR_TEXT);
        static const char letters[4] = {'q','r','b','n'};
        int cxStart = mx + 40;
        for (int i = 0; i < 4; i++) {
            SDL_Rect chip = { cxStart + i * 62 - 24, my + 60, 48, 48 };
            ui_fill_rect(chip.x, chip.y, chip.w, chip.h, COLOR_PANEL2);
            ui_stroke_rect(chip.x, chip.y, chip.w, chip.h, 1, COLOR_BORDER2);
            render_piece_chip(chip.x + 24, chip.y + 24, 18, letters[i], cv.playerColor);
            ui_hit_add(chip, CHESS_ACTION_BASE + CV_ACT_PROMOTION_CHOICE, i);
        }
    }

    if (cv.gameOver) {
        ui_fill_rect(area.x, area.y, area.w, area.h, (SDL_Color){0,0,0,170});
        int mw = 360, mh = 180;
        int mx = area.x + (area.w - mw) / 2, my = area.y + (area.h - mh) / 2;
        ui_fill_rect(mx, my, mw, mh, COLOR_PANEL);
        ui_stroke_rect(mx, my, mw, mh, 1, COLOR_BORDER2);
        ui_draw_text_centered(cv.resultTitle, mx + mw / 2, my + 30, 24, COLOR_TEXT);
        ui_draw_text_centered(cv.resultMessage, mx + mw / 2, my + 72, 15, COLOR_TEXT_DIM);
        SDL_Rect again = { mx + mw / 2 - 90, my + 116, 180, 44 };
        ui_fill_rect(again.x, again.y, again.w, again.h, COLOR_ACCENT);
        ui_draw_text_centered("再来一局", again.x + again.w / 2, again.y + 13, 16, (SDL_Color){15,31,26,255});
        ui_hit_add(again, CHESS_ACTION_BASE + CV_ACT_PLAY_AGAIN, 0);
    }
}

void chess_view_render(SDL_Rect area) {
    if (cv.subScreen == 0) render_start(area);
    else render_playing(area);
}
