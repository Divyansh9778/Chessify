#include "MoveHistory.h"
#include "Move.h"

std::string MoveHistory::formatMove(const MoveRecord& m) const
{
    if (m.type == MoveType::CastleKing)
        return m.givesMate ? "O-O#" : m.givesCheck ? "O-O+" : "O-O";

    if (m.type == MoveType::CastleQueen)
        return m.givesMate ? "O-O-O#" : m.givesCheck ? "O-O-O+" : "O-O-O";

    std::string move;
    bool isPawn = (tolower(m.movedPiece) == 'p');
    bool isCapture = (m.capturedPiece != '.');

    if (isPawn)
    {
        if (isCapture)
        {
            move += char('a' + m.fromCol);
            move += "x";
        }

        move += Move::squareName(m.toRow, m.toCol);

        if (m.type == MoveType::Promotion)
        {
            move += "=";
            move += toupper(m.promotionPiece);
        }
    }
    else
    {
        move += toupper(m.movedPiece);

        if (isCapture)
            move += "x";

        move += Move::squareName(m.toRow, m.toCol);
    }

    if (m.givesMate)
        move += "#";
    else if (m.givesCheck)
        move += "+";

    return move;
}