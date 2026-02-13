#include "Constants.h"
#include "Piece.h"
#include "Move.h"
#include "Board.h"

#include <utility>
#include <cmath>
#include <string>

const int knightMoves[8][2] = {
    {2, 1}, {2, -1}, {-2, 1}, {-2, -1}, {1, 2}, {1, -2}, {-1, 2}, {-1, -2} };

const int dirs[8][2] = {
    {1, 0}, {-1, 0}, {0, 1}, {0, -1}, // Rook dirs
    {1, 1},
    {1, -1},
    {-1, 1},
    {-1, -1} // Bishop dirs
};

struct MoveResult
{
    MoveType type = MoveType::Illegal;
    bool legal = false;
};
MoveType lastMoveType = MoveType::Illegal;

MoveType Move::getLastMoveType()
{
    return lastMoveType;
}

std::string Move::squareName(int r, int c)
{
    char file = 'a' + c;
    char rank = '1' + (7 - r);
    return std::string() + file + rank;
}

bool Move::isValidMove(Piece* piece, int newRow, int newCol, int oldRow, int oldCol, Piece* board[BOARD_SIZE][BOARD_SIZE])
{
    if (!piece)
        return false;

    // Check if the destination is occupied by a piece of the same color
    if (board[newRow][newCol] && board[newRow][newCol]->isWhite == piece->isWhite)
        return false;

    int rowDiff = abs(newRow - oldRow);
    int colDiff = abs(newCol - oldCol);
    std::string type = piece->type;
    lastMoveType = MoveType::Illegal;

    // Pawn
    if (type == "wp" || type == "bp")
    {
        int direction = piece->isWhite ? -1 : 1;
        int startRow = piece->isWhite ? 6 : 1;

        // Move
        if (newCol == oldCol)
        {
            // Regular move
            if (newRow == oldRow + direction)
            {
                if (!board[newRow][newCol])
                {
                    lastMoveType = MoveType::Normal;
                    if (leavesKingInCheck(piece, newRow, newCol, oldRow, oldCol, board))
                        return false;
                    return true;
                }
            }

            // Double move
            if (newRow == oldRow + 2 * direction && oldRow == startRow)
            {
                if (!board[oldRow + direction][newCol] && !board[newRow][newCol])
                {
                    lastMoveType = MoveType::Normal;
                    if (leavesKingInCheck(piece, newRow, newCol, oldRow, oldCol, board))
                        return false;
                    return true;
                }
            }
        }

        // Capture
        else if (colDiff == 1 && newRow == oldRow + direction)
        {
            // Regular capture
            if (board[newRow][newCol] && board[newRow][newCol]->isWhite != piece->isWhite)
            {
                lastMoveType = MoveType::Capture;
                if (leavesKingInCheck(piece, newRow, newCol, oldRow, oldCol, board))
                    return false;
                return true;
            }

            // En-passant capture
            Piece* lastMoved = Board::getLastMoved();
            if (lastMoved && lastMoved->enPassantEligible && lastMoved->type == (piece->isWhite ? "bp" : "wp"))
            {
                int lastRow = lastMoved->yPos / SQUARE_SIZE;
                int lastCol = lastMoved->xPos / SQUARE_SIZE;

                // Must be horizontally adjacent and on the same rank
                if (oldRow == lastRow && newCol == lastCol)
                {

                    // Your rank check (white on row 3, black on row 4)
                    if ((piece->isWhite && oldRow == 3) ||
                        (!piece->isWhite && oldRow == 4))
                    {
                        if (!board[newRow][newCol])
                        {
                            lastMoveType = MoveType::EnPassant;
                            if (leavesKingInCheck(piece, newRow, newCol, oldRow, oldCol, board))
                                return false;
                            return true;
                        }
                    }
                }
            }

            return false;
        }
    }

    // Rook
    else if (type == "wr" || type == "br")
    {
        if (oldRow == newRow || oldCol == newCol)
        {
            if (Move::isPathClear(oldRow, oldCol, newRow, newCol, board))
            {
                lastMoveType = (board[newRow][newCol] ? MoveType::Capture : MoveType::Normal);
                if (leavesKingInCheck(piece, newRow, newCol, oldRow, oldCol, board))
                    return false;
                return true;
            }
        }
        return false;
    }

    // Bishop
    else if (type == "wb" || type == "bb")
    {
        if (rowDiff == colDiff)
        {
            if (Move::isPathClear(oldRow, oldCol, newRow, newCol, board))
            {
                lastMoveType = (board[newRow][newCol] ? MoveType::Capture : MoveType::Normal);
                if (leavesKingInCheck(piece, newRow, newCol, oldRow, oldCol, board))
                    return false;
                return true;
            }
        }
        return false;
    }

    // Queen
    else if (type == "wq" || type == "bq")
    {
        if (oldRow == newRow || oldCol == newCol || rowDiff == colDiff)
        {
            if (Move::isPathClear(oldRow, oldCol, newRow, newCol, board))
            {
                lastMoveType = (board[newRow][newCol] ? MoveType::Capture : MoveType::Normal);
                if (leavesKingInCheck(piece, newRow, newCol, oldRow, oldCol, board))
                    return false;
                return true;
            }
        }
        return false;
    }

    // Knight
    else if (type == "wn" || type == "bn")
    {
        if ((rowDiff == 2 && colDiff == 1) || (rowDiff == 1 && colDiff == 2))
        {
            lastMoveType = (board[newRow][newCol] ? MoveType::Capture : MoveType::Normal);
            if (leavesKingInCheck(piece, newRow, newCol, oldRow, oldCol, board))
                return false;
            return true;
        }
        return false;
    }

    // King
    else if (type == "wk" || type == "bk")
    {
        if (rowDiff <= 1 && colDiff <= 1)
        {
            Piece* captured = board[newRow][newCol];

            board[oldRow][oldCol] = nullptr;
            board[newRow][newCol] = piece;

            int oldX = piece->xPos, oldY = piece->yPos;
            piece->xPos = newCol * SQUARE_SIZE;
            piece->yPos = newRow * SQUARE_SIZE;

            bool squareAttacked = Move::isSquareAttacked(newRow, newCol, !piece->isWhite, board);
            bool inCheck = Board::isKingInCheck(piece->isWhite, board);

            board[oldRow][oldCol] = piece;
            board[newRow][newCol] = captured;
            piece->xPos = oldX;
            piece->yPos = oldY;

            if (!squareAttacked && !inCheck)
            {
                lastMoveType = (captured ? MoveType::Capture : MoveType::Normal);
                return true;
            }
        }

        // Castling Logic
        else if (colDiff == 2 && oldRow == newRow)
        {
            if (Move::canCastle(piece, newRow, newCol, board))
            {
                lastMoveType = (newCol == 6 ? MoveType::CastleKing : MoveType::CastleQueen);
                return true;
            }
        }

        return false;
    }

    return false;
}

