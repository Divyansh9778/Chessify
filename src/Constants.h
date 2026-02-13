#ifndef CONSTANTS_H
#define CONSTANTS_H

const int PANEL_WIDTH = 360;

constexpr int BOARD_SIZE = 8;   // Standard chessboard size
constexpr int BORDER_WIDTH = 50; // If there's a border around the board
constexpr int SQUARE_SIZE = 100; // Each square is 80x80 pixels
constexpr int PIECE_SIZE = SQUARE_SIZE; // Size of the pieces
constexpr float promoFactor = 0.9f; // Factor for the promotion squares
constexpr float PROMO_SIZE = SQUARE_SIZE * promoFactor; // Size of the promotion squares

constexpr int BOARD_WIDTH = BOARD_SIZE * SQUARE_SIZE;
constexpr int SCREEN_WIDTH = BOARD_WIDTH + 2 * BORDER_WIDTH;
constexpr int SCREEN_HEIGHT = BOARD_WIDTH + 2 * BORDER_WIDTH;

#endif // CONSTANTS_H
