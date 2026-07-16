#include "MoveRecord.h"
#include "GameController.h"

GameController::GameController(Board &b,
                               MoveHistory &h,
                               NetworkClient &n)
    : board(b),
      history(h),
      network(n)
{
}

bool GameController::playMove(SDL_Renderer *renderer,
                              Piece *piece,
                              int mouseX,
                              int mouseY,
                              char promotion)
{
    if (!network.isMyTurn())
        return false;

    size_t oldSize = history.size();

    board.movePiece(renderer,
                    piece,
                    mouseX,
                    mouseY,
                    history,
                    promotion);

    bool movePlayed = (history.size() > oldSize);
    if (movePlayed)
    {
        const MoveRecord& lastMove = history.getMoves().back();
        network.sendMove(lastMove);

        // Multiplayer will use this.
        // Replay will use this.
        // PGN export will use this.
    }

    return movePlayed;
}

void GameController::applyRemoteMove(const MoveMessage& move)
{
    Piece* piece = board.getPieceAt(move.fromRow, move.fromCol);

    if (!piece)
        return;

    board.executeMove(
        piece,
        move.toRow,
        move.toCol,
        history,
        move.promotion
    );
}