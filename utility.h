#pragma once
#include"raylib.h"
#include<string>

enum COLOR {
	PWHITE,PBLACK
};

struct Position {
	int row,col;
};

struct Move {
	Position from;
	Position to;
};

struct PromotionTextures {
    Texture2D whiteQueen;
    Texture2D whiteRook;
    Texture2D whiteBishop;
    Texture2D whiteKnight;
    Texture2D whitePawn;

    Texture2D blackQueen;
    Texture2D blackRook;
    Texture2D blackBishop;
    Texture2D blackKnight;
    Texture2D blackPawn;
};

PromotionTextures loadPromotionTextures();
void unloadPromotionTextures(PromotionTextures& pt);

enum GameState {
    MENU,
    GAME
};

enum MenuChoice {
    NONE,
    NEW_GAME,
    NEW_GAME_BOT,
    NEW_GAME_ONLINE,
    LOAD_GAME,
    EXIT_GAME
};

MenuChoice ShowStartMenu();

enum EngineType {
    ENGINE_INTERNAL,
    ENGINE_STOCKFISH
};

struct BotProfile {
    const char* name;
    const char* engineBadge; // e.g. "Stockfish 19 NNUE" or "Internal AI"
    int rating;              // Displayed Elo rating
    EngineType engine;
    int stockfishElo;        // Target Elo (1320..3190, or -1 for uncapped max)
    int skillLevel;          // Skill level (0..20)
    int movetimeMs;          // Think time limit in ms
    int depth;               // search depth for fallback minimax
    bool random;             // pick random legal move
};

#define BOT_PROFILE_COUNT 6
extern const BotProfile BOT_PROFILES[BOT_PROFILE_COUNT];

struct TimeOption {
    const char* label;
    float seconds; // -1 means untimed
};

#define TIME_OPTION_COUNT 6
extern const TimeOption TIME_OPTIONS[TIME_OPTION_COUNT];

// Blocking selection screens, shown one after another when starting a game.
// Return -1 (or -999.0f for time) if the user clicked Back / pressed ESC.
int ChooseColor();            // returns PWHITE (0), PBLACK (1), or -1 if cancelled
int ChooseBotDifficulty();    // returns index into BOT_PROFILES, or -1 if cancelled
float ChooseTimeControl();    // returns seconds per side, -1 for untimed, or -999.0f if cancelled

// --- Online play (LAN / direct IP) -------------------------------------
// Forward-declared so utility.h doesn't need to include the socket headers
// pulled in by Network.h; only pointers/references to it are used here.
class NetworkSession;

enum class OnlineHostJoinChoice { HOST, JOIN, CANCELLED };
OnlineHostJoinChoice ChooseOnlineHostOrJoin();

// Text-entry screen for "host[:port]". Returns an empty string if the user
// cancelled.
std::string PromptJoinAddress();

// Host: opens a listening socket and shows a "waiting for opponent" screen
// with the host's LAN IP so they can share it with a friend. Returns true
// once a peer connects and the color/clock handshake completes; false if
// the user cancelled or WindowShouldClose() fired.
bool RunHostWaitScreen(NetworkSession& session, int port, COLOR hostColor,
    float timeControlSeconds, std::string& outError);

// Join: connects to host:port and completes the handshake, filling in the
// color/clock the host assigned. Returns false if cancelled, refused, or
// unreachable.
bool RunJoinConnectScreen(NetworkSession& session, const std::string& address, int port,
    COLOR& outAssignedColor, float& outTimeControlSeconds, std::string& outError);

// Small reusable "OK" dismissable message screen for connection errors.
void ShowErrorScreen(const std::string& title, const std::string& message);

// --- Resizable window / fullscreen / maximize support -----------------
//
// All game and menu code draws at a fixed 830x1000 "virtual" canvas, same
// as before. To make the real window resizable/fullscreen/maximizable
// without having to rewrite every hardcoded pixel coordinate in Board.cpp,
// Source.cpp, etc., we render each frame into an off-screen texture at the
// virtual resolution, then scale+letterbox that texture onto whatever the
// actual window size is. Mouse input is mapped back into virtual space the
// same way, so clicks land on the right square regardless of window size.
#define VIRTUAL_WIDTH 1240
#define VIRTUAL_HEIGHT 900

// Call once, right after InitWindow(). Makes the window resizable and sets
// up the virtual canvas.
void InitVirtualScreen();

// Use these in place of BeginDrawing()/EndDrawing() everywhere. Draw calls
// in between still use the original coordinates.
void BeginVirtualScreen();
void EndVirtualScreen();

// Use in place of GetMousePosition() anywhere gameplay/menu code reads the
// mouse, so clicks map correctly onto the virtual canvas at any window size.
Vector2 GetVirtualMousePosition();

// Call once per frame (anywhere before EndVirtualScreen). F11 toggles real
// OS fullscreen; F10 toggles a maximized window.
void HandleWindowControls();

struct GameSounds {
    Sound gameStart;     // game-start.mp3
    Sound gameEnd;       // game-end.mp3
    Sound capture;       // capture.mp3
    Sound castle;        // castle.mp3
    Sound premove;       // premove.mp3
    Sound moveSelf;      // move-self.mp3
    Sound moveOpponent;  // move-opponent.mp3
    Sound moveCheck;     // move-check.mp3
    Sound promote;       // promote.mp3
    Sound notify;        // notify.mp3
    Sound illegal;       // illegal.mp3
    Sound tenSeconds;    // tenseconds.mp3

    // Compatibility aliases
    Sound move;
    Sound check;
    Sound gameOver;
};
GameSounds loadGameSounds();
void unloadGameSounds(GameSounds& sounds);
void playChessSound(GameSounds& sounds, bool isSelf, bool isCapture, bool isCastle,
    bool isPromote, bool isCheck, bool isGameOver, bool isCheckmate);
void playMoveSound(GameSounds& sounds, bool isCapture, bool isCheck, bool isGameOver);

// Draws a sleek Chess.com right-click arrow with shaft and arrowhead
void DrawChessArrow(Vector2 start, Vector2 end, Color color);

// Reusable animated button: rounded corners, smooth hover brighten + grow.
// hoverProgress must be a persistent float (declared once outside the render
// loop, one per button) so the animation can ease in/out across frames.
// Returns true the frame it's clicked.
bool DrawButton(Rectangle rect, const char* label, int fontSize, float& hoverProgress,
    Color baseColor = GRAY, Color hoverColor = LIGHTGRAY, Color textColor = BLACK);

// Draws a shrinking black overlay for a simple fade-in-on-enter screen
// transition. Call once per frame with a counter that increments each frame.
void DrawFadeOverlay(int frameCount, int totalFrames = 18);
