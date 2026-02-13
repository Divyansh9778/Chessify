#include "Constants.h"
#include "Piece.h"
#include "Move.h"
#include "Board.h"

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_ttf.h>
#include <SDL3/SDL_timer.h>

#include <iostream>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <string>

Piece* Board::lastMovedPiece = nullptr;
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
    // Free all loaded textures
    for (auto& pair : pieceTextures)
        SDL_DestroyTexture(pair.second);
    pieceTextures.clear();

    // Free dynamically allocated pieces
    for (Piece* piece : pieces)
        delete piece;
    pieces.clear();
}

void Board::loadTextures(SDL_Renderer* renderer)
{
    std::vector<std::string> pieceNames = {
        "wp", "wr", "wn", "wb", "wq", "wk",
        "bp", "br", "bn", "bb", "bq", "bk" };

    for (const std::string& name : pieceNames)
    {
        std::string path = "assets/pieces/" + name + ".bmp"; // Adjust path as needed
        SDL_Surface* surface = SDL_LoadBMP(path.c_str());
        if (!surface)
        {
            std::cerr << "Failed to load image: " << path << " Error: " << SDL_GetError() << std::endl;
            continue;
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface); // Free surface after creating texture

        if (!texture)
        {
            std::cerr << "Failed to create texture from " << path << " Error: " << SDL_GetError() << std::endl;
            continue;
        }

        pieceTextures[name] = texture;
    }
}

