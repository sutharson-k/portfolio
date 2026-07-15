/* 国际象棋规则引擎：棋盘表示、走法生成、将军/将死判定、易位、吃过路兵、
   兵的升变、FEN 局面串、代数记谱法。移植自原 public/chess.js。 */
#include <string.h>
#include <ctype.h>
#include "chess.h"

static void push_move(Move out[], int *count, Move m) {
    if (*count < MAX_MOVES) out[(*count)++] = m;
}

int chess_in_bounds(int r, int c) {
    return r >= 0 && r < 8 && c >= 0 && c < 8;
}

static int clear_path(const ChessState *s, int r1, int c1, int r2, int c2) {
    int dr = (r2 > r1) - (r2 < r1);
    int dc = (c2 > c1) - (c2 < c1);
    int r = r1 + dr, c = c1 + dc;
    while (r != r2 || c != c2) {
        if (s->piece[r][c] != 0) return 0;
        r += dr;
        c += dc;
    }
    return 1;
}

static int can_attack(const ChessState *s, int fr, int fc, int tr, int tc) {
    char piece = s->piece[fr][fc];
    int dr = tr - fr, dc = tc - fc;
    int adr = dr < 0 ? -dr : dr;
    int adc = dc < 0 ? -dc : dc;

    switch (piece) {
        case 'p': {
            int dir = s->color[fr][fc] == 'w' ? -1 : 1;
            return dr == dir && adc == 1;
        }
        case 'n':
            return (adr == 2 && adc == 1) || (adr == 1 && adc == 2);
        case 'b':
            return adr == adc && adr != 0 && clear_path(s, fr, fc, tr, tc);
        case 'r':
            return (dr == 0 || dc == 0) && !(dr == 0 && dc == 0) && clear_path(s, fr, fc, tr, tc);
        case 'q':
            return ((adr == adc && adr != 0) || ((dr == 0) != (dc == 0))) && clear_path(s, fr, fc, tr, tc);
        case 'k':
            return adr <= 1 && adc <= 1 && !(dr == 0 && dc == 0);
    }
    return 0;
}

int chess_is_attacked(const ChessState *s, int r, int c, char byColor) {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (s->piece[row][col] == 0 || s->color[row][col] != byColor) continue;
            if (can_attack(s, row, col, r, c)) return 1;
        }
    }
    return 0;
}

int chess_find_king(const ChessState *s, char color, int *kr, int *kc) {
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            if (s->piece[r][c] == 'k' && s->color[r][c] == color) {
                *kr = r; *kc = c;
                return 1;
            }
    return 0;
}

int chess_in_check(const ChessState *s, char color) {
    int kr, kc;
    if (!chess_find_king(s, color, &kr, &kc)) return 0;
    return chess_is_attacked(s, kr, kc, color == 'w' ? 'b' : 'w');
}

