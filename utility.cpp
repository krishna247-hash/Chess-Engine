#include "utility.h"
#include "NetworkManager.h"
#include <cstdio>
#include <cmath>
#include <string>
#include <cstring>
#include <cctype>
#include <algorithm>

#define SCREENWIDTH 830
#define SCREENHEIGHT 1000

static RenderTexture2D gVirtualScreen;

// Computes where the virtual 830x1000 canvas lands inside the actual
// (possibly resized/fullscreen/maximized) window, preserving aspect ratio
// and centering with black letterbox bars on the sides that don't fit.
static Rectangle GetVirtualDestRect(float& outScale) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    float scale = fminf((float)screenW / VIRTUAL_WIDTH, (float)screenH / VIRTUAL_HEIGHT);
    if (scale <= 0.0f) scale = 1.0f;
    outScale = scale;

    float destW = VIRTUAL_WIDTH * scale;
    float destH = VIRTUAL_HEIGHT * scale;
    return Rectangle{ (screenW - destW) * 0.5f, (screenH - destH) * 0.5f, destW, destH };
}

void InitVirtualScreen() {
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowMinSize(415, 500);
    gVirtualScreen = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    SetTextureFilter(gVirtualScreen.texture, TEXTURE_FILTER_BILINEAR);
}

static int gVirtualScreenDepth = 0;

void BeginVirtualScreen() {
    if (gVirtualScreenDepth == 0) {
        BeginTextureMode(gVirtualScreen);
        ClearBackground(BLACK);
    }
    gVirtualScreenDepth++;
}

void EndVirtualScreen() {
    if (gVirtualScreenDepth <= 0) return;
    gVirtualScreenDepth--;
    if (gVirtualScreenDepth == 0) {
        EndTextureMode();

        float scale;
        Rectangle dest = GetVirtualDestRect(scale);

        BeginDrawing();
        ClearBackground(BLACK); // letterbox bars when window aspect ratio differs
        // Render texture is flipped vertically (OpenGL convention), hence the
        // negative height on the source rect.
        Rectangle src = { 0, 0, (float)gVirtualScreen.texture.width, -(float)gVirtualScreen.texture.height };
        DrawTexturePro(gVirtualScreen.texture, src, dest, { 0, 0 }, 0.0f, WHITE);
        EndDrawing();
    }
}

Vector2 GetVirtualMousePosition() {
    float scale;
    Rectangle dest = GetVirtualDestRect(scale);
    Vector2 mouse = GetMousePosition();
    return Vector2{ (mouse.x - dest.x) / scale, (mouse.y - dest.y) / scale };
}

void HandleWindowControls() {
    if (IsKeyPressed(KEY_F11)) {
        ToggleFullscreen();
    }
    if (IsKeyPressed(KEY_F10)) {
        if (IsWindowMaximized()) RestoreWindow();
        else MaximizeWindow();
    }
}

PromotionTextures loadPromotionTextures() {
    PromotionTextures pt;

    pt.blackRook = LoadTexture("PNGs/black-rook.png");
    pt.whiteRook = LoadTexture("PNGs/white-rook.png");

    pt.blackBishop = LoadTexture("PNGs/black-bishop.png");
    pt.whiteBishop = LoadTexture("PNGs/white-bishop.png");

    pt.blackKnight = LoadTexture("PNGs/black-knight.png");
    pt.whiteKnight = LoadTexture("PNGs/white-knight.png");

    pt.blackQueen = LoadTexture("PNGs/black-queen.png");
    pt.whiteQueen = LoadTexture("PNGs/white-queen.png");

    pt.blackPawn = LoadTexture("PNGs/black-pawn.png");
    pt.whitePawn = LoadTexture("PNGs/white-pawn.png");

    return pt;
}

void unloadPromotionTextures(PromotionTextures& pt) {
    if (pt.whiteQueen.id > 0) UnloadTexture(pt.whiteQueen);
    if (pt.whiteRook.id > 0) UnloadTexture(pt.whiteRook);
    if (pt.whiteBishop.id > 0) UnloadTexture(pt.whiteBishop);
    if (pt.whiteKnight.id > 0) UnloadTexture(pt.whiteKnight);
    if (pt.whitePawn.id > 0) UnloadTexture(pt.whitePawn);

    if (pt.blackQueen.id > 0) UnloadTexture(pt.blackQueen);
    if (pt.blackRook.id > 0) UnloadTexture(pt.blackRook);
    if (pt.blackBishop.id > 0) UnloadTexture(pt.blackBishop);
    if (pt.blackKnight.id > 0) UnloadTexture(pt.blackKnight);
    if (pt.blackPawn.id > 0) UnloadTexture(pt.blackPawn);
}

GameSounds loadGameSounds() {
    GameSounds s;
    s.gameStart = LoadSound("sounds/game-start.mp3");
    s.gameEnd = LoadSound("sounds/game-end.mp3");
    s.capture = LoadSound("sounds/capture.mp3");
    s.castle = LoadSound("sounds/castle.mp3");
    s.premove = LoadSound("sounds/premove.mp3");
    s.moveSelf = LoadSound("sounds/move-self.mp3");
    s.moveOpponent = LoadSound("sounds/move-opponent.mp3");
    s.moveCheck = LoadSound("sounds/move-check.mp3");
    s.promote = LoadSound("sounds/promote.mp3");
    s.notify = LoadSound("sounds/notify.mp3");
    s.illegal = LoadSound("sounds/illegal.mp3");
    s.tenSeconds = LoadSound("sounds/tenseconds.mp3");

    // Aliases
    s.move = s.moveSelf;
    s.check = s.moveCheck;
    s.gameOver = s.gameEnd;
    return s;
}

