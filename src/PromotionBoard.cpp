//#include "PromotionBoard.h"
//#include "Constants.h"
//#include "Piece.h"
//
//PromotionBoard::PromotionBoard(SDL_Renderer* renderer, bool isWhite) {
//    xOffset = 8 * SQUARE_SIZE + 10; // right of board
//    yOffset = 0;
//
//    std::vector<std::string> types = { "q", "r", "n", "b"};
//    for (int i = 0; i < 4; ++i) {
//        Piece* piece = new Piece(types[i], xOffset + i * 0.9 * SQUARE_SIZE, yOffset, isWhite);
//        promoBoard[0][i] = piece;
//        promoPieces.push_back(piece);
//    }
//}
//
//PromotionBoard::~PromotionBoard() {
//    for (Piece* p : promoPieces) delete p;
//}
//
//void PromotionBoard::draw(SDL_Renderer* renderer) {
//    for (int i = 0; i < 4; ++i) {
//        promoBoard[0][i]->drawPiece(renderer, pieceTextures);
//    }
//}
//
//int PromotionBoard::handleClick(int mouseX, int mouseY) {
//    int relativeX = mouseX - xOffset;
//    int col = relativeX / (0.9 * SQUARE_SIZE);
//    if (mouseY >= yOffset && mouseY < yOffset + SQUARE_SIZE && col >= 0 && col < 4) {
//        return col; // return choice index (0 to 3)
//    }
//    return -1;
//}
