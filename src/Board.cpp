#include "Constants.h"
#include "Settings.h"
#include "Piece.h"
#include "Move.h"
#include "MoveHistory.h"
#include "MoveRecord.h"
#include "Board.h"

#include <SDL3/SDL_blendmode.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <vector>
#include <iostream>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <string>
#include <string.h>
#include <utility>
#include <cctype>

Piece* Board::lastMovedPiece = nullptr;
int lastFromRow = -1;
int lastFromCol = -1;
int lastToRow = -1;
int lastToCol = -1;

std::unordered_map<std::string, SDL_Texture*> Board::pieceTextures;
Board* Board::instance = nullptr;

Board::Board()
{
    instance = this;

    // Initialize board array with nullptrs
    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            board[row][col] = nullptr;
        }
    }

    // Initialize the board with all pieces in correct positions
    initializePieces();

    // Place pieces in the board array
    for (Piece* piece : pieces)
    {
        if (!piece)
            continue;

        int row = piece->yPos / SQUARE_SIZE;
        int col = piece->xPos / SQUARE_SIZE;

        if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE)
            continue;

        board[row][col] = piece;
    }
}

Board::~Board()
{
    for (auto& pair : pieceTextures)
        SDL_DestroyTexture(pair.second);
    pieceTextures.clear();

    for (Piece* piece : pieces)
        delete piece;
    pieces.clear();
}

int Board::pieceValue(const std::string& type)
{
    char p = type[1];
    switch (p)
    {
    case 'p': return 1;
    case 'n': return 3;
    case 'b': return 3;
    case 'r': return 5;
    case 'q': return 9;
    default: return 0;
    }
}

void Board::loadTextures(SDL_Renderer* renderer)
{
    std::vector<std::string> pieceNames = {
        "wp", "wr", "wn", "wb", "wq", "wk",
        "bp", "br", "bn", "bb", "bq", "bk"};

    for (const std::string& name : pieceNames)
    {
        std::string path = "assets/pieces/" + name + ".bmp";
        SDL_Surface* surface = SDL_LoadBMP(path.c_str());
        if (!surface)
        {
            std::cerr << "Failed to load image: " << path << " Error: " << SDL_GetError() << std::endl;
            continue;
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);

        if (!texture)
        {
            std::cerr << "Failed to create texture from " << path << " Error: " << SDL_GetError() << std::endl;
            continue;
        }

        pieceTextures[name] = texture;
    }
}

void Board::resetBoardState()
{
    for (Piece* p : pieces)
        delete p;

    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++)
            board[r][c] = nullptr;

    initializePieces();

    for (Piece* piece : pieces)
    {
        if (!piece) continue;

        int row = piece->yPos / SQUARE_SIZE;
        int col = piece->xPos / SQUARE_SIZE;

        if (row >= 0 && row < BOARD_SIZE &&
            col >= 0 && col < BOARD_SIZE)
            board[row][col] = piece;
    }

    isWhiteTurn = true;
    whiteCaptured.clear();
    blackCaptured.clear();
    whiteMaterial = 0;
    blackMaterial = 0;

    gameOver = false;
    stalemate = false;
    losingKing = nullptr;
    promotionActive = false;
    promoPawn = nullptr;

    lastMovedPiece = nullptr;
    lastFromRow = -1;
    lastFromCol = -1;
    lastToRow = -1;
    lastToCol = -1;
}

void Board::replayTo(MoveHistory& history, int index)
{
    resetBoardState();

    for (int i = 0; i < index; i++)
    {
        const MoveRecord& m = history.getMoves()[i];

        Piece* p = board[m.fromRow][m.fromCol];
        if (!p) continue;

        // simulate without adding to history
        simulateMoveFromRecord(p, m);

        // update last move highlight
        lastFromRow = m.fromRow;
        lastFromCol = m.fromCol;
        lastToRow = m.toRow;
        lastToCol = m.toCol;
    }

    currentMoveIndex = index;

    if (index == 0)
    {
        lastFromRow = -1;
        lastFromCol = -1;
        lastToRow = -1;
        lastToCol = -1;
    }

    viewingHistory = (index != history.size());

    // recompute game state after replay
    gameOver = false;
    stalemate = false;
    losingKing = nullptr;

    if (Board::isCheckmate(isWhiteTurn))
    {
        gameOver = true;
        whiteLost = isWhiteTurn;

        std::string kingType = whiteLost ? "wk" : "bk";

        for (Piece* p : pieces)
        {
            if (p->type == kingType)
            {
                losingKing = p;
                break;
            }
        }
    }
    else if (Board::isStalemate(isWhiteTurn))
    {
        gameOver = true;
        stalemate = true;
    }
}

