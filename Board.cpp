#include "Board.h"
#include"Pawn.h"
#include"Rook.h"
#include"Bishop.h"
#include"Knight.h"
#include"King.h"
#include"Queen.h"
#include<fstream>
#include<sstream>
#include<cctype>
#include<algorithm>
#include<climits>
#include<cstdlib>
#include<cmath>
#include"utility.h"
#define BOXSIZE 104
#define GRIDSIZE 8

using namespace std;

Board::Board() {
	for (int i = 0; i < GRIDSIZE; i++) {
		for (int j = 0; j < GRIDSIZE; j++) {
			board[i][j] = nullptr;
		}
	}

	for (int i = 0; i < GRIDSIZE; i++) {
		for (int j = 0; j < GRIDSIZE; j++) {
			highlight[i][j] =false;
		}
	}
}

Board::~Board() {
	for (int i = 0; i < GRIDSIZE; ++i) {
		for (int j = 0; j < GRIDSIZE; ++j) {
			delete board[i][j];
			board[i][j] = nullptr;
		}
	}
}


struct BoardTextures {
	Texture2D blackPawn, whitePawn;
	Texture2D blackRook, whiteRook;
	Texture2D blackBishop, whiteBishop;
	Texture2D blackKnight, whiteKnight;
	Texture2D blackKing, whiteKing;
	Texture2D blackQueen, whiteQueen;
	bool loaded = false;
};

static BoardTextures sBoardTex;

static void ensureBoardTextures() {
	if (sBoardTex.loaded) return;
	sBoardTex.blackPawn = LoadTexture("PNGs/black-pawn.png");
	sBoardTex.whitePawn = LoadTexture("PNGs/white-pawn.png");
	sBoardTex.blackRook = LoadTexture("PNGs/black-rook.png");
	sBoardTex.whiteRook = LoadTexture("PNGs/white-rook.png");
	sBoardTex.blackBishop = LoadTexture("PNGs/black-bishop.png");
	sBoardTex.whiteBishop = LoadTexture("PNGs/white-bishop.png");
	sBoardTex.blackKnight = LoadTexture("PNGs/black-knight.png");
	sBoardTex.whiteKnight = LoadTexture("PNGs/white-knight.png");
	sBoardTex.blackKing = LoadTexture("PNGs/black-king.png");
	sBoardTex.whiteKing = LoadTexture("PNGs/white-king.png");
	sBoardTex.blackQueen = LoadTexture("PNGs/black-queen.png");
	sBoardTex.whiteQueen = LoadTexture("PNGs/white-queen.png");
	sBoardTex.loaded = true;
}

void Board::unloadTextures() {
	if (!sBoardTex.loaded) return;
	if (sBoardTex.blackPawn.id > 0) UnloadTexture(sBoardTex.blackPawn);
	if (sBoardTex.whitePawn.id > 0) UnloadTexture(sBoardTex.whitePawn);
	if (sBoardTex.blackRook.id > 0) UnloadTexture(sBoardTex.blackRook);
	if (sBoardTex.whiteRook.id > 0) UnloadTexture(sBoardTex.whiteRook);
	if (sBoardTex.blackBishop.id > 0) UnloadTexture(sBoardTex.blackBishop);
	if (sBoardTex.whiteBishop.id > 0) UnloadTexture(sBoardTex.whiteBishop);
	if (sBoardTex.blackKnight.id > 0) UnloadTexture(sBoardTex.blackKnight);
	if (sBoardTex.whiteKnight.id > 0) UnloadTexture(sBoardTex.whiteKnight);
	if (sBoardTex.blackKing.id > 0) UnloadTexture(sBoardTex.blackKing);
	if (sBoardTex.whiteKing.id > 0) UnloadTexture(sBoardTex.whiteKing);
	if (sBoardTex.blackQueen.id > 0) UnloadTexture(sBoardTex.blackQueen);
	if (sBoardTex.whiteQueen.id > 0) UnloadTexture(sBoardTex.whiteQueen);
	sBoardTex.loaded = false;
}

void Board::initillize() {
	for (int i = 0; i < GRIDSIZE; ++i) {
		for (int j = 0; j < GRIDSIZE; ++j) {
			delete board[i][j];
			board[i][j] = nullptr;
		}
	}

	ensureBoardTextures();

	for (int i = 0; i < GRIDSIZE; i++) {
		board[1][i] = new Pawn(PBLACK, sBoardTex.blackPawn);
		board[6][i] = new Pawn(PWHITE, sBoardTex.whitePawn);
	}

	board[0][0] = new Rook(PBLACK, sBoardTex.blackRook); board[0][7] = new Rook(PBLACK, sBoardTex.blackRook);
	board[7][0] = new Rook(PWHITE, sBoardTex.whiteRook); board[7][7] = new Rook(PWHITE, sBoardTex.whiteRook);

	board[0][2] = new Bishop(PBLACK, sBoardTex.blackBishop); board[0][5] = new Bishop(PBLACK, sBoardTex.blackBishop);
	board[7][2] = new Bishop(PWHITE, sBoardTex.whiteBishop); board[7][5] = new Bishop(PWHITE, sBoardTex.whiteBishop);

	board[0][1] = new Knight(PBLACK, sBoardTex.blackKnight); board[0][6] = new Knight(PBLACK, sBoardTex.blackKnight);
	board[7][1] = new Knight(PWHITE, sBoardTex.whiteKnight); board[7][6] = new Knight(PWHITE, sBoardTex.whiteKnight);

	board[0][4] = new King(PBLACK, sBoardTex.blackKing); board[7][4] = new King(PWHITE, sBoardTex.whiteKing);

	board[0][3] = new Queen(PBLACK, sBoardTex.blackQueen); board[7][3] = new Queen(PWHITE, sBoardTex.whiteQueen);
}

struct ThemeColors {
	Color lightSquare;
	Color darkSquare;
	const char* name;
};

