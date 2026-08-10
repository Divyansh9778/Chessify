#ifndef MOVEPANEL_H
#define MOVEPANEL_H

#include "network/PlayerColor.h"

#include <SDL3_ttf/SDL_ttf.h>
#include "MoveHistory.h"

class MovePanel
{
public:
    MovePanel(float x, float y, float w, float h);
    void draw(SDL_Renderer* renderer, TTF_Font* font, const MoveHistory& history, PlayerColor playerColor);

    float scrollY = 0.0f;
    float maxScroll = 0.0f;

private:
    float x, y, width, height;
};

#endif