void unloadGameSounds(GameSounds& s) {
    if (IsSoundValid(s.gameStart)) UnloadSound(s.gameStart);
    if (IsSoundValid(s.gameEnd)) UnloadSound(s.gameEnd);
    if (IsSoundValid(s.capture)) UnloadSound(s.capture);
    if (IsSoundValid(s.castle)) UnloadSound(s.castle);
    if (IsSoundValid(s.premove)) UnloadSound(s.premove);
    if (IsSoundValid(s.moveSelf)) UnloadSound(s.moveSelf);
    if (IsSoundValid(s.moveOpponent)) UnloadSound(s.moveOpponent);
    if (IsSoundValid(s.moveCheck)) UnloadSound(s.moveCheck);
    if (IsSoundValid(s.promote)) UnloadSound(s.promote);
    if (IsSoundValid(s.notify)) UnloadSound(s.notify);
    if (IsSoundValid(s.illegal)) UnloadSound(s.illegal);
    if (IsSoundValid(s.tenSeconds)) UnloadSound(s.tenSeconds);
}

void playChessSound(GameSounds& sounds, bool isSelf, bool isCapture, bool isCastle,
    bool isPromote, bool isCheck, bool isGameOver, bool isCheckmate) {

    if (isGameOver) {
        if (isCheckmate) {
            // "When the game ends (move-check sound + game-end sound = Checkmate)"
            if (IsSoundValid(sounds.moveCheck)) PlaySound(sounds.moveCheck);
            if (IsSoundValid(sounds.gameEnd)) PlaySound(sounds.gameEnd);
        } else {
            if (IsSoundValid(sounds.gameEnd)) PlaySound(sounds.gameEnd);
        }
        return;
    }

    if (isCheck) {
        if (IsSoundValid(sounds.moveCheck)) PlaySound(sounds.moveCheck);
    } else if (isPromote) {
        if (IsSoundValid(sounds.promote)) PlaySound(sounds.promote);
    } else if (isCapture) {
        if (IsSoundValid(sounds.capture)) PlaySound(sounds.capture);
    } else if (isCastle) {
        if (IsSoundValid(sounds.castle)) PlaySound(sounds.castle);
    } else {
        if (isSelf) {
            if (IsSoundValid(sounds.moveSelf)) PlaySound(sounds.moveSelf);
        } else {
            if (IsSoundValid(sounds.moveOpponent)) PlaySound(sounds.moveOpponent);
        }
    }
}

void playMoveSound(GameSounds& sounds, bool isCapture, bool isCheck, bool isGameOver) {
    playChessSound(sounds, true, isCapture, false, false, isCheck, isGameOver, isGameOver && isCheck);
}

// --- Reusable animated UI ---

bool DrawButton(Rectangle rect, const char* label, int fontSize, float& hoverProgress,
    Color baseColor, Color hoverColor, Color textColor) {

    Vector2 mouse = GetVirtualMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, rect);

    float target = hovered ? 1.0f : 0.0f;
    float speed = 8.0f * GetFrameTime();
    if (hoverProgress < target) {
        hoverProgress += speed;
        if (hoverProgress > target) hoverProgress = target;
    }
    else if (hoverProgress > target) {
        hoverProgress -= speed;
        if (hoverProgress < target) hoverProgress = target;
    }

    Color drawColor = ColorLerp(baseColor, hoverColor, hoverProgress);

    float grow = hoverProgress * 6.0f;
    Rectangle drawRect = { rect.x - grow / 2, rect.y - grow / 2, rect.width + grow, rect.height + grow };

    // soft drop shadow
    Rectangle shadowRect = { drawRect.x + 3, drawRect.y + 4, drawRect.width, drawRect.height };
    DrawRectangleRounded(shadowRect, 0.25f, 8, Fade(BLACK, 0.25f));

    DrawRectangleRounded(drawRect, 0.25f, 8, drawColor);
    DrawRectangleRoundedLinesEx(drawRect, 0.25f, 8, 2.0f, Fade(RAYWHITE, 0.6f + 0.4f * hoverProgress));

    int textWidth = MeasureText(label, fontSize);
    DrawText(label,
        (int)(drawRect.x + (drawRect.width - textWidth) / 2),
        (int)(drawRect.y + (drawRect.height - fontSize) / 2),
        fontSize, textColor);

    return hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

void DrawFadeOverlay(int frameCount, int totalFrames) {
    if (frameCount >= totalFrames) return;
    float alpha = 1.0f - ((float)frameCount / (float)totalFrames);
    DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, Fade(BLACK, alpha));
}

void DrawChessArrow(Vector2 start, Vector2 end, Color color) {
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist < 15.0f) return;

    float angle = atan2f(dy, dx);
    float headLen = 26.0f;
    float headWidth = 24.0f;

    // Shorten line slightly so arrow head finishes cleanly
    Vector2 shaftEnd = {
        end.x - cosf(angle) * (headLen * 0.7f),
        end.y - sinf(angle) * (headLen * 0.7f)
    };

    DrawLineEx(start, shaftEnd, 10.0f, color);

    Vector2 tip = end;
    Vector2 left = {
        end.x - cosf(angle) * headLen + sinf(angle) * (headWidth * 0.5f),
        end.y - sinf(angle) * headLen - cosf(angle) * (headWidth * 0.5f)
    };
    Vector2 right = {
        end.x - cosf(angle) * headLen - sinf(angle) * (headWidth * 0.5f),
        end.y - sinf(angle) * headLen + cosf(angle) * (headWidth * 0.5f)
    };

    DrawTriangle(tip, left, right, color);
}

