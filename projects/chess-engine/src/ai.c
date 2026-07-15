/* 国际象棋 AI：极小极大搜索 + alpha-beta 剪枝 + 静态搜索(quiescence) +
   开局库 + 走法排序。移植自原 public/ai.js。 */
#include <stdlib.h>
#include <string.h>
#include "ai.h"

#define AI_INF 1000000000

static const int PIECE_VALUES[128] = { ['p']=100, ['n']=320, ['b']=330, ['r']=500, ['q']=900, ['k']=20000 };

static const int PST_P[8][8] = {
    {0,  0,  0,  0,  0,  0,  0,  0},
    {50, 50, 50, 50, 50, 50, 50, 50},
    {10, 10, 20, 30, 30, 20, 10, 10},
    {5,  5, 10, 25, 25, 10,  5,  5},
    {0,  0,  0, 20, 20,  0,  0,  0},
    {5, -5,-10,  0,  0,-10, -5,  5},
    {5, 10, 10,-20,-20, 10, 10,  5},
    {0,  0,  0,  0,  0,  0,  0,  0}
};
static const int PST_N[8][8] = {
    {-50,-40,-30,-30,-30,-30,-40,-50},
    {-40,-20,  0,  0,  0,  0,-20,-40},
    {-30,  0, 10, 15, 15, 10,  0,-30},
    {-30,  5, 15, 20, 20, 15,  5,-30},
    {-30,  0, 15, 20, 20, 15,  0,-30},
    {-30,  5, 10, 15, 15, 10,  5,-30},
    {-40,-20,  0,  5,  5,  0,-20,-40},
    {-50,-40,-30,-30,-30,-30,-40,-50}
};
static const int PST_B[8][8] = {
    {-20,-10,-10,-10,-10,-10,-10,-20},
    {-10,  0,  0,  0,  0,  0,  0,-10},
    {-10,  0, 10, 10, 10, 10,  0,-10},
    {-10,  5,  5, 10, 10,  5,  5,-10},
    {-10,  0,  5, 10, 10,  5,  0,-10},
    {-10, 10, 10, 10, 10, 10, 10,-10},
    {-10,  5,  0,  0,  0,  0,  5,-10},
    {-20,-10,-10,-10,-10,-10,-10,-20}
};
static const int PST_R[8][8] = {
    {0,  0,  0,  0,  0,  0,  0,  0},
    {5, 10, 10, 10, 10, 10, 10,  5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {0,  0,  0,  5,  5,  0,  0,  0}
};
static const int PST_Q[8][8] = {
    {-20,-10,-10, -5, -5,-10,-10,-20},
    {-10,  0,  0,  0,  0,  0,  0,-10},
    {-10,  0,  5,  5,  5,  5,  0,-10},
    {-5,  0,  5,  5,  5,  5,  0, -5},
    {0,  0,  5,  5,  5,  5,  0, -5},
    {-10,  5,  5,  5,  5,  5,  0,-10},
    {-10,  0,  5,  0,  0,  0,  0,-10},
    {-20,-10,-10, -5, -5,-10,-10,-20}
};
static const int PST_K[8][8] = {
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-20,-30,-30,-40,-40,-30,-30,-20},
    {-10,-20,-20,-20,-20,-20,-20,-10},
    {20, 20,  0,  0,  0,  0, 20, 20},
    {20, 30, 10,  0,  0, 10, 30, 20}
};

typedef struct {
    const char *fen;
    const char *moves[6];
    int count;
} BookEntry;

