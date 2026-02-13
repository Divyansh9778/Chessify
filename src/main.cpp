#include <SDL3/SDL_init.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_ttf.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_blendmode.h>

#include "Constants.h"
#include "Settings.h"
#include "Piece.h"
#include "Board.h"
#include "Engine.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <Windows.h>
#include <process.h>
#include <cmath>

enum class UIState
{
    START_MENU,
    PLAYING,
    EXIT_CONFIRM
};
UIState prevState = UIState::START_MENU;
UIState currState = UIState::START_MENU;

const SDL_Color COLOR_BG = { 25, 25, 32, 255 };
const SDL_Color COLOR_BTN = { 55, 115, 255, 255 };
const SDL_Color COLOR_BTN_HOVER = { 75, 135, 255, 255 };
const SDL_Color COLOR_BTN_TWO = { 60, 170, 95, 255 };
const SDL_Color COLOR_SHADOW = { 0, 0, 0, 80 };
const SDL_Color COLOR_TEXT = { 255, 255, 255, 255 };
const SDL_Color COLOR_TITLE1 = { 255, 255, 255, 255 };
const SDL_Color COLOR_TITLE2 = { 180, 180, 200, 255 };

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
    {"bk", "assets/pieces/bk.bmp"} };

SDL_Renderer* renderer = nullptr;
static bool init(SDL_Window*& window, SDL_Renderer*& renderer, TTF_Font*& font, SDL_Surface* icon)
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

    window = SDL_CreateWindow("Chess", 1500, 900, SDL_WINDOW_RESIZABLE);
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

static void DrawRounded(SDL_Renderer* r, float x, float y, float w, float h, float radius, SDL_Color c)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);

    // 1. Center rectangle
    SDL_FRect core = { x + radius, y, w - 2 * radius, h };
    SDL_RenderFillRect(r, &core);

    // 2. Left rectangle
    SDL_FRect left = { x, y + radius, radius, h - 2 * radius };
    SDL_RenderFillRect(r, &left);

    // 3. Right rectangle
    SDL_FRect right = { x + w - radius, y + radius, radius, h - 2 * radius };
    SDL_RenderFillRect(r, &right);

    // 4. Perfect corner circles
    for (float dy = -radius; dy <= radius; dy++)
    {
        float dx = sqrtf(radius * radius - dy * dy);

        // top-left corner
        SDL_RenderLine(r,
            x + radius - dx, y + radius + dy,
            x + radius + dx, y + radius + dy
        );

        // top-right corner
        SDL_RenderLine(r,
            x + w - radius - dx, y + radius + dy,
            x + w - radius + dx, y + radius + dy
        );

        // bottom-left
        SDL_RenderLine(r,
            x + radius - dx, y + h - radius + dy,
            x + radius + dx, y + h - radius + dy
        );

        // bottom-right
        SDL_RenderLine(r,
            x + w - radius - dx, y + h - radius + dy,
            x + w - radius + dx, y + h - radius + dy
        );
    }
}

static void DrawShadow(SDL_Renderer* r, float x, float y, float w, float h)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 120);
    SDL_FRect s{ x + 6, y + 6, w, h };
    SDL_RenderFillRect(r, &s);
}

