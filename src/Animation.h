#ifndef ANIMATION_H
#define ANIMATION_H

#include "Piece.h"
#include "Board.h"
#include <SDL3/SDL.h>

class Animation {
public:
    static void animateMove(Piece* piece, int newX, int newY, SDL_Renderer* renderer, Board* board);
};

#endif // ANIMATION_H
