#ifndef AI_H
#define AI_H

#include "chess.h"

typedef struct {
    int maxDepth;
} ChessAI;

void ai_init(ChessAI *ai, int depth);
/* 返回 1 并写出 *out 表示找到走法；返回 0 表示无棋可走（被将死/逼和）。 */
int ai_get_best_move(ChessAI *ai, const Chess *game, Move *out);

#endif