void Board::simulateMoveFromRecord(Piece* piece, const MoveRecord& m)
{
    Piece* target = board[m.toRow][m.toCol];
    if (target)
    {
        std::string type = target->type;

        if (target->isWhite)
        {
            whiteCaptured[type]++;
            blackMaterial += pieceValue(type);
        }
        else
        {
            blackCaptured[type]++;
            whiteMaterial += pieceValue(type);
        }

        auto it = std::find(pieces.begin(), pieces.end(), target);
        if (it != pieces.end())
            pieces.erase(it);

        delete target;
        board[m.toRow][m.toCol] = nullptr;
    }

    board[m.fromRow][m.fromCol] = nullptr;

    switch (m.type)
    {
    case MoveType::Normal:
    case MoveType::Capture:
    {
        piece->setPosition(
            m.toCol * SQUARE_SIZE,
            m.toRow * SQUARE_SIZE);

        board[m.toRow][m.toCol] = piece;
    }
    break;

    case MoveType::EnPassant:
    {
        int capturedRow = piece->isWhite ? m.toRow + 1 : m.toRow - 1;
        Piece* pawn = board[capturedRow][m.toCol];

        if (pawn)
        {
            std::string type = pawn->type;

            if (pawn->isWhite)
            {
                whiteCaptured[type]++;
                blackMaterial += pieceValue(type);
            }
            else
            {
                blackCaptured[type]++;
                whiteMaterial += pieceValue(type);
            }

            auto it = std::find(pieces.begin(), pieces.end(), pawn);
            if (it != pieces.end())
            {
                pieces.erase(it);
                delete pawn;
            }

            board[capturedRow][m.toCol] = nullptr;
        }

        piece->setPosition(
            m.toCol * SQUARE_SIZE,
            m.toRow * SQUARE_SIZE);

        board[m.toRow][m.toCol] = piece;
    }
    break;

    case MoveType::CastleKing:
    {
        int row = m.fromRow;
        Piece* rook = board[row][7];

        board[row][7] = nullptr;
        board[row][5] = rook;

        if (rook)
            rook->setPosition(5 * SQUARE_SIZE, row * SQUARE_SIZE);

        piece->setPosition(6 * SQUARE_SIZE, row * SQUARE_SIZE);

        board[row][6] = piece;
    }
    break;

    case MoveType::CastleQueen:
    {
        int row = m.fromRow;
        Piece* rook = board[row][0];

        board[row][0] = nullptr;
        board[row][3] = rook;

        if (rook)
            rook->setPosition(3 * SQUARE_SIZE, row * SQUARE_SIZE);

        piece->setPosition(2 * SQUARE_SIZE, row * SQUARE_SIZE);

        board[row][2] = piece;
    }
    break;

    case MoveType::Promotion:
    {
        bool isWhite = piece->isWhite;

        auto it = std::find(pieces.begin(), pieces.end(), piece);
        if (it != pieces.end())
            pieces.erase(it);
        delete piece;

        std::string newType =
            std::string(1, isWhite ? 'w' : 'b') +
            (char)tolower(m.promotionPiece);

        Piece* promoted = new Piece(
            newType,
            m.toCol * SQUARE_SIZE,
            m.toRow * SQUARE_SIZE,
            isWhiteTurn,
            this
        );

        pieces.push_back(promoted);
        board[m.toRow][m.toCol] = promoted;

        int pawnValue = 1;
        int newValue = pieceValue(newType);
        int gain = newValue - pawnValue;

        if (isWhiteTurn)
            whiteMaterial += gain;
        else
            blackMaterial += gain;
    }
    break;

    default:
        break;
    }

    isWhiteTurn = !isWhiteTurn;
}

void Board::stepBackward(MoveHistory& history)
{
    if (currentMoveIndex <= 0)
        return;

    replayTo(history, currentMoveIndex - 1);
}

void Board::stepForward(MoveHistory& history)
{
    if (currentMoveIndex >= history.size())
        return;

    replayTo(history, currentMoveIndex + 1);
}
        