static const ThemeColors BOARD_THEMES[THEME_COUNT] = {
	{ Color{ 235, 236, 208, 255 }, Color{ 115, 149, 82, 255 }, "Green" },     // Chess.com Signature Green
	{ Color{ 78, 81, 87, 255 },    Color{ 46, 48, 52, 255 },    "Dark" },      // Chess.com Dark Mode
	{ Color{ 240, 217, 181, 255 }, Color{ 181, 136, 99, 255 }, "Wood" },      // Tournament Wood
	{ Color{ 222, 227, 230, 255 }, Color{ 140, 162, 173, 255 }, "Ocean" }     // Ocean Glass
};

void Board::cycleTheme() {
	currentTheme = static_cast<BoardThemeType>((currentTheme + 1) % THEME_COUNT);
}

const char* Board::getThemeName() const {
	return BOARD_THEMES[currentTheme].name;
}

void Board::display(int boardX, int boardY, int boxSize) {
	ThemeColors theme = BOARD_THEMES[currentTheme];

	// Board shadow / frame
	DrawRectangle(boardX - 4, boardY - 4, 8 * boxSize + 8, 8 * boxSize + 8, Color{ 20, 20, 20, 180 });
	DrawRectangle(boardX - 2, boardY - 2, 8 * boxSize + 4, 8 * boxSize + 4, Color{ 48, 46, 43, 255 });

	for (int r = 0; r < 8; r++) {
		for (int c = 0; c < 8; c++) {
			int dr = flipped ? 7 - r : r;
			int dc = flipped ? 7 - c : c;

			int sx = boardX + dc * boxSize;
			int sy = boardY + dr * boxSize;

			bool isLight = ((r + c) % 2 == 0);
			Color sqColor = isLight ? theme.lightSquare : theme.darkSquare;
			Color coordColor = isLight ? theme.darkSquare : theme.lightSquare;

			DrawRectangle(sx, sy, boxSize, boxSize, sqColor);

			// Coordinates in corners of border squares (Chess.com style)
			// Rank numbers on leftmost column (dc == 0)
			if (dc == 0) {
				char rankStr[2] = { (char)(flipped ? ('1' + dr) : ('8' - dr)), '\0' };
				DrawText(rankStr, sx + 5, sy + 4, 15, coordColor);
			}

			// File letters on bottom row (dr == 7)
			if (dr == 7) {
				char fileStr[2] = { (char)(flipped ? ('h' - dc) : ('a' + dc)), '\0' };
				DrawText(fileStr, sx + boxSize - 13, sy + boxSize - 18, 15, coordColor);
			}
		}
	}
}

void Board::movePiece(Position S, Position D) {
	Piece* moving = getPiece(S);

	// En passant: the captured pawn sits on the source row / destination
	// column, not on the destination square itself, so it has to be
	// removed separately from the normal capture handling below.
	bool ep = isEnPassantMove(S, D);
	if (ep) {
		Piece* epCaptured = board[S.row][D.col];
		delete epCaptured;
		board[S.row][D.col] = nullptr;
	}

	Piece* captured = board[D.row][D.col];
	if (captured != nullptr and captured != moving) {
		delete captured;
	}

	board[D.row][D.col] = moving;
	board[S.row][S.col] = nullptr;

	moving->sethasMoved(true);

	// A fresh en-passant target only exists for the very next move, right
	// after a two-square pawn push; any other move clears it.
	char movedSymbol = moving->getSymbol();
	if ((movedSymbol == 'P' or movedSymbol == 'p') and abs(D.row - S.row) == 2) {
		enPassantTarget = { (S.row + D.row) / 2, S.col };
	}
	else {
		enPassantTarget = { -1, -1 };
	}

	if (dynamic_cast<King*>(moving) and abs(D.col - S.col) == 2) {
		
		if (D.col == 6) {
			board[D.row][5] = board[D.row][7];
			board[D.row][7] = nullptr;
			if (board[D.row][5]) board[D.row][5]->sethasMoved(true);
		}
		
		else if (D.col == 2) {
			board[D.row][3] = board[D.row][0];
			board[D.row][0] = nullptr;
			if (board[D.row][3]) board[D.row][3]->sethasMoved(true);
		}
	}
}



bool Board::isInside(int row, int col) const {
	return row >= 0 and row < 8 and col >= 0 and col < 8;
}

bool Board::isEmpty(int row, int col) const {
	return isInside(row, col) and board[row][col] == nullptr;
}

