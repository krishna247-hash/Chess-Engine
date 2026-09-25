#include "Knight.h"
#include<cstdlib>
#include"Board.h"

Knight::Knight(COLOR _color, Texture2D _texture)
	:Piece(_color, _texture) {
}

bool Knight::isLegal(Board* board, Position S, Position D) {
   
    int sr = S.row;
    int sc = S.col;
    int er = D.row;
    int ec = D.col;

    if (!board->isInside(er, ec)) return false;

    int dr = abs(sr - er);
    int dc = abs(sc - ec);

    if ((dr == 2 and dc == 1) or (dr == 1 and dc == 2)) {
        Piece* target = board->getPiece(er, ec);
        return (target == nullptr or target->getColor() != color);
    }

    return false;
}
void Knight::draw(int row, int col, int BOXSIZE, int boardX, int boardY) {
	DrawTexture(texture, boardX + (col * BOXSIZE) - 14, boardY + (row * BOXSIZE) - 14, WHITE);
}

char Knight::getSymbol() const {
    return (color == PWHITE) ? 'N' : 'n';
}
