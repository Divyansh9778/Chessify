#include "NetworkMessages.h"

#include <sstream>

std::string serialize(const MoveMessage& move)
{
    std::ostringstream out;

    out << move.fromRow << ' '
        << move.fromCol << ' '
        << move.toRow << ' '
        << move.toCol << ' '
        << move.promotion;

    return out.str();
}

MoveMessage deserialize(const std::string& text)
{
    MoveMessage move;

    std::istringstream in(text);

    in >> move.fromRow
        >> move.fromCol
        >> move.toRow
        >> move.toCol
        >> move.promotion;

    return move;
}