void Board::drawBoard(Position selected, bool highlightSelected,
	Position skip1, Position skip2, int boardX, int boardY, int boxSize) {

	display(boardX, boardY, boxSize);

	// 1. Last Move Highlight (Chess.com contrasting yellow/olive tints)
	if (lastMove.from.row >= 0 && lastMove.from.col >= 0 &&
		lastMove.to.row >= 0 && lastMove.to.col >= 0) {
		int dr1 = flipped ? 7 - lastMove.from.row : lastMove.from.row;
		int dc1 = flipped ? 7 - lastMove.from.col : lastMove.from.col;
		int dr2 = flipped ? 7 - lastMove.to.row : lastMove.to.row;
		int dc2 = flipped ? 7 - lastMove.to.col : lastMove.to.col;

		bool isLight1 = ((lastMove.from.row + lastMove.from.col) % 2 == 0);
		bool isLight2 = ((lastMove.to.row + lastMove.to.col) % 2 == 0);

		Color tintFrom = isLight1 ? Color{ 245, 246, 130, 135 } : Color{ 185, 202, 67, 155 };
		Color tintTo   = isLight2 ? Color{ 245, 246, 130, 160 } : Color{ 185, 202, 67, 185 };

		Rectangle rectFrom{ (float)(boardX + dc1 * boxSize), (float)(boardY + dr1 * boxSize), (float)boxSize, (float)boxSize };
		Rectangle rectTo{ (float)(boardX + dc2 * boxSize), (float)(boardY + dr2 * boxSize), (float)boxSize, (float)boxSize };

		DrawRectangleRec(rectFrom, tintFrom);
		DrawRectangleRec(rectTo, tintTo);

		// Chess.com subtle border accent on arrival square & departure square
		DrawRectangleLinesEx(rectTo, 2.5f, Color{ 255, 245, 120, 185 });
		DrawRectangleLinesEx(rectFrom, 1.5f, Color{ 220, 215, 100, 110 });
	}

	// 2. Right-click marked squares (orange/red tint)
	for (const auto& sq : markedSquares) {
		int dr = flipped ? 7 - sq.row : sq.row;
		int dc = flipped ? 7 - sq.col : sq.col;
		DrawRectangle(boardX + dc * boxSize, boardY + dr * boxSize, boxSize, boxSize, Color{ 235, 97, 80, 130 });
	}

	// 3. Selected square highlight (warm gold)
	if (highlightSelected && selected.row >= 0 && selected.col >= 0) {
		int dr = flipped ? 7 - selected.row : selected.row;
		int dc = flipped ? 7 - selected.col : selected.col;
		Rectangle selRect{ (float)(boardX + dc * boxSize), (float)(boardY + dr * boxSize), (float)boxSize, (float)boxSize };
		DrawRectangleRec(selRect, Color{ 246, 246, 105, 160 });
	}

	// 4. King in check distress glow
	for (int colInt = 0; colInt < 2; colInt++) {
		COLOR col = (colInt == 0) ? PWHITE : PBLACK;
		if (isInCheck(col)) {
			char targetSym = (col == PWHITE) ? 'K' : 'k';
			for (int r = 0; r < 8; r++) {
				for (int c = 0; c < 8; c++) {
					if (board[r][c] && board[r][c]->getSymbol() == targetSym) {
						int dr = flipped ? 7 - r : r;
						int dc = flipped ? 7 - c : c;
						float pulse = 0.6f + 0.4f * sinf((float)GetTime() * 6.0f);
						float cx = boardX + dc * boxSize + boxSize / 2.0f;
						float cy = boardY + dr * boxSize + boxSize / 2.0f;
						DrawCircleGradient(Vector2{ cx, cy }, boxSize * 0.55f, Fade(RED, 0.8f * pulse), Fade(RED, 0.0f));
						break;
					}
				}
			}
		}
	}

	// 5. Pieces
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			bool skipped = (i == skip1.row && j == skip1.col) || (i == skip2.row && j == skip2.col);
			bool isDragging = (draggingPiecePos.row == i && draggingPiecePos.col == j);
			if (board[i][j] && !skipped && !isDragging) {
				int dr = flipped ? 7 - i : i;
				int dc = flipped ? 7 - j : j;
				board[i][j]->draw(dr, dc, boxSize, boardX, boardY);
			}
		}
	}

	// 6. Move hints (Chess.com dots & capture rings)
	for (int r = 0; r < 8; ++r) {
		for (int c = 0; c < 8; ++c) {
			if (highlight[r][c]) {
				int dr = flipped ? 7 - r : r;
				int dc = flipped ? 7 - c : c;
				float cx = boardX + dc * boxSize + boxSize / 2.0f;
				float cy = boardY + dr * boxSize + boxSize / 2.0f;

				bool isCapture = (board[r][c] != nullptr) || isEnPassantMove(selected, { r, c });

				if (!isCapture) {
					// Subtle dark center dot for empty squares
					DrawCircle((int)cx, (int)cy, boxSize * 0.16f, Color{ 0, 0, 0, 55 });
				}
				else {
					// Elegant ring for capture squares
					DrawRing(Vector2{ cx, cy }, boxSize * 0.38f, boxSize * 0.46f, 0, 360, 36, Color{ 0, 0, 0, 75 });
				}
			}
		}
	}

	// 7. Last Move Trajectory Arrow (Chess.com signature golden arrow)
	if (showLastMoveArrow && lastMove.from.row >= 0 && lastMove.from.col >= 0 &&
		lastMove.to.row >= 0 && lastMove.to.col >= 0) {
		int fdr = flipped ? 7 - lastMove.from.row : lastMove.from.row;
		int fdc = flipped ? 7 - lastMove.from.col : lastMove.from.col;
		int tdr = flipped ? 7 - lastMove.to.row : lastMove.to.row;
		int tdc = flipped ? 7 - lastMove.to.col : lastMove.to.col;

		Vector2 startPos{ boardX + fdc * boxSize + boxSize / 2.0f, boardY + fdr * boxSize + boxSize / 2.0f };
		Vector2 endPos{ boardX + tdc * boxSize + boxSize / 2.0f, boardY + tdr * boxSize + boxSize / 2.0f };
		DrawChessArrow(startPos, endPos, Color{ 255, 215, 60, 165 });
	}

	// 8. Arrows (Right-click drawn arrows)
	for (const auto& arr : arrows) {
		int fdr = flipped ? 7 - arr.from.row : arr.from.row;
		int fdc = flipped ? 7 - arr.from.col : arr.from.col;
		int tdr = flipped ? 7 - arr.to.row : arr.to.row;
		int tdc = flipped ? 7 - arr.to.col : arr.to.col;

		Vector2 startPos{ boardX + fdc * boxSize + boxSize / 2.0f, boardY + fdr * boxSize + boxSize / 2.0f };
		Vector2 endPos{ boardX + tdc * boxSize + boxSize / 2.0f, boardY + tdr * boxSize + boxSize / 2.0f };
		DrawChessArrow(startPos, endPos, Color{ 255, 170, 0, 215 });
	}
}



bool Board::isPathClearHorizontal(Position S, Position D) const {
	if (!isInside(S.row, S.col) || !isInside(D.row, D.col)) return false;

	int sr = S.row;
	int sc = S.col;
	int ec = D.col;

	if (sc == ec) return false;

	int step = (ec > sc) ? 1 : -1;
	for (int c = sc + step; c != ec; c += step)
		if (board[sr][c] != nullptr)
			return false;
	return true;
}

bool Board::isPathClearVertical(Position S, Position D) const {
	if (!isInside(S.row, S.col) || !isInside(D.row, D.col)) return false;

	int sr = S.row;
	int sc = S.col;
	int er = D.row;

	if (sr == er) return false;

	int step = (er > sr) ? 1 : -1;
	for (int r = sr + step; r != er; r += step)
		if (board[r][sc] != nullptr)
			return false;
	return true;
}