bool Board::isKingInCheck(bool isWhite, Piece* board[BOARD_SIZE][BOARD_SIZE])
{
    int kingRow = -1, kingCol = -1;

    // Find the king's position
    for (int r = 0; r < BOARD_SIZE; r++)
    {
        for (int c = 0; c < BOARD_SIZE; c++)
        {
            Piece* piece = board[r][c];
            if (piece && piece->isWhite == isWhite && piece->type == (isWhite ? "wk" : "bk"))
            {
                kingRow = r;
                kingCol = c;
                break;
            }
        }
        if (kingRow != -1)
            break;
    }

    bool attacked = Move::isSquareAttacked(kingRow, kingCol, !isWhite, board);
    return attacked;
}

bool Move::isSquareAttacked(int row, int col, bool byWhite, Piece* board[BOARD_SIZE][BOARD_SIZE])
{
    // Knight
    for (auto& m : knightMoves)
    {
        int r = row + m[0], c = col + m[1];
        if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE)
            continue;

        Piece* p = board[r][c];
        if (p && p->isWhite == byWhite && (p->type == "wn" || p->type == "bn"))
            return true;
    }

    // Pawn
    int dir = byWhite ? 1 : -1;
    int pr = row + dir;

    if (pr >= 0 && pr < BOARD_SIZE)
    {
        for (int dc : {-1, 1})
        {
            int pc = col + dc;
            if (pc < 0 || pc >= BOARD_SIZE)
                continue;

            Piece* p = board[pr][pc];
            if (p && p->isWhite == byWhite &&
                ((byWhite && p->type == "wp") || (!byWhite && p->type == "bp")))
            {
                return true;
            }
        }
    }

    // Rook, Bishop, Queen
    for (auto& d : dirs)
    {
        int r = row + d[0], c = col + d[1];

        while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE)
        {
            Piece* p = board[r][c];

            if (p)
            {
                if (p->isWhite == byWhite)
                {
                    // Rook or Queen
                    if ((d[0] == 0 || d[1] == 0) &&
                        (p->type == "wr" || p->type == "br" ||
                            p->type == "wq" || p->type == "bq"))
                        return true;

                    // Bishop or Queen
                    if ((d[0] != 0 && d[1] != 0) &&
                        (p->type == "wb" || p->type == "bb" ||
                            p->type == "wq" || p->type == "bq"))
                        return true;
                }
                break; // Blocked by any piece
            }

            r += d[0];
            c += d[1];
        }
    }

    // King
    for (auto& d : dirs)
    {
        int r = row + d[0], c = col + d[1];
        if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE)
            continue;

        Piece* p = board[r][c];
        if (p && p->isWhite == byWhite && (p->type == "wk" || p->type == "bk"))
            return true;
    }

    return false; // No attackers found
}

