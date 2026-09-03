#include <SDL3/SDL_video.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_blendmode.h>

#include <SDL3_ttf/SDL_ttf.h>

#include "Constants.h"
#include "Move.h"
#include "Settings.h"
#include "Piece.h"
#include "Board.h"
#include "Engine.h"
#include "MovePanel.h"
#include "MoveHistory.h"
#include "game/GameController.h"
#include "network/NetworkClient.h"
#include "network/PlayerColor.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <process.h>
#include <cmath>
#include <algorithm>
#include <cctype>

#include "network/ProtocolUtils.h"
#include "network/NetworkPacket.h"

enum class UIState
{
    START_MENU,
    CONNECTING,
    PLAYING,
    EXIT_CONFIRM,
    CONNECTION_LOST,
    CREATE_ROOM,
    JOIN_ROOM,
    WAITING_ROOM
};

UIState prevState = UIState::START_MENU;
UIState currState = UIState::START_MENU;
bool exitYesSelected = true;

const SDL_Color COLOR_BG = {25, 25, 32, 255};
const SDL_Color COLOR_BTN = {55, 115, 255, 255};
const SDL_Color COLOR_BTN_HOVER = {75, 135, 255, 255};
const SDL_Color COLOR_BTN_TWO = {60, 170, 95, 255};
const SDL_Color COLOR_SHADOW = {0, 0, 0, 80};
const SDL_Color COLOR_TEXT = {255, 255, 255, 255};
const SDL_Color COLOR_TITLE1 = {255, 255, 255, 255};
const SDL_Color COLOR_TITLE2 = {180, 180, 200, 255};

std::unordered_map<std::string, std::string> pieceFiles = {
    {"wp", "assets/pieces/wp.bmp"},
    {"wr", "assets/pieces/wr.bmp"},
    {"wn", "assets/pieces/wn.bmp"},
    {"wb", "assets/pieces/wb.bmp"},
    {"wq", "assets/pieces/wq.bmp"},
    {"wk", "assets/pieces/wk.bmp"},
    {"bp", "assets/pieces/bp.bmp"},
    {"br", "assets/pieces/br.bmp"},
    {"bn", "assets/pieces/bn.bmp"},
    {"bb", "assets/pieces/bb.bmp"},
    {"bq", "assets/pieces/bq.bmp"},
    {"bk", "assets/pieces/bk.bmp"}};

SDL_Renderer *renderer = nullptr;
static bool init(SDL_Window *&window, SDL_Renderer *&renderer, TTF_Font *&font, SDL_Surface *icon)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL Initialization Failed! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    if (!TTF_Init())
    {
        std::cerr << "TTF Initialization Failed! SDL_ttf Error: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("Chessify", 0, 0, SDL_WINDOW_FULLSCREEN);
    if (!window)
    {
        std::cerr << "Window Creation Failed! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    icon = SDL_LoadBMP("assets/icons/chess_icon.bmp");
    if (icon)
    {
        SDL_SetWindowIcon(window, icon);
        SDL_DestroySurface(icon);
    }
    else
    {
        std::cerr << "Failed to load icon! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer)
    {
        std::cerr << "Renderer Creation Failed! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    font = TTF_OpenFont("assets/fonts/arial.ttf", 48);
    if (!font)
    {
        std::cerr << "Font loading failed! SDL_ttf Error: " << SDL_GetError() << std::endl;
        return false;
    }

    return true;
}

static void drawNumbers(SDL_Renderer *renderer, TTF_Font *font, bool whitePerspective)
{
    // Set the color for the numbers (white)
    SDL_Color textColor = {255, 255, 255, 255};

    for (int i = 0; i < BOARD_SIZE; i++)
    {
        int boardRow = whitePerspective ? i : BOARD_SIZE - 1 - i;
        std::string text = std::to_string(8 - boardRow);

        // Create a surface with the number as text
        SDL_Surface *textSurface = TTF_RenderText_Blended(font, text.c_str(), text.length(), textColor);
        if (!textSurface)
        {
            std::cerr << "Text rendering failed! SDL_ttf Error: " << SDL_GetError() << std::endl;
            return;
        }

        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_DestroySurface(textSurface);

        if (!textTexture)
        {
            std::cerr << "Texture creation failed! SDL_Error: " << SDL_GetError() << std::endl;
            return;
        }

        // Define the destination rectangle (positioning it on the left border, vertically)
        SDL_Rect textRect = {BOARD_OFFSET_X + BORDER_WIDTH / 2 - 8, SQUARE_SIZE / 2 + BOARD_OFFSET_Y + BORDER_WIDTH + i * SQUARE_SIZE - 8, 12, 20}; // Adjust size
        SDL_FRect fTextRect;
        SDL_RectToFRect(&textRect, &fTextRect);

        SDL_RenderTexture(renderer, textTexture, nullptr, &fTextRect);
        SDL_DestroyTexture(textTexture);
    }
}

static void drawLetters(SDL_Renderer *renderer, TTF_Font *font, bool whitePerspective)
{
    SDL_Color textColor = {255, 255, 255, 255}; // White color for text

    // Loop through letters 'A' to 'H'
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        char letter = whitePerspective ? ('a' + i) : ('h' - i);
        std::string letterStr(1, letter); // Convert the letter to a string

        // Render the letter with smoother anti-aliasing
        SDL_Surface *textSurface = TTF_RenderText_Blended(font, letterStr.c_str(), letterStr.length(), textColor);

        if (!textSurface)
        {
            std::cerr << "Text rendering failed! SDL_ttf Error: " << SDL_GetError() << std::endl;
            return;
        }

        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_DestroySurface(textSurface);

        if (!textTexture)
        {
            std::cerr << "Texture creation failed! SDL_Error: " << SDL_GetError() << std::endl;
            return;
        }

        // Define the destination rectangle (positioning it on the left border, vertically)
        SDL_Rect textRect = {BORDER_WIDTH_X + i * SQUARE_SIZE + (SQUARE_SIZE / 2) - 5, BORDER_WIDTH_Y + BOARD_SIZE * SQUARE_SIZE + 8, 10, 22};
        SDL_FRect fTextRect;
        SDL_RectToFRect(&textRect, &fTextRect);

        SDL_RenderTexture(renderer, textTexture, nullptr, &fTextRect);
        SDL_DestroyTexture(textTexture);
    }
}

static void DrawRounded(SDL_Renderer *r, float x, float y, float w, float h, float radius, SDL_Color c)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);

    // 1. Center rectangle
    SDL_FRect core = {x + radius, y, w - 2 * radius, h};
    SDL_RenderFillRect(r, &core);

    // 2. Left rectangle
    SDL_FRect left = {x, y + radius, radius, h - 2 * radius};
    SDL_RenderFillRect(r, &left);

    // 3. Right rectangle
    SDL_FRect right = {x + w - radius, y + radius, radius, h - 2 * radius};
    SDL_RenderFillRect(r, &right);

    // 4. Perfect corner circles
    for (float dy = -radius; dy <= radius; dy++)
    {
        float dx = sqrtf(radius * radius - dy * dy);

        // top-left corner
        SDL_RenderLine(r,
                       x + radius - dx, y + radius + dy,
                       x + radius + dx, y + radius + dy);

        // top-right corner
        SDL_RenderLine(r,
                       x + w - radius - dx, y + radius + dy,
                       x + w - radius + dx, y + radius + dy);

        // bottom-left
        SDL_RenderLine(r,
                       x + radius - dx, y + h - radius + dy,
                       x + radius + dx, y + h - radius + dy);

        // bottom-right
        SDL_RenderLine(r,
                       x + w - radius - dx, y + h - radius + dy,
                       x + w - radius + dx, y + h - radius + dy);
    }
}

static void DrawShadow(SDL_Renderer *r, float x, float y, float w, float h)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 120);
    SDL_FRect s{x + 6, y + 6, w, h};
    SDL_RenderFillRect(r, &s);
}