bool Board::isPathClearDiagonal(Position S, Position D) const {
	if (!isInside(S.row, S.col) || !isInside(D.row, D.col)) return false;

	int sr = S.row;
	int sc = S.col;
	int er = D.row;
	int ec = D.col;

	if (sr == er or sc == ec) return false;

	int dr = (er > sr) ? 1 : -1;
	int dc = (ec > sc) ? 1 : -1;
	int r = sr + dr;
	int c = sc + dc;
	while (r != er and c != ec) {
		if (board[r][c] != nullptr) return false;
		r += dr;
		c += dc;
	}
	return true;
}


bool Board::isEnPassantMove(Position S, Position D) const {
	if (!isInside(S.row, S.col) || !isInside(D.row, D.col)) return false;
	Piece* moving = board[S.row][S.col];
	if (!moving) return false;

	char s = moving->getSymbol();
	if (s != 'P' and s != 'p') return false;

	if (S.col == D.col) return false;                 // en passant is a diagonal step
	if (board[D.row][D.col] != nullptr) return false;  // a normal diagonal capture, not en passant

	return D.row == enPassantTarget.row and D.col == enPassantTarget.col;
}

Piece* Board::getPiece(Position X) {
	if (!isInside(X.row, X.col)) return nullptr;
	return board[X.row][X.col];
}
Piece* Board::getPiece(int row , int col) {
	if (!isInside(row, col)) return nullptr;
	return board[row][col];
}


void Board::setPiece(Position pos, Piece* piece) {
	if (!isInside(pos.row, pos.col)) {
		delete piece;
		return;
	}
	if (board[pos.row][pos.col] != nullptr) {
		delete board[pos.row][pos.col];
	}
	board[pos.row][pos.col] = piece;
}



void Board::clearHighlight() {
	for (int i = 0; i < 8; i++)
		for (int j = 0; j < 8; j++)
			highlight[i][j] = false;
}

void Board::computeHighlight(Position S) {
	int sr = S.row;
	int sc = S.col;

	clearHighlight();

	if (!isInside(sr, sc)) return;

	Piece* p = board[sr][sc];
	if (!p) return;

	for (int r = 0; r < 8; ++r) {
		for (int c = 0; c < 8; ++c) {
			if (p->isLegal(this, S, { r , c }) && !isSelfCheck(S, { r , c }, p->getColor()))
				highlight[r][c] = true;
		}
	}
}



char Board::Promotion(COLOR color) {
	const char* labels[] = { "Q", "R", "B", "N" };
	Rectangle buttons[4];
	float hover[4] = { 0, 0, 0, 0 };

	float modalW = 460;
	float modalH = 220;
	float modalX = (VIRTUAL_WIDTH - modalW) / 2.0f;
	float modalY = (VIRTUAL_HEIGHT - modalH) / 2.0f;

	for (int i = 0; i < 4; ++i)
		buttons[i] = { modalX + 25 + i * 105, modalY + 115, 95, 65 };

	while (!WindowShouldClose()) {
		BeginVirtualScreen();
		HandleWindowControls();

		// Translucent modal backdrop
		DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, Color{ 0, 0, 0, 160 });

		// Modal card
		DrawRectangleRounded(Rectangle{ modalX, modalY, modalW, modalH }, 0.08f, 8, Color{ 38, 36, 34, 250 });
		DrawRectangleRoundedLinesEx(Rectangle{ modalX, modalY, modalW, modalH }, 0.08f, 8, 2.0f, Color{ 70, 68, 65, 255 });

		const char* heading = (color == PWHITE ? "Pawn Promotion - White" : "Pawn Promotion - Black");
		int headW = MeasureText(heading, 22);
		DrawText(heading, modalX + (modalW - headW) / 2, modalY + 25, 22, RAYWHITE);

		const char* sub = "Choose a piece to promote to:";
		int subW = MeasureText(sub, 16);
		DrawText(sub, modalX + (modalW - subW) / 2, modalY + 60, 16, LIGHTGRAY);

		for (int i = 0; i < 4; ++i) {
			if (DrawButton(buttons[i], labels[i], 32, hover[i], Color{ 54, 52, 49, 255 }, Color{ 129, 182, 76, 255 })) {
				EndVirtualScreen();
				return labels[i][0];
			}
		}

		EndVirtualScreen();
	}

	return 'Q'; // window closed mid-dialog — fall back to auto-queen
}

void Board::handlePawnPromotion(int row, int col,
	Texture2D queen, Texture2D rook, Texture2D bishop, Texture2D knight) {
	(void)queen; (void)rook; (void)bishop; (void)knight;

	Piece* pawn = getPiece(row, col);
	if (!pawn or (pawn->getSymbol() != 'P' and pawn->getSymbol() != 'p'))
		return;

	COLOR color = pawn->getColor();
	char choice = Promotion(color); 

	ensureBoardTextures();

	Piece* promoted = nullptr;
	switch (choice) {
	case 'Q':
		promoted = new Queen(color, color == PWHITE ? sBoardTex.whiteQueen : sBoardTex.blackQueen);
		break;

	case 'R':
		promoted = new Rook(color, color == PWHITE ? sBoardTex.whiteRook : sBoardTex.blackRook);
		break;

	case 'B': 
		promoted = new Bishop(color, color == PWHITE ? sBoardTex.whiteBishop : sBoardTex.blackBishop);
		break;
	
	case 'N':
	case 'K':
		promoted = new Knight(color, color == PWHITE ? sBoardTex.whiteKnight : sBoardTex.blackKnight);
		break;
	}

	setPiece({ row, col }, promoted);  
}



bool Board::isInCheck(COLOR color) {
	int kingRow = -1, kingCol = -1;

	for (int r = 0; r < 8; ++r) {
		for (int c = 0; c < 8; ++c) {
			Piece* piece = getPiece(r, c);
			if (piece and piece->getColor() == color) {
				King* king = dynamic_cast<King*>(piece);
				if (king) {
					kingRow = r;
					kingCol = c;
					break;
				}
			}
		}
		if (kingRow != -1) break;
	}

	if (kingRow == -1)
		return false;

	return isSquareAttacked(kingRow, kingCol, color);
}