static const BookEntry OPENING_BOOK[] = {
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", {"e2e4","d2d4","c2c4","g1f3"}, 4},
    {"rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR", {"e7e5","c7c5","e7e6","c7c6"}, 4},
    {"rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR", {"d7d5","g8f6","e7e6"}, 3},
    {"rnbqkbnr/pppppppp/8/8/2P5/8/PP1PPPPP/RNBQKBNR", {"e7e5","g8f6","c7c5"}, 3},
    {"rnbqkbnr/pppppppp/8/8/8/5N2/PPPPPPPP/RNBQKB1R", {"d7d5","g8f6","e7e5"}, 3},
    {"rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR", {"b1c3","g1f3","f2f4"}, 3},
    {"rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R", {"b8c6","d7d6","e7e6"}, 3},
    {"rnbqkbnr/pppp1ppp/4p3/8/4P3/8/PPPP1PPP/RNBQKBNR", {"d2d4","g1f3","b1c3"}, 3},
    {"rnbqkbnr/pppp1ppp/4p3/8/4P3/5N2/PPPP1PPP/RNBQKB1R", {"d7d5","b8c6","f8b4"}, 3},
    {"rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR", {"e4d5","b1c3","e4e5"}, 3},
    {"rnbqkbnr/ppp1pppp/8/3p4/4P3/5N2/PPPP1PPP/RNBQKB1R", {"e4d5","d2d4","b1c3"}, 3},
};
#define OPENING_BOOK_COUNT (int)(sizeof(OPENING_BOOK) / sizeof(OPENING_BOOK[0]))

static double frand(void) { return rand() / (double)RAND_MAX; }

static int pst_value(char piece, char color, int r, int c) {
    const int (*table)[8] = NULL;
    switch (piece) {
        case 'p': table = PST_P; break;
        case 'n': table = PST_N; break;
        case 'b': table = PST_B; break;
        case 'r': table = PST_R; break;
        case 'q': table = PST_Q; break;
        case 'k': table = PST_K; break;
        default: return 0;
    }
    return color == 'w' ? table[r][c] : table[7 - r][c];
}

static int evaluate(const ChessState *s) {
    int score = 0;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            char piece = s->piece[r][c];
            if (piece == 0) continue;
            char color = s->color[r][c];
            int total = PIECE_VALUES[(int)piece] + pst_value(piece, color, r, c);
            score += color == 'w' ? total : -total;
        }
    }
    Move buf[MAX_MOVES];
    int whiteMoves = chess_get_all_legal_moves(s, 'w', buf);
    int blackMoves = chess_get_all_legal_moves(s, 'b', buf);
    score += (whiteMoves - blackMoves) * 5;
    return s->turn == 'w' ? score : -score;
}

static int cmp_captures_desc(const void *a, const void *b) {
    const Move *ma = (const Move *)a, *mb = (const Move *)b;
    int va = ma->capture ? PIECE_VALUES[(int)ma->capture] : 0;
    int vb = mb->capture ? PIECE_VALUES[(int)mb->capture] : 0;
    return vb - va;
}

static int quiescence(const ChessState *s, int alpha, int beta, int isMaximizing, int depth) {
    int standPat = evaluate(s);
    if (depth > 6) return standPat;

    if (isMaximizing) {
        if (standPat >= beta) return beta;
        if (standPat > alpha) alpha = standPat;
    } else {
        if (standPat <= alpha) return alpha;
        if (standPat < beta) beta = standPat;
    }

    Move moves[MAX_MOVES];
    int n = chess_get_all_legal_moves(s, s->turn, moves);
    Move captures[MAX_MOVES];
    int nc = 0;
    for (int i = 0; i < n; i++) if (moves[i].capture) captures[nc++] = moves[i];
    qsort(captures, nc, sizeof(Move), cmp_captures_desc);

    if (isMaximizing) {
        for (int i = 0; i < nc; i++) {
            ChessState test = *s;
            chess_make_move_raw(&test, &captures[i]);
            int score = quiescence(&test, alpha, beta, 0, depth + 1);
            if (score >= beta) return beta;
            if (score > alpha) alpha = score;
        }
        return alpha;
    } else {
        for (int i = 0; i < nc; i++) {
            ChessState test = *s;
            chess_make_move_raw(&test, &captures[i]);
            int score = quiescence(&test, alpha, beta, 1, depth + 1);
            if (score <= alpha) return alpha;
            if (score < beta) beta = score;
        }
        return beta;
    }
}

