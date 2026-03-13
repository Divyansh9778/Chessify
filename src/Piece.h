#ifndef PIECE_H
#define PIECE_H

#include <SDL3/SDL.h>

#include <string>
#include <unordered_map>

// Forward declaration of the Board class to avoid circular dependencies
class Board;

class Piece
{
public:
    std::string type;
    int xPos, yPos;
    bool isWhite;
    bool hasMoved = false;
    bool hasCastled = false;        // Add this flag for kings
    bool isPromoting = false;       // Add this flag for pawns
    bool enPassantEligible = false; // Add this flag

    Board* board; // Pointer to the board

    // Constructor
    Piece(const std::string& type, int x, int y, bool isWhite, Board* board = nullptr);

    // Set position of the piece
    void setPosition(int x, int y);

    // Render the piece using SDL
    void drawPiece(SDL_Renderer* renderer, const std::unordered_map<std::string, SDL_Texture*>& textures);
};

#endif
