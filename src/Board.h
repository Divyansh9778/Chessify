#ifndef BOARD_H
#define BOARD_H

#include "Constants.h"
#include "Piece.h"
#include "MoveHistory.h"
#include "MoveRecord.h"

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <vector>
#include <unordered_map>
#include <string>
#include <utility>

class Board
{
public:
    Board();
    ~Board();

    int whiteMaterial = 0; // material white gained
    int blackMaterial = 0; // material black gained
    int pieceValue(const std::string &type);

    int currentMoveIndex = 0;
    bool viewingHistory = false;
    void resetBoardState();
    void replayTo(MoveHistory &history, int index);
    void simulateMoveFromRecord(Piece *piece, const MoveRecord &m);
    void stepBackward(MoveHistory &history);
    void stepForward(MoveHistory &history);

    MoveHistory moveHistory;
    std::string uciHistory = "position startpos moves"; // UCI move history for engine
    std::string coordToUCI(int row, int col);
    std::pair<int, int> uciToCoord(const std::string &uci);

    static Board *instance; // Add this to maintain single instance
    static std::unordered_map<std::string, SDL_Texture *> pieceTextures;

    bool isWhiteTurn = true;              // Track if it's white's turn
    Piece *board[BOARD_SIZE][BOARD_SIZE]; // 2D array of piece pointers

    void loadTextures(SDL_Renderer *renderer);              // Loads textures for pieces
    void drawBoard(SDL_Renderer *renderer, TTF_Font *font); // Renders the board

    Piece *selectPiece(int x, int y);                                                                                       // Select a piece based on coordinates
    void movePiece(SDL_Renderer *renderer, Piece *piece, int x, int y, MoveHistory &moveHistory, char forcedPromotion = 0); // Move a piece to the new position
    std::vector<Piece *> &getPieces();                                                                                      // Get all pieces on the board

    static Piece *lastMovedPiece;                                      // Track the last moved piece (shared across all Board instances)
    static void setLastMoved(Piece *piece) { lastMovedPiece = piece; } // Update the last moved piece
    static Piece *getLastMoved() { return lastMovedPiece; }            // Get the last moved piece

    bool blinkCheck = false;
    Uint64 blinkStart = 0;
    static bool isKingInCheck(bool isWhite, Piece *board[BOARD_SIZE][BOARD_SIZE]); // Check if the king of the specified color is in check
    void highlightCheckedKing(SDL_Renderer *renderer);

    std::unordered_map<std::string, int> whiteCaptured;
    std::unordered_map<std::string, int> blackCaptured;
    void drawCaptured(SDL_Renderer *renderer, TTF_Font *font);

    bool hasAnyLegalMove(bool isWhite);
    bool isCheckmate(bool isWhite);

    bool stalemate = false;
    bool isStalemate(bool isWhite);

    bool promotionActive = false;
    int promoRow = -1, promoCol = -1;
    int promoFromRow = -1, promoFromCol = -1;
    char promoCaptured = '.';
    Piece *promoPawn = nullptr;

    std::string pendingPromoMove = "";
    Piece *promotePawn(SDL_Renderer *renderer, Piece *pawn, int row, int col);
    void drawPromotionMenu(SDL_Renderer *renderer);
    void handlePromotionClick(int mouseX, int mouseY, MoveHistory &moveHistory);
    void applyPromotion(char promoChar, MoveHistory &moveHistory);

    Piece *losingKing = nullptr;
    bool gameOver = false;
    bool whiteLost = false;     // true = white king lost, false = black lost
    float kingTiltAngle = 0.0f; // for rotation animation
    Uint64 gameOverStart = 0;   // fade in timing
    void drawGameOverScreen(SDL_Renderer *renderer, TTF_Font *font);
    void drawEndText(SDL_Renderer *renderer, TTF_Font *font);

private:
    void initializePieces(); // Initializes pieces on the board
    std::vector<Piece *> pieces; // List of all pieces for easier management

    Piece *selectedPiece = nullptr;
    SDL_Renderer *renderer;
};

#endif