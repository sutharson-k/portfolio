#ifndef CHESS_H
#define CHESS_H

#define MAX_MOVES 256
#define MAX_HISTORY 600
#define MAX_CAPTURED 16

typedef struct {
    int fr, fc, tr, tc;
    char piece;       /* 'p','n','b','r','q','k' */
    char color;       /* 'w' or 'b' */
    char capture;     /* 0 if none, else captured piece type */
    char promotion;   /* 0 if none, else promoted-to piece type */
    int enPassant;    /* 1 if this move is an en-passant capture */
    int doublePush;   /* 1 if pawn double push */
    char castle;      /* 0 none, 'k' kingside, 'q' queenside */
} Move;

typedef struct {
    char piece[8][8];   /* 0 = empty square */
    char color[8][8];   /* 'w' or 'b', undefined if piece==0 */
    char turn;          /* 'w' or 'b' */
    int castleWK, castleWQ, castleBK, castleBQ;
    int epRow, epCol;   /* -1,-1 if no en-passant target */
    int halfMoves;
    int fullMoves;
    char capturedW[MAX_CAPTURED]; int capturedWCount;
    char capturedB[MAX_CAPTURED]; int capturedBCount;
} ChessState;

typedef struct {
    ChessState state;
    ChessState history[MAX_HISTORY];
    int historyCount;
    char posHistory[MAX_HISTORY][80];
    int posHistoryCount;
} Chess;

void chess_reset(Chess *g);
int chess_in_bounds(int r, int c);
int chess_is_attacked(const ChessState *s, int r, int c, char byColor);
int chess_in_check(const ChessState *s, char color);
int chess_get_pseudo_moves(const ChessState *s, int r, int c, Move out[MAX_MOVES]);
int chess_get_legal_moves(const ChessState *s, int r, int c, Move out[MAX_MOVES]);
int chess_get_all_legal_moves(const ChessState *s, char color, Move out[MAX_MOVES]);
void chess_make_move_raw(ChessState *s, const Move *m);
int chess_make_move(Chess *g, int fr, int fc, int tr, int tc, char promotion);
int chess_apply_move(Chess *g, Move m);
void chess_record_position(Chess *g);
void chess_to_fen_board(const ChessState *s, char out[80]);
int chess_find_king(const ChessState *s, char color, int *kr, int *kc);
int chess_is_checkmate(const ChessState *s, char color);
int chess_is_stalemate(const ChessState *s, char color);
int chess_is_draw_material_or_50(const ChessState *s);
int chess_is_draw_full(const Chess *g);
void chess_to_algebraic(const ChessState *before, const Move *m, char out[16]);
int chess_undo(Chess *g);
int chess_material_score(const ChessState *s);
int chess_find_move(const ChessState *s, int fr, int fc, int tr, int tc, char promotion, Move *out);

#endif