int chess_get_pseudo_moves(const ChessState *s, int r, int c, Move out[MAX_MOVES]) {
    int count = 0;
    char piece = s->piece[r][c];
    if (piece == 0) return 0;
    char color = s->color[r][c];

    if (piece == 'p') {
        int dir = color == 'w' ? -1 : 1;
        int startRow = color == 'w' ? 6 : 1;
        int promoRow = color == 'w' ? 0 : 7;

        if (chess_in_bounds(r + dir, c) && s->piece[r + dir][c] == 0) {
            Move m = {0}; m.fr = r; m.fc = c; m.tr = r + dir; m.tc = c; m.piece = 'p'; m.color = color;
            if (r + dir == promoRow) m.promotion = 'q';
            push_move(out, &count, m);
            if (r == startRow && s->piece[r + 2 * dir][c] == 0) {
                Move m2 = {0}; m2.fr = r; m2.fc = c; m2.tr = r + 2 * dir; m2.tc = c;
                m2.piece = 'p'; m2.color = color; m2.doublePush = 1;
                push_move(out, &count, m2);
            }
        }
        for (int dc = -1; dc <= 1; dc += 2) {
            int tc = c + dc;
            if (!chess_in_bounds(r + dir, tc)) continue;
            char target = s->piece[r + dir][tc];
            if (target != 0 && s->color[r + dir][tc] != color) {
                Move m = {0}; m.fr = r; m.fc = c; m.tr = r + dir; m.tc = tc;
                m.piece = 'p'; m.color = color; m.capture = target;
                if (r + dir == promoRow) m.promotion = 'q';
                push_move(out, &count, m);
            }
            if (s->epRow == r + dir && s->epCol == tc) {
                Move m = {0}; m.fr = r; m.fc = c; m.tr = r + dir; m.tc = tc;
                m.piece = 'p'; m.color = color; m.capture = 'p'; m.enPassant = 1;
                push_move(out, &count, m);
            }
        }
    } else if (piece == 'n') {
        static const int deltas[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
        for (int i = 0; i < 8; i++) {
            int tr = r + deltas[i][0], tc = c + deltas[i][1];
            if (!chess_in_bounds(tr, tc)) continue;
            char target = s->piece[tr][tc];
            if (target == 0 || s->color[tr][tc] != color) {
                Move m = {0}; m.fr = r; m.fc = c; m.tr = tr; m.tc = tc;
                m.piece = 'n'; m.color = color; m.capture = target;
                push_move(out, &count, m);
            }
        }
    } else if (piece == 'b' || piece == 'r' || piece == 'q') {
        static const int bishopDirs[4][2] = {{-1,-1},{-1,1},{1,-1},{1,1}};
        static const int rookDirs[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
        static const int queenDirs[8][2] = {{-1,-1},{-1,1},{1,-1},{1,1},{-1,0},{1,0},{0,-1},{0,1}};
        const int (*dirs)[2] = piece == 'b' ? bishopDirs : piece == 'r' ? rookDirs : queenDirs;
        int ndirs = piece == 'q' ? 8 : 4;
        for (int i = 0; i < ndirs; i++) {
            int tr = r + dirs[i][0], tc = c + dirs[i][1];
            while (chess_in_bounds(tr, tc)) {
                char target = s->piece[tr][tc];
                if (target == 0) {
                    Move m = {0}; m.fr = r; m.fc = c; m.tr = tr; m.tc = tc; m.piece = piece; m.color = color;
                    push_move(out, &count, m);
                } else {
                    if (s->color[tr][tc] != color) {
                        Move m = {0}; m.fr = r; m.fc = c; m.tr = tr; m.tc = tc; m.piece = piece; m.color = color;
                        m.capture = target;
                        push_move(out, &count, m);
                    }
                    break;
                }
                tr += dirs[i][0];
                tc += dirs[i][1];
            }
        }
    } else if (piece == 'k') {
        static const int deltas[8][2] = {{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};
        for (int i = 0; i < 8; i++) {
            int tr = r + deltas[i][0], tc = c + deltas[i][1];
            if (!chess_in_bounds(tr, tc)) continue;
            char target = s->piece[tr][tc];
            if (target == 0 || s->color[tr][tc] != color) {
                Move m = {0}; m.fr = r; m.fc = c; m.tr = tr; m.tc = tc;
                m.piece = 'k'; m.color = color; m.capture = target;
                push_move(out, &count, m);
            }
        }
        if (!chess_in_check(s, color)) {
            int row = color == 'w' ? 7 : 0;
            char opp = color == 'w' ? 'b' : 'w';
            int kOk = color == 'w' ? s->castleWK : s->castleBK;
            int qOk = color == 'w' ? s->castleWQ : s->castleBQ;
            if (kOk && s->piece[row][5] == 0 && s->piece[row][6] == 0 &&
                !chess_is_attacked(s, row, 5, opp) && !chess_is_attacked(s, row, 6, opp)) {
                Move m = {0}; m.fr = r; m.fc = c; m.tr = row; m.tc = 6; m.piece = 'k'; m.color = color; m.castle = 'k';
                push_move(out, &count, m);
            }
            if (qOk && s->piece[row][3] == 0 && s->piece[row][2] == 0 && s->piece[row][1] == 0 &&
                !chess_is_attacked(s, row, 3, opp) && !chess_is_attacked(s, row, 2, opp)) {
                Move m = {0}; m.fr = r; m.fc = c; m.tr = row; m.tc = 2; m.piece = 'k'; m.color = color; m.castle = 'q';
                push_move(out, &count, m);
            }
        }
    }
    return count;
}

int chess_get_legal_moves(const ChessState *s, int r, int c, Move out[MAX_MOVES]) {
    if (s->piece[r][c] == 0 || s->color[r][c] != s->turn) return 0;
    char color = s->color[r][c];
    Move pseudo[MAX_MOVES];
    int n = chess_get_pseudo_moves(s, r, c, pseudo);
    int count = 0;
    for (int i = 0; i < n; i++) {
        ChessState test = *s;
        chess_make_move_raw(&test, &pseudo[i]);
        if (!chess_in_check(&test, color)) push_move(out, &count, pseudo[i]);
    }
    return count;
}

int chess_get_all_legal_moves(const ChessState *s, char color, Move out[MAX_MOVES]) {
    int count = 0;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            if (s->piece[r][c] != 0 && s->color[r][c] == color) {
                Move moves[MAX_MOVES];
                int n = chess_get_legal_moves(s, r, c, moves);
                for (int i = 0; i < n && count < MAX_MOVES; i++) out[count++] = moves[i];
            }
        }
    }
    return count;
}

void chess_make_move_raw(ChessState *s, const Move *move) {
    int fr = move->fr, fc = move->fc, tr = move->tr, tc = move->tc;
    char piece = s->piece[fr][fc];
    char color = s->color[fr][fc];
    if (piece == 0) return;

    if (move->capture) {
        char (*capturedArr)[MAX_CAPTURED] = color == 'w' ? &s->capturedW : &s->capturedB;
        int *capturedCount = color == 'w' ? &s->capturedWCount : &s->capturedBCount;
        if (*capturedCount < MAX_CAPTURED) (*capturedArr)[(*capturedCount)++] = move->capture;
    }

    s->piece[tr][tc] = piece;
    s->color[tr][tc] = color;
    s->piece[fr][fc] = 0;

    if (move->promotion) {
        s->piece[tr][tc] = move->promotion;
    }
    if (move->enPassant) {
        int capR = color == 'w' ? tr + 1 : tr - 1;
        char capPiece = s->piece[capR][tc];
        char (*capturedArr)[MAX_CAPTURED] = color == 'w' ? &s->capturedW : &s->capturedB;
        int *capturedCount = color == 'w' ? &s->capturedWCount : &s->capturedBCount;
        if (*capturedCount < MAX_CAPTURED) (*capturedArr)[(*capturedCount)++] = capPiece;
        s->piece[capR][tc] = 0;
    }
    if (move->castle) {
        int row = color == 'w' ? 7 : 0;
        if (move->castle == 'k') {
            s->piece[row][5] = s->piece[row][7]; s->color[row][5] = s->color[row][7];
            s->piece[row][7] = 0;
        } else {
            s->piece[row][3] = s->piece[row][0]; s->color[row][3] = s->color[row][0];
            s->piece[row][0] = 0;
        }
    }
    if (move->doublePush) {
        s->epRow = color == 'w' ? tr + 1 : tr - 1;
        s->epCol = tc;
    } else {
        s->epRow = -1;
        s->epCol = -1;
    }

    if (piece == 'k') {
        if (color == 'w') { s->castleWK = 0; s->castleWQ = 0; }
        else { s->castleBK = 0; s->castleBQ = 0; }
    }
    if (piece == 'r') {
        int row = color == 'w' ? 7 : 0;
        if (fr == row && fc == 0) { if (color == 'w') s->castleWQ = 0; else s->castleBQ = 0; }
        if (fr == row && fc == 7) { if (color == 'w') s->castleWK = 0; else s->castleBK = 0; }
    }

    s->turn = s->turn == 'w' ? 'b' : 'w';
    s->halfMoves++;
    if (s->turn == 'w') s->fullMoves++;
}

int chess_find_move(const ChessState *s, int fr, int fc, int tr, int tc, char promotion, Move *out) {
    Move legal[MAX_MOVES];
    int n = chess_get_legal_moves(s, fr, fc, legal);
    for (int i = 0; i < n; i++) {
        if (legal[i].tr == tr && legal[i].tc == tc) {
            *out = legal[i];
            if (out->promotion && promotion) out->promotion = promotion;
            return 1;
        }
    }
    return 0;
}

static void push_history(Chess *g) {
    if (g->historyCount < MAX_HISTORY) {
        g->history[g->historyCount] = g->state;
        g->historyCount++;
    }
}

void chess_record_position(Chess *g) {
    if (g->posHistoryCount < MAX_HISTORY) {
        chess_to_fen_board(&g->state, g->posHistory[g->posHistoryCount]);
        g->posHistoryCount++;
    }
}

int chess_make_move(Chess *g, int fr, int fc, int tr, int tc, char promotion) {
    Move move;
    if (!chess_find_move(&g->state, fr, fc, tr, tc, promotion, &move)) return 0;
    push_history(g);
    chess_make_move_raw(&g->state, &move);
    chess_record_position(g);
    return 1;
}

int chess_apply_move(Chess *g, Move m) {
    push_history(g);
    chess_make_move_raw(&g->state, &m);
    chess_record_position(g);
    return 1;
}

void chess_to_fen_board(const ChessState *s, char out[80]) {
    int pos = 0;
    for (int r = 0; r < 8; r++) {
        int empty = 0;
        for (int c = 0; c < 8; c++) {
            char piece = s->piece[r][c];
            if (piece == 0) {
                empty++;
            } else {
                if (empty > 0) { out[pos++] = (char)('0' + empty); empty = 0; }
                out[pos++] = s->color[r][c] == 'w' ? (char)toupper(piece) : piece;
            }
        }
        if (empty > 0) out[pos++] = (char)('0' + empty);
        if (r < 7) out[pos++] = '/';
    }
    out[pos] = 0;
}

int chess_is_checkmate(const ChessState *s, char color) {
    Move moves[MAX_MOVES];
    return chess_in_check(s, color) && chess_get_all_legal_moves(s, color, moves) == 0;
}

int chess_is_stalemate(const ChessState *s, char color) {
    Move moves[MAX_MOVES];
    return !chess_in_check(s, color) && chess_get_all_legal_moves(s, color, moves) == 0;
}

int chess_is_draw_material_or_50(const ChessState *s) {
    if (s->halfMoves >= 100) return 1;
    int count = 0, hasMinor = 0;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            char piece = s->piece[r][c];
            if (piece == 0) continue;
            count++;
            if (piece == 'b' || piece == 'n') hasMinor = 1;
        }
    }
    if (count == 2) return 1;
    if (count == 3 && hasMinor) return 1;
    return 0;
}