MenuChoice ShowStartMenu() {
    MenuChoice choice = NONE;

    const float btnW = 380;
    const float btnH = 58;
    const float startY = 230;
    const float gap = 70;
    Rectangle vsBotBtn    = { (VIRTUAL_WIDTH - btnW) / 2.0f, startY,          btnW, btnH };
    Rectangle onlineBtn   = { (VIRTUAL_WIDTH - btnW) / 2.0f, startY + gap,    btnW, btnH };
    Rectangle newGameBtn  = { (VIRTUAL_WIDTH - btnW) / 2.0f, startY + gap*2,  btnW, btnH };
    Rectangle loadGameBtn = { (VIRTUAL_WIDTH - btnW) / 2.0f, startY + gap*3,  btnW, btnH };
    Rectangle exitGameBtn = { (VIRTUAL_WIDTH - btnW) / 2.0f, startY + gap*4,  btnW, btnH };
    float hoverBot = 0, hoverOnline = 0, hoverNew = 0, hoverLoad = 0, hoverExit = 0;
    int frame = 0;

    while (!WindowShouldClose()) {
        BeginVirtualScreen();
        ClearBackground(Color{ 22, 21, 18, 255 });
        HandleWindowControls();

        if (IsKeyPressed(KEY_ESCAPE)) {
            choice = EXIT_GAME;
            EndVirtualScreen();
            break;
        }

        int titleWidth = MeasureText("CHESS", 56);
        DrawText("CHESS", (VIRTUAL_WIDTH - titleWidth) / 2, 95, 56, RAYWHITE);

        const char* sub = "Grandmaster Edition • Worldwide Multiplayer";
        int subW = MeasureText(sub, 20);
        DrawText(sub, (VIRTUAL_WIDTH - subW) / 2, 165, 20, Color{ 175, 175, 175, 255 });

        if (DrawButton(vsBotBtn, "Play vs Bot", 20, hoverBot, Color{ 48, 56, 70, 255 }, Color{ 100, 160, 220, 255 }, RAYWHITE))
            choice = NEW_GAME_BOT;
        if (DrawButton(onlineBtn, "Play Online (Worldwide) 🌐", 19, hoverOnline, Color{ 62, 52, 90, 255 }, Color{ 150, 120, 240, 255 }, RAYWHITE))
            choice = PLAY_ONLINE;
        if (DrawButton(newGameBtn, "Pass & Play (2 Players)", 20, hoverNew, Color{ 44, 62, 48, 255 }, Color{ 120, 180, 120, 255 }, RAYWHITE))
            choice = NEW_GAME;
        if (DrawButton(loadGameBtn, "Load Saved Game", 20, hoverLoad, Color{ 65, 58, 45, 255 }, Color{ 210, 180, 110, 255 }, RAYWHITE))
            choice = LOAD_GAME;
        if (DrawButton(exitGameBtn, "Exit Game", 20, hoverExit, Color{ 65, 42, 42, 255 }, Color{ 210, 85, 85, 255 }, RAYWHITE))
            choice = EXIT_GAME;

        DrawText("F11: Fullscreen   F10: Maximize   ESC: Exit", (VIRTUAL_WIDTH - MeasureText("F11: Fullscreen   F10: Maximize   ESC: Exit", 16)) / 2, 840, 16, GRAY);

        DrawFadeOverlay(frame++, 20);
        EndVirtualScreen();

        if (choice != NONE) break;
    }

    return choice;
}

const BotProfile BOT_PROFILES[BOT_PROFILE_COUNT] = {
    { "Beginner Bot",      "Rating 800",   800,  ENGINE_INTERNAL,   -1,   0,   0, 1, true  },
    { "Casual Bot",        "Rating 1350", 1350,  ENGINE_STOCKFISH, 1350,  1, 150, 1, false },
    { "Intermediate Bot",  "Rating 1600", 1600,  ENGINE_STOCKFISH, 1600,  5, 250, 2, false },
    { "Advanced Bot",      "Rating 2000", 2000,  ENGINE_STOCKFISH, 2000, 10, 400, 3, false },
    { "Master Bot",        "Rating 2400", 2400,  ENGINE_STOCKFISH, 2400, 16, 600, 3, false },
    { "Grandmaster Bot",   "Rating 3500", 3500,  ENGINE_STOCKFISH,   -1, 20, 900, 3, false },
};

const TimeOption TIME_OPTIONS[TIME_OPTION_COUNT] = {
    { "1 min (Bullet)",  60.0f  },
    { "3 min (Blitz)",   180.0f },
    { "5 min (Blitz)",   300.0f },
    { "10 min (Rapid)",  600.0f },
    { "15 min (Rapid)",  900.0f },
    { "No Timer",        -1.0f  },
};

int ChooseColor() {
    int chosen = -1;
    const float btnW = 320;
    const float btnH = 75;
    Rectangle whiteBtn = { (VIRTUAL_WIDTH - btnW) / 2.0f, 300, btnW, btnH };
    Rectangle blackBtn = { (VIRTUAL_WIDTH - btnW) / 2.0f, 400, btnW, btnH };
    Rectangle backBtn  = { (VIRTUAL_WIDTH - 220) / 2.0f, 510, 220, 50 };
    float hoverWhite = 0, hoverBlack = 0, hoverBack = 0;
    int frame = 0;
    bool picked = false;

    while (!WindowShouldClose()) {
        BeginVirtualScreen();
        ClearBackground(DARKGRAY);
        HandleWindowControls();

        if (IsKeyPressed(KEY_ESCAPE)) {
            chosen = -1; picked = true;
        }

        int titleWidth = MeasureText("Choose Your Side", 34);
        DrawText("Choose Your Side", (VIRTUAL_WIDTH - titleWidth) / 2, 200, 34, RAYWHITE);

        if (DrawButton(whiteBtn, "Play as White", 22, hoverWhite, RAYWHITE, Color{ 255,255,255,255 }, BLACK)) {
            chosen = PWHITE; picked = true;
        }
        if (DrawButton(blackBtn, "Play as Black", 22, hoverBlack, Color{ 40,40,40,255 }, Color{ 70,70,70,255 }, RAYWHITE)) {
            chosen = PBLACK; picked = true;
        }
        if (DrawButton(backBtn, "< Back", 18, hoverBack, Color{ 55,53,50,255 }, Color{ 80,78,75,255 }, RAYWHITE)) {
            chosen = -1; picked = true;
        }

        DrawFadeOverlay(frame++, 15);
        EndVirtualScreen();

        if (picked) break;
    }

    return chosen;
}