void Board::drawBoard(SDL_Renderer* renderer, TTF_Font* font)
{
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);          // Set background color
    SDL_FRect borderRect = {
        BOARD_OFFSET_X,
        BOARD_OFFSET_Y,
        BOARD_WIDTH + 2 * BORDER_WIDTH,
        BOARD_WIDTH + 2 * BORDER_WIDTH
    };

    SDL_RenderFillRect(renderer, &borderRect);                  // Fill background

    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            SDL_FRect square = {
                BORDER_WIDTH_X + col * SQUARE_SIZE,
                BORDER_WIDTH_Y + row * SQUARE_SIZE,
                SQUARE_SIZE,
                SQUARE_SIZE
            };

            if ((row + col) % 2 == 0)
                SDL_SetRenderDrawColor(renderer, 251, 233, 167, 255);
            else
                SDL_SetRenderDrawColor(renderer, 21, 95, 78, 255);

            SDL_RenderFillRect(renderer, &square);

            // highlight last move
            if ((row == lastFromRow && col == lastFromCol) ||
                (row == lastToRow && col == lastToCol))
            {
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 255, 214, 60, 90);
                SDL_RenderFillRect(renderer, &square);
            }
        }
    }

    // Highlight king if in check
    highlightCheckedKing(renderer);

    for (Piece* piece : pieces)
    {
        if (!piece)
            continue;

        if (gameOver)
        {
            bool isWhiteKing = (piece->type == "wk");
            bool isBlackKing = (piece->type == "bk");

            if ((whiteLost && isWhiteKing) || (!whiteLost && isBlackKing))
                continue;
        }

        piece->drawPiece(renderer, pieceTextures);
    }

    if (promotionActive && promoPawn)
        drawPromotionMenu(renderer);

    drawCaptured(renderer, font);
}

void Board::initializePieces()
{
    pieces.clear();
    for (int i = 0; i < 8; i++)
    {
        pieces.emplace_back(new Piece("wp", i * SQUARE_SIZE, 6 * SQUARE_SIZE, true, this));
        pieces.emplace_back(new Piece("bp", i * SQUARE_SIZE, 1 * SQUARE_SIZE, false, this));
    }

    pieces.emplace_back(new Piece("wr", 0 * SQUARE_SIZE, 7 * SQUARE_SIZE, true));
    pieces.emplace_back(new Piece("wr", 7 * SQUARE_SIZE, 7 * SQUARE_SIZE, true));
    pieces.emplace_back(new Piece("br", 0 * SQUARE_SIZE, 0 * SQUARE_SIZE, false));
    pieces.emplace_back(new Piece("br", 7 * SQUARE_SIZE, 0 * SQUARE_SIZE, false));

    pieces.emplace_back(new Piece("wn", 1 * SQUARE_SIZE, 7 * SQUARE_SIZE, true));
    pieces.emplace_back(new Piece("wn", 6 * SQUARE_SIZE, 7 * SQUARE_SIZE, true));
    pieces.emplace_back(new Piece("bn", 1 * SQUARE_SIZE, 0 * SQUARE_SIZE, false));
    pieces.emplace_back(new Piece("bn", 6 * SQUARE_SIZE, 0 * SQUARE_SIZE, false));

    pieces.emplace_back(new Piece("wb", 2 * SQUARE_SIZE, 7 * SQUARE_SIZE, true));
    pieces.emplace_back(new Piece("wb", 5 * SQUARE_SIZE, 7 * SQUARE_SIZE, true));
    pieces.emplace_back(new Piece("bb", 2 * SQUARE_SIZE, 0 * SQUARE_SIZE, false));
    pieces.emplace_back(new Piece("bb", 5 * SQUARE_SIZE, 0 * SQUARE_SIZE, false));

    pieces.emplace_back(new Piece("wq", 3 * SQUARE_SIZE, 7 * SQUARE_SIZE, true));
    pieces.emplace_back(new Piece("bq", 3 * SQUARE_SIZE, 0 * SQUARE_SIZE, false));

    pieces.emplace_back(new Piece("wk", 4 * SQUARE_SIZE, 7 * SQUARE_SIZE, true));
    pieces.emplace_back(new Piece("bk", 4 * SQUARE_SIZE, 0 * SQUARE_SIZE, false));
}

Piece* Board::selectPiece(int x, int y)
{
    int col = (x - BORDER_WIDTH_X) / SQUARE_SIZE;
    int row = (y - BORDER_WIDTH_Y) / SQUARE_SIZE;

    if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE)
    {
        Piece* piece = board[row][col];

        if (piece && piece->isWhite == isWhiteTurn)
            return piece;
    }
    return nullptr;
}

std::vector<Piece*>& Board::getPieces()
{
    return pieces;
}

std::string Board::coordToUCI(int row, int col)
{
    char file = 'a' + col;
    char rank = '1' + (7 - row);
    return std::string() + file + rank;
}