static void DrawTextCentered(SDL_Renderer *renderer, TTF_Font *font,
                             const std::string &text,
                             float x, float y, float w, float h,
                             SDL_Color color)
{
    SDL_Surface *surf = TTF_RenderText_Blended(font, text.c_str(), text.length(), color);
    if (!surf)
        return;

    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    int tw = surf->w;
    int th = surf->h;
    SDL_DestroySurface(surf);

    SDL_FRect dst{0.0f, 0.0f, 0.0f, 0.0f};
    dst.w = (float)tw;
    dst.h = (float)th;
    dst.x = x + (w - tw) / 2;
    dst.y = y + (h - th) / 2;

    SDL_RenderTexture(renderer, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}

static bool Hover(int mx, int my, float x, float y, float w, float h)
{
    return mx >= x && mx <= x + w && my >= y && my <= y + h;
}

static void FancyButton(SDL_Renderer *r, int mx, int my,
                        float x, float y, float w, float h,
                        const std::string &txt, TTF_Font *font)
{
    SDL_Color base = {55, 115, 255, 255};
    SDL_Color hover = {75, 135, 255, 255};

    SDL_Color col = Hover(mx, my, x, y, w, h) ? hover : base;

    DrawRounded(r, x, y, w, h, 10.0f, col);
    DrawTextCentered(r, font, txt, x, y, w, h, COLOR_TEXT);
}

static void DepthButton(SDL_Renderer *r, int mx, int my,
                        float x, float y, const char *symbol,
                        int &depth, int delta, TTF_Font *font)
{
    SDL_Color col = {90, 90, 220, 255};
    SDL_Color hov = {110, 110, 240, 255};
    bool h = Hover(mx, my, x, y, 50, 50);

    DrawRounded(r, x, y, 50, 50, 10, h ? hov : col);
    DrawTextCentered(r, font, symbol, x, y, 50, 50, {255, 255, 255, 255});

    if (h && SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_MASK(SDL_BUTTON_LEFT))
    {
        depth += delta;
        depth = std::max(1000, std::min(depth, 2500));
        SDL_Delay(180); // debounce
    }
}

static void DrawOverlay(SDL_Renderer *r)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 60);
    SDL_FRect full = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderFillRect(r, &full);
}

static void DrawBox(SDL_Renderer *r, float x, float y, float w, float h, SDL_Color c)
{
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_FRect rect{x, y, w, h};
    SDL_RenderFillRect(r, &rect);
}

