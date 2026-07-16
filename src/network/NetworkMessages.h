#pragma once
#include <string>

struct MoveMessage
{
    int fromRow;
    int fromCol;

    int toRow;
    int toCol;

    char promotion = '.';

    MoveMessage() = default;

    MoveMessage(int fr, int fc,
        int tr, int tc,
        char promo = '.')
        : fromRow(fr),
        fromCol(fc),
        toRow(tr),
        toCol(tc),
        promotion(promo)
    {}
};

std::string serialize(const MoveMessage& move);
MoveMessage deserialize(const std::string& text);