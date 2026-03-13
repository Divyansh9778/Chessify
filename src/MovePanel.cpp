#include "Constants.h"
#include "MovePanel.h"
#include "MoveHistory.h"

#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_pixels.h>

#include <algorithm>
#include <string>
#include <string.h>

MovePanel::MovePanel(float px, float py, float w, float h) : x(px), y(py), width(w), height(h) {}

void MovePanel::draw(SDL_Renderer* renderer, TTF_Font* font, const MoveHistory& history)
{
    // Background
    SDL_SetRenderDrawColor(renderer, 28, 28, 28, 255);
    SDL_FRect panel = { x, y, width, height };
    SDL_RenderFillRect(renderer, &panel);

    SDL_Rect clip = { (int)x, (int)y, (int)width, (int)height };
    SDL_SetRenderClipRect(renderer, &clip);

    SDL_Color textColor = { 230, 230, 230, 255 };

    const char* title = "Moves";
    int textW, textH;

    SDL_Surface* titleSurf = TTF_RenderText_Blended(font, title, 5, textColor);
    SDL_Texture* titleTex = SDL_CreateTextureFromSurface(renderer, titleSurf);

    TTF_GetStringSize(font, title, strlen(title), &textW, &textH);

    SDL_FRect titleRect = {
        x + (PANEL_WIDTH - textW) / 2.0f,   // centered horizontally
        y + 10,
        (float)textW * 1.2,
        (float)textH * 1.2
    };

    SDL_RenderTexture(renderer, titleTex, nullptr, &titleRect);

    SDL_DestroySurface(titleSurf);
    SDL_DestroyTexture(titleTex);

    // Render move list
    float lineHeight = TTF_GetFontHeight(font) - 10;
    float startY = y + 100;

    const auto& moves = history.getMoves();

    float totalRows = (float)(moves.size() + 1) / 2;
    float totalHeight = totalRows * lineHeight;

    maxScroll = std::max(0.0f, totalHeight - (height - 120));

    // auto follow latest move
    scrollY = std::min(scrollY, maxScroll);

    for (size_t i = 0; i < moves.size(); i += 2)
    {
        int row = i / 2;
        int moveNumber = row + 1;

        float yPos = startY + row * lineHeight - scrollY;

        // ---- MOVE NUMBER COLUMN ----
        std::string numText = std::to_string(moveNumber) + ".";

        SDL_Surface* nSurf =
            TTF_RenderText_Blended(font, numText.c_str(), numText.length(), textColor);
        SDL_Texture* nTex =
            SDL_CreateTextureFromSurface(renderer, nSurf);

        SDL_FRect nRect =
        {
            x + 50 - 20 * (moveNumber > 9),
            yPos,
            (float)nSurf->w * 0.8,
            (float)nSurf->h * 0.8
        };

        SDL_RenderTexture(renderer, nTex, nullptr, &nRect);

        SDL_DestroySurface(nSurf);
        SDL_DestroyTexture(nTex);

        // ---- WHITE MOVE COLUMN ----
        std::string whiteMove = history.formatMove(moves[i]);

        SDL_Surface* wSurf =
            TTF_RenderText_Blended(font, whiteMove.c_str(), whiteMove.length(), textColor);
        SDL_Texture* wTex =
            SDL_CreateTextureFromSurface(renderer, wSurf);

        SDL_FRect wRect =
        {
            x + 110,
            yPos,
            (float)wSurf->w * 0.8,
            (float)wSurf->h * 0.8
        };

        SDL_RenderTexture(renderer, wTex, nullptr, &wRect);

        SDL_DestroySurface(wSurf);
        SDL_DestroyTexture(wTex);

        // ---- Black move (RIGHT COLUMN) ----
        if (i + 1 < moves.size())
        {
            std::string blackText = history.formatMove(moves[i + 1]);

            SDL_Surface* bSurf =
                TTF_RenderText_Blended(font, blackText.c_str(), blackText.length(), textColor);
            SDL_Texture* bTex =
                SDL_CreateTextureFromSurface(renderer, bSurf);

            SDL_FRect bRect =
            {
                x + 50 + width / 2,
                yPos,
                (float)bSurf->w * 0.8,
                (float)bSurf->h * 0.8
            };

            SDL_RenderTexture(renderer, bTex, nullptr, &bRect);

            SDL_DestroySurface(bSurf);
            SDL_DestroyTexture(bTex);
        }
    }

    SDL_SetRenderClipRect(renderer, nullptr);
}