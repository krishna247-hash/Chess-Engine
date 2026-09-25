#pragma once
#include"Piece.h"
#include"King.h"
#include<vector>
#include<string>

struct MoveRecord {
	Move move;
	std::string san;
	COLOR turn;
	char promoChar;
};

enum BoardThemeType {
	THEME_CHESS_COM_GREEN = 0,
	THEME_DARK_MODE,
	THEME_TOURNAMENT_WOOD,
	THEME_OCEAN_BLUE,
	THEME_COUNT
};

struct Arrow {
	Position from;
	Position to;
};

class Board
{

	Piece* board[8][8];
	static const int GRID_SIZE = 8;

public:

	bool highlight[8][8];
	BoardThemeType currentTheme = THEME_CHESS_COM_GREEN;
	Move lastMove = { {-1,-1}, {-1,-1} };
	bool showLastMoveArrow = false;
	std::vector<MoveRecord> moveHistory;
	std::vector<Arrow> arrows;
	std::vector<Position> markedSquares;
	Position draggingPiecePos = { -1, -1 };

	Board();
	~Board();
	void initillize();
	static void unloadTextures();

	Piece* getPiece(int row, int col);

	bool isInside(int row, int col) const;
	bool isEmpty(int row, int col) const;

	void movePiece(Position S, Position D);

	bool isPathClearHorizontal(Position S, Position D) const;
	bool isPathClearVertical(Position S, Position D) const;
	bool isPathClearDiagonal(Position S, Position D)const;

	void clearHighlight();
	void computeHighlight(Position S);
	
	void display(int boardX = 68, int boardY = 50, int boxSize = 100);

	// Chess.com style drawBoard with board offset, last move, dots, rings, coords, arrows
	void drawBoard(Position selected, bool highlightSelected,
		Position skip1 = Position{ -1,-1 }, Position skip2 = Position{ -1,-1 },
		int boardX = 68, int boardY = 50, int boxSize = 100);

	Piece* getPiece(Position X);
	void setPiece(Position X , Piece* P);

	Position getEnPassantTarget() const { return enPassantTarget; }
	bool isEnPassantMove(Position S, Position D) const;

	bool isSquareAttacked(int row, int col, COLOR defenderColor);
	bool isInCheck(COLOR color);
	bool isSelfCheck(Position S, Position D, COLOR color);
	bool hasLegalMove(COLOR color);

	char Promotion(COLOR color);
	void handlePawnPromotion(int row, int col,
		Texture2D queen, Texture2D rook,
		Texture2D bishop, Texture2D knight);

	void save(COLOR turn);
	void load(COLOR &turn);

	std::string snapshot(COLOR turn);
	void restoreSnapshot(const std::string& data, COLOR& turn);

	// Board rotation ("play from Black's side")
	bool flipped = false;
	void toggleFlip() { flipped = !flipped; }

	// Chess.com Board Themes
	void cycleTheme();
	void toggleNightMode() { cycleTheme(); }
	const char* getThemeName() const;

	void setLastMove(Move m) { lastMove = m; }
	Move getLastMove() const { return lastMove; }
	void toggleLastMoveArrow() { showLastMoveArrow = !showLastMoveArrow; }

	std::string generateSAN(Move m, char promo = '\0');
	void recordMove(Move m, const std::string& san, COLOR turn, char promo = '\0');
	const std::vector<MoveRecord>& getMoveHistory() const { return moveHistory; }
	void popLastMoveRecord();
	void clearMoveHistory() { moveHistory.clear(); lastMove = { {-1,-1}, {-1,-1} }; }

	void addArrow(Position from, Position to);
	void toggleMarkedSquare(Position pos);
	void clearArrows() { arrows.clear(); }
	void clearMarkedSquares() { markedSquares.clear(); }

	void setDraggingPiece(Position pos) { draggingPiecePos = pos; }
	Position getDraggingPiece() const { return draggingPiecePos; }

	int getMaterialAdvantage() const;
	std::vector<char> getCapturedPieces(COLOR byColor) const;

	// --- Bot / AI support ---
	std::string toFEN(COLOR turn) const;
	std::vector<Move> generateLegalMoves(COLOR color);
	int pieceValue(char symbol) const;
	int evaluateBoard();
	Move findBestMove(COLOR color, int depth);
	Move findRandomMove(COLOR color);

private:
	Position enPassantTarget{ -1, -1 };

	// Bookkeeping so simulateMove/undoMove (used by the bot's search) can
	// correctly remove and restore the captured pawn on an en passant move,
	// even though that pawn doesn't sit on the destination square. Calls
	// are always properly nested with the recursive search, so a simple
	// stack is enough.
	struct EPInfo { bool isEP; Position epPos; Piece* epPiece; };
	std::vector<EPInfo> epStack;

	int minimax(int depth, int alpha, int beta, COLOR toMove);
	Piece* simulateMove(Position S, Position D);
	void undoMove(Position S, Position D, Piece* captured);
};