std::pair<int, int> Board::uciToCoord(const std::string& uci)
{
    if (uci.size() < 2)
        return {};

    int row = 7 - (uci[1] - '1');
    int col = uci[0] - 'a';
    return { row, col };
}

void Board::movePiece(SDL_Renderer* renderer, Piece* piece, int x, int y, MoveHistory& moveHistory, char forcedPromotion)
{
    if (viewingHistory)
        replayTo(moveHistory, moveHistory.size());

    if (!piece)
        return;
    if ((isWhiteTurn && !piece->isWhite) || (!isWhiteTurn && piece->isWhite))
        return;

    // If promotion UI is active, treat click as promotion selection
    if (promotionActive)
    {
        handlePromotionClick(x, y, moveHistory);
        return;
    }

    if (gameOver) return;
    int materialDelta = 0;

    int oldRow = piece->yPos / SQUARE_SIZE;
    int oldCol = piece->xPos / SQUARE_SIZE;
    int newRow = (y - BORDER_WIDTH_Y) / SQUARE_SIZE;
    int newCol = (x - BORDER_WIDTH_X) / SQUARE_SIZE;

    if (oldRow < 0 || oldRow >= BOARD_SIZE ||
        oldCol < 0 || oldCol >= BOARD_SIZE ||
        newRow < 0 || newRow >= BOARD_SIZE ||
        newCol < 0 || newCol >= BOARD_SIZE)
        return;

    if (!Move::isValidMove(piece, newRow, newCol, oldRow, oldCol, board))
    {
        // If king is in check and this move does NOT fix it = trigger blink
        if (Board::isKingInCheck(isWhiteTurn, board))
        {
            blinkCheck = true;
            blinkStart = SDL_GetTicks();
        }
        return;
    }

    MoveType type = Move::getLastMoveType();

    // Capture info BEFORE deletion
    char captured = '.';
    if (board[newRow][newCol])
        captured = board[newRow][newCol]->type[1];

    // Reset en-passant eligibility
    for (Piece* p : pieces)
    {
        if (p != piece && (p->type == "wp" || p->type == "bp"))
            p->enPassantEligible = false;
    }

    switch (type)
    {
    case MoveType::Normal:
    {
        Piece* target = board[newRow][newCol];
        if (target)
        {
            std::string type = target->type;
            materialDelta += pieceValue(target->type);

            if (target->isWhite)
            {
                whiteCaptured[type]++;     // white piece lost
                blackMaterial += pieceValue(type);
            }
            else
            {
                blackCaptured[type]++;     // black piece lost
                whiteMaterial += pieceValue(type);
            }

            auto it = std::find(pieces.begin(), pieces.end(), target);
            if (it != pieces.end())
                pieces.erase(it);
            delete target;
        }

        board[oldRow][oldCol] = nullptr;
        board[newRow][newCol] = piece;
        piece->setPosition(newCol * SQUARE_SIZE, newRow * SQUARE_SIZE);

        if ((piece->type == "wp" || piece->type == "bp") && abs(newRow - oldRow) == 2)
            piece->enPassantEligible = true;
    }
    break;

    case MoveType::Capture:
    {
        Piece* target = board[newRow][newCol];
        if (target)
        {
            std::string type = target->type;
            materialDelta += pieceValue(target->type);

            if (target->isWhite)
            {
                whiteCaptured[type]++;     // white piece lost
                blackMaterial += pieceValue(type);
            }
            else
            {
                blackCaptured[type]++;     // black piece lost
                whiteMaterial += pieceValue(type);
            }

            auto it = std::find(pieces.begin(), pieces.end(), target);
            if (it != pieces.end())
                pieces.erase(it);
            delete target;
        }

        board[oldRow][oldCol] = nullptr;
        board[newRow][newCol] = piece;
        piece->setPosition(newCol * SQUARE_SIZE, newRow * SQUARE_SIZE);
    }
    break;

    case MoveType::EnPassant:
    {
        Piece* last = Board::getLastMoved();
        if (last)
        {
            int lastRow = last->yPos / SQUARE_SIZE;
            int lastCol = last->xPos / SQUARE_SIZE;

            std::string type = last->type;
            materialDelta += pieceValue(last->type);

            if (last->isWhite)
            {
                whiteCaptured[type]++;     // white piece lost
                blackMaterial += pieceValue(type);
            }
            else
            {
                blackCaptured[type]++;     // black piece lost
                whiteMaterial += pieceValue(type);
            }

            // Delete pawn BEHIND the moved pawn's new square
            auto it = std::find(pieces.begin(), pieces.end(), last);
            if (it != pieces.end())
                pieces.erase(it);

            delete board[lastRow][lastCol];
            board[lastRow][lastCol] = nullptr;
        }

        // Move capturing pawn
        board[oldRow][oldCol] = nullptr;
        board[newRow][newCol] = piece;
        piece->setPosition(newCol * SQUARE_SIZE, newRow * SQUARE_SIZE);
    }
    break;

    case MoveType::CastleKing:
    {
        int row = oldRow;
        Piece* rook = board[row][7];

        board[row][7] = nullptr;
        board[row][5] = rook;
        rook->setPosition(5 * SQUARE_SIZE, row * SQUARE_SIZE);
        rook->hasMoved = true;

        // Move the king
        board[row][oldCol] = nullptr;
        board[row][6] = piece;
        piece->setPosition(6 * SQUARE_SIZE, row * SQUARE_SIZE);
        piece->hasCastled = true;
    }
    break;

    case MoveType::CastleQueen:
    {
        int row = oldRow;
        Piece* rook = board[row][0];

        board[row][0] = nullptr;
        board[row][3] = rook;
        rook->setPosition(3 * SQUARE_SIZE, row * SQUARE_SIZE);
        rook->hasMoved = true;

        // Move king
        board[row][oldCol] = nullptr;
        board[row][2] = piece;
        piece->setPosition(2 * SQUARE_SIZE, row * SQUARE_SIZE);
        piece->hasCastled = true;
    }
    break;

    case MoveType::Promotion:
    {
        // Handle capture first
        Piece* target = board[newRow][newCol];
        if (target)
        {
            std::string type = target->type;

            if (target->isWhite)
            {
                whiteCaptured[type]++;     // white piece lost
                blackMaterial += pieceValue(type);
            }
            else
            {
                blackCaptured[type]++;     // black piece lost
                whiteMaterial += pieceValue(type);
            }

            auto it = std::find(pieces.begin(), pieces.end(), target);
            if (it != pieces.end())
            {
                delete* it;
                pieces.erase(it);
            }
        }

        // Move pawn
        board[oldRow][oldCol] = nullptr;
        board[newRow][newCol] = piece;
        piece->setPosition(newCol * SQUARE_SIZE, newRow * SQUARE_SIZE);

        promoFromRow = oldRow;
        promoFromCol = oldCol;
        promoCaptured = captured;
        promoRow = newRow;
        promoCol = newCol;
        promoPawn = piece;

        bool engineMove = SETTINGS.vsEngine && !piece->isWhite;

        if (engineMove)
            applyPromotion(forcedPromotion, moveHistory);
        else
            promotionActive = true;

        return;
    }
    break;

    default:
        return;
    }

    Board::setLastMoved(piece);

    lastFromRow = oldRow;
    lastFromCol = oldCol;
    lastToRow = newRow;
    lastToCol = newCol;

    piece->hasMoved = true;
    isWhiteTurn = !isWhiteTurn;

    bool givesCheck = Board::isKingInCheck(isWhiteTurn, board);

    bool givesMate = false;
    if (givesCheck && Board::isCheckmate(isWhiteTurn)) givesMate = true;

    uciHistory += " " + Move::squareName(oldRow, oldCol) + Move::squareName(newRow, newCol);
    if (type == MoveType::Promotion)
    {
        char promoChar = forcedPromotion ? forcedPromotion : piece->type[1];
        uciHistory += std::string(1, tolower(promoChar));
    }

    // Record move
    if (!(type == MoveType::Promotion && !SETTINGS.vsEngine))
    {
        MoveRecord record(
            oldRow,
            oldCol,
            newRow,
            newCol,
            (type == MoveType::Promotion ? 'P' : piece->type[1]),
            captured,
            type,
            (type == MoveType::Promotion ? toupper(piece->type[1]) : 0),
            givesCheck,
            givesMate,
            materialDelta
        );
        moveHistory.addMove(record);
    }

    currentMoveIndex = moveHistory.size();
    viewingHistory = false;

    moveHistory.printMoves();

    std::cout << "\nBoard after Move:\n-------------------------\n";
    for (int r = 0; r < 8; r++)
    {
        std::cout << "|";
        for (int c = 0; c < 8; c++)
        {
            if (board[r][c])
                std::cout << board[r][c]->type << "|";
            else
                std::cout << "--|";
        }
        std::cout << "\n";
    }
    std::cout << "-------------------------\n";

    if (Board::isCheckmate(isWhiteTurn))
    {
        gameOver = true;
        whiteLost = isWhiteTurn;

        std::string kingType = whiteLost ? "wk" : "bk";
        losingKing = nullptr;

        for (Piece* p : pieces)
        {
            if (!p)
                continue;
            if (p->type == kingType)
            {
                losingKing = p;
                break;
            }
        }

        if (losingKing)
        {
            board[losingKing->yPos / SQUARE_SIZE][losingKing->xPos / SQUARE_SIZE] = nullptr;
            auto it = std::find(pieces.begin(), pieces.end(), losingKing);
            if (it != pieces.end())
                pieces.erase(it);
        }

        gameOverStart = SDL_GetTicks();
        return;
    }

    if (Board::isStalemate(isWhiteTurn))
    {
        gameOver = true;
        stalemate = true;

        for (int r = 0; r < BOARD_SIZE; r++)
            for (int c = 0; c < BOARD_SIZE; c++)
                if (board[r][c] && board[r][c]->type != "wk" && board[r][c]->type != "bk")
                    board[r][c] = nullptr;

        pieces.erase(std::remove_if(pieces.begin(), pieces.end(), [](Piece* p)
            {
                if (p->type == "wk" || p->type == "bk")
                    return false;
                delete p;
                return true;
            }), pieces.end());

        gameOverStart = SDL_GetTicks();
        return;
    }
}

