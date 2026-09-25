#include "Rook.h"
#include"Board.h"
#include"math.h"

Rook::Rook(COLOR _color, Texture2D _texture)
	:Piece(_color, _texture) {

}
bool Rook::isLegal(Board* board, Position S, Position D) {
    int sr = S.row;
    int sc = S.col;
    int er = D.row;
    int ec = D.col;

    if (!board->isInside(er, ec)) return false;
    if (sr == er and sc == ec) return false;

    if (sr == er) {
        return board->isPathClearHorizontal(S,D) and
            (board->isEmpty(er, ec) or board->getPiece(er, ec)->getColor() != color);
    }
    else if (sc == ec) {
        return board->isPathClearVertical(S,D) and
            (board->isEmpty(er, ec) or board->getPiece(er, ec)->getColor() != color);
    }
    return false;
}

void Rook::draw(int row, int col, int BOXSIZE, int boardX, int boardY) {
	DrawTexture(texture, boardX + (col * BOXSIZE) - 14, boardY + (row * BOXSIZE) - 14, WHITE);
}

char Rook::getSymbol() const {
    return (color == PWHITE) ? 'R' : 'r';
}