static GameSettings ShowStartScreen(SDL_Renderer* r, TTF_Font* font, Board& board)
{
    GameSettings gs;
    SDL_Event e;

    int depth = 1200;
    bool pickingDepth = false;

    int mx = 0;
    int my = 0;

    const int colorButtonW = 95;
    const int colorButtonH = 70;
    const int colorGap = 8;

    const int colorY = MENU_Y1;

    const int totalColorWidth =
        colorButtonW * 3 + colorGap * 2;

    const int colorStartX =
        MENU_BUTTON_X +
        (MENU_BUTTON_W - totalColorWidth) / 2;

    const int blackX = colorStartX;
    const int randomX =
        blackX + colorButtonW + colorGap;
    const int whiteX =
        randomX + colorButtonW + colorGap;

    PlayerColorChoice playerColor =
        PlayerColorChoice::RANDOM;

    while (true)
    {
        SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        SDL_RenderClear(r);

        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT)
                exit(0);

            if (e.type == SDL_EVENT_MOUSE_MOTION)
            {
                mx = static_cast<int>(e.motion.x);
                my = static_cast<int>(e.motion.y);
            }

            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                // ==================================================
                // MAIN MENU
                // ==================================================

                if (!pickingDepth)
                {
                    // Create Room
                    if (Hover(mx, my,
                        MENU_BUTTON_X,
                        MENU_Y1,
                        MENU_BUTTON_W,
                        MENU_BUTTON_H))
                    {
                        gs.vsEngine = false;
                        currState = UIState::CREATE_ROOM;
                        return gs;
                    }

                    // Join Room
                    if (Hover(mx, my,
                        MENU_BUTTON_X,
                        MENU_Y2,
                        MENU_BUTTON_W,
                        MENU_BUTTON_H))
                    {
                        gs.vsEngine = false;
                        currState = UIState::JOIN_ROOM;
                        return gs;
                    }

                    // Stockfish
                    if (Hover(mx, my,
                        MENU_BUTTON_X,
                        MENU_Y3,
                        MENU_BUTTON_W,
                        MENU_BUTTON_H))
                    {
                        pickingDepth = true;
                    }

                    // Quit
                    if (Hover(mx, my,
                        MENU_SMALL_X,
                        MENU_BACK_Y,
                        MENU_SMALL_W,
                        MENU_SMALL_H))
                    {
                        exit(0);
                    }
                }

                // ==================================================
                // STOCKFISH SCREEN
                // ==================================================

                else
                {
                    // Black
                    if (Hover(
                        mx,
                        my,
                        blackX,
                        colorY,
                        colorButtonW,
                        colorButtonH))
                    {
                        playerColor =
                            PlayerColorChoice::BLACK;

                        std::cout
                            << "[UI] Selected BLACK\n";
                    }

                    // Random
                    else if (Hover(
                        mx,
                        my,
                        randomX,
                        colorY,
                        colorButtonW,
                        colorButtonH))
                    {
                        playerColor =
                            PlayerColorChoice::RANDOM;

                        std::cout
                            << "[UI] Selected RANDOM\n";
                    }

                    // White
                    else if (Hover(
                        mx,
                        my,
                        whiteX,
                        colorY,
                        colorButtonW,
                        colorButtonH))
                    {
                        playerColor =
                            PlayerColorChoice::WHITE;

                        std::cout
                            << "[UI] Selected WHITE\n";
                    }


                    // Start
                    if (Hover(mx, my,
                        MENU_BUTTON_X,
                        MENU_Y3,
                        MENU_BUTTON_W,
                        MENU_BUTTON_H))
                    {
                        gs.vsEngine = true;

                        gs.engineDepth =
                            static_cast<int>(1.05 * depth + 300);

                        // Resolve RANDOM here
                        if (playerColor == PlayerColorChoice::RANDOM)
                        {
                            gs.playerColor = (rand() % 2 == 0) ? PlayerColorChoice::WHITE : PlayerColorChoice::BLACK;
							playerColor = gs.playerColor; // Update the playerColor variable to reflect the resolved color
                        }
                        else
                            gs.playerColor = playerColor;

                        if (gs.playerColor == PlayerColorChoice::WHITE)
                            board.setPerspective(true);
                        else if (gs.playerColor == PlayerColorChoice::BLACK)
                            board.setPerspective(false);

                        std::cout
                            << "[Stockfish] Player color = "
                            << (gs.playerColor == PlayerColorChoice::WHITE
                                ? "WHITE"
                                : "BLACK")
                            << '\n';

                        return gs;
                    }

                    // BACK
                    if (Hover(mx, my,
                        MENU_SMALL_X,
                        MENU_BACK_Y,
                        MENU_SMALL_W,
                        MENU_SMALL_H))
                    {
                        pickingDepth = false;
                    }
                }
            }

            // ==================================================
            // KEYBOARD
            // ==================================================

            if (e.type == SDL_EVENT_KEY_DOWN)
            {
                if (pickingDepth)
                {
                    if (e.key.key == SDLK_LEFT)
                    {
                        depth -= 50;

                        depth = std::max(
                            1000,
                            std::min(depth, 2500));
                    }
                    else if (e.key.key == SDLK_RIGHT)
                    {
                        depth += 50;

                        depth = std::max(
                            1000,
                            std::min(depth, 2500));
                    }
                    else if (e.key.key == SDLK_RETURN)
                    {
                        gs.vsEngine = true;

                        gs.engineDepth =
                            static_cast<int>(1.05 * depth + 300);

                        // Resolve RANDOM here
                        if (playerColor == PlayerColorChoice::RANDOM)
                        {
                            gs.playerColor =
                                (rand() % 2 == 0)
                                ? PlayerColorChoice::WHITE
                                : PlayerColorChoice::BLACK;
							playerColor = gs.playerColor; // Update the playerColor variable to reflect the resolved color
                        }
                        else
                            gs.playerColor = playerColor;

                        if (gs.playerColor == PlayerColorChoice::WHITE)
                            board.setPerspective(true);
                        else if (gs.playerColor == PlayerColorChoice::BLACK)
                            board.setPerspective(false);

                        std::cout
                            << "[Stockfish] Player color = "
                            << (gs.playerColor == PlayerColorChoice::WHITE
                                ? "WHITE"
                                : "BLACK"
                                )
                            << '\n';

                        return gs;
                    }
                    else if (e.key.key == SDLK_ESCAPE)
                    {
                        pickingDepth = false;
                    }
                }
            }
        }

        // ==================================================
        // BACKGROUND
        // ==================================================

        board.drawBoard(r, font);

        SDL_SetRenderDrawColor(r, 28, 28, 28, 255);

        SDL_FRect panelBG = {
            MENU_PANEL_X,
            0,
            PANEL_WIDTH,
            SCREEN_HEIGHT
        };

        SDL_RenderFillRect(r, &panelBG);

        drawNumbers(r, font, board.whitePerspective);
        drawLetters(r, font, board.whitePerspective);

        DrawOverlay(r);

        // ==================================================
        // MAIN PANEL
        // ==================================================

        DrawShadow(
            r,
            MENU_PX,
            MENU_PY,
            MENU_PANEL_W,
            MENU_PANEL_H);

        DrawRounded(
            r,
            MENU_PX,
            MENU_PY,
            MENU_PANEL_W,
            MENU_PANEL_H,
            12,
            { 40, 40, 40, 230 });

        // ==================================================
        // MAIN MENU
        // ==================================================

        if (!pickingDepth)
        {
            DrawTextCentered(
                r,
                font,
                "Chessify",
                MENU_PX,
                MENU_PY + 20,
                MENU_PANEL_W,
                50,
                COLOR_TEXT);

            // Create Room
            FancyButton(
                r, mx, my,
                MENU_BUTTON_X,
                MENU_Y1,
                MENU_BUTTON_W,
                MENU_BUTTON_H,
                "Create Room",
                font);

            // Join Room
            FancyButton(
                r, mx, my,
                MENU_BUTTON_X,
                MENU_Y2,
                MENU_BUTTON_W,
                MENU_BUTTON_H,
                "Join Room",
                font);

            // Stockfish
            FancyButton(
                r, mx, my,
                MENU_BUTTON_X,
                MENU_Y3,
                MENU_BUTTON_W,
                MENU_BUTTON_H,
                "v/s Stockfish",
                font);

            // Quit
            DrawRounded(
                r,
                MENU_SMALL_X,
                MENU_BACK_Y,
                MENU_SMALL_W,
                MENU_SMALL_H,
                10,
                { 200, 60, 60, 255 });

            DrawTextCentered(
                r,
                font,
                "Quit",
                MENU_SMALL_X,
                MENU_BACK_Y,
                MENU_SMALL_W,
                MENU_SMALL_H,
                COLOR_TEXT);
        }

        else
        {
            // ==================================================
            // STOCKFISH SCREEN
            // ==================================================

            // TITLE
            DrawTextCentered(
                r,
                font,
                "Play Stockfish",
                MENU_PX,
                MENU_PY + 20,
                MENU_PANEL_W,
                50,
                COLOR_TEXT);

            // ==================================================
            // LOAD KING TEXTURES
            // ==================================================

            SDL_Texture* blackKing =
                Board::pieceTextures["bk"];

            SDL_Texture* whiteKing =
                Board::pieceTextures["wk"];


            // ==================================================
            // BLACK
            // ==================================================

            DrawRounded(
                r,
                blackX,
                colorY,
                colorButtonW,
                colorButtonH,
                10,
                playerColor == PlayerColorChoice::BLACK
                ? SDL_Color{ 90, 90, 90, 255 }
                : SDL_Color{ 60, 60, 60, 255 }
            );

            if (blackKing)
            {
                SDL_FRect dst = {
                    static_cast<float>(blackX + 15),
                    static_cast<float>(colorY + 5),
                    static_cast<float>(colorButtonW - 30),
                    static_cast<float>(colorButtonH - 10)
                };

                SDL_RenderTexture(
                    r,
                    blackKing,
                    nullptr,
                    &dst
                );
            }

            // ==================================================
            // RANDOM
            // ==================================================

            DrawRounded(
                r,
                randomX,
                colorY,
                colorButtonW,
                colorButtonH,
                10,
                playerColor == PlayerColorChoice::RANDOM
                ? SDL_Color{ 90, 90, 90, 255 }
                : SDL_Color{ 60, 60, 60, 255 }
            );

            if (blackKing && whiteKing)
            {
                float blackW, blackH;
                float whiteW, whiteH;

                SDL_GetTextureSize(
                    blackKing,
                    &blackW,
                    &blackH
                );

                SDL_GetTextureSize(
                    whiteKing,
                    &whiteW,
                    &whiteH
                );

                // --------------------------------------------------
                // BLACK LEFT HALF
                // --------------------------------------------------

                SDL_FRect blackSrc = {
                    0.0f,
                    0.0f,
                    blackW / 2.0f,
                    blackH
                };

                SDL_FRect blackDst = {
                    static_cast<float>(randomX + 15),
                    static_cast<float>(colorY + 5),
                    static_cast<float>((colorButtonW - 30) / 2),
                    static_cast<float>(colorButtonH - 10)
                };

                SDL_RenderTexture(
                    r,
                    blackKing,
                    &blackSrc,
                    &blackDst
                );

                // --------------------------------------------------
                // WHITE RIGHT HALF
                // --------------------------------------------------

                SDL_FRect whiteSrc = {
                    whiteW / 2.0f,
                    0.0f,
                    whiteW / 2.0f,
                    whiteH
                };

                SDL_FRect whiteDst = {
                    static_cast<float>(
                        randomX + 15 +
                        (colorButtonW - 30) / 2
                    ),
                    static_cast<float>(colorY + 5),
                    static_cast<float>((colorButtonW - 30) / 2),
                    static_cast<float>(colorButtonH - 10)
                };

                SDL_RenderTexture(
                    r,
                    whiteKing,
                    &whiteSrc,
                    &whiteDst
                );
            }

            // ==================================================
            // WHITE
            // ==================================================

            DrawRounded(
                r,
                whiteX,
                colorY,
                colorButtonW,
                colorButtonH,
                10,
                playerColor == PlayerColorChoice::WHITE
                ? SDL_Color{ 90, 90, 90, 255 }
                : SDL_Color{ 60, 60, 60, 255 }
            );

            if (whiteKing)
            {
                SDL_FRect dst = {
                    static_cast<float>(whiteX + 15),
                    static_cast<float>(colorY + 5),
                    static_cast<float>(colorButtonW - 30),
                    static_cast<float>(colorButtonH - 10)
                };

                SDL_RenderTexture(
                    r,
                    whiteKing,
                    nullptr,
                    &dst
                );
            }


            // ==================================================
            // RATING
            // ==================================================

            DepthButton(
                r,
                mx,
                my,
                MENU_BUTTON_X,
                MENU_Y2,
                "-",
                depth,
                -50,
                font
            );

            DrawTextCentered(
                r,
                font,
                std::to_string(depth),
                MENU_BUTTON_X + 55,
                MENU_Y2,
                MENU_BUTTON_W - 110,
                MENU_BUTTON_H,
                COLOR_TEXT
            );

            DepthButton(
                r,
                mx,
                my,
                MENU_BUTTON_X + MENU_BUTTON_W - 50,
                MENU_Y2,
                "+",
                depth,
                +50,
                font
            );


            // ==================================================
            // START
            // ==================================================

            FancyButton(
                r,
                mx,
                my,
                MENU_BUTTON_X,
                MENU_Y3,
                MENU_BUTTON_W,
                MENU_BUTTON_H,
                "Start",
                font
            );


            // ==================================================
            // BACK
            // ==================================================

            DrawRounded(
                r,
                MENU_SMALL_X,
                MENU_BACK_Y,
                MENU_SMALL_W,
                MENU_SMALL_H,
                10,
                { 200, 60, 60, 255 }
            );

            DrawTextCentered(
                r,
                font,
                "Back",
                MENU_SMALL_X,
                MENU_BACK_Y,
                MENU_SMALL_W,
                MENU_SMALL_H,
                COLOR_TEXT
            );
        }

        SDL_RenderPresent(r);
        SDL_Delay(12);
    }
}

