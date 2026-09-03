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

bool GameController::playMove(
    SDL_Renderer* renderer,
    Piece* piece,
    int mouseX,
    int mouseY,
    char promotion)
{
    std::cout
        << "[GameController] playMove: "
        << "connected=" << network.isConnected()
        << " myTurn=" << network.isMyTurn()
        << '\n';

    if (network.isConnected())
    {
        // Server has not assigned a color yet
        if (!network.isReady())
            return false;

        // Not our turn
        if (!network.isMyTurn())
            return false;

        // Prevent moving the opponent's pieces
        if (piece)
        {
            PlayerColor myColor =
                network.getPlayerColor();

            if (myColor == PlayerColor::White &&
                !piece->isWhite)
                return false;

            if (myColor == PlayerColor::Black &&
                piece->isWhite)
                return false;
        }
    }

    std::cout
        << "[GameController] Trying piece: "
        << (piece ? piece->type : "NULL")
        << '\n';

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
            if (history.size() <= oldSize)
            {
                std::cout
                    << "[GameController] ERROR: move played "
                    "but history was not updated!\n";

                return false;
            }

            const MoveRecord& lastMove =
                history.getMoves().back();

            std::cout
                << "[GameController] Sending latest move: "
                << lastMove.fromRow << " "
                << lastMove.fromCol << " "
                << lastMove.toRow << " "
                << lastMove.toCol << " "
                << lastMove.promotionPiece
                << '\n';

            network.sendMove(lastMove);
        }

        board.clearPromotionFinishedFlag();
    }

    return movePlayed;
}

bool GameController::applyRemoteMove(
    const MoveMessage& move)
{
    std::cout
        << "[GameController] Applying remote move: "
        << move.fromRow << " "
        << move.fromCol << " "
        << move.toRow << " "
        << move.toCol
        << '\n';

    Piece* piece =
        board.getPieceAt(
            move.fromRow,
            move.fromCol);

    if (!piece)
    {
        std::cout
            << "[GameController] Remote move failed: "
            << "no piece at "
            << move.fromRow << ","
            << move.fromCol
            << '\n';

        return false;
    }

    std::cout
        << "[GameController] Remote piece: "
        << piece->type
        << " color="
        << (piece->isWhite ? "WHITE" : "BLACK")
        << '\n';

    size_t oldSize =
        history.size();

    board.executeMove(
        piece,
        move.toRow,
        move.toCol,
        history,
        move.promotion
    );

    if (history.size() == oldSize)
    {
        std::cout
            << "[GameController] ERROR: "
            << "executeMove() did not add move to history\n";

        return false;
    }

    network.setMyTurn(true);

    std::cout
        << "[GameController] Remote move applied successfully\n";

    std::cout
        << "[GameController] My turn = "
        << network.isMyTurn()
        << '\n';

    return true;
}