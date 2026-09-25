#include "Pawn.h"
#include"Board.h"
#include<cstdlib>

Pawn::Pawn(COLOR _color , Texture2D _texture)
	:Piece(_color , _texture) {

}
bool Pawn::isLegal(Board* board, Position S, Position D) {
    int sr = S.row;
    int sc = S.col;
    int er = D.row;
    int ec = D.col;

    if (!board->isInside(er, ec)) return false;

    
    int direction = (color == PWHITE) ? -1 : 1;
    int startRow = (color == PWHITE) ? 6 : 1;

    Piece* target = board->getPiece(er, ec);

 
    if (sc == ec and er == sr + direction and board->isEmpty(er, ec))
        return true;

    if (sc == ec and sr == startRow and er == sr + 2 * direction and
        board->isEmpty(sr + direction, sc) and board->isEmpty(er, ec)) {
        return true;
    }

  
    if (abs(sc - ec) == 1 and er == sr + direction and
        target != nullptr and target->getColor() != color) {
        return true;
    }

    // En passant: diagonal step into the empty square just vacated by an
    // opposing pawn that pushed two squares on the immediately preceding move.
    if (abs(sc - ec) == 1 and er == sr + direction and target == nullptr) {
        Position ep = board->getEnPassantTarget();
        if (ep.row == er and ep.col == ec) {
            Piece* adjacent = board->getPiece(sr, ec);
            if (adjacent and adjacent->getColor() != color)
                return true;
        }
    }

    return false;
}

void Pawn::draw(int row, int col, int BOXSIZE, int boardX, int boardY) {
	DrawTexture(texture, boardX + (col * BOXSIZE) - 14, boardY + (row * BOXSIZE) - 14, WHITE);
}

char Pawn::getSymbol() const {
    return (color == PWHITE) ? 'P' : 'p';
}
