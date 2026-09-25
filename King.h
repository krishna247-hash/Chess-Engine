#pragma once
#include"Piece.h"
class King :public Piece
{

public:
	King(COLOR _color, Texture2D _texture);
	bool isLegal(Board* board, Position Source, Position Destination) override;
	void draw(int row, int col, int BOXSIZE, int boardX = 0, int boardY = 0) override;
	char getSymbol() const override;
};