int chess_is_draw_full(const Chess *g) {
    if (chess_is_draw_material_or_50(&g->state)) return 1;
    char currentFen[80];
    chess_to_fen_board(&g->state, currentFen);
    int matches = 0;
    for (int i = 0; i < g->posHistoryCount; i++) {
        if (strcmp(g->posHistory[i], currentFen) == 0) {
            matches++;
            if (matches >= 3) return 1;
        }
    }
    return 0;
}

static char piece_letter(char piece) {
    switch (piece) {
        case 'k': return 'K';
        case 'q': return 'Q';
        case 'r': return 'R';
        case 'b': return 'B';
        case 'n': return 'N';
        default: return 0;
    }
}

void chess_to_algebraic(const ChessState *before, const Move *move, char out[16]) {
    if (move->castle) {
        strcpy(out, move->castle == 'k' ? "O-O" : "O-O-O");
        return;
    }

    static const char files[] = "abcdefgh";
    static const char ranks[] = "87654321";
    int pos = 0;
    int fr = move->fr, fc = move->fc, tr = move->tr, tc = move->tc;

    if (move->piece != 'p') {
        char letter = piece_letter(move->piece);
        if (letter) out[pos++] = letter;

        Move allMoves[MAX_MOVES];
        int n = chess_get_all_legal_moves(before, move->color, allMoves);
        int sameFile = 0, sameRank = 0, hasDisambig = 0;
        for (int i = 0; i < n; i++) {
            Move *m = &allMoves[i];
            if (m->piece == move->piece && m->tr == tr && m->tc == tc &&
                (m->fr != fr || m->fc != fc)) {
                hasDisambig = 1;
                if (m->fc == fc) sameFile = 1;
                if (m->fr == fr) sameRank = 1;
            }
        }
        if (hasDisambig) {
            if (!sameFile) {
                out[pos++] = files[fc];
            } else if (!sameRank) {
                out[pos++] = ranks[fr];
            } else {
                out[pos++] = files[fc];
                out[pos++] = ranks[fr];
            }
        }
    } else if (move->capture) {
        out[pos++] = files[fc];
    }

    if (move->capture) out[pos++] = 'x';

    out[pos++] = files[tc];
    out[pos++] = ranks[tr];

    if (move->promotion) {
        out[pos++] = '=';
        out[pos++] = piece_letter(move->promotion);
    }

    ChessState after = *before;
    chess_make_move_raw(&after, move);
    char opp = move->color == 'w' ? 'b' : 'w';
    if (chess_is_checkmate(&after, opp)) {
        out[pos++] = '#';
    } else if (chess_in_check(&after, opp)) {
        out[pos++] = '+';
    }

    out[pos] = 0;
}

