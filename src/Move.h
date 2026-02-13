#ifndef MOVE_H
#define MOVE_H

#include "Constants.h"
#include "Piece.h"

#include <string>

enum class MoveType
{
    Illegal,
    Normal,
    Capture,
    EnPassant,
    CastleKing,
    CastleQueen,
    Promotion
};

class Move
{
public:
    static std::string squareName(int r, int c);
    static bool isValidMove(Piece* piece, int newRow, int newCol, int oldRow, int oldCol, Piece* board[BOARD_SIZE][BOARD_SIZE]);
    static bool isPathClear(int oldRow, int oldCol, int newRow, int newCol, Piece* board[BOARD_SIZE][BOARD_SIZE]);
    static bool canCastle(Piece* king, int newRow, int newCol, Piece* board[BOARD_SIZE][BOARD_SIZE]);
    static MoveType getLastMoveType();
    static bool leavesKingInCheck(Piece* piece, int newRow, int newCol, int oldRow, int oldCol, Piece* board[BOARD_SIZE][BOARD_SIZE]);
    static bool isSquareAttacked(int row, int col, bool byWhite, Piece* board[BOARD_SIZE][BOARD_SIZE]);
};

#endif