static void ShowCreateRoomScreen(
    SDL_Renderer* r,
    TTF_Font* font,
    Board& board,
    NetworkClient& network)
{
    SDL_Event e;

    int mx = 0;
    int my = 0;

    // ============================================================
    // CREATE ROOM AUTOMATICALLY WHEN ENTERING THIS SCREEN
    // ============================================================

    std::cout << "[UI] Entered Create Room screen\n";

    if (!network.isConnected())
    {
        std::cout
            << "[Network] Connecting to server...\n";

        if (!network.connect(
            "chessify-production.up.railway.app",
            443))
        {
            std::cout
                << "[Network] Could not connect to server\n";

            currState = UIState::START_MENU;
            return;
        }
    }

    std::cout
        << "[Network] Creating room...\n";

    if (!network.createRoom())
    {
        std::cout
            << "[Network] Failed to create room\n";

        currState = UIState::START_MENU;
        return;
    }

    std::cout
        << "[Network] Create room request sent\n";

    // ============================================================
    // SCREEN LOOP
    // ============================================================

    while (currState == UIState::CREATE_ROOM)
    {
        SDL_SetRenderDrawColor(
            r,
            0, 0, 0, 255);

        SDL_RenderClear(r);

        // ========================================================
        // EVENTS
        // ========================================================

        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT)
                exit(0);

            if (e.type == SDL_EVENT_MOUSE_MOTION)
            {
                mx = static_cast<int>(e.motion.x);
                my = static_cast<int>(e.motion.y);
            }

            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                // =================================================
                // COPY CODE
                // =================================================

                if (Hover(
                    mx, my,
                    MENU_BUTTON_X,
                    MENU_Y2,
                    MENU_BUTTON_W,
                    MENU_BUTTON_H))
                {
                    std::string roomCode =
                        network.getRoomCode();

                    if (!roomCode.empty())
                    {
                        SDL_SetClipboardText(
                            roomCode.c_str());

                        std::cout
                            << "[UI] Room code copied: "
                            << roomCode
                            << '\n';
                    }
                    else
                    {
                        std::cout
                            << "[UI] No room code yet\n";
                    }
                }

                // =================================================
                // CREATE ROOM TILE
                //
                // Room is already being created automatically.
                // Therefore this does nothing.
                // =================================================

                else if (Hover(
                    mx, my,
                    MENU_BUTTON_X,
                    MENU_Y3,
                    MENU_BUTTON_W,
                    MENU_BUTTON_H))
                {
                    std::cout
                        << "[UI] Create Room tile clicked\n";

                    currState = UIState::WAITING_ROOM;
                    return;
                }

                // =================================================
                // BACK
                // =================================================

                else if (Hover(
                    mx, my,
                    MENU_SMALL_X,
                    MENU_BACK_Y,
                    MENU_SMALL_W,
                    MENU_SMALL_H))
                {
                    std::cout
                        << "[UI] Back clicked\n";

                    if (network.isConnected())
                        network.disconnect();

                    currState =
                        UIState::START_MENU;

                    return;
                }
            }

            // ========================================================
            // ESCAPE
            // ========================================================

            if (e.type == SDL_EVENT_KEY_DOWN)
            {
                if (e.key.key == SDLK_ESCAPE)
                {
                    if (network.isConnected())
                        network.disconnect();

                    currState =
                        UIState::START_MENU;

                    return;
                }

                if (e.key.key == SDLK_RETURN)
                {
                    std::cout << "[UI] Enter pressed - Create Room\n";

                    if (!network.isConnected())
                    {
                        std::cout << "[Network] Connecting to server...\n";

                        if (!network.connect(
                            "chessify-production.up.railway.app",
                            443))
                        {
                            std::cout << "[Network] Could not connect to server\n";
                            continue;
                        }
                    }

                    if (network.createRoom())
                    {
                        std::cout << "[Network] Create room request sent\n";

                        currState = UIState::WAITING_ROOM;
                        return;
                    }
                }
            }
        }

        // ============================================================
        // GET ROOM CODE
        //
        // NetworkClient receives ROOM_CREATED asynchronously.
        // ============================================================

        std::string roomCode =
            network.getRoomCode();

        // ============================================================
        // BACKGROUND
        // ============================================================

        board.drawBoard(r, font);

        SDL_SetRenderDrawColor(
            r,
            28, 28, 28, 255);

        SDL_FRect panelBG = {
            MENU_PANEL_X,
            0,
            PANEL_WIDTH,
            SCREEN_HEIGHT
        };

        SDL_RenderFillRect(
            r,
            &panelBG);

        drawNumbers(r, font, board.whitePerspective);
        drawLetters(r, font, board.whitePerspective);

        DrawOverlay(r);

        // ============================================================
        // PANEL
        // ============================================================

        DrawShadow(
            r,
            MENU_PX,
            MENU_PY,
            MENU_PANEL_W,
            MENU_PANEL_H);

        DrawRounded(
            r,
            MENU_PX,
            MENU_PY,
            MENU_PANEL_W,
            MENU_PANEL_H,
            12,
            { 40, 40, 40, 230 });

        DrawTextCentered(
            r,
            font,
            "Create Room",
            MENU_PX,
            MENU_PY + 20,
            MENU_PANEL_W,
            50,
            COLOR_TEXT);

        // ============================================================
        // ROOM CODE
        // Same position as CREATE ROOM on main screen
        // ============================================================

        DrawRounded(
            r,
            MENU_BUTTON_X,
            MENU_Y1,
            MENU_BUTTON_W,
            MENU_BUTTON_H,
            10,
            { 60, 60, 60, 255 });

        if (roomCode.empty())
        {
            DrawTextCentered(
                r,
                font,
                "Room Code",
                MENU_BUTTON_X,
                MENU_Y1,
                MENU_BUTTON_W,
                MENU_BUTTON_H,
                { 150, 150, 150, 255 });
        }
        else
        {
            DrawTextCentered(
                r,
                font,
                roomCode,
                MENU_BUTTON_X,
                MENU_Y1,
                MENU_BUTTON_W,
                MENU_BUTTON_H,
                COLOR_TEXT);
        }

        // ============================================================
        // COPY CODE
        // Same position as JOIN ROOM
        // ============================================================

        FancyButton(
            r,
            mx,
            my,
            MENU_BUTTON_X,
            MENU_Y2,
            MENU_BUTTON_W,
            MENU_BUTTON_H,
            "Copy Code",
            font);

        // ============================================================
        // CREATE ROOM
        // Same position as STOCKFISH
        // ============================================================

        FancyButton(
            r,
            mx,
            my,
            MENU_BUTTON_X,
            MENU_Y3,
            MENU_BUTTON_W,
            MENU_BUTTON_H,
            "Create",
            font);

        // ============================================================
        // BACK
        // Same position as QUIT
        // ============================================================

        DrawRounded(
            r,
            MENU_SMALL_X,
            MENU_BACK_Y,
            MENU_SMALL_W,
            MENU_SMALL_H,
            10,
            { 200, 60, 60, 255 });

        DrawTextCentered(
            r,
            font,
            "Back",
            MENU_SMALL_X,
            MENU_BACK_Y,
            MENU_SMALL_W,
            MENU_SMALL_H,
            COLOR_TEXT);

        SDL_RenderPresent(r);

        SDL_Delay(12);
    }
}