void Board::drawCaptured(SDL_Renderer* renderer, TTF_Font* font)
{
    int iconSize = SQUARE_SIZE * 0.5f;
    int stackOffset = iconSize * 0.3f;
    int spacingBetweenTypes = 0;

    std::vector<char> order = {'p', 'b', 'n', 'r', 'q'};
    std::unordered_map<char, int> maxDisplay = {{'p', 8}, {'b', 2}, {'n', 2}, {'r', 2}, {'q', 1}};

    auto drawSide = [&](std::unordered_map<std::string, int>& map, bool whiteLost, int& outEndX)
    {
        int yPos = whiteLost ? BOARD_OFFSET_Y - 60 : BORDER_WIDTH_Y + BOARD_WIDTH + 40;
        int xPos = BORDER_WIDTH_X;

        for (char p : order)
        {
            std::string type = std::string(1, whiteLost ? 'w' : 'b') + p;

            int count = std::min(map[type], maxDisplay[p]);
            if (count <= 0) continue;

            SDL_Texture* tex = pieceTextures[type];
            if (!tex) continue;

            // Draw stacked pieces
            for (int i = 0; i < count; i++)
            {
                SDL_FRect dst = {
                    (float)(xPos + i * stackOffset),
                    (float)yPos,
                    (float)iconSize,
                    (float)iconSize
                };

                SDL_RenderTexture(renderer, tex, nullptr, &dst);
            }

            xPos += stackOffset * (count - 1)
                + iconSize
                + spacingBetweenTypes;

        }
        outEndX = xPos;
    };

    int blackEndX = 0;
    int whiteEndX = 0;

    drawSide(blackCaptured, false, blackEndX);
    drawSide(whiteCaptured, true, whiteEndX);

    // ---- Material Difference ----
    int diff = whiteMaterial - blackMaterial;
    if (diff != 0)
    {
        bool whiteWinning = diff > 0;
        int value = abs(diff);

        std::string text = "+" + std::to_string(value);
        SDL_Color col = { 255,255,255,255 };

        SDL_Surface* surf =
            TTF_RenderText_Blended(font,
                text.c_str(),
                text.length(),
                col);

        if (surf)
        {
            SDL_Texture* tex =
                SDL_CreateTextureFromSurface(renderer, surf);

            int yPos = whiteWinning ? BORDER_WIDTH_Y + BOARD_WIDTH + 45 : BOARD_OFFSET_Y - 45;
            int xPos = whiteWinning ? blackEndX : whiteEndX;

            float scale = 0.8f;

            SDL_FRect dst = {
                (float)xPos,
                (float)yPos + 2,
                (float)surf->w * scale,
                (float)surf->h * scale
            };

            SDL_DestroySurface(surf);
            SDL_RenderTexture(renderer, tex, nullptr, &dst);
            SDL_DestroyTexture(tex);
        }
    }
}

