#ifndef MOVERECORD_H
#define MOVERECORD_H

#include "Move.h"   // Needed for MoveType

struct MoveRecord
{
    int fromRow, fromCol;
    int toRow, toCol;

    char movedPiece;        // 'P', 'N', 'B', 'R', 'Q', 'K'
    char capturedPiece;     // '.' if none

    MoveType type;          // Normal, Capture, Castle, etc.
    char promotionPiece = '.';    // 'Q', 'R', 'B', 'N' or '.' if none
    
    bool givesCheck = false;
    bool givesMate = false;

    MoveRecord(int fr, int fc, int tr, int tc,
        char moved, char captured,
        MoveType t,
        char promo = 0,
        bool check = false,
        bool mate = false)
        : fromRow(fr), fromCol(fc),
        toRow(tr), toCol(tc),
        movedPiece(moved),
        capturedPiece(captured),
        type(t),
        promotionPiece(promo),
        givesCheck(check),
        givesMate(mate)
    {
    }
};

#endif