int chess_undo(Chess *g) {
    if (g->historyCount == 0) return 0;
    g->historyCount--;
    g->state = g->history[g->historyCount];
    /* posHistory only ever grows in step with real moves in this build's usage
       pattern (one record per applied move), so it mirrors historyCount 1:1
       after the initial position; drop the same number of trailing entries. */
    if (g->posHistoryCount > 0) g->posHistoryCount--;
    return 1;
}

int chess_material_score(const ChessState *s) {
    static const int values[128] = { ['p']=100, ['n']=320, ['b']=330, ['r']=500, ['q']=900, ['k']=0 };
    int score = 0;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            char piece = s->piece[r][c];
            if (piece == 0) continue;
            int v = values[(int)piece];
            score += s->color[r][c] == 'w' ? v : -v;
        }
    }
    return score;
}

void chess_reset(Chess *g) {
    static const char backRank[8] = {'r','n','b','q','k','b','n','r'};
    memset(g, 0, sizeof(*g));
    ChessState *s = &g->state;
    for (int c = 0; c < 8; c++) {
        s->piece[0][c] = backRank[c]; s->color[0][c] = 'b';
        s->piece[1][c] = 'p';         s->color[1][c] = 'b';
        s->piece[6][c] = 'p';         s->color[6][c] = 'w';
        s->piece[7][c] = backRank[c]; s->color[7][c] = 'w';
    }
    s->turn = 'w';
    s->castleWK = s->castleWQ = s->castleBK = s->castleBQ = 1;
    s->epRow = -1; s->epCol = -1;
    s->halfMoves = 0;
    s->fullMoves = 1;
    g->historyCount = 0;
    g->posHistoryCount = 0;
    chess_record_position(g);
}
