#include "Piece.h"

Piece::Piece(COLOR _color , Texture2D _texture)
	:color(_color) , texture(_texture) {

}
Piece::~Piece() {
	
}

COLOR Piece::getColor() { return color; }

void Piece::drawAtPixel(float x, float y, Color tint) const {
	DrawTexture(texture, (int)(x - 14), (int)(y - 14), tint);
}