static void ShowWaitingRoomScreen(
    SDL_Renderer* r,
    TTF_Font* font,
    Board& board,
    NetworkClient& network)
{
    SDL_Event e;

    int mx = 0;
    int my = 0;

    while (currState == UIState::WAITING_ROOM)
    {
        // Opponent has joined
        if (network.isReady())
        {
            std::cout << "[Network] Opponent joined! Starting game.\n";

            // Make sure no mouse button from the previous screen
            // is carried into the chess board.
            while (SDL_GetMouseState(nullptr, nullptr) &
                (SDL_BUTTON_MASK(SDL_BUTTON_LEFT) |
                    SDL_BUTTON_MASK(SDL_BUTTON_RIGHT)))
            {
                SDL_PumpEvents();
                SDL_Delay(10);
            }

            SDL_FlushEvents(
                SDL_EVENT_FIRST,
                SDL_EVENT_LAST);

            currState = UIState::PLAYING;
            return;
        }

        SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        SDL_RenderClear(r);

        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT)
                exit(0);

            if (e.type == SDL_EVENT_MOUSE_MOTION)
            {
                mx = static_cast<int>(e.motion.x);
                my = static_cast<int>(e.motion.y);
            }

            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                // BACK / CANCEL
                if (Hover(mx, my,
                    MENU_SMALL_X,
                    MENU_BACK_Y,
                    MENU_SMALL_W,
                    MENU_SMALL_H))
                {
                    if (network.isConnected())
                        network.disconnect();

                    currState = UIState::START_MENU;
                    return;
                }
            }

            if (e.type == SDL_EVENT_KEY_DOWN)
            {
                if (e.key.key == SDLK_ESCAPE)
                {
                    if (network.isConnected())
                        network.disconnect();

                    currState = UIState::START_MENU;
                    return;
                }
            }
        }

        // ====================================================
        // BACKGROUND
        // ====================================================

        board.drawBoard(r, font);

        SDL_SetRenderDrawColor(r, 28, 28, 28, 255);

        SDL_FRect panelBG = {
            MENU_PANEL_X,
            0,
            PANEL_WIDTH,
            SCREEN_HEIGHT
        };

        SDL_RenderFillRect(r, &panelBG);

        drawNumbers(r, font, board.whitePerspective);
        drawLetters(r, font, board.whitePerspective);

        DrawOverlay(r);

        // ====================================================
        // PANEL
        // ====================================================

        DrawShadow(
            r,
            MENU_PX,
            MENU_PY,
            MENU_PANEL_W,
            MENU_PANEL_H);

        DrawRounded(
            r,
            MENU_PX,
            MENU_PY,
            MENU_PANEL_W,
            MENU_PANEL_H,
            12,
            { 40, 40, 40, 230 });

        DrawTextCentered(
            r,
            font,
            "Waiting for Opponent",
            MENU_PX,
            MENU_PY + 20,
            MENU_PANEL_W,
            50,
            COLOR_TEXT);

        // ====================================================
        // ROOM CODE LABEL
        // ====================================================

        DrawTextCentered(
            r,
            font,
            "Your Room Code",
            MENU_PX,
            MENU_PY + 75,
            MENU_PANEL_W,
            40,
            { 190, 190, 190, 255 });

        // ====================================================
        // ROOM CODE
        // ====================================================

        std::string roomCode = network.getRoomCode();

        DrawRounded(
            r,
            MENU_BUTTON_X,
            MENU_Y1,
            MENU_BUTTON_W,
            MENU_BUTTON_H,
            10,
            { 60, 60, 60, 255 });

        if (roomCode.empty())
        {
            DrawTextCentered(
                r,
                font,
                "Generating...",
                MENU_BUTTON_X,
                MENU_Y1,
                MENU_BUTTON_W,
                MENU_BUTTON_H,
                { 150, 150, 150, 255 });
        }
        else
        {
            DrawTextCentered(
                r,
                font,
                roomCode,
                MENU_BUTTON_X,
                MENU_Y1,
                MENU_BUTTON_W,
                MENU_BUTTON_H,
                COLOR_TEXT);
        }

        // ====================================================
        // STATUS
        // ====================================================

        DrawTextCentered(
            r,
            font,
            "Share this code with your opponent",
            MENU_PX,
            MENU_Y2,
            MENU_PANEL_W,
            MENU_BUTTON_H,
            { 190, 190, 190, 255 });

        DrawTextCentered(
            r,
            font,
            "Waiting for opponent...",
            MENU_PX,
            MENU_Y3,
            MENU_PANEL_W,
            MENU_BUTTON_H,
            { 190, 190, 190, 255 });

        // ====================================================
        // BACK
        // ====================================================

        DrawRounded(
            r,
            MENU_SMALL_X,
            MENU_BACK_Y,
            MENU_SMALL_W,
            MENU_SMALL_H,
            10,
            { 90, 90, 90, 255 });

        DrawTextCentered(
            r,
            font,
            "CANCEL",
            MENU_SMALL_X,
            MENU_BACK_Y,
            MENU_SMALL_W,
            MENU_SMALL_H,
            COLOR_TEXT);

        SDL_RenderPresent(r);
        SDL_Delay(12);
    }
}

