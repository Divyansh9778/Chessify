#ifndef MOVEHISTORY_H
#define MOVEHISTORY_H

#include <vector>
#include <iostream>
#include "MoveRecord.h"
#include "Move.h"

class MoveHistory
{
private:
    std::vector<MoveRecord> moves;

public:
    void addMove(const MoveRecord& move)
    {
        moves.push_back(move);
    }

    void printMoves() const
    {
        std::cout << "\n===== Move History =====\n";

        if (moves.empty())
        {
            std::cout << "No moves played yet.\n";
            return;
        }

        for (size_t i = 0; i < moves.size(); ++i)
        {
            const MoveRecord& m = moves[i];

            // Print move number only for white
            if (i % 2 == 0)
                std::cout << (i / 2 + 1) << ". ";

            std::string moveStr;

            switch (m.type)
            {
            case MoveType::CastleKing:
                moveStr = "O-O";
                break;

            case MoveType::CastleQueen:
                moveStr = "O-O-O";
                break;

            case MoveType::Promotion:
            {
                moveStr = Move::squareName(m.toRow, m.toCol);
                moveStr += "=" + m.promotionPiece;
                break;
            }

            case MoveType::Capture:
            {
                moveStr =
                    Move::squareName(m.fromRow, m.fromCol) +
                    "x" +
                    Move::squareName(m.toRow, m.toCol);
                break;
            }

            case MoveType::EnPassant:
            {
                moveStr =
                    Move::squareName(m.fromRow, m.fromCol) +
                    "x" +
                    Move::squareName(m.toRow, m.toCol) +
                    " e.p.";
                break;
            }

            default: // Normal
            {
                moveStr =
                    Move::squareName(m.fromRow, m.fromCol) +
                    Move::squareName(m.toRow, m.toCol);
                break;
            }
            }

            std::cout << moveStr << " ";

            if (i % 2 == 1)
                std::cout << "\n";
        }

        std::cout << "\n";
    }

    void undoLastMove()
    {
        if (!moves.empty())
            moves.pop_back();
    }

    bool empty() const
    {
        return moves.empty();
    }

    size_t size() const
    {
        return moves.size();
    }

    std::string formatMove(const MoveRecord& m) const;

    const std::vector<MoveRecord>& getMoves() const
    {
        return moves;
    }
};

#endif