bool Board::isSquareAttacked(int row, int col, COLOR defenderColor) {
	for (int r = 0; r < 8; ++r) {
		for (int c = 0; c < 8; ++c) {
			if (r == row && c == col) continue;
			Piece* attacker = getPiece(r, c);
			if (attacker && attacker->getColor() != defenderColor) {
				Pawn* pawn = dynamic_cast<Pawn*>(attacker);
				if (pawn) {
					int direction = (pawn->getColor() == PWHITE) ? -1 : 1;
					if (row == r + direction && abs(col - c) == 1) return true;
					continue;
				}
				King* king = dynamic_cast<King*>(attacker);
				if (king) {
					if (abs(row - r) <= 1 && abs(col - c) <= 1) return true;
					continue;
				}
				if (attacker->isLegal(this, { r, c }, { row, col })) {
					return true;
				}
			}
		}
	}
	return false;
}




bool Board::hasLegalMove(COLOR color) {

	for (int sr = 0; sr < 8; ++sr) {
		for (int sc = 0; sc < 8; ++sc) {
			Piece* piece = getPiece(sr, sc);
			if (piece and piece->getColor() == color) {
				for (int er = 0; er < 8; ++er) {
					for (int ec = 0; ec < 8; ++ec) {
						Position source = { sr, sc };
						Position dest = { er, ec };

						if (piece->isLegal(this, source, dest)) {
							if (!isSelfCheck({ sr, sc }, { er, ec }, color)) {
								return true;
							}
						}
					}
				}
			}
		}
	}
	return false;
}


bool Board::isSelfCheck(Position S, Position D, COLOR color) {
	if (!isInside(S.row, S.col) || !isInside(D.row, D.col)) return true;
	Piece* moving = getPiece(S);
	if (!moving) return true;
	Piece* p = getPiece(D);

	// An en passant capture removes a pawn that isn't on the destination
	// square, which can expose the king along that rank (a classic
	// "en passant discovered check" case) — account for it here too.
	bool ep = isEnPassantMove(S, D);
	Position epPos{ -1, -1 };
	Piece* epCaptured = nullptr;
	if (ep) {
		epPos = { S.row, D.col };
		epCaptured = board[epPos.row][epPos.col];
		board[epPos.row][epPos.col] = nullptr;
	}

	board[D.row][D.col] = moving;
	board[S.row][S.col] = nullptr;

	bool check = isInCheck(color);


	board[S.row][S.col] = moving;
	board[D.row][D.col] = p;

	if (ep) {
		board[epPos.row][epPos.col] = epCaptured;
	}

	return check;
}

std::string Board::snapshot(COLOR turn) {
	ostringstream out;

	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			if (board[i][j] != nullptr)
				out << board[i][j]->getSymbol();
			else
				out << "-";
		}
		out << "\n";
	}

	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			if (board[i][j] != nullptr)
				out << (board[i][j]->gethasMoved() ? '1' : '0');
			else
				out << "-";
		}
		out << "\n";
	}

	out << turn << "\n";
	out << enPassantTarget.row << " " << enPassantTarget.col << "\n";
	out << lastMove.from.row << " " << lastMove.from.col << " " << lastMove.to.row << " " << lastMove.to.col << "\n";
	return out.str();
}

void Board::restoreSnapshot(const std::string& data, COLOR& turn) {
	istringstream rdr(data);

	for (int i = 0; i < 8; ++i)
		for (int j = 0; j < 8; ++j) {
			delete board[i][j];
			board[i][j] = nullptr;
		}

	ensureBoardTextures();

	char ch;

	for (int row = 0; row < 8; ++row) {
		for (int col = 0; col < 8; ++col) {
			rdr >> ch;

			if (ch == '-') continue;

			COLOR color = isupper(ch) ? PWHITE : PBLACK;
			ch = tolower(ch);

			switch (ch) {
			case 'p': board[row][col] = new Pawn(color, color == PWHITE ? sBoardTex.whitePawn : sBoardTex.blackPawn); break;
			case 'r': board[row][col] = new Rook(color, color == PWHITE ? sBoardTex.whiteRook : sBoardTex.blackRook); break;
			case 'n': board[row][col] = new Knight(color, color == PWHITE ? sBoardTex.whiteKnight : sBoardTex.blackKnight); break;
			case 'b': board[row][col] = new Bishop(color, color == PWHITE ? sBoardTex.whiteBishop : sBoardTex.blackBishop); break;
			case 'q': board[row][col] = new Queen(color, color == PWHITE ? sBoardTex.whiteQueen : sBoardTex.blackQueen); break;
			case 'k': board[row][col] = new King(color, color == PWHITE ? sBoardTex.whiteKing : sBoardTex.blackKing); break;
			}
		}
	}

	for (int row = 0; row < 8; ++row) {
		for (int col = 0; col < 8; ++col) {
			if (!(rdr >> ch)) break;
			if (board[row][col] != nullptr and ch == '1')
				board[row][col]->sethasMoved(true);
		}
	}

	int turnValue;
	rdr >> turnValue;
	turn = static_cast<COLOR>(turnValue);

	int epRow = -1, epCol = -1;
	if (rdr >> epRow >> epCol)
		enPassantTarget = { epRow, epCol };
	else
		enPassantTarget = { -1, -1 };

	int lmFr = -1, lmFc = -1, lmTr = -1, lmTc = -1;
	if (rdr >> lmFr >> lmFc >> lmTr >> lmTc) {
		lastMove = { { lmFr, lmFc }, { lmTr, lmTc } };
	} else {
		if (!moveHistory.empty()) lastMove = moveHistory.back().move;
		else lastMove = { { -1, -1 }, { -1, -1 } };
	}
}