Piece* Board::promotePawn(SDL_Renderer* renderer, Piece* pawn, int row, int col)
{
    if (!pawn)
        return nullptr;

    promotionActive = true;
    promoRow = row, promoCol = col;
    promoPawn = pawn;

    return pawn;
}

void Board::drawPromotionMenu(SDL_Renderer* renderer)
{
    float x = BORDER_WIDTH_X + promoCol * SQUARE_SIZE;
    float y = BORDER_WIDTH_Y + promoRow * SQUARE_SIZE;

    float w = SQUARE_SIZE / 2.0f;

    SDL_FRect rects[4] = {
        {x, y, w, w},        // 0 Queen
        {x + w, y, w, w},    // 1 Rook
        {x, y + w, w, w},    // 2 Bishop
        {x + w, y + w, w, w} // 3 Knight
    };

    std::string base = promoPawn->isWhite ? "w" : "b";
    std::string types[4] = { base + "q", base + "r", base + "b", base + "n" };

    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            SDL_FRect promoSquare = {
                x + j * w,
                y + i * w,
                w,
                w
            };

            if ((i + j) % 2 == 0)
                SDL_SetRenderDrawColor(renderer, 150, 116, 83, 127);
            else
                SDL_SetRenderDrawColor(renderer, 10, 47, 39, 127);

            SDL_RenderFillRect(renderer, &promoSquare);

            SDL_Texture* tex = pieceTextures[types[i * 2 + j]];
            if (tex)
                SDL_RenderTexture(renderer, tex, nullptr, &rects[i * 2 + j]);
        }
    }
}