static void ShowJoinRoomScreen(
    SDL_Window* window,
    SDL_Renderer* r,
    TTF_Font* font,
    Board& board,
    NetworkClient& network)
{
    SDL_Event e;
    int mx = 0, my = 0;

    std::string roomCode;
    bool inputActive = false;

    SDL_StartTextInput(window);

    while (currState == UIState::JOIN_ROOM)
    {
        SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        SDL_RenderClear(r);


        if (network.consumeRoomFull())
        {
            roomCode.clear();
            inputActive = false;

            SDL_StartTextInput(window);
        }

        if (network.consumeRoomJoined())
        {
            SDL_StopTextInput(window);

            currState = UIState::WAITING_ROOM;
            return;
        }

        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT)
                exit(0);

            // ================= TEXT INPUT =================

            if (e.type == SDL_EVENT_TEXT_INPUT)
            {
                if (inputActive)
                {
                    for (const char* p = e.text.text; *p; ++p)
                    {
                        if (std::isalnum(
                            static_cast<unsigned char>(*p)))
                        {
                            roomCode += static_cast<char>(
                                std::toupper(
                                    static_cast<unsigned char>(*p)));
                        }
                    }

                    // Room codes are 6 characters
                    if (roomCode.length() > 6)
                        roomCode.resize(6);
                }
            }

            // ================= MOUSE =================

            if (e.type == SDL_EVENT_MOUSE_MOTION)
            {
                mx = static_cast<int>(e.motion.x);
                my = static_cast<int>(e.motion.y);
            }

            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                // ================= CODE INPUT =================

                if (Hover(mx, my,
                    MENU_BUTTON_X,
                    MENU_Y1,
                    MENU_BUTTON_W,
                    MENU_BUTTON_H))
                {
                    inputActive = true;
                }

                // ================= PASTE CODE =================

                if (Hover(mx, my,
                    MENU_BUTTON_X,
                    MENU_Y2,
                    MENU_BUTTON_W,
                    MENU_BUTTON_H))
                {
                    char* clipboardText = SDL_GetClipboardText();

                    if (clipboardText)
                    {
                        roomCode.clear();

                        for (const char* p = clipboardText;
                            *p && roomCode.length() < 6;
                            ++p)
                        {
                            if (std::isalnum(
                                static_cast<unsigned char>(*p)))
                            {
                                roomCode += static_cast<char>(
                                    std::toupper(
                                        static_cast<unsigned char>(*p)));
                            }
                        }

                        SDL_free(clipboardText);

                        inputActive = true;

                        std::cout
                            << "[UI] Pasted room code: "
                            << roomCode
                            << '\n';
                    }
                    else
                    {
                        std::cout
                            << "[UI] Clipboard is empty\n";
                    }
                }

                // ================= JOIN =================

                if (Hover(mx, my,
                    MENU_BUTTON_X,
                    MENU_Y3,
                    MENU_BUTTON_W,
                    MENU_BUTTON_H))
                {
                    std::cout
                        << "[UI] Join Room clicked\n";

                    if (roomCode.length() != 6)
                    {
                        std::cout
                            << "[Network] Invalid room code\n";

                        continue;
                    }

                    if (!network.isConnected())
                    {
                        std::cout
                            << "[Network] Connecting to server...\n";

                        if (!network.connect(
                            "chessify-production.up.railway.app",
                            443))
                        {
                            std::cout
                                << "[Network] Could not connect to server\n";

                            continue;
                        }
                    }

                    if (network.joinRoom(roomCode))
                    {
                        std::cout
                            << "[Network] Join request sent for room: "
                            << roomCode
                            << "\n";
                    }
                    else
                    {
                        std::cout
                            << "[Network] Failed to join room\n";
                    }
                }

                // ================= BACK =================

                if (Hover(mx, my,
                    MENU_SMALL_X,
                    MENU_BACK_Y,
                    MENU_SMALL_W,
                    MENU_SMALL_H))
                {
                    std::cout
                        << "[UI] Back clicked\n";

                    SDL_StopTextInput(window);

                    if (network.isConnected())
                        network.disconnect();

                    currState = UIState::START_MENU;
                    return;
                }
            }

            // ================= KEYBOARD =================

            if (e.type == SDL_EVENT_KEY_DOWN)
            {
                // Backspace
                if (e.key.key == SDLK_BACKSPACE &&
                    inputActive)
                {
                    if (!roomCode.empty())
                        roomCode.pop_back();
                }

                // Escape
                if (e.key.key == SDLK_ESCAPE)
                {
                    SDL_StopTextInput(window);

                    if (network.isConnected())
                        network.disconnect();

                    currState = UIState::START_MENU;
                    return;
                }

                if (e.key.key == SDLK_RETURN && inputActive)
                {
                    if (roomCode.length() != 6)
                    {
                        std::cout << "[Network] Invalid room code\n";
                        continue;
                    }

                    std::cout << "[UI] Enter pressed - Join Room\n";

                    if (!network.isConnected())
                    {
                        std::cout << "[Network] Connecting to server...\n";

                        if (!network.connect(
                            "chessify-production.up.railway.app",
                            443))
                        {
                            std::cout << "[Network] Could not connect to server\n";
                            continue;
                        }
                    }

                    if (network.joinRoom(roomCode))
                    {
                        std::cout << "[Network] Join request sent for room: "
                            << roomCode << "\n";
                    }

                    std::cout << "[Network] Failed to join room\n";
                }
            }
        }

        // ==================================================
        // BACKGROUND
        // ==================================================

        board.drawBoard(r, font);

        SDL_SetRenderDrawColor(r, 28, 28, 28, 255);

        SDL_FRect panelBG = {
            MENU_PANEL_X,
            0,
            PANEL_WIDTH,
            SCREEN_HEIGHT
        };

        SDL_RenderFillRect(r, &panelBG);

        drawNumbers(r, font, board.whitePerspective);
        drawLetters(r, font, board.whitePerspective);

        DrawOverlay(r);

        // ==================================================
        // PANEL
        // ==================================================

        DrawShadow(
            r,
            MENU_PX,
            MENU_PY,
            MENU_PANEL_W,
            MENU_PANEL_H);

        DrawRounded(
            r,
            MENU_PX,
            MENU_PY,
            MENU_PANEL_W,
            MENU_PANEL_H,
            12,
            { 40, 40, 40, 230 });

        // ================= TITLE =================

        DrawTextCentered(
            r,
            font,
            "Join Room",
            MENU_PX,
            MENU_PY + 20,
            MENU_PANEL_W,
            50,
            COLOR_TEXT);

        // ==================================================
        // ROOM CODE INPUT
        // ==================================================

        SDL_Color inputColor =
            inputActive
            ? SDL_Color{ 75, 75, 75, 255 }
        : SDL_Color{ 60, 60, 60, 255 };

        DrawRounded(
            r,
            MENU_BUTTON_X,
            MENU_Y1,
            MENU_BUTTON_W,
            MENU_BUTTON_H,
            10,
            inputColor);

        std::string displayCode = roomCode;

        if (displayCode.empty())
            displayCode = "Enter Code";

        SDL_Color codeColor =
            roomCode.empty()
            ? SDL_Color{ 150, 150, 150, 255 }
        : COLOR_TEXT;

        DrawTextCentered(
            r,
            font,
            displayCode,
            MENU_BUTTON_X,
            MENU_Y1,
            MENU_BUTTON_W,
            MENU_BUTTON_H,
            codeColor);

        // ==================================================
        // PASTE CODE
        // ==================================================

        FancyButton(
            r,
            mx,
            my,
            MENU_BUTTON_X,
            MENU_Y2,
            MENU_BUTTON_W,
            MENU_BUTTON_H,
            "Paste Code",
            font);

        // ==================================================
        // JOIN
        // ==================================================

        FancyButton(
            r,
            mx,
            my,
            MENU_BUTTON_X,
            MENU_Y3,
            MENU_BUTTON_W,
            MENU_BUTTON_H,
            "Join",
            font);

        // ==================================================
        // BACK
        // ==================================================

        DrawRounded(
            r,
            MENU_SMALL_X,
            MENU_BACK_Y,
            MENU_SMALL_W,
            MENU_SMALL_H,
            10,
            { 200, 60, 60, 255 });

        DrawTextCentered(
            r,
            font,
            "Back",
            MENU_SMALL_X,
            MENU_BACK_Y,
            MENU_SMALL_W,
            MENU_SMALL_H,
            COLOR_TEXT);

        SDL_RenderPresent(r);
        SDL_Delay(12);
    }
}

static void DrawConnectingScreen(SDL_Renderer* renderer, TTF_Font* font, Board& board, const std::string& text)
{
    // Draw board
    board.drawBoard(renderer, font);

    // Right panel background
    SDL_SetRenderDrawColor(renderer, 28, 28, 28, 255);

    SDL_FRect panelBG =
    {
        2 * BORDER_WIDTH_X + BOARD_SIZE * SQUARE_SIZE,
        0,
        PANEL_WIDTH,
        SCREEN_HEIGHT
    };

    SDL_RenderFillRect(renderer, &panelBG);

    drawNumbers(renderer, font, board.whitePerspective);
    drawLetters(renderer, font, board.whitePerspective);

    // Dark overlay
    DrawOverlay(renderer);

    const float PW = 420;
    const float PH = 220;

    float rightPanelX =
        2 * BORDER_WIDTH_X + BOARD_SIZE * SQUARE_SIZE;

    float PX =
        rightPanelX + (PANEL_WIDTH - PW) / 2;

    float PY =
        (SCREEN_HEIGHT - PH) / 2;

    //DrawShadow(renderer, PX, PY, PW, PH);

    //DrawRounded(
    //    renderer,
    //    PX,
    //    PY,
    //    PW,
    //    PH,
    //    12,
    //    { 40,40,40,230 });

    DrawTextCentered(
        renderer,
        font,
        "Chessify Online",
        PX,
        PY + 25,
        PW,
        50,
        COLOR_TEXT);

    DrawTextCentered(
        renderer,
        font,
        text,
        PX,
        PY + 100,
        PW,
        50,
        COLOR_TEXT);

    SDL_RenderPresent(renderer);

    //int mx = 0, my = 0;
    //while (true)
    //{
    //    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    //    SDL_RenderClear(r);

    //    while (SDL_PollEvent(&e))
    //    {
    //        if (e.type == SDL_EVENT_QUIT)
    //            exit(0);

    //        if (e.type == SDL_EVENT_MOUSE_MOTION)
    //            mx = (int)e.motion.x;
    //        my = (int)e.motion.y;

    //        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    //        {
    //            // Quit button
    //            if (Hover(mx, my, PX + 145, PY + 300, 130, 40))
    //                exit(0);
    //        }
    //    }
    //}
}

static SDL_Texture *loadTexture(SDL_Renderer *renderer, const std::string &path)
{
    SDL_Surface *surface = SDL_LoadBMP(path.c_str());
    if (!surface)
    {
        std::cerr << "Image Load Failed: " << path << " SDL_Error: " << SDL_GetError() << std::endl;
        return nullptr;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);

    if (!texture)
        std::cerr << "Texture Creation Failed: " << path << " SDL_Error: " << SDL_GetError() << std::endl;
    return texture;
}

