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

//bool GameController::playMove(SDL_Renderer* renderer,
//    Piece* piece,
//    int mouseX,
//    int mouseY,
//    char promotion)
//{
//    // -----------------------------
//    // Multiplayer validation
//    // -----------------------------
//    if (network.isConnected())
//    {
//        // Server has not assigned WHITE/BLACK yet
//        if (!network.isReady())
//            return false;
//
//        // Not our turn
//        if (!network.isMyTurn())
//            return false;
//
//        // Do not allow the opponent's pieces
//        if (piece)
//        {
//            PlayerColor myColor = network.getPlayerColor();
//
//            if (myColor == PlayerColor::White && !piece->isWhite)
//                return false;
//
//            if (myColor == PlayerColor::Black && piece->isWhite)
//                return false;
//        }
//    }
//
//    size_t oldSize = history.size();
//
//    board.movePiece(
//        renderer,
//        piece,
//        mouseX,
//        mouseY,
//        history,
//        promotion
//    );
//
//    // ----------------------------------------
//    // Normal move OR completed promotion
//    // ----------------------------------------
//    bool movePlayed =
//        (history.size() > oldSize) ||
//        board.hasFinishedPromotionMove();
//
//    if (movePlayed)
//    {
//        // Make sure a move actually exists
//        if (!history.getMoves().empty())
//        {
//            const MoveRecord& lastMove =
//                history.getMoves().back();
//
//            if (network.isConnected())
//            {
//                network.sendMove(lastMove);
//            }
//        }
//
//        board.clearPromotionFinishedFlag();
//    }
//
//    return movePlayed;
//}

bool GameController::playMove(
    SDL_Renderer* renderer,
    Piece* piece,
    int mouseX,
    int mouseY,
    char promotion)
{
    if (network.isConnected() &&
        !network.isMyTurn())
    {
        return false;
    }

    size_t oldSize =
        history.size();

    board.movePiece(
        renderer,
        piece,
        mouseX,
        mouseY,
        history,
        promotion
    );

    bool movePlayed =
        (history.size() > oldSize) ||
        board.hasFinishedPromotionMove();

    if (movePlayed)
    {
        if (network.isConnected())
        {
            const MoveRecord& lastMove =
                history.getMoves().back();

            network.sendMove(lastMove);
        }

        board.clearPromotionFinishedFlag();
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