int ChooseBotDifficulty() {
    int chosen = -1;
    Rectangle buttons[BOT_PROFILE_COUNT];
    float hover[BOT_PROFILE_COUNT] = { 0 };
    float hoverBack = 0;
    const float btnW = 540;
    const float btnH = 62;
    const float startY = 150;
    const float stepY = 72;

    for (int i = 0; i < BOT_PROFILE_COUNT; ++i)
        buttons[i] = { (VIRTUAL_WIDTH - btnW) / 2.0f, startY + i * stepY, btnW, btnH };

    Rectangle backBtn = { (VIRTUAL_WIDTH - 220) / 2.0f, startY + BOT_PROFILE_COUNT * stepY + 12, 220, 48 };

    int frame = 0;
    bool picked = false;

    while (!WindowShouldClose()) {
        BeginVirtualScreen();
        ClearBackground(DARKGRAY);
        HandleWindowControls();

        if (IsKeyPressed(KEY_ESCAPE)) {
            chosen = -1; picked = true;
        }

        int titleWidth = MeasureText("Select Bot Difficulty", 34);
        DrawText("Select Bot Difficulty", (VIRTUAL_WIDTH - titleWidth) / 2, 55, 34, RAYWHITE);

        const char* sub = "Select your opponent rating";
        int subW = MeasureText(sub, 18);
        DrawText(sub, (VIRTUAL_WIDTH - subW) / 2, 102, 18, LIGHTGRAY);

        for (int i = 0; i < BOT_PROFILE_COUNT; ++i) {
            char label[96];
            snprintf(label, sizeof(label), "%s  (Elo %d)", BOT_PROFILES[i].name, BOT_PROFILES[i].rating);

            Color hoverColor = Color{ 140, 190, 150, 255 };
            if (BOT_PROFILE_COUNT - 1 == i) {
                hoverColor = Color{ 230, 190, 90, 255 };
            } else if (BOT_PROFILES[i].engine == ENGINE_STOCKFISH) {
                hoverColor = Color{ 120, 180, 220, 255 };
            }

            if (DrawButton(buttons[i], label, 20, hover[i], GRAY, hoverColor, RAYWHITE)) {
                chosen = i; picked = true;
            }

            const char* badge = BOT_PROFILES[i].engineBadge;
            int bW = MeasureText(badge, 12);
            DrawText(badge, (int)(buttons[i].x + buttons[i].width - bW - 16), (int)(buttons[i].y + 40), 12, GOLD);
        }

        if (DrawButton(backBtn, "< Back", 18, hoverBack, Color{ 55,53,50,255 }, Color{ 80,78,75,255 }, RAYWHITE)) {
            chosen = -1; picked = true;
        }

        const char* tip = "F11: Fullscreen | Play offline anytime | ESC: Back";
        int tipW = MeasureText(tip, 15);
        DrawText(tip, (VIRTUAL_WIDTH - tipW) / 2, 850, 15, GRAY);

        DrawFadeOverlay(frame++, 15);
        EndVirtualScreen();

        if (picked) break;
    }

    return chosen;
}

float ChooseTimeControl() {
    float chosen = -999.0f;
    Rectangle buttons[TIME_OPTION_COUNT];
    float hover[TIME_OPTION_COUNT] = { 0 };
    float hoverBack = 0;
    const float btnW = 460;
    const float btnH = 60;
    const float startY = 150;
    const float stepY = 72;

    for (int i = 0; i < TIME_OPTION_COUNT; ++i)
        buttons[i] = { (VIRTUAL_WIDTH - btnW) / 2.0f, startY + i * stepY, btnW, btnH };

    Rectangle backBtn = { (VIRTUAL_WIDTH - 220) / 2.0f, startY + TIME_OPTION_COUNT * stepY + 12, 220, 48 };

    int frame = 0;
    bool picked = false;

    while (!WindowShouldClose()) {
        BeginVirtualScreen();
        ClearBackground(DARKGRAY);
        HandleWindowControls();

        if (IsKeyPressed(KEY_ESCAPE)) {
            chosen = -999.0f; picked = true;
        }

        int titleWidth = MeasureText("Choose Time Control", 34);
        DrawText("Choose Time Control", (VIRTUAL_WIDTH - titleWidth) / 2, 80, 34, RAYWHITE);

        for (int i = 0; i < TIME_OPTION_COUNT; ++i) {
            if (DrawButton(buttons[i], TIME_OPTIONS[i].label, 22, hover[i], GRAY, Color{ 160,180,220,255 })) {
                chosen = TIME_OPTIONS[i].seconds; picked = true;
            }
        }

        if (DrawButton(backBtn, "< Back", 18, hoverBack, Color{ 55,53,50,255 }, Color{ 80,78,75,255 }, RAYWHITE)) {
            chosen = -999.0f; picked = true;
        }

        DrawFadeOverlay(frame++, 15);
        EndVirtualScreen();

        if (picked) break;
    }

    return chosen;
}

// --- Interactive Online Multiplayer Lobby ---

static bool DrawInputBox(Rectangle rect, char* buffer, int maxLen, bool& isFocused,
    const char* placeholder, bool upperOnly = false) {

    Vector2 mouse = GetVirtualMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, rect);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        isFocused = hovered;
    }

    if (isFocused) {
        // Typing characters
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 126) {
                int len = (int)strlen(buffer);
                if (len < maxLen - 1) {
                    char c = (char)key;
                    if (upperOnly) c = (char)toupper(c);
                    buffer[len] = c;
                    buffer[len + 1] = '\0';
                }
            }
            key = GetCharPressed();
        }

        // Backspace
        if (IsKeyPressed(KEY_BACKSPACE)) {
            int len = (int)strlen(buffer);
            if (len > 0) {
                buffer[len - 1] = '\0';
            }
        }

        // Clipboard paste (Ctrl+V / Cmd+V)
        bool pastePressed = (IsKeyPressed(KEY_V) &&
            (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL) ||
             IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER)));
        if (pastePressed) {
            const char* clip = GetClipboardText();
            if (clip) {
                int len = (int)strlen(buffer);
                for (int i = 0; clip[i] != '\0' && len < maxLen - 1; ++i) {
                    char c = clip[i];
                    if (c >= 32 && c <= 126) {
                        if (upperOnly) c = (char)toupper(c);
                        buffer[len++] = c;
                    }
                }
                buffer[len] = '\0';
            }
        }
    }

    // Box rendering
    Color bgColor = isFocused ? Color{ 48, 46, 43, 255 } : Color{ 32, 30, 28, 255 };
    Color borderColor = isFocused ? Color{ 129, 182, 76, 255 } : (hovered ? Color{ 110, 108, 104, 255 } : Color{ 62, 60, 56, 255 });

    DrawRectangleRounded(rect, 0.2f, 6, bgColor);
    DrawRectangleRoundedLinesEx(rect, 0.2f, 6, isFocused ? 2.0f : 1.0f, borderColor);

    int len = (int)strlen(buffer);
    int fontSize = (rect.height >= 48) ? 24 : 18;
    int textY = (int)(rect.y + (rect.height - fontSize) / 2.0f);

    if (len == 0 && placeholder) {
        DrawText(placeholder, (int)rect.x + 14, textY, fontSize, Color{ 110, 108, 104, 255 });
    } else {
        DrawText(buffer, (int)rect.x + 14, textY, fontSize, RAYWHITE);
    }

    if (isFocused) {
        int textW = MeasureText(buffer, fontSize);
        if (fmodf((float)GetTime(), 0.8f) < 0.4f) {
            DrawRectangle((int)rect.x + 16 + textW, textY, 2, fontSize, Color{ 129, 182, 76, 255 });
        }
    }

    return isFocused;
}