void Board::handlePromotionClick(int mouseX, int mouseY, MoveHistory& moveHistory)
{
    float x0 = BORDER_WIDTH_X + promoCol * SQUARE_SIZE;
    float y0 = BORDER_WIDTH_Y + promoRow * SQUARE_SIZE;

    float w = SQUARE_SIZE / 2.0f;
    float h = SQUARE_SIZE / 2.0f;

    float rx = mouseX - x0;
    float ry = mouseY - y0;

    // If click outside promotion box
    if (rx < 0 || rx >= 2 * w || ry < 0 || ry >= 2 * h)
        return;

    int c = (rx >= w);
    int r = (ry >= h);
    int choice = r * 2 + c;

    char choices[4] = { 'q','r','b','n' };
    applyPromotion(choices[choice], moveHistory);
}

void Board::applyPromotion(char promoChar, MoveHistory& moveHistory)
{
    if (!promoPawn)
        return;

    bool isWhite = promoPawn->isWhite;

    std::string newType =
        std::string(1, isWhite ? 'w' : 'b') + promoChar;

    int row = promoRow;
    int col = promoCol;

    // Remove pawn
    auto it = std::find(pieces.begin(), pieces.end(), promoPawn);
    if (it != pieces.end())
    {
        delete* it;
        pieces.erase(it);
    }

    // Create promoted piece
    Piece* promoted = 
        new Piece(newType,
        col * SQUARE_SIZE,
        row * SQUARE_SIZE,
        isWhite,
        this
    );

    pieces.push_back(promoted);
    board[row][col] = promoted;

    int pawnValue = 1;
    int newValue = pieceValue(newType);
    int gain = newValue - pawnValue;

    if (isWhite)
        whiteMaterial += gain;
    else
        blackMaterial += gain;

    // Clear promotion state
    promotionActive = false;
    promoPawn = nullptr;

    Board::setLastMoved(promoted);
    promoted->hasMoved = true;

    isWhiteTurn = !isWhiteTurn;

    // Unified recording here
    MoveRecord record(
        promoFromRow,
        promoFromCol,
        row,
        col,
        'P',
        promoCaptured,
        MoveType::Promotion,
        toupper(promoChar),
        gain
    );

    moveHistory.addMove(record);

    uciHistory += " " +
        Move::squareName(promoFromRow, promoFromCol) +
        Move::squareName(row, col) +
        std::string(1, promoChar);

    currentMoveIndex = moveHistory.size();
    viewingHistory = false;
}

void Board::highlightCheckedKing(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Check both kings
    bool whiteInCheck = Board::isKingInCheck(true, board);
    bool blackInCheck = Board::isKingInCheck(false, board);

    if (!whiteInCheck && !blackInCheck)
        return;

    std::string kingType = whiteInCheck ? "wk" : "bk";

    // Blink logic (for illegal attempts)
    bool useBlink = false;

    if (blinkCheck)
    {
        Uint64 now = SDL_GetTicks();
        Uint64 elapsed = now - blinkStart;

        if (elapsed < 1200)                         // blink for 600ms
            useBlink = ((now / 200) % 2 == 0);      // toggle every 200 ms
        else
            blinkCheck = false;                     // stop blinking
    }

    // Find king position
    for (Piece* p : pieces)
    {
        if (!p)
            continue;

        if ((p->type == "wk" && whiteInCheck) || (p->type == "bk" && blackInCheck))
        {
            int row = p->yPos / SQUARE_SIZE;
            int col = p->xPos / SQUARE_SIZE;

            float x = BORDER_WIDTH_X + col * SQUARE_SIZE;
            float y = BORDER_WIDTH_Y + row * SQUARE_SIZE;

            SDL_FRect square = { x, y, SQUARE_SIZE, SQUARE_SIZE };

            // Semi-transparent red so the piece is still visible
            if (useBlink)
            {
                if ((row + col) % 2 == 0)
                    SDL_SetRenderDrawColor(renderer, 251, 233, 167, 255);   // light square
                else
                    SDL_SetRenderDrawColor(renderer, 21, 95, 78, 255);      // dark square
            }
            else
                SDL_SetRenderDrawColor(renderer, 200, 0, 0, 204);   // solid red

            SDL_RenderFillRect(renderer, &square);
            return;
        }
    }
}