bool Move::isPathClear(int oldRow, int oldCol, int newRow, int newCol, Piece* board[BOARD_SIZE][BOARD_SIZE])
{
    int rowStep = (newRow > oldRow) ? 1 : (newRow < oldRow) ? -1
        : 0;
    int colStep = (newCol > oldCol) ? 1 : (newCol < oldCol) ? -1
        : 0;

    int r = oldRow + rowStep, c = oldCol + colStep;
    while (r != newRow || c != newCol)
    {
        if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE)
            return false;
        if (board[r][c])
            return false;
        r += rowStep;
        c += colStep;
    }
    return true;
}

bool Move::canCastle(Piece* king, int newRow, int newCol, Piece* board[BOARD_SIZE][BOARD_SIZE])
{
    if (king->hasMoved)
        return false; // King must not have moved

    int row = (king->isWhite ? 7 : 0);
    int rookCol = (newCol == 6) ? 7 : 0; // Kingside (6) or Queenside (2)
    Piece* rook = board[row][rookCol];

    if (!rook || rook->type != (king->isWhite ? "wr" : "br") || rook->hasMoved)
        return false; // Rook must not have moved

    int start = (rookCol == 7 ? 5 : 3);
    int end = (rookCol == 7 ? 6 : 2);
    if (start > end)
        std::swap(start, end);

    for (int c = start; c <= end; c++)
    {
        if (board[row][c] != nullptr)
            return false;
    }

    // Ensure king doesnt move through check
    int step = (newCol == 6) ? 1 : -1;
    int col = 4;

    while (col != newCol)
    {
        int testCol = col + step;

        board[row][col] = nullptr;
        board[row][testCol] = king;

        if (Board::isKingInCheck(king->isWhite, board))
        {
            board[row][testCol] = nullptr;
            board[row][col] = king;
            return false;
        }

        board[row][testCol] = nullptr;
        board[row][col] = king;
        col = testCol;
    }

    return true;
}

bool Move::leavesKingInCheck(Piece* piece, int newRow, int newCol, int oldRow, int oldCol, Piece* board[BOARD_SIZE][BOARD_SIZE])
{
    Piece* temp = board[newRow][newCol];

    board[oldRow][oldCol] = nullptr;
    board[newRow][newCol] = piece;

    int oldX = piece->xPos, oldY = piece->yPos;
    piece->xPos = newCol * SQUARE_SIZE;
    piece->yPos = newRow * SQUARE_SIZE;

    bool stillInCheck = Board::isKingInCheck(piece->isWhite, board);

    board[oldRow][oldCol] = piece;
    board[newRow][newCol] = temp;
    piece->xPos = oldX;
    piece->yPos = oldY;

    return stillInCheck;
}