void Board::save(COLOR turn) {
	ofstream wtr("haha.txt");

	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++)
		{
			if (board[i][j] != nullptr)
				wtr << board[i][j]->getSymbol();
			else
				wtr << "-";
		}
		wtr << endl;
	}

	// hasMoved grid: needed so castling legality survives a reload.
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++)
		{
			if (board[i][j] != nullptr)
				wtr << (board[i][j]->gethasMoved() ? '1' : '0');
			else
				wtr << "-";
		}
		wtr << endl;
	}

	wtr << turn << endl;
	wtr << enPassantTarget.row << " " << enPassantTarget.col << endl;
	wtr << lastMove.from.row << " " << lastMove.from.col << " " << lastMove.to.row << " " << lastMove.to.col << endl;
}

void Board::load(COLOR& turn) {
	ifstream rdr("haha.txt");
	if (!rdr.is_open()) {
		initillize();
		turn = PWHITE;
		return;
	}

	for (int i = 0; i < 8; ++i)
		for (int j = 0; j < 8; ++j) {
			delete board[i][j];
			board[i][j] = nullptr;
		}

	
	ensureBoardTextures();

	char ch;

	for (int row = 0; row < 8; ++row) {
		for (int col = 0; col < 8; ++col) {
			rdr >> ch;

			if (ch == '-') continue;

			COLOR color = isupper(ch) ? PWHITE : PBLACK;
			ch = tolower(ch);

			switch (ch) {
			case 'p': board[row][col] = new Pawn(color, color == PWHITE ? sBoardTex.whitePawn : sBoardTex.blackPawn); break;
			case 'r': board[row][col] = new Rook(color, color == PWHITE ? sBoardTex.whiteRook : sBoardTex.blackRook); break;
			case 'n': board[row][col] = new Knight(color, color == PWHITE ? sBoardTex.whiteKnight : sBoardTex.blackKnight); break;
			case 'b': board[row][col] = new Bishop(color, color == PWHITE ? sBoardTex.whiteBishop : sBoardTex.blackBishop); break;
			case 'q': board[row][col] = new Queen(color, color == PWHITE ? sBoardTex.whiteQueen : sBoardTex.blackQueen); break;
			case 'k': board[row][col] = new King(color, color == PWHITE ? sBoardTex.whiteKing : sBoardTex.blackKing); break;
			}
		}
	}

	// hasMoved grid (see Board::save). If it's missing — an older save
	// file — this simply reads nothing further and every piece keeps its
	// default "hasn't moved" state, same as before this fix.
	for (int row = 0; row < 8; ++row) {
		for (int col = 0; col < 8; ++col) {
			if (!(rdr >> ch)) break;
			if (board[row][col] != nullptr and ch == '1')
				board[row][col]->sethasMoved(true);
		}
	}

	int turnValue;
	rdr >> turnValue;
	turn = static_cast<COLOR>(turnValue);

	int epRow = -1, epCol = -1;
	if (rdr >> epRow >> epCol)
		enPassantTarget = { epRow, epCol };
	else
		enPassantTarget = { -1, -1 };

	int lmFr = -1, lmFc = -1, lmTr = -1, lmTc = -1;
	if (rdr >> lmFr >> lmFc >> lmTr >> lmTc) {
		lastMove = { { lmFr, lmFc }, { lmTr, lmTc } };
	} else {
		lastMove = { { -1, -1 }, { -1, -1 } };
	}

	rdr.close();
}


// ============================ Bot / AI support ============================

std::string Board::toFEN(COLOR turn) const {
	std::string fen;

	for (int r = 0; r < 8; ++r) {
		int emptyCount = 0;
		for (int c = 0; c < 8; ++c) {
			Piece* p = board[r][c];
			if (!p) {
				emptyCount++;
			}
			else {
				if (emptyCount > 0) {
					fen += std::to_string(emptyCount);
					emptyCount = 0;
				}
				fen += p->getSymbol();
			}
		}
		if (emptyCount > 0) {
			fen += std::to_string(emptyCount);
		}
		if (r < 7) fen += '/';
	}

	fen += (turn == PWHITE) ? " w " : " b ";

	std::string castling = "";
	// White Kingside: King at e1 (7,4), Rook at h1 (7,7)
	Piece* wKing = board[7][4];
	Piece* wRookK = board[7][7];
	if (wKing && wKing->getSymbol() == 'K' && !wKing->gethasMoved()) {
		if (wRookK && wRookK->getSymbol() == 'R' && !wRookK->gethasMoved()) {
			castling += 'K';
		}
	}
	// White Queenside: King at e1 (7,4), Rook at a1 (7,0)
	Piece* wRookQ = board[7][0];
	if (wKing && wKing->getSymbol() == 'K' && !wKing->gethasMoved()) {
		if (wRookQ && wRookQ->getSymbol() == 'R' && !wRookQ->gethasMoved()) {
			castling += 'Q';
		}
	}
	// Black Kingside: King at e8 (0,4), Rook at h8 (0,7)
	Piece* bKing = board[0][4];
	Piece* bRookK = board[0][7];
	if (bKing && bKing->getSymbol() == 'k' && !bKing->gethasMoved()) {
		if (bRookK && bRookK->getSymbol() == 'r' && !bRookK->gethasMoved()) {
			castling += 'k';
		}
	}
	// Black Queenside: King at e8 (0,4), Rook at a8 (0,0)
	Piece* bRookQ = board[0][0];
	if (bKing && bKing->getSymbol() == 'k' && !bKing->gethasMoved()) {
		if (bRookQ && bRookQ->getSymbol() == 'r' && !bRookQ->gethasMoved()) {
			castling += 'q';
		}
	}

	if (castling.empty()) castling = "-";
	fen += castling + " ";

	if (enPassantTarget.row >= 0 && enPassantTarget.row < 8 &&
		enPassantTarget.col >= 0 && enPassantTarget.col < 8) {
		char f = static_cast<char>('a' + enPassantTarget.col);
		char r = static_cast<char>('8' - enPassantTarget.row);
		fen += f;
		fen += r;
	}
	else {
		fen += "-";
	}

	fen += " 0 1";
	return fen;
}