void Board::drawBoard(SDL_Renderer* renderer, TTF_Font* font)
{
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);          // Set background color
    SDL_FRect borderRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT }; // Full screen
    SDL_RenderFillRect(renderer, &borderRect);                  // Fill background

    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            SDL_FRect square = {
                BORDER_WIDTH + col * SQUARE_SIZE,
                BORDER_WIDTH + row * SQUARE_SIZE,
                SQUARE_SIZE,
                SQUARE_SIZE };

            if ((row + col) % 2 == 0)
                SDL_SetRenderDrawColor(renderer, 251, 233, 167, 255);
            else
                SDL_SetRenderDrawColor(renderer, 21, 95, 78, 255);

            SDL_RenderFillRect(renderer, &square);
        }
    }

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

    // Highlight king if in check
    highlightCheckedKing(renderer);

    if (gameOver)
        drawGameOverScreen(renderer, font);
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
    int col = (x - BORDER_WIDTH) / SQUARE_SIZE;
    int row = (y - BORDER_WIDTH) / SQUARE_SIZE;

    if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE)
    {
        Piece* piece = board[row][col];

        if (piece && piece->isWhite == isWhiteTurn)
        {
            return piece;
        }
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

void Board::movePiece(SDL_Renderer* renderer, Piece* piece, int x, int y)
{
    if (!piece)
        return;
    if ((isWhiteTurn && !piece->isWhite) || (!isWhiteTurn && piece->isWhite))
        return;

    // If promotion UI is active, treat click as promotion selection
    if (promotionActive)
    {
        handlePromotionClick(x, y);
        return;
    }

    if (gameOver)
        return;

    int oldRow = piece->yPos / SQUARE_SIZE;
    int oldCol = piece->xPos / SQUARE_SIZE;
    int newRow = (y - BORDER_WIDTH) / SQUARE_SIZE;
    int newCol = (x - BORDER_WIDTH) / SQUARE_SIZE;

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

    // Reset en-passant eligibility
    for (Piece* p : pieces)
    {
        if (p != piece && (p->type == "wp" || p->type == "bp"))
            p->enPassantEligible = false;
    }

    MoveType type = Move::getLastMoveType();
    switch (type)
    {
    case MoveType::Normal:
    {
        Piece* target = board[newRow][newCol];
        if (target)
        {
            auto it = std::find(pieces.begin(), pieces.end(), target);
            if (it != pieces.end())
                pieces.erase(it);
            delete target;
        }

        board[oldRow][oldCol] = nullptr;
        board[newRow][newCol] = piece;
        piece->setPosition(newCol * SQUARE_SIZE, newRow * SQUARE_SIZE);

        // PROMOTION CHECK
        if ((piece->type == "wp" && newRow == 0) || (piece->type == "bp" && newRow == 7))
        {
            if (SETTINGS.vsEngine)
            {
                // --- Engine mode: DELAY promotion for correct UCI ---
                promotionActive = true;
                promoPawn = piece;

                // Store move WITHOUT promotion letter yet (e.g. "e7e8")
                pendingPromoMove = coordToUCI(oldRow, oldCol) + coordToUCI(newRow, newCol);

                // DO NOT promote now, Stockfish will wait until user picks Q/R/B/N
                return;
            }

            Piece* promoted = promotePawn(renderer, piece, newRow, newCol);
            piece = promoted;
            Board::setLastMoved(piece);
        }

        if ((piece->type == "wp" || piece->type == "bp") && abs(newRow - oldRow) == 2)
            piece->enPassantEligible = true;
    }
    break;

    case MoveType::Capture:
    {
        Piece* target = board[newRow][newCol];
        if (target)
        {
            auto it = std::find(pieces.begin(), pieces.end(), target);
            if (it != pieces.end())
                pieces.erase(it);
            delete target;
        }

        board[oldRow][oldCol] = nullptr;
        board[newRow][newCol] = piece;
        piece->setPosition(newCol * SQUARE_SIZE, newRow * SQUARE_SIZE);

        // PROMOTION CHECK
        if ((piece->type == "wp" && newRow == 0) || (piece->type == "bp" && newRow == 7))
        {
            Piece* promoted = promotePawn(renderer, piece, newRow, newCol);
            piece = promoted;
            Board::setLastMoved(piece);
        }
    }
    break;

    case MoveType::EnPassant:
    {
        Piece* last = Board::getLastMoved();
        if (last)
        {
            int lastRow = last->yPos / SQUARE_SIZE;
            int lastCol = last->xPos / SQUARE_SIZE;

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

        // Move the rook
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

        // Move rook first
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

    default:
        return;
    }

    Board::setLastMoved(piece);
    piece->hasMoved = true;
    isWhiteTurn = !isWhiteTurn;

    uciHistory += " " + Move::squareName(oldRow, oldCol) + Move::squareName(newRow, newCol);

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

        pieces.erase(
            std::remove_if(pieces.begin(), pieces.end(), [](Piece* p)
                {
                    if (p->type == "wk" || p->type == "bk")
                        return false;
                    delete p;
                    return true; }),
            pieces.end());

        gameOverStart = SDL_GetTicks();
        return;
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
    float x = BORDER_WIDTH + promoCol * SQUARE_SIZE;
    float y = BORDER_WIDTH + promoRow * SQUARE_SIZE;

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
                w };

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

void Board::handlePromotionClick(int mouseX, int mouseY)
{
    float x0 = BORDER_WIDTH + promoCol * SQUARE_SIZE;
    float y0 = BORDER_WIDTH + promoRow * SQUARE_SIZE;

    float w = SQUARE_SIZE / 2.0f;
    float h = SQUARE_SIZE / 2.0f;

    float rx = mouseX - x0;
    float ry = mouseY - y0;

    // If click outside promotion box
    if (rx < 0 || rx >= 2 * w || ry < 0 || ry >= 2 * h)
        return;

    int c = (rx >= w);
    int r = (ry >= h);
    int choice = r * 2 + c; // 0..3

    applyPromotion(choice);
}

void Board::applyPromotion(int choice)
{
    if (!promoPawn)
        return;

    bool isWhite = promoPawn->isWhite;
    int row = promoRow;
    int col = promoCol;

    std::string base = isWhite ? "w" : "b";
    std::string types[4] = { base + "q", base + "r", base + "b", base + "n" };

    // Remove pawn
    board[row][col] = nullptr;
    auto it = std::find(pieces.begin(), pieces.end(), promoPawn);
    if (it != pieces.end())
        pieces.erase(it);
    delete promoPawn;

    // Create new piece
    Piece* promoted = new Piece(types[choice], col * SQUARE_SIZE, row * SQUARE_SIZE, isWhite, this);
    pieces.push_back(promoted);
    board[row][col] = promoted;

    // Clear promotion state
    promotionActive = false;
    promoPawn = nullptr;
    promoRow = promoCol = -1;

    Board::setLastMoved(promoted);
}

void Board::highlightCheckedKing(SDL_Renderer* renderer)
{
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

        if (elapsed < 600)
        {                                      // blink for 600ms
            useBlink = ((now / 120) % 2 == 0); // toggle every 120 ms
        }
        else
            blinkCheck = false; // stop blinking
    }

    // Find king position
    for (Piece* p : pieces)
    {
        if (!p)
            continue;

        if ((p->type == "wk" && whiteInCheck) ||
            (p->type == "bk" && blackInCheck))
        {
            int row = p->yPos / SQUARE_SIZE;
            int col = p->xPos / SQUARE_SIZE;

            float x = BORDER_WIDTH + col * SQUARE_SIZE;
            float y = BORDER_WIDTH + row * SQUARE_SIZE;

            // Semi-transparent red so the piece is still visible
            if (useBlink)
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
            else
                SDL_SetRenderDrawColor(renderer, 144, 0, 0, 255);

            float t = 6.0f;

            SDL_FRect top = { x, y, SQUARE_SIZE, t };
            SDL_FRect bottom = { x, y + SQUARE_SIZE - t, SQUARE_SIZE, t };
            SDL_FRect left = { x, y, t, SQUARE_SIZE };
            SDL_FRect right = { x + SQUARE_SIZE - t, y, t, SQUARE_SIZE };

            SDL_RenderFillRect(renderer, &top);
            SDL_RenderFillRect(renderer, &bottom);
            SDL_RenderFillRect(renderer, &left);
            SDL_RenderFillRect(renderer, &right);

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
            BORDER_WIDTH + col * SQUARE_SIZE,
            BORDER_WIDTH + row * SQUARE_SIZE,
            SQUARE_SIZE,
            SQUARE_SIZE };

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
            SDL_FLIP_NONE);
    }

    drawEndText(renderer, font);
}

void Board::drawEndText(SDL_Renderer* renderer, TTF_Font* font)
{
    const char* msg = (stalemate ? "Stalemate!!" : "Checkmate??");

    SDL_Color col;
    if (stalemate)
        col = { 0, 0, 0, 204 };
    else
        col = { 204, 0, 0, 204 };

    int w, h;
    TTF_GetStringSize(font, msg, strlen(msg), &w, &h);

    float scale = 1.5f; // BIGGER TEXT

    SDL_Surface* surf = TTF_RenderText_Blended(font, msg, strlen(msg), col);
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_DestroySurface(surf);

    SDL_FRect rect = {
        (SCREEN_WIDTH - w * scale) / 2.0f,
        (SCREEN_HEIGHT - h * scale) / 2.0f,
        w * scale,
        h * scale };

    SDL_RenderTexture(renderer, tex, nullptr, &rect);
    SDL_DestroyTexture(tex);
}
