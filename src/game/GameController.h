#pragma once

#include "../Board.h"
#include "../MoveHistory.h"
#include "../network/NetworkClient.h"

class GameController
{
public:
    GameController(Board &board,
                   MoveHistory &history,
                   NetworkClient &network);

    bool playMove(SDL_Renderer *renderer,
                  Piece *piece,
                  int mouseX,
                  int mouseY,
                  char promotion = 0);

    bool applyRemoteMove(const MoveMessage& move);

private:
    Board &board;
    MoveHistory &history;
    NetworkClient &network;
};