// Standard material values. King is weighted heavily so the search treats
// losing it (checkmate) as catastrophic without needing a special case in
// the middle of the recursion.
int Board::pieceValue(char symbol) const {
	char s = tolower(symbol);
	switch (s) {
	case 'p': return 100;
	case 'n': return 320;
	case 'b': return 330;
	case 'r': return 500;
	case 'q': return 900;
	case 'k': return 20000;
	}
	return 0;
}

// Positive score favors White, negative favors Black.
int Board::evaluateBoard() {
	int score = 0;
	for (int r = 0; r < 8; ++r) {
		for (int c = 0; c < 8; ++c) {
			Piece* p = board[r][c];
			if (!p) continue;
			int val = pieceValue(p->getSymbol());
			score += (p->getColor() == PWHITE) ? val : -val;
		}
	}
	return score;
}

std::vector<Move> Board::generateLegalMoves(COLOR color) {
	std::vector<Move> moves;

	for (int sr = 0; sr < 8; ++sr) {
		for (int sc = 0; sc < 8; ++sc) {
			Piece* p = board[sr][sc];
			if (!p or p->getColor() != color) continue;

			for (int er = 0; er < 8; ++er) {
				for (int ec = 0; ec < 8; ++ec) {
					Position S{ sr, sc };
					Position D{ er, ec };
					if (p->isLegal(this, S, D) and !isSelfCheck(S, D, color)) {
						moves.push_back({ S, D });
					}
				}
			}
		}
	}

	return moves;
}

// Temporarily move a piece on the live board without deleting the captured
// piece, so the search can explore and cheaply roll back positions.
Piece* Board::simulateMove(Position S, Position D) {
	if (!isInside(S.row, S.col) || !isInside(D.row, D.col)) {
		epStack.push_back({ false, {-1, -1}, nullptr });
		return nullptr;
	}

	EPInfo info{ false, {-1, -1}, nullptr };
	if (isEnPassantMove(S, D)) {
		info.isEP = true;
		info.epPos = { S.row, D.col };
		info.epPiece = board[info.epPos.row][info.epPos.col];
		board[info.epPos.row][info.epPos.col] = nullptr;
	}
	epStack.push_back(info);

	Piece* moving = board[S.row][S.col];
	Piece* captured = board[D.row][D.col];
	board[D.row][D.col] = moving;
	board[S.row][S.col] = nullptr;
	return captured;
}

void Board::undoMove(Position S, Position D, Piece* captured) {
	if (epStack.empty()) return;
	if (!isInside(S.row, S.col) || !isInside(D.row, D.col)) {
		epStack.pop_back();
		return;
	}

	board[S.row][S.col] = board[D.row][D.col];
	board[D.row][D.col] = captured;

	EPInfo info = epStack.back();
	epStack.pop_back();
	if (info.isEP && isInside(info.epPos.row, info.epPos.col)) {
		board[info.epPos.row][info.epPos.col] = info.epPiece;
	}
}

int Board::minimax(int depth, int alpha, int beta, COLOR toMove) {
	if (depth == 0) return evaluateBoard();

	std::vector<Move> moves = generateLegalMoves(toMove);

	if (moves.empty()) {
		if (isInCheck(toMove)) {
			// Checkmate: very bad for whoever is toMove.
			return (toMove == PWHITE) ? -1000000 : 1000000;
		}
		return 0; // stalemate
	}

	COLOR next = (toMove == PWHITE) ? PBLACK : PWHITE;

	if (toMove == PWHITE) {
		int maxEval = INT_MIN;
		for (auto& mv : moves) {
			Piece* captured = simulateMove(mv.from, mv.to);
			int eval = minimax(depth - 1, alpha, beta, next);
			undoMove(mv.from, mv.to, captured);

			maxEval = std::max(maxEval, eval);
			alpha = std::max(alpha, eval);
			if (beta <= alpha) break;
		}
		return maxEval;
	}
	else {
		int minEval = INT_MAX;
		for (auto& mv : moves) {
			Piece* captured = simulateMove(mv.from, mv.to);
			int eval = minimax(depth - 1, alpha, beta, next);
			undoMove(mv.from, mv.to, captured);

			minEval = std::min(minEval, eval);
			beta = std::min(beta, eval);
			if (beta <= alpha) break;
		}
		return minEval;
	}
}

Move Board::findBestMove(COLOR color, int depth) {
	std::vector<Move> moves = generateLegalMoves(color);
	if (moves.empty()) return { {-1, -1}, {-1, -1} };

	// Try captures first - improves alpha-beta pruning significantly.
	std::sort(moves.begin(), moves.end(), [this](const Move& a, const Move& b) {
		bool aCap = board[a.to.row][a.to.col] != nullptr;
		bool bCap = board[b.to.row][b.to.col] != nullptr;
		return aCap and !bCap;
	});

	Move best = moves.front();
	int bestScore = (color == PWHITE) ? INT_MIN : INT_MAX;
	int alpha = INT_MIN, beta = INT_MAX;
	COLOR next = (color == PWHITE) ? PBLACK : PWHITE;

	for (auto& mv : moves) {
		Piece* captured = simulateMove(mv.from, mv.to);
		int eval = minimax(depth - 1, alpha, beta, next);
		undoMove(mv.from, mv.to, captured);

		if (color == PWHITE) {
			if (eval > bestScore) { bestScore = eval; best = mv; }
			alpha = std::max(alpha, eval);
		}
		else {
			if (eval < bestScore) { bestScore = eval; best = mv; }
			beta = std::min(beta, eval);
		}
	}

	return best;
}

Move Board::findRandomMove(COLOR color) {
	std::vector<Move> moves = generateLegalMoves(color);
	if (moves.empty()) return { {-1, -1}, {-1, -1} };
	int idx = rand() % (int)moves.size();
	return moves[idx];
}

void Board::addArrow(Position from, Position to) {
	for (auto it = arrows.begin(); it != arrows.end(); ++it) {
		if (it->from.row == from.row && it->from.col == from.col &&
			it->to.row == to.row && it->to.col == to.col) {
			arrows.erase(it);
			return;
		}
	}
	arrows.push_back({ from, to });
}

