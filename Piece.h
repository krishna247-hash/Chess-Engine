#pragma once
#include"utility.h"
#include"raylib.h"


class Board;
class Piece
{
protected:
	COLOR color;
	Texture2D texture;
	bool hasMoved = false;
public:

	Piece(COLOR _color, Texture2D _texture);
	virtual ~Piece();

	virtual bool isLegal(Board* board, Position Source, Position Destination) = 0;

	virtual void draw(int row, int col, int BOXSIZE, int boardX = 0, int boardY = 0) = 0;

	// Draws this piece at an exact pixel position instead of a grid
	// square — used to animate a piece sliding between squares or dragging.
	void drawAtPixel(float x, float y, Color tint = WHITE) const;

	virtual char getSymbol() const = 0;

	COLOR getColor();

	bool gethasMoved() { return hasMoved; }
	void sethasMoved(bool haha) {  hasMoved = haha; }

};

