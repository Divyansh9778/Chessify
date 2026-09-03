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

const float MENU_PANEL_W = 420.0f;
const float MENU_PANEL_H = 450.0f;

const float MENU_PANEL_X = 2 * BORDER_WIDTH_X + BOARD_SIZE * SQUARE_SIZE;
const float MENU_PX = MENU_PANEL_X + (PANEL_WIDTH - MENU_PANEL_W) / 2;
const float MENU_PY = (SCREEN_HEIGHT - MENU_PANEL_H) / 2;

const float MENU_BUTTON_X = MENU_PX + 60;
const float MENU_BUTTON_W = 300;
const float MENU_BUTTON_H = 50;

const float MENU_Y1 = MENU_PY + 120;
const float MENU_Y2 = MENU_PY + 190;
const float MENU_Y3 = MENU_PY + 280;

const float MENU_SMALL_X = MENU_PX + 145;
const float MENU_SMALL_W = 130;
const float MENU_SMALL_H = 40;
const float MENU_BACK_Y = MENU_PY + 370;

enum class PlayerColorChoice
{
    BLACK,
    RANDOM,
    WHITE
};

#endif // CONSTANTS_H