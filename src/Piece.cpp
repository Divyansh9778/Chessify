#include "Constants.h"
#include "Piece.h"

#include <unordered_map>

// Constructor for the Piece class
Piece::Piece(const std::string& type, int x, int y, bool isWhite, Board* board)
    : type(type), xPos(x), yPos(y), isWhite(isWhite), board(board)
{
}

// Set position of the piece
void Piece::setPosition(int x, int y)
{
    xPos = x;
    yPos = y;
}

// Render the piece using SDL
void Piece::drawPiece(SDL_Renderer* renderer, const std::unordered_map<std::string, SDL_Texture*>& pieceTextures)
{
    if (!renderer || pieceTextures.find(type) == pieceTextures.end())
        return;

    auto it = pieceTextures.find(type);
    SDL_Texture* texture = it->second;

    SDL_FRect fPieceRect = {
        BORDER_WIDTH_X + xPos, // Center X
        BORDER_WIDTH_Y + yPos, // Center Y
        SQUARE_SIZE,         // Width of piece
        SQUARE_SIZE          // Height of piece
    };

    SDL_RenderTexture(renderer, texture, nullptr, &fPieceRect);
}