static int minimax(const ChessState *s, int depth, int alpha, int beta, int isMaximizing, char aiColor, int maxDepth) {
    if (depth == 0) return quiescence(s, alpha, beta, isMaximizing, 0);

    Move moves[MAX_MOVES];
    int n = chess_get_all_legal_moves(s, s->turn, moves);
    if (n == 0) {
        if (chess_in_check(s, s->turn)) {
            return s->turn == aiColor ? -100000 + (maxDepth - depth) : 100000 - (maxDepth - depth);
        }
        return 0;
    }
    if (chess_is_draw_material_or_50(s)) return 0;

    if (isMaximizing) {
        int maxEval = -AI_INF;
        for (int i = 0; i < n; i++) {
            ChessState test = *s;
            chess_make_move_raw(&test, &moves[i]);
            int ev = minimax(&test, depth - 1, alpha, beta, 0, aiColor, maxDepth);
            if (ev > maxEval) maxEval = ev;
            if (ev > alpha) alpha = ev;
            if (beta <= alpha) break;
        }
        return maxEval;
    } else {
        int minEval = AI_INF;
        for (int i = 0; i < n; i++) {
            ChessState test = *s;
            chess_make_move_raw(&test, &moves[i]);
            int ev = minimax(&test, depth - 1, alpha, beta, 1, aiColor, maxDepth);
            if (ev < minEval) minEval = ev;
            if (ev < beta) beta = ev;
            if (beta <= alpha) break;
        }
        return minEval;
    }
}

void ai_init(ChessAI *ai, int depth) {
    ai->maxDepth = depth;
}

static int cmp_move_order(const void *a, const void *b) {
    const Move *ma = (const Move *)a, *mb = (const Move *)b;
    int sa = 0, sb = 0;
    if (ma->capture) sa += PIECE_VALUES[(int)ma->capture] * 10 - PIECE_VALUES[(int)ma->piece];
    if (mb->capture) sb += PIECE_VALUES[(int)mb->capture] * 10 - PIECE_VALUES[(int)mb->piece];
    if (ma->promotion) sa += 800;
    if (mb->promotion) sb += 800;
    return sb - sa;
}

static int try_book_move(const ChessState *s, Move *out) {
    char fen[80];
    chess_to_fen_board(s, fen);
    for (int i = 0; i < OPENING_BOOK_COUNT; i++) {
        if (strcmp(OPENING_BOOK[i].fen, fen) != 0) continue;
        const BookEntry *entry = &OPENING_BOOK[i];
        const char *pick = entry->moves[(int)(frand() * entry->count)];
        int fromC = pick[0] - 'a';
        int fromR = 8 - (pick[1] - '0');
        int toC = pick[2] - 'a';
        int toR = 8 - (pick[3] - '0');
        Move legal[MAX_MOVES];
        int n = chess_get_all_legal_moves(s, s->turn, legal);
        for (int j = 0; j < n; j++) {
            if (legal[j].fr == fromR && legal[j].fc == fromC &&
                legal[j].tr == toR && legal[j].tc == toC) {
                *out = legal[j];
                return 1;
            }
        }
        return 0;
    }
    return 0;
}

int ai_get_best_move(ChessAI *ai, const Chess *game, Move *out) {
    const ChessState *s = &game->state;

    if (try_book_move(s, out)) return 1;

    Move moves[MAX_MOVES];
    int n = chess_get_all_legal_moves(s, s->turn, moves);
    if (n == 0) return 0;

    double randomFactor = ai->maxDepth <= 2 ? 0.2 : ai->maxDepth <= 3 ? 0.1 : 0.04;
    char color = s->turn;

    for (int i = n - 1; i > 0; i--) {
        int j = (int)(frand() * (i + 1));
        Move tmp = moves[i]; moves[i] = moves[j]; moves[j] = tmp;
    }
    qsort(moves, n, sizeof(Move), cmp_move_order);

    Move bestMove = moves[0];
    double bestScore = -1e18;
    for (int i = 0; i < n; i++) {
        ChessState test = *s;
        chess_make_move_raw(&test, &moves[i]);
        int score = minimax(&test, ai->maxDepth - 1, -AI_INF, AI_INF, 0, color, ai->maxDepth);
        double adjustedScore = score + (frand() * 50.0 - 25.0) * randomFactor;
        if (adjustedScore > bestScore) {
            bestScore = adjustedScore;
            bestMove = moves[i];
        }
    }
    *out = bestMove;
    return 1;
}