// Function to render the piece
static void render(
    SDL_Renderer* renderer,
    TTF_Font* font,
    Board& board,
    MovePanel& panel,
    MoveHistory& moveHistory,
    NetworkClient& network)
{
    board.drawBoard(renderer, font);

    PlayerColor displayColor = PlayerColor::None;

    // Online multiplayer
    if (network.isConnected())
    {
        displayColor = network.getPlayerColor();
    }
    // Stockfish
    else if (SETTINGS.vsEngine)
    {
        if (SETTINGS.playerColor == PlayerColorChoice::WHITE)
            displayColor = PlayerColor::White;
        else if (SETTINGS.playerColor == PlayerColorChoice::BLACK)
            displayColor = PlayerColor::Black;
    }

    panel.draw(
        renderer,
        font,
        moveHistory,
        displayColor
    );

    if (board.gameOver)
    {
        board.drawGameOverScreen(renderer, font);
        return;
    }

    drawNumbers(renderer, font, board.whitePerspective);
    drawLetters(renderer, font, board.whitePerspective);
}
static void DrawConnectionLostScreen(SDL_Renderer* renderer,
    TTF_Font* font,
    Board& board,
    float mx,
    float my)
{
    // Draw board
    board.drawBoard(renderer, font);

    // Right panel background
    SDL_SetRenderDrawColor(renderer, 28, 28, 28, 255);

    SDL_FRect panelBG =
    {
        2 * BORDER_WIDTH_X + BOARD_SIZE * SQUARE_SIZE,
        0,
        PANEL_WIDTH,
        SCREEN_HEIGHT
    };

    SDL_RenderFillRect(renderer, &panelBG);

    drawNumbers(renderer, font, board.whitePerspective);
    drawLetters(renderer, font, board.whitePerspective);

    // Dark overlay
    DrawOverlay(renderer);

    const float PW = 420;
    const float PH = 260;

    float rightPanelX =
        2 * BORDER_WIDTH_X + BOARD_SIZE * SQUARE_SIZE;

    float PX =
        rightPanelX + (PANEL_WIDTH - PW) / 2;

    float PY =
        (SCREEN_HEIGHT - PH) / 2;

    DrawTextCentered(
        renderer,
        font,
        "Connection Lost",
        PX,
        PY + 25,
        PW,
        50,
        COLOR_TEXT);

    DrawTextCentered(
        renderer,
        font,
        "Your opponent disconnected.",
        PX,
        PY + 90,
        PW,
        50,
        COLOR_TEXT);

    // Back button
    const float BW = 180;
    const float BH = 45;

    float BX = PX + (PW - BW) / 2;
    float BY = PY + 170;

    bool hover =
        mx >= BX && mx <= BX + BW &&
        my >= BY && my <= BY + BH;

    SDL_Color btnColor =
        hover ? SDL_Color{ 90, 150, 255, 255 }
    : SDL_Color{ 65, 105, 225, 255 };

    DrawRounded(
        renderer,
        BX,
        BY,
        BW,
        BH,
        8,
        btnColor);

    DrawTextCentered(
        renderer,
        font,
        "Back to Menu",
        BX,
        BY + 5,
        BW,
        BH,
        { 255,255,255,255 });

    //SDL_RenderPresent(renderer);
}

static void DrawExitConfirm(SDL_Renderer *r, TTF_Font *font, float mx, float my)
{
    // Dark overlay
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 160);
    SDL_FRect bg = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderFillRect(r, &bg);

    // Dialog box
    float w = 360, h = 180;
    float x = (SCREEN_WIDTH - w) / 2;
    float y = (SCREEN_HEIGHT - h) / 2;

    DrawRounded(r, x, y, w, h, 20, {40, 40, 40, 240});

    DrawTextCentered(r, font, "Quit Game?", x, y + 20, w, 40, {255, 255, 255, 255});

    // Button colors based on selection
    SDL_Color yesColor = exitYesSelected ? SDL_Color{240, 90, 90, 255} : SDL_Color{200, 70, 70, 255};

    SDL_Color noColor = !exitYesSelected ? SDL_Color{100, 170, 255, 255} : SDL_Color{70, 140, 255, 255};

    // YES button
    DrawRounded(r, x + 40, y + 100, 120, 40, 12, yesColor);
    DrawTextCentered(r, font, "YES", x + 40, y + 100, 120, 40, {255, 255, 255, 255});

    // NO button
    DrawRounded(r, x + 200, y + 100, 120, 40, 12, noColor);
    DrawTextCentered(r, font, "NO", x + 200, y + 100, 120, 40, {255, 255, 255, 255});
}

static void close(SDL_Window *window, SDL_Renderer *renderer, TTF_Font *font)
{
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
    if (font)
        TTF_CloseFont(font);

    TTF_Quit();
    SDL_Quit();
}

