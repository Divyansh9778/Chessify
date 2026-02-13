#ifndef BOARD_H
#define BOARD_H

#include "Constants.h"
#include "Settings.h"
#include "Piece.h"

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_ttf.h>
#include <SDL3/SDL_stdinc.h>

#include <unordered_map>
#include <vector>
#include <string>

class Board
{
public:
    Board();
    ~Board();
    std::string uciHistory = "position startpos moves"; // UCI move history for engine
    std::string coordToUCI(int row, int col);
    std::pair<int, int> uciToCoord(const std::string& uci);

    static Board* instance; // Add this to maintain single instance
    static std::unordered_map<std::string, SDL_Texture*> pieceTextures;

    bool isWhiteTurn = true; // Track if it's white's turn
    Piece* board[BOARD_SIZE][BOARD_SIZE]; // 2D array of piece pointers

    void loadTextures(SDL_Renderer* renderer);              // Loads textures for pieces
    void drawBoard(SDL_Renderer* renderer, TTF_Font* font); // Renders the board

    Piece* selectPiece(int x, int y);                                   // Select a piece based on coordinates
    void movePiece(SDL_Renderer* renderer, Piece* piece, int x, int y); // Move a piece to the new position
    std::vector<Piece*>& getPieces();                                  // Get all pieces on the board

    static Piece* lastMovedPiece;                                      // Track the last moved piece (shared across all Board instances)
    static void setLastMoved(Piece* piece) { lastMovedPiece = piece; } // Update the last moved piece
    static Piece* getLastMoved() { return lastMovedPiece; }            // Get the last moved piece

    bool blinkCheck = false;
    Uint64 blinkStart = 0;
    static bool isKingInCheck(bool isWhite, Piece* board[BOARD_SIZE][BOARD_SIZE]); // Check if the king of the specified color is in check
    void highlightCheckedKing(SDL_Renderer* renderer);

    bool hasAnyLegalMove(bool isWhite);
    bool isCheckmate(bool isWhite);

    bool stalemate = false;
    bool isStalemate(bool isWhite);

    bool promotionActive = false;
    int promoRow = -1, promoCol = -1;
    Piece* promoPawn = nullptr;

    std::string pendingPromoMove = "";
    Piece* promotePawn(SDL_Renderer* renderer, Piece* pawn, int row, int col);
    void drawPromotionMenu(SDL_Renderer* renderer);
    void handlePromotionClick(int mouseX, int mouseY);
    void applyPromotion(int choice);

    Piece* losingKing = nullptr;
    bool gameOver = false;
    bool whiteLost = false;     // true = white king lost, false = black lost
    float kingTiltAngle = 0.0f; // for rotation animation
    Uint64 gameOverStart = 0;   // fade in timing
    void drawGameOverScreen(SDL_Renderer* renderer, TTF_Font* font);
    void drawEndText(SDL_Renderer* renderer, TTF_Font* font);

private:
    void initializePieces();              // Initializes pieces on the board

    std::vector<Piece*> pieces; // List of all pieces for easier management

    Piece* selectedPiece = nullptr;
    SDL_Renderer* renderer;

    std::unordered_map<std::string, int> positionCounts;
};

#endif