bool Board::isCheckmate(bool isWhite)
{
    // 1. King must be in check
    if (!Board::isKingInCheck(isWhite, board))
        return false;

    // 2. Side must have no legal moves
    if (hasAnyLegalMove(isWhite))
        return false;

    return true; // Check + No legal moves = Checkmate
}

bool Board::hasAnyLegalMove(bool isWhite)
{
    for (Piece* piece : pieces)
    {
        if (piece->isWhite != isWhite)
            continue;

        int oldRow = piece->yPos / SQUARE_SIZE;
        int oldCol = piece->xPos / SQUARE_SIZE;

        for (int r = 0; r < BOARD_SIZE; r++)
        {
            for (int c = 0; c < BOARD_SIZE; c++)
            {
                // Skip if moving to same square
                if (r == oldRow && c == oldCol)
                    continue;

                // First: movement pattern legal?
                if (!Move::isValidMove(piece, r, c, oldRow, oldCol, board))
                    continue;

                // Second: simulate move ? does it leave king in check?
                if (Move::leavesKingInCheck(piece, r, c, oldRow, oldCol, board))
                    continue;

                // Found at least one legal move
                return true;
            }
        }
    }

    return false; // No legal moves exist
};

bool Board::isStalemate(bool isWhite)
{
    // 1. King must NOT be in check
    if (Board::isKingInCheck(isWhite, board))
        return false;

    // 2. No legal moves must exist
    if (hasAnyLegalMove(isWhite))
        return false;

    return true; // Not in check & no moves = stalemate
}

void Board::drawGameOverScreen(SDL_Renderer* renderer, TTF_Font* font)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Dark translucent overlay
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 80);
    SDL_FRect overlay = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &overlay);

    // Draw king fall + red square (if checkmate)
    if (losingKing)
    {
        int row = losingKing->yPos / SQUARE_SIZE;
        int col = losingKing->xPos / SQUARE_SIZE;

        SDL_FRect sq = {
            BORDER_WIDTH_X + col * SQUARE_SIZE,
            BORDER_WIDTH_Y + row * SQUARE_SIZE,
            SQUARE_SIZE,
            SQUARE_SIZE
        };

        SDL_SetRenderDrawColor(renderer, 144, 0, 0, 180); // solid red tile
        SDL_RenderFillRect(renderer, &sq);

        // Tilt king animation
        Uint64 t = SDL_GetTicks() - gameOverStart;
        float angle = std::min(90.0f, t * 0.15f);
        kingTiltAngle = angle;

        // Draw fallen king
        SDL_RenderTextureRotated(
            renderer,
            Board::pieceTextures[losingKing->type],
            nullptr,
            &sq,
            angle,
            nullptr,
            SDL_FLIP_NONE
        );
    }

    drawEndText(renderer, font);
}

void Board::drawEndText(SDL_Renderer* renderer, TTF_Font* font)
{
    const char* msg = (stalemate ? "Stalemate!!" : "Checkmate??");

    SDL_Color col;
    if (stalemate)
        col = {0, 0, 0, 204};
    else
        col = {204, 0, 0, 204};

    int w, h;
    TTF_GetStringSize(font, msg, strlen(msg), &w, &h);

    float scale = 2.0f; // BIGGER TEXT

    SDL_Surface* surf = TTF_RenderText_Blended(font, msg, strlen(msg), col);
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_DestroySurface(surf);

    SDL_FRect rect = {
        (SCREEN_WIDTH - w * scale) / 2.0f,
        (SCREEN_HEIGHT - h * scale) / 2.0f,
        w * scale,
        h * scale
    };

    SDL_RenderTexture(renderer, tex, nullptr, &rect);
    SDL_DestroyTexture(tex);
}