int main()
{
    SDL_Window *window = nullptr;
    SDL_Surface *icon = nullptr;
    SDL_Renderer *renderer = nullptr;
    TTF_Font *font = nullptr;

    if (!init(window, renderer, font, icon))
        return -1;

    Board board;
    board.loadTextures(renderer);

    MoveHistory moveHistory;

    NetworkClient network;
    GameController gameController(board, moveHistory, network);

    MovePanel panel(
        2 * BORDER_WIDTH_X + BOARD_SIZE * SQUARE_SIZE,
        0,
        PANEL_WIDTH,
        1080);

    bool quit = false, playerMoved = false, startMenuDone = false;
    Piece *selectedPiece = nullptr;

    while (!quit)
    {
        if (currState == UIState::PLAYING && network.hasDisconnected()) {
            std::cout << "[UI] Switching to CONNECTION_LOST\n";

            board.setConnectionGameOver();
        }

        // ==========================================
        // PROCESS REMOTE NETWORK MOVES
        // ==========================================

        if (network.isConnected() &&
            network.isReady())
        {
            PlayerColor color = network.getPlayerColor();

            if (color == PlayerColor::White)
                board.setPerspective(true);
            else if (color == PlayerColor::Black)
                board.setPerspective(false);

            while (network.hasPendingMessages())
            {
                std::cout
                    << "[MAIN] Processing incoming network move\n";

                MoveMessage move =
                    network.getNextMove();

                std::cout
                    << "[MAIN] Got move: "
                    << move.fromRow << " "
                    << move.fromCol << " "
                    << move.toRow << " "
                    << move.toCol
                    << '\n';

                bool moveApplied =
                    gameController.applyRemoteMove(move);

                std::cout
                    << "[MAIN] applyRemoteMove returned: "
                    << moveApplied
                    << '\n';

                if (!moveApplied)
                {
                    std::cout
                        << "[MAIN] ERROR: Remote move could not be applied\n";
                }
            }
        }

        if (network.hasDisconnected() && !board.gameOver)
            board.setConnectionGameOver();

        playerMoved = false;

        float mx, my;
        SDL_GetMouseState(&mx, &my);

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (currState == UIState::START_MENU)
                continue;

            switch (event.type)
            {
            case SDL_EVENT_QUIT:
            {
                if (SETTINGS.vsEngine)
                    Engine::stop();

                quit = true;
            }
            break;

            case SDL_EVENT_KEY_DOWN:
            {
                if (event.key.key == SDLK_ESCAPE)
                {
                    if (currState == UIState::EXIT_CONFIRM)
                        currState = prevState;
                    else
                    {
                        prevState = currState;
                        currState = UIState::EXIT_CONFIRM;
                    }
                }

                else if (currState == UIState::EXIT_CONFIRM)
                {
                    if (event.key.key == SDLK_LEFT)
                        exitYesSelected = true;
                    else if (event.key.key == SDLK_RIGHT)
                        exitYesSelected = false;

                    else if (event.key.key == SDLK_RETURN)
                    {
                        if (exitYesSelected)
                            quit = true;
                        else
                            currState = prevState;
                    }
                }
                else if (event.key.key == SDLK_LEFT)
                {
                    if (!board.promotionActive)
                        board.stepBackward(moveHistory);
                }
                else if (event.key.key == SDLK_RIGHT)
                {
                    if (!board.promotionActive)
                        board.stepForward(moveHistory);
                }
            }
            break;

            case SDL_EVENT_MOUSE_WHEEL:
            {
                panel.scrollY -= event.wheel.y * 25;

                if (panel.scrollY < 0)
                    panel.scrollY = 0;

                if (panel.scrollY > panel.maxScroll)
                    panel.scrollY = panel.maxScroll;
            }
            break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                int mouseX = static_cast<int>(event.button.x);
                int mouseY = static_cast<int>(event.button.y);

                if (currState == UIState::CONNECTION_LOST)
                {
                    const float PW = 420;
                    const float PH = 260;

                    float rightPanelX =
                        2 * BORDER_WIDTH_X + BOARD_SIZE * SQUARE_SIZE;

                    float PX =
                        rightPanelX + (PANEL_WIDTH - PW) / 2;

                    float PY =
                        (SCREEN_HEIGHT - PH) / 2;

                    const float BW = 180;
                    const float BH = 45;

                    float BX = PX + (PW - BW) / 2;
                    float BY = PY + 170;

                    if (mouseX >= BX && mouseX <= BX + BW &&
                        mouseY >= BY && mouseY <= BY + BH)
                    {
                        network.clearDisconnectFlag();

                        board.reset();
                        moveHistory.reset();

                        selectedPiece = nullptr;
                        playerMoved = false;

                        currState = UIState::START_MENU;
                    }

                    continue;
                }

                if (currState == UIState::EXIT_CONFIRM)
                {
                    float w = 360, h = 180;
                    float x = (SCREEN_WIDTH - w) / 2;
                    float y = (SCREEN_HEIGHT - h) / 2;

                    // YES = quit
                    if (mouseX >= x + 40 && mouseX <= x + 160 &&
                        mouseY >= y + 100 && mouseY <= y + 140)
                        quit = true;

                    // NO = resume
                    if (mouseX >= x + 200 && mouseX <= x + 320 &&
                        mouseY >= y + 100 && mouseY <= y + 140)
                        currState = prevState;

                    continue;
                }

                if (board.currentMoveIndex != moveHistory.getMoves().size())
                    break;

                if (event.button.button == SDL_BUTTON_LEFT && board.promotionActive)
                {
                    board.handlePromotionClick(mouseX, mouseY, moveHistory);
                    selectedPiece = nullptr;

                    if (board.hasFinishedPromotionMove())
                    {
                        if (network.isConnected())
                        {
                            const MoveRecord& lastMove =
                                moveHistory.getMoves().back();

                            network.sendMove(lastMove);
                        }

                        board.clearPromotionFinishedFlag();
                    }

                    playerMoved = true;
                    break;
                }

                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    if (board.currentMoveIndex != moveHistory.getMoves().size())
                        board.replayTo(moveHistory, moveHistory.getMoves().size());
                    selectedPiece = board.selectPiece(mouseX, mouseY);
                }
                else if (event.button.button == SDL_BUTTON_RIGHT)
                {
                    if (board.currentMoveIndex != moveHistory.getMoves().size())
                        board.replayTo(moveHistory, moveHistory.getMoves().size());

                    if (!selectedPiece)
                        selectedPiece = board.selectPiece(mouseX, mouseY);
                    else
                    {
                        playerMoved = gameController.playMove(renderer, selectedPiece, mouseX, mouseY);
                        selectedPiece = nullptr;
                    }
                }
            }
            break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                int mouseX = static_cast<int>(event.button.x);
                int mouseY = static_cast<int>(event.button.y);

                if (event.button.button == SDL_BUTTON_LEFT && selectedPiece)
                {
                    playerMoved = gameController.playMove(renderer, selectedPiece, mouseX, mouseY);
                    selectedPiece = nullptr;
                }
            }
            break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 48, 48, 48, 255);
        SDL_RenderClear(renderer);

        if (currState == UIState::START_MENU)
        {
            SETTINGS = ShowStartScreen(renderer, font, board);

            // ================= CREATE ROOM =================

            if (currState == UIState::CREATE_ROOM)
            {
                ShowCreateRoomScreen(renderer, font, board, network);
                continue;
            }

            else if (currState == UIState::WAITING_ROOM)
            {
                ShowWaitingRoomScreen(renderer, font, board, network);
                continue;
            }

            // ================= JOIN ROOM =================

            else if (currState == UIState::JOIN_ROOM)
            {
                ShowJoinRoomScreen(window, renderer, font, board, network);
                continue;
            }

            // ================= STOCKFISH =================

            if (SETTINGS.vsEngine)
            {
                if (!Engine::start())
                {
                    std::cerr << "Failed to start engine.\n";
                    return -1;
                }

                if (!Engine::init())
                {
                    std::cerr << "Failed to init engine.\n";
                    return -1;
                }

                currState = UIState::PLAYING;
            }

            // ================= OLD ONLINE FALLBACK =================

            //else
            //{
            //    std::cout << "Attempting to connect...\n";

            //    currState = UIState::CONNECTING;

            //    DrawConnectingScreen(
            //        renderer,
            //        font,
            //        board,
            //        "Waiting for the opponent...");

            //    if (network.connect("chessify-production.up.railway.app", 443))
            //    {
            //        std::cout << "Connected to Railway!\n";
            //        network.joinRoom("R27PWX");
            //    }
            //    else
            //    {
            //        DrawConnectingScreen(
            //            renderer,
            //            font,
            //            board,
            //            "Server not found!");

            //        SDL_Delay(2000);

            //        currState = UIState::START_MENU;
            //        continue;
            //    }

            //    while (!network.isReady())
            //    {
            //        DrawConnectingScreen(
            //            renderer,
            //            font,
            //            board,
            //            "Waiting for the opponent...");

            //        SDL_PumpEvents();
            //        SDL_Delay(16);
            //    }

            //    // Wait until the user releases all mouse buttons
            //    while (SDL_GetMouseState(nullptr, nullptr) &
            //        (SDL_BUTTON_MASK(SDL_BUTTON_LEFT) |
            //            SDL_BUTTON_MASK(SDL_BUTTON_RIGHT)))
            //    {
            //        SDL_PumpEvents();
            //        SDL_Delay(10);
            //    }

            //    SDL_FlushEvents(
            //        SDL_EVENT_FIRST,
            //        SDL_EVENT_LAST);

            //    selectedPiece = nullptr;
            //    playerMoved = false;

            //    currState = UIState::PLAYING;
            //}
        }
        else if (currState == UIState::CREATE_ROOM)
        {
            // Create Room screen handles its own rendering
        }
        else if (currState == UIState::JOIN_ROOM)
        {
            // Join Room screen handles its own rendering
        }
        else if (currState == UIState::CONNECTION_LOST)
        {
            DrawConnectionLostScreen(renderer, font, board, mx, my);
        }
        else
            render(renderer, font, board, panel, moveHistory, network);

        if (currState == UIState::EXIT_CONFIRM)
            DrawExitConfirm(renderer, font, mx, my);

        SDL_RenderPresent(renderer);

        bool engineTurn =
            (SETTINGS.playerColor == PlayerColorChoice::WHITE &&
                !board.isWhiteTurn) ||
            (SETTINGS.playerColor == PlayerColorChoice::BLACK &&
                board.isWhiteTurn);

        if (SETTINGS.vsEngine &&
            engineTurn &&
            !board.gameOver &&
            currState == UIState::PLAYING &&
            !board.viewingHistory)
        {
            Engine::send(board.uciHistory);
            Engine::send("go depth 18");

            std::string bm = Engine::waitBestmove(), move;

            if (bm.empty())
                std::cerr << "No bestmove found.\n";
            else
                move = Engine::extractMove(bm);

            auto from = board.uciToCoord(move.substr(0, 2));
            int fromRow = from.first, fromCol = from.second;

            auto to = board.uciToCoord(move.substr(2, 2));
            int toRow = to.first, toCol = to.second;

            char promo = 0;
            if (move.length() == 5)
                promo = move[4]; // 'q','r','b','n'

            Piece *p = board.board[fromRow][fromCol];

            if (!p)
            {
                std::cerr << "Engine tried to move a non-existent piece.\n";
                continue;
            }

            // Validate before applying
            if (!Move::isValidMove(p, to.first, to.second,
                                   from.first, from.second,
                                   board.board))
            {
                std::cerr << "Illegal engine move detected.\n";
                continue;
            }

            int dispRow = board.whitePerspective ? toRow : BOARD_SIZE - 1 - toRow;
            int dispCol = board.whitePerspective ? toCol : BOARD_SIZE - 1 - toCol;

            gameController.playMove(
                renderer,
                p,
                BORDER_WIDTH_X + dispCol * SQUARE_SIZE,
                BORDER_WIDTH_Y + dispRow * SQUARE_SIZE,
                promo);
        }
    }

    close(window, renderer, font);
    return 0;
}