static void DrawTextCentered(SDL_Renderer* renderer, TTF_Font* font,
    const std::string& text,
    float x, float y, float w, float h,
    SDL_Color color)
{
    SDL_Surface* surf = TTF_RenderText_Blended(font, text.c_str(), text.length(), color);
    if (!surf) return;

    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    int tw = surf->w;
    int th = surf->h;
    SDL_DestroySurface(surf);

    SDL_FRect dst{ 0.0f, 0.0f, 0.0f, 0.0f };
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

static void FancyButton(SDL_Renderer* r, int mx, int my,
    float x, float y, float w, float h,
    const std::string& txt, TTF_Font* font)
{
    SDL_Color base = { 55,115,255,255 };
    SDL_Color hover = { 75,135,255,255 };

    SDL_Color col = Hover(mx, my, x, y, w, h) ? hover : base;

    DrawRounded(r, x, y, w, h, 10.0f, col);
    DrawTextCentered(r, font, txt, x, y, w, h, COLOR_TEXT);
}

static void DepthButton(SDL_Renderer* r, int mx, int my,
    float x, float y, const char* symbol,
    int& depth, int delta, TTF_Font* font)
{
    SDL_Color col = { 90,90,220,255 };
    SDL_Color hov = { 110,110,240,255 };
    bool h = Hover(mx, my, x, y, 50, 50);

    DrawRounded(r, x, y, 50, 50, 10, h ? hov : col);
    DrawTextCentered(r, font, symbol, x, y, 50, 50, { 255,255,255,255 });

    if (h && SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_MASK(SDL_BUTTON_LEFT))
    {
        depth += delta;
        depth = max(1, min(depth, 12));
        SDL_Delay(180); // debounce
    }
}

static void DrawOverlay(SDL_Renderer* r)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 160);
    SDL_FRect full = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    SDL_RenderFillRect(r, &full);
}

static void DrawBox(SDL_Renderer* r, float x, float y, float w, float h, SDL_Color c)
{
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_FRect rect{ x, y, w, h };
    SDL_RenderFillRect(r, &rect);
}

static GameSettings ShowStartScreen(SDL_Renderer* r, TTF_Font* font, Board& board)
{
    GameSettings gs;
    SDL_Event e;

    int depth = 6;
    bool pickingDepth = false;

    const float PW = 420, PH = 360;
    const float PX = SCREEN_WIDTH - PW + 500;
    const float PY = (SCREEN_HEIGHT - PH) / 2;

    int mx = 0, my = 0;

    while (true)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT)
                exit(0);

            if (e.type == SDL_EVENT_MOUSE_MOTION)
                mx = (int)e.motion.x; my = (int)e.motion.y;

            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                // Quit button
                if (Hover(mx, my, PX + 145, PY + 300, 130, 40))
                    exit(0);

                if (!pickingDepth)
                {
                    // Vs Human
                    if (Hover(mx, my, PX + 60, PY + 120, 300, 50))
                    {
                        gs.vsEngine = false;
                        return gs;
                    }
                    // Vs Stockfish
                    if (Hover(mx, my, PX + 60, PY + 190, 300, 50))
                    {
                        pickingDepth = true;
                    }
                }
                else
                {
                    // OK
                    if (Hover(mx, my, PX + 145, PY + 240, 130, 40))
                    {
                        gs.vsEngine = true;
                        gs.engineDepth = depth;
                        return gs;
                    }
                }
            }
        }

        // Draw board first (for transparent background)
        board.drawBoard(r, font);

        // Overlay
        DrawOverlay(r);

        // Panel with shadow
        DrawShadow(r, PX, PY, PW, PH);
        DrawRounded(r, PX, PY, PW, PH, 12, { 40,40,40,230 });

        DrawTextCentered(r, font, "CHESS GAME", PX, PY + 20, PW, 50, { 255,255,255,255 });

        if (!pickingDepth)
        {
            FancyButton(r, mx, my, PX + 60, PY + 120, 300, 50, "Play vs Human", font);
            FancyButton(r, mx, my, PX + 60, PY + 190, 300, 50, "Play vs Stockfish", font);
        }
        else
        {
            DrawTextCentered(r, font, "Choose Depth", PX, PY + 90, PW, 40, COLOR_TEXT);

            // Minus
            DepthButton(r, mx, my, PX + 90, PY + 150, "-", depth, -1, font);

            // Depth value
            DrawTextCentered(r, font, std::to_string(depth),
                PX + 170, PY + 145, 80, 60, COLOR_TEXT);

            // Plus
            DepthButton(r, mx, my, PX + 270, PY + 150, "+", depth, +1, font);

            // OK button
            FancyButton(r, mx, my, PX + 145, PY + 240, 130, 40, "OK", font);
        }

        // Quit button
        DrawRounded(r, PX + 145, PY + 300, 130, 40, 10, { 200,60,60,255 });
        DrawTextCentered(r, font, "QUIT", PX + 145, PY + 300, 130, 40, COLOR_TEXT);

        SDL_RenderPresent(r);
        SDL_Delay(12);
    }
}

static SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path)
{
    SDL_Surface* surface = SDL_LoadBMP(path.c_str());
    if (!surface)
    {
        std::cerr << "Image Load Failed: " << path << " SDL_Error: " << SDL_GetError() << std::endl;
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);

    if (!texture)
        std::cerr << "Texture Creation Failed: " << path << " SDL_Error: " << SDL_GetError() << std::endl;
    return texture;
}

static void drawNumbers(SDL_Renderer* renderer, TTF_Font* font)
{
    // Set the color for the numbers (white)
    SDL_Color textColor = { 255, 255, 255, 255 };

    for (int i = 0; i < BOARD_SIZE; i++)
    {
        std::string text = std::to_string(8 - i);

        // Create a surface with the number as text
        SDL_Surface* textSurface = TTF_RenderText_Blended(font, text.c_str(), text.length(), textColor);
        if (!textSurface)
        {
            std::cerr << "Text rendering failed! SDL_ttf Error: " << SDL_GetError() << std::endl;
            return;
        }

        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_DestroySurface(textSurface);

        if (!textTexture)
        {
            std::cerr << "Texture creation failed! SDL_Error: " << SDL_GetError() << std::endl;
            return;
        }

        // Define the destination rectangle (positioning it on the left border, vertically)
        SDL_Rect textRect = { BORDER_WIDTH / 2 - 3, SQUARE_SIZE / 2 + BORDER_WIDTH + i * SQUARE_SIZE - 8, 12, 20 }; // Adjust size
        SDL_FRect fTextRect;
        SDL_RectToFRect(&textRect, &fTextRect);

        SDL_RenderTexture(renderer, textTexture, nullptr, &fTextRect);
        SDL_DestroyTexture(textTexture);
    }
}

static void drawLetters(SDL_Renderer* renderer, TTF_Font* font)
{
    SDL_Color textColor = { 255, 255, 255, 255 }; // White color for text

    // Loop through letters 'A' to 'H'
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        char letter = 'a' + i;            // Generate the letter (a, b, c, ..., h)
        std::string letterStr(1, letter); // Convert the letter to a string

        // Render the letter with smoother anti-aliasing
        SDL_Surface* textSurface = TTF_RenderText_Blended(font, letterStr.c_str(), letterStr.length(), textColor);

        if (!textSurface)
        {
            std::cerr << "Text rendering failed! SDL_ttf Error: " << SDL_GetError() << std::endl;
            return;
        }

        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_DestroySurface(textSurface);

        if (!textTexture)
        {
            std::cerr << "Texture creation failed! SDL_Error: " << SDL_GetError() << std::endl;
            return;
        }

        // Define the destination rectangle (positioning it on the left border, vertically)
        SDL_Rect textRect = { BORDER_WIDTH + i * SQUARE_SIZE + (SQUARE_SIZE / 2) - 5, BORDER_WIDTH + BOARD_SIZE * SQUARE_SIZE + 10, 10, 22 };
        SDL_FRect fTextRect;
        SDL_RectToFRect(&textRect, &fTextRect);

        SDL_RenderTexture(renderer, textTexture, nullptr, &fTextRect);
        SDL_DestroyTexture(textTexture);
    }
}

// Function to render the piece
static void render(SDL_Renderer* renderer, TTF_Font* font, Board& board)
{
    board.drawBoard(renderer, font);

    for (Piece* piece : board.getPieces())
    {
        if (board.promotionActive && piece == board.promoPawn)
            continue;

        piece->drawPiece(renderer, Board::pieceTextures);
    }

    drawNumbers(renderer, font);
    drawLetters(renderer, font);
}