bool ShowOnlineLobby(std::string& outRoomCode, COLOR& outMyColor, float& outTimeControl,
    std::string& outOpponentName, std::string& outPlayerName) {

    NetworkManager& net = NetworkManager::getInstance();
    net.loadConfig();

    char nameBuf[64];
    memset(nameBuf, 0, sizeof(nameBuf));
    snprintf(nameBuf, sizeof(nameBuf), "%s", net.getPlayerName().empty() ? "Player" : net.getPlayerName().c_str());

    char roomCodeBuf[16] = "";
    char hostBuf[128];
    memset(hostBuf, 0, sizeof(hostBuf));
    snprintf(hostBuf, sizeof(hostBuf), "%s", net.serverHost.c_str());

    char portBuf[16];
    memset(portBuf, 0, sizeof(portBuf));
    snprintf(portBuf, sizeof(portBuf), "%d", net.serverPort);

    int activeTab = 0; // 0: Create, 1: Join, 2: Server Settings
    int selectedColorIdx = 0; // 0: White, 1: Random, 2: Black
    int selectedTimeIdx = 3;  // Default 10 min Rapid (index 3 in TIME_OPTIONS)

    bool waitingForOpponent = false;
    bool focusName = false, focusRoomCode = false, focusHost = false, focusPort = false;

    float copyFeedbackTimer = 0.0f;
    std::string statusMsg = "";
    Color statusColor = RAYWHITE;

    float hoverTab0 = 0, hoverTab1 = 0, hoverTab2 = 0;
    float hoverColorW = 0, hoverColorR = 0, hoverColorB = 0;
    float hoverTime[TIME_OPTION_COUNT] = { 0 };
    float hoverCreate = 0, hoverJoin = 0, hoverCopy = 0, hoverPaste = 0, hoverCancel = 0, hoverBack = 0;
    float hoverPresetLocal = 0, hoverPresetLAN = 0, hoverPresetNgrok = 0, hoverTest = 0;

    int frame = 0;
    bool success = false;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (copyFeedbackTimer > 0.0f) copyFeedbackTimer -= dt;

        // Process network events
        NetEvent ev;
        while (net.pollEvent(ev)) {
            if (ev.type == NET_EVENT_ROOM_CREATED) {
                outRoomCode = ev.roomCode;
                outMyColor = ev.color;
                outTimeControl = ev.timeControl;
                waitingForOpponent = true;
                statusMsg = "Room created! Waiting for opponent to join...";
                statusColor = Color{ 129, 182, 76, 255 };
            }
            else if (ev.type == NET_EVENT_ROOM_JOINED || ev.type == NET_EVENT_OPPONENT_JOINED) {
                outRoomCode = net.getRoomCode();
                outMyColor = net.getMyColor();
                outTimeControl = net.getTimeControl();
                outOpponentName = net.getOpponentName();
                outPlayerName = nameBuf;
                success = true;
                break;
            }
            else if (ev.type == NET_EVENT_ERROR) {
                statusMsg = ev.message;
                statusColor = Color{ 230, 80, 80, 255 };
                waitingForOpponent = false;
            }
        }

        if (success) break;

        BeginVirtualScreen();
        ClearBackground(Color{ 22, 21, 18, 255 });
        HandleWindowControls();

        if (IsKeyPressed(KEY_ESCAPE)) {
            if (waitingForOpponent) {
                net.disconnect();
                waitingForOpponent = false;
                statusMsg = "Room cancelled.";
                statusColor = GRAY;
            } else {
                net.disconnect();
                EndVirtualScreen();
                return false;
            }
        }

        // Header Title
        int titleW = MeasureText("PLAY ONLINE WITH FRIENDS", 38);
        DrawText("PLAY ONLINE WITH FRIENDS", (VIRTUAL_WIDTH - titleW) / 2, 45, 38, RAYWHITE);

        const char* sub = "Worldwide Peer-to-Peer & Cloud Play • 5-Character Room Codes";
        int subW = MeasureText(sub, 18);
        DrawText(sub, (VIRTUAL_WIDTH - subW) / 2, 92, 18, Color{ 170, 168, 164, 255 });

        // Tab Navigation Bar
        const float tabW = 220;
        const float tabH = 44;
        const float tabStartX = (VIRTUAL_WIDTH - tabW * 3 - 20) / 2.0f;
        const float tabY = 130;

        Rectangle tab0 = { tabStartX,               tabY, tabW, tabH };
        Rectangle tab1 = { tabStartX + tabW + 10,   tabY, tabW, tabH };
        Rectangle tab2 = { tabStartX + (tabW + 10)*2, tabY, tabW, tabH };

        Color tabActiveBg = Color{ 129, 182, 76, 255 };
        Color tabInactiveBg = Color{ 48, 46, 43, 255 };

        if (!waitingForOpponent) {
            if (DrawButton(tab0, "Create Room", 18, hoverTab0, activeTab == 0 ? tabActiveBg : tabInactiveBg, Color{ 100, 160, 220, 255 }, RAYWHITE))
                activeTab = 0;
            if (DrawButton(tab1, "Join Room", 18, hoverTab1, activeTab == 1 ? tabActiveBg : tabInactiveBg, Color{ 100, 160, 220, 255 }, RAYWHITE))
                activeTab = 1;
            if (DrawButton(tab2, "Server Settings", 18, hoverTab2, activeTab == 2 ? tabActiveBg : tabInactiveBg, Color{ 100, 160, 220, 255 }, RAYWHITE))
                activeTab = 2;
        }

        // Main Content Card
        const float cardW = 760;
        const float cardH = 550;
        Rectangle cardRect = { (VIRTUAL_WIDTH - cardW) / 2.0f, 190, cardW, cardH };
        DrawRectangleRounded(cardRect, 0.04f, 8, Color{ 36, 34, 32, 255 });
        DrawRectangleRoundedLinesEx(cardRect, 0.04f, 8, 1.5f, Color{ 58, 56, 52, 255 });

        // --- TAB 0: CREATE ROOM ---
        if (activeTab == 0) {
            if (!waitingForOpponent) {
                // Player Name
                DrawText("YOUR NAME / NICKNAME:", (int)cardRect.x + 50, (int)cardRect.y + 35, 15, Color{ 180, 178, 172, 255 });
                Rectangle nameBox = { cardRect.x + 50, cardRect.y + 60, cardW - 100, 44 };
                DrawInputBox(nameBox, nameBuf, sizeof(nameBuf), focusName, "Enter your display name...");

                // Choose Color
                DrawText("PLAY AS:", (int)cardRect.x + 50, (int)cardRect.y + 125, 15, Color{ 180, 178, 172, 255 });
                float colBtnW = (cardW - 120) / 3.0f;
                Rectangle colBtnWht = { cardRect.x + 50,                    cardRect.y + 150, colBtnW, 46 };
                Rectangle colBtnRnd = { cardRect.x + 50 + colBtnW + 10,     cardRect.y + 150, colBtnW, 46 };
                Rectangle colBtnBlk = { cardRect.x + 50 + (colBtnW + 10)*2, cardRect.y + 150, colBtnW, 46 };

                if (DrawButton(colBtnWht, "White [W]", 18, hoverColorW,
                    selectedColorIdx == 0 ? Color{ 220, 220, 220, 255 } : Color{ 48, 46, 43, 255 },
                    Color{ 240, 240, 240, 255 }, selectedColorIdx == 0 ? BLACK : RAYWHITE))
                    selectedColorIdx = 0;

                if (DrawButton(colBtnRnd, "Random [?]", 18, hoverColorR,
                    selectedColorIdx == 1 ? Color{ 100, 160, 220, 255 } : Color{ 48, 46, 43, 255 },
                    Color{ 120, 180, 240, 255 }, RAYWHITE))
                    selectedColorIdx = 1;

                if (DrawButton(colBtnBlk, "Black [B]", 18, hoverColorB,
                    selectedColorIdx == 2 ? Color{ 20, 20, 20, 255 } : Color{ 48, 46, 43, 255 },
                    Color{ 35, 35, 35, 255 }, RAYWHITE))
                    selectedColorIdx = 2;

                // Time Control Selection (2 rows of 3)
                DrawText("TIME CONTROL:", (int)cardRect.x + 50, (int)cardRect.y + 220, 15, Color{ 180, 178, 172, 255 });
                float tcW = (cardW - 120) / 3.0f;
                for (int i = 0; i < TIME_OPTION_COUNT; ++i) {
                    int row = i / 3;
                    int col = i % 3;
                    Rectangle tcBtn = { cardRect.x + 50 + col * (tcW + 10), cardRect.y + 245 + row * 52, tcW, 44 };

                    Color baseCol = (selectedTimeIdx == i) ? Color{ 129, 182, 76, 255 } : Color{ 48, 46, 43, 255 };
                    if (DrawButton(tcBtn, TIME_OPTIONS[i].label, 16, hoverTime[i], baseCol, Color{ 145, 202, 85, 255 }, RAYWHITE)) {
                        selectedTimeIdx = i;
                    }
                }

                // Action: Create Room
                Rectangle createBtn = { (cardRect.x + (cardW - 360) / 2.0f), cardRect.y + 380, 360, 58 };
                if (DrawButton(createBtn, "CREATE GAME ROOM", 20, hoverCreate, Color{ 129, 182, 76, 255 }, Color{ 148, 208, 88, 255 }, RAYWHITE)) {
                    if (strlen(nameBuf) == 0) snprintf(nameBuf, sizeof(nameBuf), "Host");

                    int port = atoi(portBuf);
                    if (port <= 0) port = 4000;

                    statusMsg = "Connecting to chess server...";
                    statusColor = Color{ 220, 200, 100, 255 };

                    bool connected = net.isConnected();
                    if (!connected) {
                        connected = net.connectToServer(hostBuf, port);
                    }

                    if (!connected) {
                        statusMsg = "Connection failed! Check server host & port in 'Server Settings'.";
                        statusColor = Color{ 230, 80, 80, 255 };
                    } else {
                        COLOR prefCol = (selectedColorIdx == 0) ? PWHITE : (selectedColorIdx == 2 ? PBLACK : (rand() % 2 == 0 ? PWHITE : PBLACK));
                        float tc = TIME_OPTIONS[selectedTimeIdx].seconds;
                        net.createRoom(tc, prefCol, nameBuf);
                        statusMsg = "Creating room...";
                        statusColor = Color{ 120, 180, 240, 255 };
                    }
                }
            } else {
                // WAITING FOR OPPONENT DISPLAY
                DrawText("ROOM READY! SHARE THIS CODE WITH YOUR FRIEND", (int)cardRect.x + 50, (int)cardRect.y + 40, 20, RAYWHITE);

                // Huge 5-letter Room Code Badge
                Rectangle codeBox = { (cardRect.x + (cardW - 380) / 2.0f), cardRect.y + 90, 380, 85 };
                DrawRectangleRounded(codeBox, 0.2f, 6, Color{ 24, 22, 20, 255 });
                DrawRectangleRoundedLinesEx(codeBox, 0.2f, 6, 2.5f, Color{ 212, 175, 55, 255 });

                const char* rCode = net.getRoomCode().c_str();
                int cw = MeasureText(rCode, 52);
                DrawText(rCode, (int)(codeBox.x + (codeBox.width - cw) / 2.0f), (int)(codeBox.y + 16), 52, Color{ 255, 215, 0, 255 });

                // Copy Code Button
                Rectangle copyBtn = { (cardRect.x + (cardW - 320) / 2.0f), cardRect.y + 195, 320, 48 };
                const char* copyLabel = (copyFeedbackTimer > 0.0f) ? "✓ Copied to Clipboard!" : "📋 Copy Room Code";
                if (DrawButton(copyBtn, copyLabel, 18, hoverCopy, Color{ 58, 62, 75, 255 }, Color{ 100, 160, 230, 255 }, RAYWHITE)) {
                    SetClipboardText(rCode);
                    copyFeedbackTimer = 2.5f;
                }

                // Friendly instructions
                const char* guide1 = "Your friend can be anywhere in the world on any computer.";
                const char* guide2 = "They open the game, click 'Play Online' -> 'Join Room', and paste this code.";
                DrawText(guide1, (int)(cardRect.x + (cardW - MeasureText(guide1, 16)) / 2.0f), (int)cardRect.y + 265, 16, Color{ 180, 178, 172, 255 });
                DrawText(guide2, (int)(cardRect.x + (cardW - MeasureText(guide2, 16)) / 2.0f), (int)cardRect.y + 292, 16, Color{ 220, 200, 100, 255 });

                // Animated Pulsing Spinner
                float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 4.0f);
                DrawCircle((int)(cardRect.x + cardW / 2.0f - 140), (int)cardRect.y + 360, 8.0f + 2.0f * pulse, Color{ 129, 182, 76, (unsigned char)(180 + 75 * pulse) });
                DrawText("Waiting for opponent to connect...", (int)(cardRect.x + cardW / 2.0f - 120), (int)cardRect.y + 350, 18, RAYWHITE);

                // Cancel Room Button
                Rectangle cancelBtn = { (cardRect.x + (cardW - 220) / 2.0f), cardRect.y + 440, 220, 46 };
                if (DrawButton(cancelBtn, "Cancel Room", 17, hoverCancel, Color{ 68, 38, 38, 255 }, Color{ 180, 50, 50, 255 }, RAYWHITE)) {
                    net.disconnect();
                    waitingForOpponent = false;
                    statusMsg = "Room cancelled.";
                    statusColor = GRAY;
                }
            }
        }
        // --- TAB 1: JOIN ROOM ---
        else if (activeTab == 1) {
            // Player Name
            DrawText("YOUR NAME / NICKNAME:", (int)cardRect.x + 50, (int)cardRect.y + 40, 15, Color{ 180, 178, 172, 255 });
            Rectangle nameBox = { cardRect.x + 50, cardRect.y + 65, cardW - 100, 44 };
            DrawInputBox(nameBox, nameBuf, sizeof(nameBuf), focusName, "Enter your display name...");

            // Room Code Input & Paste Button
            DrawText("ENTER 5-CHARACTER ROOM CODE:", (int)cardRect.x + 50, (int)cardRect.y + 140, 15, Color{ 180, 178, 172, 255 });

            Rectangle codeInputBox = { cardRect.x + 50, cardRect.y + 170, 420, 56 };
            DrawInputBox(codeInputBox, roomCodeBuf, 6, focusRoomCode, "e.g. K9X2A", true);

            Rectangle pasteBtn = { cardRect.x + 485, cardRect.y + 170, cardW - 100 - 435, 56 };
            if (DrawButton(pasteBtn, "📋 Paste Code", 18, hoverPaste, Color{ 48, 56, 75, 255 }, Color{ 100, 160, 230, 255 }, RAYWHITE)) {
                const char* clip = GetClipboardText();
                if (clip) {
                    int cIdx = 0;
                    for (int i = 0; clip[i] != '\0' && cIdx < 5; ++i) {
                        char c = clip[i];
                        if (isalnum((unsigned char)c)) {
                            roomCodeBuf[cIdx++] = (char)toupper(c);
                        }
                    }
                    roomCodeBuf[cIdx] = '\0';
                }
            }

            // Instructions
            const char* joinTip1 = "Ask your friend for their 5-character Room Code.";
            const char* joinTip2 = "Once entered, click JOIN MATCH to start playing instantly!";
            DrawText(joinTip1, (int)cardRect.x + 50, (int)cardRect.y + 250, 16, Color{ 180, 178, 172, 255 });
            DrawText(joinTip2, (int)cardRect.x + 50, (int)cardRect.y + 276, 16, Color{ 220, 200, 100, 255 });

            // Action: Join Match
            Rectangle joinBtn = { (cardRect.x + (cardW - 360) / 2.0f), cardRect.y + 360, 360, 58 };
            if (DrawButton(joinBtn, "JOIN MATCH", 20, hoverJoin, Color{ 100, 160, 230, 255 }, Color{ 120, 180, 255, 255 }, RAYWHITE)) {
                if (strlen(nameBuf) == 0) snprintf(nameBuf, sizeof(nameBuf), "Guest");

                if (strlen(roomCodeBuf) < 3) {
                    statusMsg = "Please enter a valid 5-letter Room Code!";
                    statusColor = Color{ 230, 80, 80, 255 };
                } else {
                    int port = atoi(portBuf);
                    if (port <= 0) port = 4000;

                    statusMsg = "Connecting to server...";
                    statusColor = Color{ 220, 200, 100, 255 };

                    bool connected = net.isConnected();
                    if (!connected) {
                        connected = net.connectToServer(hostBuf, port);
                    }

                    if (!connected) {
                        statusMsg = "Connection failed! Check server host & port in 'Server Settings'.";
                        statusColor = Color{ 230, 80, 80, 255 };
                    } else {
                        net.joinRoom(roomCodeBuf, nameBuf);
                        statusMsg = TextFormat("Joining room %s...", roomCodeBuf);
                        statusColor = Color{ 120, 180, 240, 255 };
                    }
                }
            }
        }
        // --- TAB 2: SERVER SETTINGS (WORLDWIDE CLOUD HOSTING) ---
        else if (activeTab == 2) {
            DrawText("WORLDWIDE / CLOUD SERVER CONFIGURATION", (int)cardRect.x + 50, (int)cardRect.y + 30, 18, RAYWHITE);
            const char* servInfo = "Connect to any cloud server (Render, Railway, Fly.io, or Ngrok TCP tunnel) anywhere on Earth.";
            DrawText(servInfo, (int)cardRect.x + 50, (int)cardRect.y + 55, 14, Color{ 180, 178, 172, 255 });

            // Host & Port inputs
            DrawText("SERVER HOST / IP / DOMAIN:", (int)cardRect.x + 50, (int)cardRect.y + 90, 15, Color{ 180, 178, 172, 255 });
            Rectangle hostBox = { cardRect.x + 50, cardRect.y + 115, 460, 44 };
            DrawInputBox(hostBox, hostBuf, sizeof(hostBuf), focusHost, "127.0.0.1 or cloud domain");

            DrawText("PORT:", (int)cardRect.x + 525, (int)cardRect.y + 90, 15, Color{ 180, 178, 172, 255 });
            Rectangle portBox = { cardRect.x + 525, cardRect.y + 115, cardW - 100 - 475, 44 };
            DrawInputBox(portBox, portBuf, sizeof(portBuf), focusPort, "4000");

            // Quick Presets
            DrawText("QUICK CONNECTION PRESETS:", (int)cardRect.x + 50, (int)cardRect.y + 185, 15, Color{ 180, 178, 172, 255 });
            float preW = (cardW - 120) / 3.0f;
            Rectangle pre1 = { cardRect.x + 50,                cardRect.y + 210, preW, 44 };
            Rectangle pre2 = { cardRect.x + 50 + preW + 10,     cardRect.y + 210, preW, 44 };
            Rectangle pre3 = { cardRect.x + 50 + (preW + 10)*2, cardRect.y + 210, preW, 44 };

            if (DrawButton(pre1, "Localhost (Same PC)", 15, hoverPresetLocal, Color{ 48, 46, 43, 255 }, Color{ 70, 80, 100, 255 }, RAYWHITE)) {
                snprintf(hostBuf, sizeof(hostBuf), "127.0.0.1");
                snprintf(portBuf, sizeof(portBuf), "4000");
                statusMsg = "Set to Localhost (127.0.0.1:4000)";
                statusColor = RAYWHITE;
            }
            if (DrawButton(pre2, "LAN / Home Wi-Fi", 15, hoverPresetLAN, Color{ 48, 46, 43, 255 }, Color{ 70, 80, 100, 255 }, RAYWHITE)) {
                statusMsg = "Tip: Find your local IP (e.g. 192.168.1.X) and enter it as Host.";
                statusColor = Color{ 220, 200, 100, 255 };
            }
            if (DrawButton(pre3, "Free Cloud / Ngrok", 15, hoverPresetNgrok, Color{ 48, 46, 43, 255 }, Color{ 70, 80, 100, 255 }, RAYWHITE)) {
                statusMsg = "Run 'ngrok tcp 4000' and paste the assigned host & port here!";
                statusColor = Color{ 120, 180, 240, 255 };
            }

            // Test Connection Button
            Rectangle testBtn = { (cardRect.x + (cardW - 320) / 2.0f), cardRect.y + 285, 320, 48 };
            if (DrawButton(testBtn, "⚡ Test Server Connection", 17, hoverTest, Color{ 58, 62, 75, 255 }, Color{ 100, 160, 230, 255 }, RAYWHITE)) {
                int port = atoi(portBuf);
                if (port <= 0) port = 4000;
                bool ok = net.connectToServer(hostBuf, port);
                if (ok) {
                    statusMsg = TextFormat("SUCCESS: Connected to server at %s:%d! Ping: %dms", hostBuf, port, net.getPingMs());
                    statusColor = Color{ 129, 182, 76, 255 };
                } else {
                    statusMsg = TextFormat("FAILED: Cannot connect to %s:%d (%s)", hostBuf, port, net.getLastError().c_str());
                    statusColor = Color{ 230, 80, 80, 255 };
                }
            }

            // Free Cloud Deployment Tip Box
            Rectangle tipBox = { cardRect.x + 50, cardRect.y + 360, cardW - 100, 140 };
            DrawRectangleRounded(tipBox, 0.08f, 6, Color{ 28, 26, 24, 255 });
            DrawRectangleRoundedLinesEx(tipBox, 0.08f, 6, 1.0f, Color{ 52, 50, 46, 255 });
            DrawText("HOW TO HOST WORLDWIDE FOR FREE (24/7):", (int)tipBox.x + 18, (int)tipBox.y + 14, 14, Color{ 220, 200, 100, 255 });
            DrawText("1. The project includes a full zero-dependency server in the /server directory.", (int)tipBox.x + 18, (int)tipBox.y + 38, 13, RAYWHITE);
            DrawText("2. Deploy with 1 click to Render, Railway, or Fly.io (see server/README.md).", (int)tipBox.x + 18, (int)tipBox.y + 60, 13, RAYWHITE);
            DrawText("3. Or run 'node server/server.js' and 'ngrok tcp 4000' for an instant worldwide link.", (int)tipBox.x + 18, (int)tipBox.y + 82, 13, RAYWHITE);
            DrawText("4. Share your game build with friends anywhere in the world and play together!", (int)tipBox.x + 18, (int)tipBox.y + 104, 13, Color{ 129, 182, 76, 255 });
        }

        // Status Message Banner (bottom of card)
        if (!statusMsg.empty()) {
            int msgW = MeasureText(statusMsg.c_str(), 16);
            DrawText(statusMsg.c_str(), (int)(cardRect.x + (cardW - msgW) / 2.0f), (int)(cardRect.y + cardH - 35), 16, statusColor);
        }

        // Bottom: Back to Main Menu Button
        Rectangle backBtn = { (VIRTUAL_WIDTH - 240) / 2.0f, 765, 240, 48 };
        if (DrawButton(backBtn, "< Back to Main Menu", 17, hoverBack, Color{ 50, 48, 44, 255 }, Color{ 75, 72, 68, 255 }, RAYWHITE)) {
            net.disconnect();
            EndVirtualScreen();
            return false;
        }

        DrawFadeOverlay(frame++, 15);
        EndVirtualScreen();
    }

    return success;
}
