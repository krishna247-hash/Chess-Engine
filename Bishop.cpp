#include "Bishop.h"
#include"Board.h"
#include<cstdlib>

Bishop::Bishop(COLOR _color, Texture2D _texture)
	:Piece(_color, _texture) {

}
bool Bishop::isLegal(Board* board, Position S, Position D) {
	int sr = S.row;
	int sc = S.col;
	int er = D.row;
	int ec = D.col;

	if (!board->isInside(er, ec)) return false;

	int dr = abs(er - sr);
	int dc = abs(ec - sc);
	if (dr == dc and dr > 0) {
		return board->isPathClearDiagonal(S,D) and
			(board->isEmpty(er, ec) or board->getPiece(er, ec)->getColor() != color);
	}

	return false;

}
void Bishop::draw(int row, int col, int BOXSIZE, int boardX, int boardY) {
	DrawTexture(texture, boardX + (col * BOXSIZE) - 14, boardY + (row * BOXSIZE) - 14, WHITE);
}

char Bishop::getSymbol() const {
	return (color == PWHITE) ? 'B' : 'b';
}