void Board::toggleMarkedSquare(Position pos) {
	for (auto it = markedSquares.begin(); it != markedSquares.end(); ++it) {
		if (it->row == pos.row && it->col == pos.col) {
			markedSquares.erase(it);
			return;
		}
	}
	markedSquares.push_back(pos);
}

std::string Board::generateSAN(Move m, char promo) {
	if (!isInside(m.from.row, m.from.col) || !isInside(m.to.row, m.to.col)) return "";
	Piece* moving = getPiece(m.from);
	if (!moving) return "";

	char sym = moving->getSymbol();
	COLOR col = moving->getColor();
	char upperSym = toupper(sym);

	// Castling
	if (upperSym == 'K' && abs(m.to.col - m.from.col) == 2) {
		if (m.to.col == 6) return "O-O";
		if (m.to.col == 2) return "O-O-O";
	}

	std::string san = "";
	bool isCapture = (getPiece(m.to) != nullptr) || isEnPassantMove(m.from, m.to);

	if (upperSym == 'P') {
		if (isCapture) {
			san += (char)('a' + m.from.col);
			san += 'x';
		}
		san += (char)('a' + m.to.col);
		san += (char)('8' - m.to.row);
		if (promo != '\0') {
			san += '=';
			san += (char)toupper(promo);
		}
	}
	else {
		san += upperSym;

		// Disambiguation
		bool needFile = false;
		bool needRank = false;
		for (int r = 0; r < 8; ++r) {
			for (int c = 0; c < 8; ++c) {
				if (r == m.from.row && c == m.from.col) continue;
				Piece* other = board[r][c];
				if (other && other->getColor() == col && toupper(other->getSymbol()) == upperSym) {
					if (other->isLegal(this, { r, c }, m.to) && !isSelfCheck({ r, c }, m.to, col)) {
						if (c != m.from.col) needFile = true;
						else if (r != m.from.row) needRank = true;
						else { needFile = true; needRank = true; }
					}
				}
			}
		}
		if (needFile) san += (char)('a' + m.from.col);
		if (needRank) san += (char)('8' - m.from.row);

		if (isCapture) san += 'x';
		san += (char)('a' + m.to.col);
		san += (char)('8' - m.to.row);
	}

	// Check / Checkmate suffix
	COLOR oppColor = (col == PWHITE) ? PBLACK : PWHITE;
	bool ep = isEnPassantMove(m.from, m.to);
	Position epPos{ -1, -1 };
	Piece* epCaptured = nullptr;
	if (ep) {
		epPos = { m.from.row, m.to.col };
		epCaptured = board[epPos.row][epPos.col];
		board[epPos.row][epPos.col] = nullptr;
	}
	Piece* savedDest = board[m.to.row][m.to.col];
	board[m.to.row][m.to.col] = moving;
	board[m.from.row][m.from.col] = nullptr;

	if (isInCheck(oppColor)) {
		if (!hasLegalMove(oppColor)) san += '#';
		else san += '+';
	}

	board[m.from.row][m.from.col] = moving;
	board[m.to.row][m.to.col] = savedDest;
	if (ep) {
		board[epPos.row][epPos.col] = epCaptured;
	}

	return san;
}

void Board::recordMove(Move m, const std::string& san, COLOR turn, char promo) {
	moveHistory.push_back({ m, san, turn, promo });
	lastMove = m;
}

void Board::popLastMoveRecord() {
	if (!moveHistory.empty()) {
		moveHistory.pop_back();
		if (!moveHistory.empty()) {
			lastMove = moveHistory.back().move;
		}
		else {
			lastMove = { { -1, -1 }, { -1, -1 } };
		}
	}
}

int Board::getMaterialAdvantage() const {
	int whiteMat = 0;
	int blackMat = 0;
	for (int r = 0; r < 8; ++r) {
		for (int c = 0; c < 8; ++c) {
			if (board[r][c]) {
				char sym = toupper(board[r][c]->getSymbol());
				int val = 0;
				switch (sym) {
				case 'P': val = 1; break;
				case 'N': val = 3; break;
				case 'B': val = 3; break;
				case 'R': val = 5; break;
				case 'Q': val = 9; break;
				default: break;
				}
				if (board[r][c]->getColor() == PWHITE) whiteMat += val;
				else blackMat += val;
			}
		}
	}
	return whiteMat - blackMat;
}

std::vector<char> Board::getCapturedPieces(COLOR byColor) const {
	// If byColor is PWHITE, we count captured black pieces (q, r, b, n, p)
	// If byColor is PBLACK, we count captured white pieces (Q, R, B, N, P)
	COLOR targetColor = (byColor == PWHITE) ? PBLACK : PWHITE;

	int pawns = 8, knights = 2, bishops = 2, rooks = 2, queens = 1;

	for (int r = 0; r < 8; ++r) {
		for (int c = 0; c < 8; ++c) {
			if (board[r][c] && board[r][c]->getColor() == targetColor) {
				char sym = toupper(board[r][c]->getSymbol());
				switch (sym) {
				case 'P': pawns--; break;
				case 'N': knights--; break;
				case 'B': bishops--; break;
				case 'R': rooks--; break;
				case 'Q': queens--; break;
				default: break;
				}
			}
		}
	}

	std::vector<char> captured;
	char qChar = (targetColor == PWHITE) ? 'Q' : 'q';
	char rChar = (targetColor == PWHITE) ? 'R' : 'r';
	char bChar = (targetColor == PWHITE) ? 'B' : 'b';
	char nChar = (targetColor == PWHITE) ? 'N' : 'n';
	char pChar = (targetColor == PWHITE) ? 'P' : 'p';

	for (int i = 0; i < queens; ++i) captured.push_back(qChar);
	for (int i = 0; i < rooks; ++i) captured.push_back(rChar);
	for (int i = 0; i < bishops; ++i) captured.push_back(bChar);
	for (int i = 0; i < knights; ++i) captured.push_back(nChar);
	for (int i = 0; i < pawns; ++i) captured.push_back(pChar);

	return captured;
}


