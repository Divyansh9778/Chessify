#include "Animation.h"

void Animation::animateMove(Piece* piece, int targetX, int targetY, SDL_Renderer* renderer, Board* board) {
    int startX = piece->xPos;
    int startY = piece->yPos;

    const int frames = 30;  // Adjust for smoother motion
    const float deltaX = (targetX - startX) / static_cast<float>(frames);
    const float deltaY = (targetY - startY) / static_cast<float>(frames);

    for (int i = 0; i < frames; i++) {
        piece->setPosition(startX + deltaX * i, startY + deltaY * i);

        // Redraw board and pieces at each step
        SDL_RenderClear(renderer);
        board->drawBoard(renderer, font);
        SDL_RenderPresent(renderer);

        SDL_Delay(8);  // Adjust delay for smoothness
    }

    // Set final position
    piece->setPosition(targetX, targetY);
}

//void Animation::animateSelection(Piece* piece, SDL_Renderer* renderer, Board* board) {
//    if (!piece) return;
//
//    const int scaleFrames = 10;
//    float scaleFactor = 1.1f;  // Slightly enlarge the piece
//
//    for (int i = 0; i < scaleFrames; i++) {
//        SDL_RenderClear(renderer);
//        board->drawBoard(renderer);
//
//        // Draw glowing effect (optional)
//        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 128);
//        SDL_FRect fHighlight = { piece->xPos - 3, piece->yPos - 3, piece->width + 6, piece->height + 6 };
//        SDL_RenderRect(renderer, &fHighlight);
//
//        // Scale the piece slightly
//        SDL_Rect scaledRect = { piece->xPos - 2, piece->yPos - 2, int(piece->width * scaleFactor), int(piece->height * scaleFactor) };
//        piece->draw(renderer, &scaledRect);
//
//        SDL_RenderPresent(renderer);
//        SDL_Delay(10);
//    }
//}
