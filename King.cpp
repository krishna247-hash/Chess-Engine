#include "King.h"
#include"Board.h"
#include<cstdlib>

King::King(COLOR _color, Texture2D _texture)
	:Piece(_color, _texture) {

}

void King::draw(int row, int col, int BOXSIZE, int boardX, int boardY) {
	DrawTexture(texture, boardX + (col * BOXSIZE) - 14, boardY + (row * BOXSIZE) - 14, WHITE);
}

bool King::isLegal(Board* board, Position start, Position end) {
    if (!board->isInside(end.row, end.col)) return false;

    int dr = abs(end.row - start.row);
    int dc = abs(end.col - start.col);

    if (dr == 0 and dc == 0) return false;

    Piece* dest = board->getPiece(end);
    if ((dr <= 1 and dc <= 1) and (dest == nullptr || dest->getColor() != this->color)) {
        return true;
    }

   
    if (!this->gethasMoved() and dr == 0 and dc == 2) {
        int row = start.row;

        // Can't castle while already in check.
        if (board->isInCheck(this->color)) return false;

        if (end.col == 6) {
            Piece* rook = board->getPiece(row, 7);
            if (rook and !rook->gethasMoved() and board->isPathClearHorizontal(start, { row, 7 }) and
                !board->isSquareAttacked(row, 5, this->color) and
                !board->isSquareAttacked(row, 6, this->color))
                return true;
        }

    
        if (end.col == 2) {
            Piece* rook = board->getPiece(row, 0);
            if (rook and !rook->gethasMoved() and board->isPathClearHorizontal(start, { row, 0 }) and
                !board->isSquareAttacked(row, 3, this->color) and
                !board->isSquareAttacked(row, 2, this->color))
                return true;
        }
    }

    return false;
}

char King::getSymbol() const {
    return (color == PWHITE) ? 'K' : 'k';
}
