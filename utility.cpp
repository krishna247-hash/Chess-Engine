#include"utility.h"
#include<cstdio>
#include<cmath>

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

    const float btnW = 340;
    const float btnH = 64;
    Rectangle newGameBtn = { (VIRTUAL_WIDTH - btnW) / 2.0f, 280, btnW, btnH };
    Rectangle vsBotBtn = { (VIRTUAL_WIDTH - btnW) / 2.0f, 360, btnW, btnH };
    Rectangle loadGameBtn = { (VIRTUAL_WIDTH - btnW) / 2.0f, 440, btnW, btnH };
    Rectangle exitGameBtn = { (VIRTUAL_WIDTH - btnW) / 2.0f, 520, btnW, btnH };
    float hoverNew = 0, hoverBot = 0, hoverLoad = 0, hoverExit = 0;
    int frame = 0;

    while (!WindowShouldClose()) {
        BeginVirtualScreen();
        ClearBackground(DARKGRAY);
        HandleWindowControls();

        if (IsKeyPressed(KEY_ESCAPE)) {
            choice = EXIT_GAME;
            EndVirtualScreen();
            break;
        }

        int titleWidth = MeasureText("CHESS", 52);
        DrawText("CHESS", (VIRTUAL_WIDTH - titleWidth) / 2, 130, 52, RAYWHITE);

        const char* sub = "Chess.com Edition";
        int subW = MeasureText(sub, 20);
        DrawText(sub, (VIRTUAL_WIDTH - subW) / 2, 195, 20, LIGHTGRAY);

        if (DrawButton(newGameBtn, "Pass & Play (2 Players)", 20, hoverNew, GRAY, Color{ 120,180,120,255 }))
            choice = NEW_GAME;
        if (DrawButton(vsBotBtn, "Play vs Bot", 20, hoverBot, GRAY, Color{ 120,170,220,255 }))
            choice = NEW_GAME_BOT;
        if (DrawButton(loadGameBtn, "Load Saved Game", 20, hoverLoad, GRAY, Color{ 210,180,110,255 }))
            choice = LOAD_GAME;
        if (DrawButton(exitGameBtn, "Exit Game", 20, hoverExit, GRAY, Color{ 210,90,90,255 }, RAYWHITE))
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