static void DrawExitConfirm(SDL_Renderer* r, TTF_Font* font, float mx, float my)
{
    // Dark overlay
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 160);
    SDL_FRect bg = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    SDL_RenderFillRect(r, &bg);

    // Dialog box
    float w = 360, h = 180;
    float x = (SCREEN_WIDTH - w) / 2;
    float y = (SCREEN_HEIGHT - h) / 2;

    DrawRounded(r, x, y, w, h, 12, { 40,40,40,240 });

    DrawTextCentered(r, font, "Quit Game?", x, y + 20, w, 40, { 255,255,255,255 });

    // YES
    DrawRounded(r, x + 40, y + 100, 120, 40, 10, { 200,70,70,255 });
    DrawTextCentered(r, font, "YES", x + 40, y + 100, 120, 40, { 255,255,255,255 });

    // NO
    DrawRounded(r, x + 200, y + 100, 120, 40, 10, { 70,140,255,255 });
    DrawTextCentered(r, font, "NO", x + 200, y + 100, 120, 40, { 255,255,255,255 });
}

static void close(SDL_Window* window, SDL_Renderer* renderer, TTF_Font* font)
{
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
    if (font)
        TTF_CloseFont(font);

    if (SETTINGS.vsEngine) Engine::stop();
    TTF_Quit();
    SDL_Quit();
}

int main()
{
    SDL_Window* window = nullptr;
    SDL_Surface* icon = nullptr;
    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;

    if (!init(window, renderer, font, icon))
        return -1;

    Board board;
    board.loadTextures(renderer);

    bool quit = false, playerMoved = false, startMenuDone = false;
    Piece* selectedPiece = nullptr;

    while (!quit)
    {
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
                quit = true;
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
            }
            break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                int mouseX = static_cast<int>(event.button.x);
                int mouseY = static_cast<int>(event.button.y);

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


                if (event.button.button == SDL_BUTTON_LEFT && board.promotionActive)
                {
                    board.handlePromotionClick(mouseX, mouseY);
                    selectedPiece = nullptr;
                    break;
                }

                if (event.button.button == SDL_BUTTON_LEFT)
                    selectedPiece = board.selectPiece(mouseX, mouseY);
                else if (event.button.button == SDL_BUTTON_RIGHT)
                {
                    if (!selectedPiece)
                        selectedPiece = board.selectPiece(mouseX, mouseY);
                    else
                    {
                        board.movePiece(renderer, selectedPiece, mouseX, mouseY);
                        selectedPiece = nullptr;
                        playerMoved = true;
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
                    board.movePiece(renderer, selectedPiece, mouseX, mouseY);
                    selectedPiece = nullptr;
                    playerMoved = true;
                }
            }
            break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (currState == UIState::START_MENU)
        {
            SETTINGS = ShowStartScreen(renderer, font, board);

            if (SETTINGS.vsEngine)
            {
                if (!Engine::start()) {
                    std::cerr << "Failed to start engine.\n";
                    return -1;
                }
                if (!Engine::init()) {
                    std::cerr << "Failed to init engine.\n";
                    return -1;
                }
            }

            currState = UIState::PLAYING;
        }
        else
            render(renderer, font, board);

        if (currState == UIState::EXIT_CONFIRM)
            DrawExitConfirm(renderer, font, mx, my);

        SDL_RenderPresent(renderer);

        // === STOCKFISH PLAYS BLACK ===
        if (SETTINGS.vsEngine && playerMoved && !board.isWhiteTurn && !board.gameOver && currState == UIState::PLAYING)
        {
            Engine::send(board.uciHistory);
            Engine::send("go depth 20");

            std::string bm = Engine::waitBestmove(), move;

            if (bm.empty())
                std::cerr << "No bestmove found.\n";
            else
                move = Engine::extractMove(bm);

            auto from = board.uciToCoord(move.substr(0, 2));
            int fromRow = from.first, fromCol = from.second;

            auto to = board.uciToCoord(move.substr(2, 2));
            int toRow = to.first, toCol = to.second;

            Piece* p = board.board[fromRow][fromCol];

            if (!p)
            {
                std::cerr << "Engine tried to move a non-existent piece.\n";
                continue;
            }

            board.movePiece(renderer, p,
                BORDER_WIDTH + toCol * SQUARE_SIZE,
                BORDER_WIDTH + toRow * SQUARE_SIZE);
        }
    }

    close(window, renderer, font);
    return 0;
}
