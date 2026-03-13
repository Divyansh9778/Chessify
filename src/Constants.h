#ifndef CONSTANTS_H
#define CONSTANTS_H

constexpr int BOARD_OFFSET_X = 60;
constexpr int BOARD_OFFSET_Y = 60;

constexpr int BOARD_SIZE = 8;   // Standard chessboard size
constexpr int BORDER_WIDTH = 40; // If there's a border around the board

constexpr int BORDER_WIDTH_X = BORDER_WIDTH + BOARD_OFFSET_X; // If there's a border around the board
constexpr int BORDER_WIDTH_Y = BORDER_WIDTH + BOARD_OFFSET_Y; // If there's a border around the board

constexpr int SQUARE_SIZE = 110; // Each square is 80x80 pixels
constexpr int BOARD_PIXEL_SIZE = BOARD_SIZE * SQUARE_SIZE;
constexpr int PIECE_SIZE = SQUARE_SIZE; // Size of the pieces
constexpr float promoFactor = 0.9f; // Factor for the promotion squares
constexpr float PROMO_SIZE = SQUARE_SIZE * promoFactor; // Size of the promotion squares

constexpr int PANEL_WIDTH = 1920 - (2 * BORDER_WIDTH_X + BOARD_SIZE * SQUARE_SIZE) - BOARD_OFFSET_X;

constexpr int BOARD_WIDTH = BOARD_SIZE * SQUARE_SIZE;
constexpr int SCREEN_WIDTH = BOARD_WIDTH + 2 * BORDER_WIDTH_X;
constexpr int SCREEN_HEIGHT = BOARD_WIDTH + 2 * BORDER_WIDTH_Y;

#endif // CONSTANTS_H