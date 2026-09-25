#include <iostream>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#include "Board.h"
#include "Pawn.h"
#include "Rook.h"
#include "Bishop.h"
#include "Knight.h"
#include "Queen.h"
#include "King.h"
#include "UCIEngine.h"

#define SCREENWIDTH 1240
#define SCREENHEIGHT 900
#define BOXSIZE 100
#define GRIDSIZE 8

#define BOARD_X 68
#define BOARD_Y 50

#define EVAL_X 22
#define EVAL_Y 50
#define EVAL_WIDTH 30
#define EVAL_HEIGHT 800

#define SIDEBAR_X 888
#define SIDEBAR_Y 50
#define SIDEBAR_WIDTH 330
#define SIDEBAR_HEIGHT 800

using namespace std;

// Gets piece texture from PromotionTextures for drawing in captured tray
static Texture2D getPieceTexture(char sym, const PromotionTextures& pt) {
    switch (sym) {
    case 'Q': return pt.whiteQueen;
    case 'R': return pt.whiteRook;
    case 'B': return pt.whiteBishop;
    case 'N': return pt.whiteKnight;
    case 'P': return pt.whitePawn;
    case 'q': return pt.blackQueen;
    case 'r': return pt.blackRook;
    case 'b': return pt.blackBishop;
    case 'n': return pt.blackKnight;
    case 'p': return pt.blackPawn;
    default: return pt.whitePawn;
    }
}

// Live Evaluation Bar (Stockfish advantage meter on the left)
static void drawEvaluationBar(float x, float y, float w, float h, float evalPawns, const std::string& evalText, bool flipped) {
    float centipawns = evalPawns * 100.0f;
    float whiteProb = 1.0f / (1.0f + expf(-0.0035f * centipawns));
    whiteProb = std::clamp(whiteProb, 0.04f, 0.96f);

    static float smoothRatio = 0.5f;
    smoothRatio += (whiteProb - smoothRatio) * 0.10f;

    // Outer frame / shadow
    DrawRectangleRounded(Rectangle{ x - 2, y - 2, w + 4, h + 4 }, 0.2f, 6, Color{ 30, 29, 27, 255 });

    // When not flipped: Black is top (1 - ratio), White is bottom (ratio)
    // When flipped: White is top (ratio), Black is bottom (1 - ratio)
    float whiteFrac = smoothRatio;
    float topHeight = (flipped ? whiteFrac : (1.0f - whiteFrac)) * h;
    float bottomHeight = h - topHeight;

    Color topColor = flipped ? Color{ 240, 240, 240, 255 } : Color{ 38, 36, 34, 255 };
    Color bottomColor = flipped ? Color{ 38, 36, 34, 255 } : Color{ 240, 240, 240, 255 };

    DrawRectangleRounded(Rectangle{ x, y, w, topHeight }, 0.15f, 4, topColor);
    DrawRectangleRounded(Rectangle{ x, y + topHeight, w, bottomHeight }, 0.15f, 4, bottomColor);

    // Subtle middle indicator line
    DrawLine((int)x, (int)(y + h / 2), (int)(x + w), (int)(y + h / 2), Color{ 120, 120, 120, 140 });

    // Score text inside the bar
    const char* txt = evalText.c_str();
    int tw = MeasureText(txt, 11);
    int textX = (int)(x + (w - tw) / 2.0f);

    bool whiteWinning = (evalPawns >= 0.0f);
    int textY;
    Color textColor;

    if (!flipped) {
        if (whiteWinning) {
            textY = (int)(y + h - 18);
            textColor = Color{ 40, 40, 40, 255 };
        } else {
            textY = (int)(y + 6);
            textColor = Color{ 220, 220, 220, 255 };
        }
    } else {
        if (whiteWinning) {
            textY = (int)(y + 6);
            textColor = Color{ 40, 40, 40, 255 };
        } else {
            textY = (int)(y + h - 18);
            textColor = Color{ 220, 220, 220, 255 };
        }
    }
    DrawText(txt, textX, textY, 11, textColor);
}

// Player digital clock in the sidebar cards
static void drawPlayerClock(Rectangle box, float seconds, bool active, bool untimed) {
    Color boxColor = active ? Color{ 58, 56, 52, 255 } : Color{ 30, 29, 27, 255 };
    DrawRectangleRounded(box, 0.25f, 6, boxColor);

    if (active) {
        DrawRectangleRoundedLinesEx(box, 0.25f, 6, 2.0f, Color{ 129, 182, 76, 255 });
    } else {
        DrawRectangleRoundedLinesEx(box, 0.25f, 6, 1.0f, Color{ 48, 46, 43, 255 });
    }

    const char* label;
    Color textColor = RAYWHITE;

    if (untimed) {
        label = "--:--";
    } else {
        label = TextFormat("%02d:%02d", (int)seconds / 60, (int)seconds % 60);
        if (seconds <= 30.0f) {
            float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 6.0f);
            textColor = ColorLerp(RAYWHITE, RED, 0.5f + 0.5f * pulse);
        }
    }

    int textWidth = MeasureText(label, 20);
    DrawText(label, (int)(box.x + (box.width - textWidth) / 2.0f), (int)(box.y + (box.height - 20) / 2.0f), 20, textColor);
}

// Finds the last move played by a specific color
static int findLastMoveIndex(const std::vector<MoveRecord>& history, COLOR color) {
    for (int i = (int)history.size() - 1; i >= 0; --i) {
        if (history[i].turn == color) return i;
    }
    return -1;
}

// Player Card (Avatar, Name, Rating/Title, Clock, Captured Tray, Material Advantage, Last Move Badge)
static bool drawPlayerCard(Rectangle rect, const char* name, const char* subtitle,
    bool isWhite, bool isTurn, float timeSeconds, bool untimed,
    int matAdvantage, const std::vector<char>& capturedPieces, const PromotionTextures& promoTex,
    const char* lastMoveBadge = nullptr, bool isOpponent = false, bool isReviewingThis = false) {

    DrawRectangleRounded(rect, 0.05f, 6, Color{ 38, 36, 34, 255 });
    DrawRectangleRoundedLinesEx(rect, 0.05f, 6, 1.5f, Color{ 54, 52, 49, 255 });

    // Avatar Circle Badge
    Vector2 avatarCenter{ rect.x + 28, rect.y + 28 };
    DrawCircleV(avatarCenter, 18, isWhite ? Color{ 235, 236, 208, 255 } : Color{ 55, 53, 50, 255 });
    DrawCircleLines((int)avatarCenter.x, (int)avatarCenter.y, 18, Color{ 120, 120, 120, 180 });
    const char* badgeLetter = isWhite ? "W" : "B";
    int bw = MeasureText(badgeLetter, 16);
    DrawText(badgeLetter, (int)(avatarCenter.x - bw / 2.0f), (int)(avatarCenter.y - 8), 16, isWhite ? Color{ 40, 40, 40, 255 } : WHITE);

    // Name & Subtitle
    DrawText(name, (int)rect.x + 56, (int)rect.y + 14, 16, RAYWHITE);
    DrawText(subtitle, (int)rect.x + 56, (int)rect.y + 34, 13, GOLD);

    // Clock
    Rectangle clockBox{ rect.x + rect.width - 100, rect.y + 12, 88, 34 };
    drawPlayerClock(clockBox, timeSeconds, isTurn, untimed);

    // Last Move Badge (Chess.com prominent move pill)
    bool clickedBadge = false;
    if (lastMoveBadge && strlen(lastMoveBadge) > 0) {
        int mw = MeasureText(lastMoveBadge, 11);
        float pillW = std::max(68.0f, (float)mw + 14.0f);
        Rectangle movePill{ rect.x + rect.width - 100 - pillW - 8, rect.y + 16, pillW, 24 };

        Vector2 mouse = GetVirtualMousePosition();
        bool hovered = CheckCollisionPointRec(mouse, movePill);
        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            clickedBadge = true;
        }

        Color pillBg = isOpponent ? Color{ 70, 48, 25, 230 } : Color{ 36, 56, 32, 230 };
        Color pillBorder = isReviewingThis ? GOLD : (hovered ? Color{ 240, 220, 120, 255 } : (isOpponent ? Color{ 210, 150, 45, 230 } : Color{ 90, 155, 70, 230 }));
        Color pillText = isOpponent ? Color{ 255, 225, 140, 255 } : Color{ 210, 245, 190, 255 };

        DrawRectangleRounded(movePill, 0.35f, 4, pillBg);
        DrawRectangleRoundedLinesEx(movePill, 0.35f, 4, isReviewingThis ? 2.0f : (hovered ? 1.5f : 1.0f), pillBorder);
        DrawText(lastMoveBadge, (int)(movePill.x + (pillW - mw) / 2.0f), (int)movePill.y + 6, 11, pillText);
    }

    // Captured pieces tray & material advantage
    float trayX = rect.x + 14;
    float trayY = rect.y + 62;

    int maxDraw = (int)capturedPieces.size();
    if (maxDraw > 14) maxDraw = 14;
    for (int i = 0; i < maxDraw; ++i) {
        Texture2D tex = getPieceTexture(capturedPieces[i], promoTex);
        Rectangle src{ 0, 0, 128, 128 };
        Rectangle dst{ trayX + i * 13.0f, trayY, 20, 20 };
        DrawTexturePro(tex, src, dst, Vector2{ 0, 0 }, 0.0f, WHITE);
    }

    if (matAdvantage > 0) {
        float advX = trayX + maxDraw * 13.0f + 6;
        DrawText(TextFormat("+%d", matAdvantage), (int)advX, (int)trayY + 2, 14, Color{ 175, 175, 175, 255 });
    }

    return clickedBadge;
}

// Move History List (Chess.com layout with dedicated Last Move Banner, 2-column SAN, interactive review, and auto-scroll)
static int drawMoveHistoryTable(Rectangle rect, const std::vector<MoveRecord>& history, float& scrollY,
    int reviewPly, bool vsBot, COLOR botColor) {

    int clickedPly = -999;
    Vector2 mouse = GetVirtualMousePosition();
    DrawRectangleRounded(rect, 0.04f, 6, Color{ 38, 36, 34, 255 });
    DrawRectangleRoundedLinesEx(rect, 0.04f, 6, 1.5f, Color{ 54, 52, 49, 255 });

    // 1. Top Header bar
    DrawRectangleRounded(Rectangle{ rect.x, rect.y, rect.width, 36 }, 0.04f, 6, Color{ 48, 46, 43, 255 });
    DrawText("Move History", (int)rect.x + 14, (int)rect.y + 10, 16, RAYWHITE);
    const char* countStr = TextFormat("%d moves", (int)history.size());
    int cw = MeasureText(countStr, 13);
    DrawText(countStr, (int)(rect.x + rect.width - cw - 14), (int)rect.y + 12, 13, GRAY);

    // 2. Chess.com Last Move Banner
    float bannerY = rect.y + 36;
    float bannerH = 38.0f;
    DrawRectangle((int)rect.x + 1, (int)bannerY, (int)rect.width - 2, (int)bannerH, Color{ 30, 29, 27, 255 });
    DrawLine((int)rect.x + 1, (int)(bannerY + bannerH), (int)(rect.x + rect.width - 1), (int)(bannerY + bannerH), Color{ 55, 53, 50, 255 });

    if (reviewPly != -1 && reviewPly <= (int)history.size()) {
        Rectangle pill{ rect.x + 8, bannerY + 5, rect.width - 16, 28 };
        bool pillHovered = CheckCollisionPointRec(mouse, pill);
        if (pillHovered) {
            DrawRectangleRounded(pill, 0.28f, 6, Color{ 48, 75, 110, 245 });
            DrawRectangleRoundedLinesEx(pill, 0.28f, 6, 1.5f, Color{ 120, 185, 255, 255 });
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                clickedPly = -1; // Snap back to live
            }
        } else {
            DrawRectangleRounded(pill, 0.28f, 6, Color{ 35, 55, 80, 235 });
            DrawRectangleRoundedLinesEx(pill, 0.28f, 6, 1.5f, Color{ 85, 155, 235, 255 });
        }

        if (reviewPly == 0) {
            DrawText("◄ Start of Game (Initial Position) • [>|] Live", (int)pill.x + 12, (int)pill.y + 7, 13, RAYWHITE);
        } else {
            const auto& rec = history[reviewPly - 1];
            bool isOpp = vsBot && (rec.turn == botColor);
            int mNum = (reviewPly + 1) / 2;
            const char* numPfx = (rec.turn == PWHITE) ? TextFormat("%d.", mNum) : TextFormat("%d...", mNum);

            char f1 = 'a' + rec.move.from.col;
            char r1 = '8' - rec.move.from.row;
            char f2 = 'a' + rec.move.to.col;
            char r2 = '8' - rec.move.to.row;

            const char* txt = TextFormat("◄ %s: %s %s (%c%c->%c%c) • >| Live",
                isOpp ? "Opponent" : (vsBot ? "You" : (rec.turn == PWHITE ? "White" : "Black")),
                numPfx, rec.san.c_str(), f1, r1, f2, r2);
            DrawText(txt, (int)pill.x + 8, (int)pill.y + 7, 12, isOpp ? Color{ 255, 225, 140, 255 } : RAYWHITE);
        }
    } else if (!history.empty()) {
        const auto& lastRec = history.back();
        int moveNum = (int)((history.size() + 1) / 2);
        bool wasWhite = (lastRec.turn == PWHITE);
        bool wasOpponent = vsBot && (lastRec.turn == botColor);

        Rectangle pill{ rect.x + 8, bannerY + 5, rect.width - 16, 28 };
        bool pillHovered = CheckCollisionPointRec(mouse, pill);
        if (pillHovered) {
            if (wasOpponent) {
                DrawRectangleRounded(pill, 0.28f, 6, Color{ 88, 62, 32, 245 });
                DrawRectangleRoundedLinesEx(pill, 0.28f, 6, 1.5f, Color{ 255, 195, 75, 255 });
            } else {
                DrawRectangleRounded(pill, 0.28f, 6, Color{ 58, 98, 48, 245 });
                DrawRectangleRoundedLinesEx(pill, 0.28f, 6, 1.5f, Color{ 135, 210, 95, 255 });
            }
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                clickedPly = (int)history.size(); // Click to inspect last move
            }
        } else {
            if (wasOpponent) {
                DrawRectangleRounded(pill, 0.28f, 6, Color{ 68, 48, 25, 230 });
                DrawRectangleRoundedLinesEx(pill, 0.28f, 6, 1.5f, Color{ 220, 160, 50, 255 });
            } else {
                DrawRectangleRounded(pill, 0.28f, 6, Color{ 46, 78, 38, 230 });
                DrawRectangleRoundedLinesEx(pill, 0.28f, 6, 1.5f, Color{ 105, 172, 78, 255 });
            }
        }

        // Turn indicator dot
        DrawCircle((int)pill.x + 14, (int)pill.y + 14, 5.5f, wasWhite ? WHITE : Color{ 25, 25, 25, 255 });
        DrawCircleLines((int)pill.x + 14, (int)pill.y + 14, 5.5f, Color{ 160, 160, 160, 255 });

        const char* prefix = wasWhite ? TextFormat("%d.", moveNum) : TextFormat("%d...", moveNum);
        const char* labelPfx = wasOpponent ? "Opponent Played:" : (vsBot ? "You Played:" : "Last Move:");
        const char* moveText = TextFormat("%s %s %s", labelPfx, prefix, lastRec.san.c_str());
        DrawText(moveText, (int)pill.x + 25, (int)pill.y + 6, 13, wasOpponent ? Color{ 255, 235, 170, 255 } : RAYWHITE);

        if (lastRec.move.from.row >= 0 && lastRec.move.to.row >= 0) {
            char f1 = 'a' + lastRec.move.from.col;
            char r1 = '8' - lastRec.move.from.row;
            char f2 = 'a' + lastRec.move.to.col;
            char r2 = '8' - lastRec.move.to.row;
            const char* coordStr = TextFormat("(%c%c -> %c%c)", f1, r1, f2, r2);
            int tw = MeasureText(coordStr, 12);
            DrawText(coordStr, (int)(pill.x + pill.width - tw - 8), (int)pill.y + 8, 12,
                wasOpponent ? Color{ 255, 215, 120, 255 } : Color{ 185, 225, 165, 255 });
        }
    } else {
        Rectangle pill{ rect.x + 8, bannerY + 5, rect.width - 16, 28 };
        DrawRectangleRounded(pill, 0.28f, 6, Color{ 42, 40, 37, 200 });
        DrawRectangleRoundedLinesEx(pill, 0.28f, 6, 1.0f, Color{ 58, 56, 52, 255 });
        DrawText("Game Start • White to Move", (int)pill.x + 14, (int)pill.y + 7, 13, GRAY);
    }

    // 3. Column headers
    float colHeaderY = bannerY + bannerH;
    DrawRectangle((int)rect.x + 1, (int)colHeaderY, (int)rect.width - 2, 24, Color{ 34, 33, 30, 255 });
    DrawText("#", (int)rect.x + 16, (int)colHeaderY + 5, 12, GRAY);
    DrawText("White", (int)rect.x + 65, (int)colHeaderY + 5, 12, GRAY);
    DrawText("Black", (int)rect.x + 185, (int)colHeaderY + 5, 12, GRAY);

    // 4. Scrollable area
    float contentY = colHeaderY + 24;
    float contentH = rect.height - (contentY - rect.y) - 2;
    int totalMoves = (int)history.size();
    int totalRows = (totalMoves + 1) / 2;
    float rowHeight = 28.0f;
    float totalContentH = totalRows * rowHeight;

    // Mouse wheel scrolling
    if (CheckCollisionPointRec(mouse, Rectangle{ rect.x, contentY, rect.width, contentH })) {
        float wheel = GetMouseWheelMove();
        scrollY -= wheel * 30.0f;
    }

    float maxScroll = totalContentH > contentH ? (totalContentH - contentH) : 0.0f;
    if (scrollY > maxScroll) scrollY = maxScroll;
    if (scrollY < 0) scrollY = 0;

    BeginScissorMode((int)rect.x, (int)contentY, (int)rect.width, (int)contentH);

    for (int r = 0; r < totalRows; ++r) {
        float curY = contentY - scrollY + r * rowHeight;
        if (curY + rowHeight < contentY || curY > contentY + contentH) continue;

        if (r % 2 == 1) {
            DrawRectangle((int)rect.x + 2, (int)curY, (int)rect.width - 4, (int)rowHeight, Color{ 44, 42, 39, 180 });
        }

        // Move number
        DrawText(TextFormat("%d.", r + 1), (int)rect.x + 14, (int)curY + 7, 13, Color{ 140, 140, 140, 255 });

        // White move
        int wIdx = 2 * r;
        if (wIdx < totalMoves) {
            int ply = wIdx + 1;
            bool isReviewActive = (reviewPly == ply);
            bool isLiveLatest = (reviewPly == -1 && wIdx == totalMoves - 1);
            Rectangle badge{ rect.x + 58, curY + 2, 95, rowHeight - 4 };

            bool hovered = CheckCollisionPointRec(mouse, badge);
            if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                clickedPly = ply;
            }

            if (isReviewActive) {
                DrawRectangleRounded(badge, 0.25f, 4, Color{ 40, 68, 105, 235 });
                DrawRectangleRoundedLinesEx(badge, 0.25f, 4, 1.5f, Color{ 100, 175, 255, 255 });
            } else if (isLiveLatest) {
                DrawRectangleRounded(badge, 0.25f, 4, Color{ 69, 117, 59, 230 });
                DrawRectangleRoundedLinesEx(badge, 0.25f, 4, 1.5f, Color{ 120, 195, 95, 255 });
            } else if (hovered) {
                DrawRectangleRounded(badge, 0.25f, 4, Color{ 55, 52, 48, 200 });
                DrawRectangleRoundedLinesEx(badge, 0.25f, 4, 1.0f, Color{ 100, 95, 90, 255 });
            }
            DrawText(history[wIdx].san.c_str(), (int)rect.x + 65, (int)curY + 7, 14,
                (isReviewActive || isLiveLatest) ? WHITE : RAYWHITE);
        }

        // Black move
        int bIdx = 2 * r + 1;
        if (bIdx < totalMoves) {
            int ply = bIdx + 1;
            bool isReviewActive = (reviewPly == ply);
            bool isLiveLatest = (reviewPly == -1 && bIdx == totalMoves - 1);
            Rectangle badge{ rect.x + 178, curY + 2, 95, rowHeight - 4 };

            bool hovered = CheckCollisionPointRec(mouse, badge);
            if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                clickedPly = ply;
            }

            if (isReviewActive) {
                DrawRectangleRounded(badge, 0.25f, 4, Color{ 40, 68, 105, 235 });
                DrawRectangleRoundedLinesEx(badge, 0.25f, 4, 1.5f, Color{ 100, 175, 255, 255 });
            } else if (isLiveLatest) {
                DrawRectangleRounded(badge, 0.25f, 4, Color{ 69, 117, 59, 230 });
                DrawRectangleRoundedLinesEx(badge, 0.25f, 4, 1.5f, Color{ 120, 195, 95, 255 });
            } else if (hovered) {
                DrawRectangleRounded(badge, 0.25f, 4, Color{ 55, 52, 48, 200 });
                DrawRectangleRoundedLinesEx(badge, 0.25f, 4, 1.0f, Color{ 100, 95, 90, 255 });
            }
            DrawText(history[bIdx].san.c_str(), (int)rect.x + 185, (int)curY + 7, 14,
                (isReviewActive || isLiveLatest) ? WHITE : RAYWHITE);
        }
    }

    EndScissorMode();
    return clickedPly;
}

// Draw Review Navigation Bar (|<, <, >, >| Live)
static void drawReviewNavBar(Rectangle rect, int& reviewPly, int totalMoves,
    float& hoverFirst, float& hoverPrev, float& hoverNext, float& hoverLast) {

    DrawRectangleRounded(rect, 0.04f, 6, Color{ 34, 32, 30, 255 });
    DrawRectangleRoundedLinesEx(rect, 0.04f, 6, 1.0f, Color{ 54, 52, 49, 255 });

    Rectangle btnFirst{ rect.x + 6,   rect.y + 5, 46, 30 };
    Rectangle btnPrev { rect.x + 56,  rect.y + 5, 46, 30 };
    Rectangle btnNext { rect.x + 106, rect.y + 5, 46, 30 };
    Rectangle btnLast { rect.x + 156, rect.y + 5, 54, 30 };
    Rectangle liveBadge{ rect.x + 216, rect.y + 5, 108, 30 };

    bool hasMoves = (totalMoves > 0);

    if (DrawButton(btnFirst, "|<", 14, hoverFirst, Color{ 48, 46, 43, 255 }, Color{ 70, 68, 65, 255 }, hasMoves ? RAYWHITE : GRAY)) {
        if (hasMoves) reviewPly = 0;
    }
    if (DrawButton(btnPrev, "<", 14, hoverPrev, Color{ 48, 46, 43, 255 }, Color{ 70, 68, 65, 255 }, hasMoves ? RAYWHITE : GRAY)) {
        if (hasMoves) {
            if (reviewPly == -1) reviewPly = totalMoves - 1;
            else if (reviewPly > 0) reviewPly--;
        }
    }
    if (DrawButton(btnNext, ">", 14, hoverNext, Color{ 48, 46, 43, 255 }, Color{ 70, 68, 65, 255 }, (reviewPly != -1) ? RAYWHITE : GRAY)) {
        if (reviewPly != -1) {
            reviewPly++;
            if (reviewPly >= totalMoves) reviewPly = -1;
        }
    }

    bool isReviewing = (reviewPly != -1);
    Color lastBtnBg = isReviewing ? Color{ 105, 165, 65, 255 } : Color{ 48, 46, 43, 255 };
    Color lastBtnHov = isReviewing ? Color{ 125, 190, 78, 255 } : Color{ 70, 68, 65, 255 };
    if (DrawButton(btnLast, ">|", 14, hoverLast, lastBtnBg, lastBtnHov, isReviewing ? WHITE : (hasMoves ? RAYWHITE : GRAY))) {
        reviewPly = -1; // Snap to live
    }

    // Status pill
    if (!isReviewing) {
        DrawRectangleRounded(liveBadge, 0.3f, 4, Color{ 40, 56, 32, 220 });
        DrawRectangleRoundedLinesEx(liveBadge, 0.3f, 4, 1.0f, Color{ 90, 155, 70, 255 });
        DrawCircle((int)liveBadge.x + 14, (int)liveBadge.y + 15, 4.0f, Color{ 120, 205, 80, 255 });
        DrawText("LIVE PLAY", (int)liveBadge.x + 24, (int)liveBadge.y + 8, 12, Color{ 180, 235, 150, 255 });
    } else {
        float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 6.0f);
        DrawRectangleRounded(liveBadge, 0.3f, 4, Color{ 35, 55, 80, 230 });
        DrawRectangleRoundedLinesEx(liveBadge, 0.3f, 4, 1.5f, ColorLerp(Color{ 85, 155, 235, 255 }, GOLD, pulse));
        DrawCircle((int)liveBadge.x + 12, (int)liveBadge.y + 15, 4.0f, GOLD);
        const char* plyStr = (reviewPly == 0) ? "START" : TextFormat("MOVE %d", reviewPly);
        DrawText(plyStr, (int)liveBadge.x + 22, (int)liveBadge.y + 8, 12, Color{ 220, 240, 255, 255 });

        Vector2 mouse = GetVirtualMousePosition();
        if (CheckCollisionPointRec(mouse, liveBadge) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            reviewPly = -1; // Click badge to return to live
        }
    }
}

// Snappy move slide animation matching Chess.com speed
static void animateMove(Board& board, Position from, Position to,
    Position rookFrom = { -1,-1 }, Position rookTo = { -1,-1 },
    std::function<void()> drawExtras = nullptr) {

    Piece* moving = board.getPiece(from);
    if (!moving) return;

    Piece* rook = (rookFrom.row >= 0) ? board.getPiece(rookFrom) : nullptr;
    Piece* captured = board.getPiece(to);
    bool isCapture = (captured != nullptr);

    const float duration = 0.14f;
    float elapsed = 0.0f;

    bool flip = board.flipped;
    auto dispCol = [flip](int col) { return flip ? 7 - col : col; };
    auto dispRow = [flip](int row) { return flip ? 7 - row : row; };

    while (elapsed < duration and !WindowShouldClose()) {
        elapsed += GetFrameTime();
        float t = elapsed / duration;
        if (t > 1.0f) t = 1.0f;
        float eased = 1.0f - powf(1.0f - t, 3.0f); // ease-out cubic

        BeginVirtualScreen();
        ClearBackground(Color{ 22, 21, 18, 255 });

        if (drawExtras) drawExtras();

        Position skip2 = isCapture ? to : (rook ? rookFrom : Position{ -1,-1 });
        board.drawBoard({ -1,-1 }, false, from, skip2, BOARD_X, BOARD_Y, BOXSIZE);

        if (rook) {
            float rx = BOARD_X + (dispCol(rookFrom.col) + (dispCol(rookTo.col) - dispCol(rookFrom.col)) * eased) * BOXSIZE;
            float ry = BOARD_Y + (dispRow(rookFrom.row) + (dispRow(rookTo.row) - dispRow(rookFrom.row)) * eased) * BOXSIZE;
            rook->drawAtPixel(rx, ry);
        }

        if (isCapture) {
            float cx = BOARD_X + dispCol(to.col) * BOXSIZE;
            float cy = BOARD_Y + dispRow(to.row) * BOXSIZE;
            captured->drawAtPixel(cx, cy, Fade(WHITE, 1.0f - eased));
        }

        float px = BOARD_X + (dispCol(from.col) + (dispCol(to.col) - dispCol(from.col)) * eased) * BOXSIZE;
        float py = BOARD_Y + (dispRow(from.row) + (dispRow(to.row) - dispRow(from.row)) * eased) * BOXSIZE;
        moving->drawAtPixel(px, py);

        EndVirtualScreen();
    }
}

static void getCastleRookSquares(Piece* moving, Position S, Position D, Position& rookFrom, Position& rookTo) {
    rookFrom = { -1,-1 };
    rookTo = { -1,-1 };
    if (moving and dynamic_cast<King*>(moving) and abs(D.col - S.col) == 2) {
        int r = S.row;
        if (D.col == 6) { rookFrom = { r,7 }; rookTo = { r,5 }; }
        else if (D.col == 2) { rookFrom = { r,0 }; rookTo = { r,3 }; }
    }
}

static void checkEndConditions(Board& board, COLOR turn, bool& gameOver, const char*& finalMessage) {
    if (board.isInCheck(turn)) {
        if (!board.hasLegalMove(turn)) {
            finalMessage = (turn == PWHITE) ? "CHECKMATE! Black Wins" : "CHECKMATE! White Wins";
            gameOver = true;
        }
    }
    else if (!board.hasLegalMove(turn)) {
        finalMessage = "STALEMATE! Game is a Draw";
        gameOver = true;
    }
}

// Executes human move: records undo, handles promotion, generates SAN, plays sounds, updates eval
static bool executeHumanMove(Board& board, Position from, Position to,
    COLOR& currentTurn, PromotionTextures& promoTex, GameSounds& sounds,
    vector<string>& undoStack, vector<string>& redoStack, vector<MoveRecord>& redoMoveHistory,
    vector<string>& gameSnapshots,
    bool& gameOver, const char*& finalMessage, float& scrollY,
    std::function<void()> drawExtras = nullptr) {

    undoStack.push_back(board.snapshot(currentTurn));
    redoStack.clear();
    redoMoveHistory.clear();

    bool wasCapture = (board.getPiece(to) != nullptr) || board.isEnPassantMove(from, to);

    char promoChar = '\0';
    Piece* moving = board.getPiece(from);
    int lastRow = (currentTurn == PWHITE) ? 0 : 7;
    if (moving && dynamic_cast<Pawn*>(moving) && to.row == lastRow) {
        promoChar = board.Promotion(currentTurn);
    }

    std::string san = board.generateSAN({ from, to }, promoChar);

    Position rookFrom, rookTo;
    getCastleRookSquares(board.getPiece(from), from, to, rookFrom, rookTo);
    animateMove(board, from, to, rookFrom, rookTo, drawExtras);

    board.movePiece(from, to);

    Piece* moved = board.getPiece(to);
    if (moved && dynamic_cast<Pawn*>(moved) && to.row == lastRow) {
        Texture2D promoTexture = (currentTurn == PWHITE) ? promoTex.whiteQueen : promoTex.blackQueen;
        switch (tolower(promoChar)) {
        case 'r':
            promoTexture = (currentTurn == PWHITE) ? promoTex.whiteRook : promoTex.blackRook;
            board.setPiece(to, new Rook(currentTurn, promoTexture));
            break;
        case 'b':
            promoTexture = (currentTurn == PWHITE) ? promoTex.whiteBishop : promoTex.blackBishop;
            board.setPiece(to, new Bishop(currentTurn, promoTexture));
            break;
        case 'n':
        case 'k':
            promoTexture = (currentTurn == PWHITE) ? promoTex.whiteKnight : promoTex.blackKnight;
            board.setPiece(to, new Knight(currentTurn, promoTexture));
            break;
        case 'q':
        default:
            promoTexture = (currentTurn == PWHITE) ? promoTex.whiteQueen : promoTex.blackQueen;
            board.setPiece(to, new Queen(currentTurn, promoTexture));
            break;
        }
    }

    board.recordMove({ from, to }, san, currentTurn, promoChar);
    currentTurn = (currentTurn == PWHITE) ? PBLACK : PWHITE;
    board.save(currentTurn);
    gameSnapshots.push_back(board.snapshot(currentTurn));

    // Auto-scroll move history to bottom
    scrollY = 99999.0f;

    checkEndConditions(board, currentTurn, gameOver, finalMessage);
    bool nowInCheck = !gameOver && board.isInCheck(currentTurn);
    bool isCastle = (moving && dynamic_cast<King*>(moving) && abs(to.col - from.col) == 2);
    bool isPromote = (promoChar != '\0');
    bool isCheckmate = gameOver && finalMessage && strstr(finalMessage, "CHECKMATE");
    playChessSound(sounds, true, wasCapture, isCastle, isPromote, nowInCheck, gameOver, isCheckmate);

    if (GetStockfishEngine().isAvailable()) {
        GetStockfishEngine().evaluatePosition(board.toFEN(currentTurn), 80);
    }

    return true;
}

static void playBotMove(Board& board, COLOR botColor, const BotProfile& profile,
    PromotionTextures& promoTex, GameSounds& sounds, COLOR& currentTurn,
    vector<string>& gameSnapshots,
    bool& gameOver, const char*& finalMessage, float& scrollY,
    std::function<void()> drawExtras = nullptr) {

    if (!board.hasLegalMove(botColor)) return;

    Move botMove{ {-1,-1}, {-1,-1} };
    char promoChar = '\0';
    bool gotMove = false;

    if (profile.engine == ENGINE_STOCKFISH && GetStockfishEngine().isAvailable()) {
        GetStockfishEngine().configure(profile.stockfishElo, profile.skillLevel, profile.stockfishElo > 0);
        std::string fen = board.toFEN(botColor);
        gotMove = GetStockfishEngine().getBestMove(fen, profile.movetimeMs, botMove, promoChar);
    }

    if (!gotMove) {
        botMove = profile.random
            ? board.findRandomMove(botColor)
            : board.findBestMove(botColor, profile.depth);
    }

    Piece* moving = board.getPiece(botMove.from);
    if (!moving || moving->getColor() != botColor ||
        !moving->isLegal(&board, botMove.from, botMove.to) ||
        board.isSelfCheck(botMove.from, botMove.to, botColor)) {
        auto legals = board.generateLegalMoves(botColor);
        if (legals.empty()) return;
        botMove = legals[0];
    }

    bool wasCapture = (board.getPiece(botMove.to) != nullptr) || board.isEnPassantMove(botMove.from, botMove.to);
    std::string san = board.generateSAN(botMove, promoChar);

    Position rookFrom, rookTo;
    getCastleRookSquares(board.getPiece(botMove.from), botMove.from, botMove.to, rookFrom, rookTo);
    animateMove(board, botMove.from, botMove.to, rookFrom, rookTo, drawExtras);

    board.movePiece(botMove.from, botMove.to);

    Piece* botMoved = board.getPiece(botMove.to);
    int lastRow = (botColor == PWHITE) ? 0 : 7;
    if (botMoved and dynamic_cast<Pawn*>(botMoved) and botMove.to.row == lastRow) {
        Texture2D promoTexture = (botColor == PWHITE) ? promoTex.whiteQueen : promoTex.blackQueen;
        switch (tolower(promoChar)) {
        case 'r':
            promoTexture = (botColor == PWHITE) ? promoTex.whiteRook : promoTex.blackRook;
            board.setPiece(botMove.to, new Rook(botColor, promoTexture));
            break;
        case 'b':
            promoTexture = (botColor == PWHITE) ? promoTex.whiteBishop : promoTex.blackBishop;
            board.setPiece(botMove.to, new Bishop(botColor, promoTexture));
            break;
        case 'n':
        case 'k':
            promoTexture = (botColor == PWHITE) ? promoTex.whiteKnight : promoTex.blackKnight;
            board.setPiece(botMove.to, new Knight(botColor, promoTexture));
            break;
        case 'q':
        default:
            promoTexture = (botColor == PWHITE) ? promoTex.whiteQueen : promoTex.blackQueen;
            board.setPiece(botMove.to, new Queen(botColor, promoTexture));
            break;
        }
    }

    board.recordMove(botMove, san, botColor, promoChar);
    currentTurn = (botColor == PWHITE) ? PBLACK : PWHITE;
    board.save(currentTurn);
    gameSnapshots.push_back(board.snapshot(currentTurn));

    // Auto-scroll move history to bottom
    scrollY = 99999.0f;

    checkEndConditions(board, currentTurn, gameOver, finalMessage);
    bool nowInCheck = !gameOver and board.isInCheck(currentTurn);
    bool isCastle = (moving && dynamic_cast<King*>(moving) && abs(botMove.to.col - botMove.from.col) == 2);
    bool isPromote = (promoChar != '\0');
    bool isCheckmate = gameOver && finalMessage && strstr(finalMessage, "CHECKMATE");
    playChessSound(sounds, false, wasCapture, isCastle, isPromote, nowInCheck, gameOver, isCheckmate);

    if (GetStockfishEngine().isAvailable()) {
        GetStockfishEngine().evaluatePosition(board.toFEN(currentTurn), 80);
    }
}

int main() {
    srand((unsigned int)time(nullptr));

    InitWindow(SCREENWIDTH, SCREENHEIGHT, "Chess");
    InitVirtualScreen();
    SetTargetFPS(60);

    InitAudioDevice();
    GameSounds sounds = loadGameSounds();

    GetStockfishEngine().init();

    PromotionTextures promoTex = loadPromotionTextures();

    while (!WindowShouldClose()) {
        MenuChoice choice = ShowStartMenu();
        if (choice == NONE || choice == EXIT_GAME || WindowShouldClose()) {
            break;
        }

        Board board;
        Position selected = { -1, -1 };
        COLOR currentTurn = PWHITE;
        bool vsBot = false;
        COLOR humanColor = PWHITE;
        COLOR botColor = PBLACK;
        BotProfile botProfile = BOT_PROFILES[2];

        bool untimedGame = false;
        float whiteTime = 600.0f;
        float blackTime = 600.0f;

        vector<string> undoStack, redoStack;
        vector<MoveRecord> redoMoveHistory;

        if (choice == NEW_GAME) {
            board.initillize();
            currentTurn = PWHITE;
            vsBot = false;

            float t = ChooseTimeControl();
            if (t < -500.0f || WindowShouldClose()) continue;
            untimedGame = (t < 0);
            whiteTime = untimedGame ? 0 : t;
            blackTime = untimedGame ? 0 : t;
        }
        else if (choice == NEW_GAME_BOT) {
            board.initillize();
            currentTurn = PWHITE;
            vsBot = true;

            int c = ChooseColor();
            if (c < 0 || WindowShouldClose()) continue;
            humanColor = static_cast<COLOR>(c);
            botColor = (humanColor == PWHITE) ? PBLACK : PWHITE;

            if (humanColor == PBLACK) {
                board.flipped = true;
            }

            int botIdx = ChooseBotDifficulty();
            if (botIdx < 0 || WindowShouldClose()) continue;
            botProfile = BOT_PROFILES[botIdx];

            float t = ChooseTimeControl();
            if (t < -500.0f || WindowShouldClose()) continue;
            untimedGame = (t < 0);
            whiteTime = untimedGame ? 0 : t;
            blackTime = untimedGame ? 0 : t;

            if (botProfile.engine == ENGINE_STOCKFISH) {
                GetStockfishEngine().newGame();
            }
        }
        else if (choice == LOAD_GAME) {
            board.load(currentTurn);
            vsBot = false;
        }

        // Initial position evaluation
        if (GetStockfishEngine().isAvailable()) {
            GetStockfishEngine().evaluatePosition(board.toFEN(currentTurn), 100);
        }

        float historyScrollY = 0.0f;
        bool isDragging = false;
        Position dragStart = { -1, -1 };
        Position rightClickStart = { -1, -1 };

        bool gameOver = false;
        const char* finalMessage = nullptr;
        bool showGameOverModal = false;
        bool returnToMenu = false;
        bool showExitConfirmModal = false;
        std::string notifyBannerText = "";
        float notifyBannerTimer = 0.0f;

        // Move review scrubbing state & snapshots
        std::vector<std::string> gameSnapshots;
        gameSnapshots.push_back(board.snapshot(currentTurn));
        int reviewPly = -1;
        int activeReviewLoadedPly = -999;
        Board reviewBoard;

        // Button hover animations
        float hoverUndo = 0, hoverRedo = 0, hoverFlip = 0, hoverTheme = 0, hoverNew = 0;
        float hoverMenu = 0, hoverDraw = 0, hoverResign = 0;
        float hoverModalNew = 0, hoverModalReview = 0, hoverModalMenu = 0;
        float hoverExitSave = 0, hoverExitNoSave = 0, hoverExitCancel = 0;
        float hoverNavFirst = 0, hoverNavPrev = 0, hoverNavNext = 0, hoverNavLast = 0;

        // Play Game Start sound
        if (IsSoundValid(sounds.gameStart)) PlaySound(sounds.gameStart);

        bool whiteWarned10s = false;
        bool blackWarned10s = false;
        int botThinkingDelay = 0;

    // Lambda for Chess.com style smart undo
    auto performUndo = [&]() {
        reviewPly = -1;
        if (undoStack.empty()) return;

        auto undoSingle = [&]() {
            if (undoStack.empty()) return;
            redoStack.push_back(board.snapshot(currentTurn));
            string prev = undoStack.back();
            undoStack.pop_back();
            board.restoreSnapshot(prev, currentTurn);
            if (!board.getMoveHistory().empty()) {
                redoMoveHistory.push_back(board.getMoveHistory().back());
                board.popLastMoveRecord();
            }
            if (gameSnapshots.size() > 1) gameSnapshots.pop_back();
        };

        undoSingle();
        // In vsBot mode, undo 2 plies if currently landing on bot's turn
        if (vsBot && currentTurn == botColor && !undoStack.empty()) {
            undoSingle();
        }

        selected = { -1, -1 };
        board.clearHighlight();
        gameOver = false;
        showGameOverModal = false;
        finalMessage = nullptr;
        botThinkingDelay = 0;
        if (GetStockfishEngine().isAvailable()) {
            GetStockfishEngine().evaluatePosition(board.toFEN(currentTurn), 80);
        }
    };

    // Lambda for Chess.com style smart redo
    auto performRedo = [&]() {
        reviewPly = -1;
        if (redoStack.empty()) return;

        auto redoSingle = [&]() {
            if (redoStack.empty()) return;
            undoStack.push_back(board.snapshot(currentTurn));
            string next = redoStack.back();
            redoStack.pop_back();
            board.restoreSnapshot(next, currentTurn);
            if (!redoMoveHistory.empty()) {
                auto rec = redoMoveHistory.back();
                redoMoveHistory.pop_back();
                board.recordMove(rec.move, rec.san, rec.turn, rec.promoChar);
            }
            gameSnapshots.push_back(board.snapshot(currentTurn));
        };

        redoSingle();
        if (vsBot && currentTurn == botColor && !redoStack.empty()) {
            redoSingle();
        }

        selected = { -1, -1 };
        board.clearHighlight();
        checkEndConditions(board, currentTurn, gameOver, finalMessage);
        if (gameOver) showGameOverModal = true;
        if (GetStockfishEngine().isAvailable()) {
            GetStockfishEngine().evaluatePosition(board.toFEN(currentTurn), 80);
        }
    };

    // Reusable UI renderer for sidebar and eval bar (drawn both in regular frames and during move animations)
    auto drawUI = [&](bool interactive) {
        // Draw Left Evaluation Bar
        float evalPawns = GetStockfishEngine().isAvailable() ? GetStockfishEngine().getWhiteAdvantagePawns() : (board.getMaterialAdvantage());
        std::string evalText = GetStockfishEngine().isAvailable() ? GetStockfishEngine().getEvalText() : TextFormat("%+d", board.getMaterialAdvantage());
        drawEvaluationBar(EVAL_X, EVAL_Y, EVAL_WIDTH, EVAL_HEIGHT, evalPawns, evalText, board.flipped);

        // Right Sidebar material advantage & captured pieces
        int matAdv = board.getMaterialAdvantage();
        int whiteMatAdv = (matAdv > 0) ? matAdv : 0;
        int blackMatAdv = (matAdv < 0) ? -matAdv : 0;
        auto whiteCaptured = board.getCapturedPieces(PWHITE);
        auto blackCaptured = board.getCapturedPieces(PBLACK);

        bool topIsWhite = board.flipped;
        bool topIsBot = vsBot && (topIsWhite ? (botColor == PWHITE) : (botColor == PBLACK));
        bool botThinking = vsBot && !gameOver && (currentTurn == botColor);

        const char* topName;
        const char* topSub;
        if (vsBot) {
            if (topIsBot) {
                topName = botProfile.name;
                topSub = botThinking ? "Thinking..." : botProfile.engineBadge;
            } else {
                topName = "You";
                topSub = "Human";
            }
        } else {
            topName = topIsWhite ? "White Player" : "Black Player";
            topSub = "1500";
        }

        float topTime = topIsWhite ? whiteTime : blackTime;
        bool topTurn = topIsWhite ? (currentTurn == PWHITE) : (currentTurn == PBLACK);
        int topAdv = topIsWhite ? whiteMatAdv : blackMatAdv;
        const auto& topCaptured = topIsWhite ? whiteCaptured : blackCaptured;

        // Top Player Card Last Move calculation
        COLOR topColor = topIsWhite ? PWHITE : PBLACK;
        bool topIsOpponent = vsBot && (botColor == topColor);
        int topMoveIdx = findLastMoveIndex(board.getMoveHistory(), topColor);
        std::string topMoveBadge;
        if (topMoveIdx >= 0) {
            const auto& rec = board.getMoveHistory()[topMoveIdx];
            int mNum = (topMoveIdx / 2) + 1;
            topMoveBadge = (rec.turn == PWHITE) ? TextFormat("%d. %s", mNum, rec.san.c_str()) : TextFormat("%d... %s", mNum, rec.san.c_str());
        }
        bool topIsReviewTarget = (reviewPly > 0 && reviewPly - 1 == topMoveIdx);

        if (drawPlayerCard(Rectangle{ SIDEBAR_X, SIDEBAR_Y, SIDEBAR_WIDTH, 88 },
            topName, topSub, topIsWhite, topTurn && !gameOver, topTime, untimedGame,
            topAdv, topCaptured, promoTex,
            topMoveBadge.empty() ? nullptr : topMoveBadge.c_str(), topIsOpponent, topIsReviewTarget)) {
            if (interactive && topMoveIdx >= 0) reviewPly = topMoveIdx + 1;
        }

        // Move History List (396px height)
        int clickedPly = drawMoveHistoryTable(Rectangle{ SIDEBAR_X, SIDEBAR_Y + 98, SIDEBAR_WIDTH, 396 },
            board.getMoveHistory(), historyScrollY, reviewPly, vsBot, botColor);
        if (interactive && clickedPly != -999) {
            reviewPly = clickedPly;
        }

        // Move Review Navigation Bar (|<, <, >, >| Live)
        drawReviewNavBar(Rectangle{ SIDEBAR_X, SIDEBAR_Y + 498, SIDEBAR_WIDTH, 44 },
            reviewPly, (int)board.getMoveHistory().size(),
            hoverNavFirst, hoverNavPrev, hoverNavNext, hoverNavLast);
        // Bottom Player Card
        bool botIsWhite = !board.flipped;
        bool botIsBot = vsBot && (botIsWhite ? (botColor == PWHITE) : (botColor == PBLACK));

        const char* botName;
        const char* botSub;
        if (vsBot) {
            if (botIsBot) {
                botName = botProfile.name;
                botSub = botThinking ? "Thinking..." : botProfile.engineBadge;
            } else {
                botName = "You";
                botSub = "Human";
            }
        } else {
            botName = botIsWhite ? "White Player" : "Black Player";
            botSub = "1500";
        }

        float botTime = botIsWhite ? whiteTime : blackTime;
        bool botTurn = botIsWhite ? (currentTurn == PWHITE) : (currentTurn == PBLACK);
        int botAdv = botIsWhite ? whiteMatAdv : blackMatAdv;
        const auto& botCaptured = botIsWhite ? whiteCaptured : blackCaptured;

        // Bottom Player Card Last Move calculation
        COLOR botCol = botIsWhite ? PWHITE : PBLACK;
        bool botIsOpponent = vsBot && (botColor == botCol);
        int botMoveIdx = findLastMoveIndex(board.getMoveHistory(), botCol);
        std::string botMoveBadge;
        if (botMoveIdx >= 0) {
            const auto& rec = board.getMoveHistory()[botMoveIdx];
            int mNum = (botMoveIdx / 2) + 1;
            botMoveBadge = (rec.turn == PWHITE) ? TextFormat("%d. %s", mNum, rec.san.c_str()) : TextFormat("%d... %s", mNum, rec.san.c_str());
        }
        bool botIsReviewTarget = (reviewPly > 0 && reviewPly - 1 == botMoveIdx);

        if (drawPlayerCard(Rectangle{ SIDEBAR_X, SIDEBAR_Y + 548, SIDEBAR_WIDTH, 88 },
            botName, botSub, botIsWhite, botTurn && !gameOver, botTime, untimedGame,
            botAdv, botCaptured, promoTex,
            botMoveBadge.empty() ? nullptr : botMoveBadge.c_str(), botIsOpponent, botIsReviewTarget)) {
            if (interactive && botMoveIdx >= 0) reviewPly = botMoveIdx + 1;
        }

        // Row 1: Actions Toolbar (Menu, Draw, Resign)
        float btnY1 = SIDEBAR_Y + 646;
        float btnH1 = 40;
        Rectangle menuBtn  { SIDEBAR_X,       btnY1, 96,  btnH1 };
        Rectangle drawBtn  { SIDEBAR_X + 103, btnY1, 110, btnH1 };
        Rectangle resignBtn{ SIDEBAR_X + 220, btnY1, 110, btnH1 };

        // Row 2: Controls Toolbar (Undo, Redo, Flip, Theme, New)
        float btnY2 = SIDEBAR_Y + 694;
        float btnH2 = 42;
        Rectangle undoBtn { SIDEBAR_X,       btnY2, 60, btnH2 };
        Rectangle redoBtn { SIDEBAR_X + 66,  btnY2, 60, btnH2 };
        Rectangle flipBtn { SIDEBAR_X + 132, btnY2, 60, btnH2 };
        Rectangle themeBtn{ SIDEBAR_X + 198, btnY2, 64, btnH2 };
        Rectangle newBtn  { SIDEBAR_X + 268, btnY2, 62, btnH2 };

        if (interactive) {
            // Row 1: Actions
            if (DrawButton(menuBtn, "< Menu", 15, hoverMenu, Color{ 50, 48, 45, 255 }, Color{ 75, 73, 70, 255 }, RAYWHITE)) {
                if (gameOver) {
                    returnToMenu = true;
                } else {
                    showExitConfirmModal = true;
                }
            }
            if (DrawButton(drawBtn, "Draw", 15, hoverDraw, Color{ 50, 48, 45, 255 }, Color{ 160, 140, 50, 255 }, !gameOver ? RAYWHITE : GRAY)) {
                if (!gameOver) {
                    if (vsBot) {
                        float sfEval = GetStockfishEngine().isAvailable() ? GetStockfishEngine().getWhiteAdvantagePawns() : (float)board.getMaterialAdvantage();
                        float botAdv = (botColor == PWHITE) ? sfEval : -sfEval;
                        if (botAdv > 2.2f && botProfile.rating >= 1600) {
                            notifyBannerText = "Draw declined by Bot! (Bot has advantage)";
                            notifyBannerTimer = 3.0f;
                            if (IsSoundValid(sounds.illegal)) PlaySound(sounds.illegal);
                        } else {
                            gameOver = true;
                            finalMessage = "DRAW BY AGREEMENT";
                            showGameOverModal = true;
                            if (IsSoundValid(sounds.gameEnd)) PlaySound(sounds.gameEnd);
                        }
                    } else {
                        gameOver = true;
                        finalMessage = "DRAW BY MUTUAL AGREEMENT";
                        showGameOverModal = true;
                        if (IsSoundValid(sounds.gameEnd)) PlaySound(sounds.gameEnd);
                    }
                }
            }
            if (DrawButton(resignBtn, "Resign", 15, hoverResign, Color{ 68, 36, 36, 255 }, Color{ 175, 45, 45, 255 }, !gameOver ? RAYWHITE : GRAY)) {
                if (!gameOver) {
                    if (vsBot) {
                        gameOver = true;
                        finalMessage = (humanColor == PWHITE) ? "BLACK WINS BY RESIGNATION" : "WHITE WINS BY RESIGNATION";
                        showGameOverModal = true;
                        if (IsSoundValid(sounds.gameEnd)) PlaySound(sounds.gameEnd);
                    } else {
                        gameOver = true;
                        finalMessage = (currentTurn == PWHITE) ? "BLACK WINS BY RESIGNATION" : "WHITE WINS BY RESIGNATION";
                        showGameOverModal = true;
                        if (IsSoundValid(sounds.gameEnd)) PlaySound(sounds.gameEnd);
                    }
                }
            }

            // Row 2: Controls
            if (DrawButton(undoBtn, "Undo", 14, hoverUndo, Color{ 48, 46, 43, 255 }, Color{ 70, 68, 64, 255 })) {
                performUndo();
            }
            if (DrawButton(redoBtn, "Redo", 14, hoverRedo, Color{ 48, 46, 43, 255 }, Color{ 70, 68, 64, 255 })) {
                performRedo();
            }
            if (DrawButton(flipBtn, "Flip", 14, hoverFlip, Color{ 48, 46, 43, 255 }, Color{ 70, 68, 64, 255 })) {
                board.toggleFlip();
            }
            if (DrawButton(themeBtn, board.getThemeName(), 13, hoverTheme, Color{ 48, 46, 43, 255 }, Color{ 70, 68, 64, 255 })) {
                board.cycleTheme();
            }
            if (DrawButton(newBtn, "New", 14, hoverNew, Color{ 129, 182, 76, 255 }, Color{ 145, 202, 85, 255 })) {
                board.initillize();
                board.clearMoveHistory();
                undoStack.clear();
                redoStack.clear();
                redoMoveHistory.clear();
                currentTurn = PWHITE;
                gameSnapshots.clear();
                gameSnapshots.push_back(board.snapshot(currentTurn));
                reviewPly = -1;
                activeReviewLoadedPly = -999;
                gameOver = false;
                showGameOverModal = false;
                whiteWarned10s = false;
                blackWarned10s = false;
                botThinkingDelay = 0;
                if (IsSoundValid(sounds.gameStart)) PlaySound(sounds.gameStart);
                if (untimedGame) { whiteTime = blackTime = 0; }
                else { whiteTime = blackTime = 600.0f; }
                if (vsBot && botProfile.engine == ENGINE_STOCKFISH) {
                    GetStockfishEngine().newGame();
                }
            }
        } else {
            // Draw static buttons while move animation is playing
            DrawRectangleRounded(menuBtn, 0.25f, 6, Color{ 50, 48, 45, 255 });
            DrawText("< Menu", (int)menuBtn.x + 18, (int)menuBtn.y + 12, 15, GRAY);
            DrawRectangleRounded(drawBtn, 0.25f, 6, Color{ 50, 48, 45, 255 });
            DrawText("Draw", (int)drawBtn.x + 36, (int)drawBtn.y + 12, 15, GRAY);
            DrawRectangleRounded(resignBtn, 0.25f, 6, Color{ 68, 36, 36, 255 });
            DrawText("Resign", (int)resignBtn.x + 30, (int)resignBtn.y + 12, 15, GRAY);

            DrawRectangleRounded(undoBtn, 0.25f, 6, Color{ 48, 46, 43, 255 });
            DrawText("Undo", (int)undoBtn.x + 14, (int)undoBtn.y + 14, 14, GRAY);
            DrawRectangleRounded(redoBtn, 0.25f, 6, Color{ 48, 46, 43, 255 });
            DrawText("Redo", (int)redoBtn.x + 14, (int)redoBtn.y + 14, 14, GRAY);
            DrawRectangleRounded(flipBtn, 0.25f, 6, Color{ 48, 46, 43, 255 });
            DrawText("Flip", (int)flipBtn.x + 18, (int)flipBtn.y + 14, 14, GRAY);
            DrawRectangleRounded(themeBtn, 0.25f, 6, Color{ 48, 46, 43, 255 });
            DrawText(board.getThemeName(), (int)themeBtn.x + 8, (int)themeBtn.y + 14, 13, GRAY);
            DrawRectangleRounded(newBtn, 0.25f, 6, Color{ 129, 182, 76, 255 });
            DrawText("New", (int)newBtn.x + 17, (int)newBtn.y + 14, 14, WHITE);
        }
    };

    auto drawExtras = [&]() { drawUI(false); };

    while (!WindowShouldClose() && !returnToMenu) {
        HandleWindowControls();

        // Keyboard Shortcuts
        if (IsKeyPressed(KEY_ESCAPE)) {
            if (reviewPly != -1) {
                reviewPly = -1; // exit review mode back to live
            } else if (showExitConfirmModal) {
                showExitConfirmModal = false;
            } else if (gameOver) {
                returnToMenu = true;
            } else {
                showExitConfirmModal = true;
            }
        }
        if (IsKeyPressed(KEY_R)) board.toggleFlip();
        if (IsKeyPressed(KEY_T) || IsKeyPressed(KEY_N)) board.cycleTheme();
        if (IsKeyPressed(KEY_A)) {
            board.toggleLastMoveArrow();
            notifyBannerText = board.showLastMoveArrow ? "Last Move Arrow: ON" : "Last Move Arrow: OFF";
            notifyBannerTimer = 2.0f;
        }
        if (IsKeyPressed(KEY_LEFT) && !showExitConfirmModal && !showGameOverModal) {
            int total = (int)board.getMoveHistory().size();
            if (total > 0) {
                if (reviewPly == -1) reviewPly = total - 1;
                else if (reviewPly > 0) reviewPly--;
            }
        }
        if (IsKeyPressed(KEY_RIGHT) && !showExitConfirmModal && !showGameOverModal) {
            int total = (int)board.getMoveHistory().size();
            if (reviewPly != -1) {
                reviewPly++;
                if (reviewPly >= total) reviewPly = -1;
            }
        }
        if ((IsKeyPressed(KEY_HOME) || IsKeyPressed(KEY_UP)) && !showExitConfirmModal && !showGameOverModal) {
            if (!board.getMoveHistory().empty()) reviewPly = 0;
        }
        if ((IsKeyPressed(KEY_END) || IsKeyPressed(KEY_DOWN)) && !showExitConfirmModal && !showGameOverModal) {
            reviewPly = -1;
        }
        if (IsKeyPressed(KEY_L) && !showExitConfirmModal && !showGameOverModal) {
            int oppIdx = findLastMoveIndex(board.getMoveHistory(), vsBot ? botColor : (currentTurn == PWHITE ? PBLACK : PWHITE));
            if (oppIdx >= 0) {
                if (reviewPly == oppIdx + 1) {
                    reviewPly = -1;
                    notifyBannerText = "Resumed Live Play";
                    notifyBannerTimer = 1.5f;
                } else {
                    reviewPly = oppIdx + 1;
                    const auto& rec = board.getMoveHistory()[oppIdx];
                    int mNum = (oppIdx / 2) + 1;
                    const char* pfx = (rec.turn == PWHITE) ? TextFormat("%d.", mNum) : TextFormat("%d...", mNum);
                    notifyBannerText = TextFormat("Inspecting Opponent Move: %s %s (Press Right or >| to resume)", pfx, rec.san.c_str());
                    notifyBannerTimer = 3.0f;
                }
            } else {
                notifyBannerText = "No opponent move played yet";
                notifyBannerTimer = 2.0f;
            }
        }
        if (IsKeyPressed(KEY_H) && !showExitConfirmModal && !showGameOverModal) {
            notifyBannerText = "Left/Right: Scrub Moves | L: Opponent Move | R: Flip | Esc: Live";
            notifyBannerTimer = 4.0f;
        }
        if (IsKeyPressed(KEY_D) && !gameOver) {
            if (vsBot) {
                float sfEval = GetStockfishEngine().isAvailable() ? GetStockfishEngine().getWhiteAdvantagePawns() : (float)board.getMaterialAdvantage();
                float botAdv = (botColor == PWHITE) ? sfEval : -sfEval;
                if (botAdv > 2.2f && botProfile.rating >= 1600) {
                    notifyBannerText = "Draw declined by Bot! (Position is winning for Bot)";
                    notifyBannerTimer = 3.0f;
                    if (IsSoundValid(sounds.illegal)) PlaySound(sounds.illegal);
                } else {
                    gameOver = true;
                    finalMessage = "DRAW BY AGREEMENT";
                    showGameOverModal = true;
                    if (IsSoundValid(sounds.gameEnd)) PlaySound(sounds.gameEnd);
                }
            } else {
                gameOver = true;
                finalMessage = "DRAW BY MUTUAL AGREEMENT";
                showGameOverModal = true;
                if (IsSoundValid(sounds.gameEnd)) PlaySound(sounds.gameEnd);
            }
        }

        bool ctrlHeld = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
        if (ctrlHeld && IsKeyPressed(KEY_Z)) {
            performUndo();
        }
        if (ctrlHeld && IsKeyPressed(KEY_Y)) {
            performRedo();
        }

        // Clock Update
        if (!untimedGame && !gameOver) {
            float dt = GetFrameTime();
            if (currentTurn == PWHITE) {
                whiteTime -= dt;
                if (whiteTime <= 10.0f && whiteTime > 0.0f && !whiteWarned10s) {
                    if (IsSoundValid(sounds.tenSeconds)) PlaySound(sounds.tenSeconds);
                    whiteWarned10s = true;
                }
            } else {
                blackTime -= dt;
                if (blackTime <= 10.0f && blackTime > 0.0f && !blackWarned10s) {
                    if (IsSoundValid(sounds.tenSeconds)) PlaySound(sounds.tenSeconds);
                    blackWarned10s = true;
                }
            }
            if (whiteTime < 0) whiteTime = 0;
            if (blackTime < 0) blackTime = 0;

            if (whiteTime <= 0 || blackTime <= 0) {
                finalMessage = (whiteTime <= 0) ? "TIME OUT! Black Wins" : "TIME OUT! White Wins";
                gameOver = true;
                showGameOverModal = true;
                if (IsSoundValid(sounds.gameEnd)) PlaySound(sounds.gameEnd);
            }
        }

        // Coordinates & Mouse Hover
        Vector2 mouse = GetVirtualMousePosition();
        int boardCol = static_cast<int>((mouse.x - BOARD_X) / BOXSIZE);
        int boardRow = static_cast<int>((mouse.y - BOARD_Y) / BOXSIZE);
        if (board.flipped) { boardRow = 7 - boardRow; boardCol = 7 - boardCol; }
        bool mouseOnBoard = (mouse.x >= BOARD_X && mouse.x < BOARD_X + 8 * BOXSIZE &&
                             mouse.y >= BOARD_Y && mouse.y < BOARD_Y + 8 * BOXSIZE);

        // --- 1. Right-Click Arrow & Marked Square Interaction ---
        if (!gameOver && !showGameOverModal && !showExitConfirmModal) {
            if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
                if (mouseOnBoard) {
                    rightClickStart = { boardRow, boardCol };
                }
            }
            if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) {
                if (rightClickStart.row >= 0 && mouseOnBoard) {
                    Position rightClickEnd = { boardRow, boardCol };
                    if (rightClickStart.row == rightClickEnd.row && rightClickStart.col == rightClickEnd.col) {
                        board.toggleMarkedSquare(rightClickStart);
                    } else {
                        board.addArrow(rightClickStart, rightClickEnd);
                    }
                }
                rightClickStart = { -1, -1 };
            }
        }

        // --- 2. Left-Click & Drag-and-Drop Interaction ---
        if (!gameOver && !showGameOverModal && !showExitConfirmModal) {
            if (reviewPly != -1) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mouseOnBoard) {
                    notifyBannerText = "Review mode active. Click [>|] or press Right Arrow to resume live play.";
                    notifyBannerTimer = 2.0f;
                }
            } else if (vsBot && currentTurn == botColor) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mouseOnBoard) {
                    // Pre-move sound when player tries to move during bot's turn
                    if (IsSoundValid(sounds.premove)) PlaySound(sounds.premove);
                }
            } else {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    board.clearArrows();
                    board.clearMarkedSquares();

                    if (mouseOnBoard) {
                        Piece* clickedPiece = board.getPiece(boardRow, boardCol);

                        if (selected.row == -1) {
                            if (clickedPiece && clickedPiece->getColor() == currentTurn) {
                                selected = { boardRow, boardCol };
                                board.computeHighlight(selected);
                                dragStart = selected;
                                isDragging = true;
                                board.setDraggingPiece(selected);
                            } else if (clickedPiece && clickedPiece->getColor() != currentTurn) {
                                if (IsSoundValid(sounds.illegal)) PlaySound(sounds.illegal);
                            }
                        } else {
                            if (boardRow == selected.row && boardCol == selected.col) {
                                dragStart = selected;
                                isDragging = true;
                                board.setDraggingPiece(selected);
                            } else if (clickedPiece && clickedPiece->getColor() == currentTurn) {
                                selected = { boardRow, boardCol };
                                board.computeHighlight(selected);
                                dragStart = selected;
                                isDragging = true;
                                board.setDraggingPiece(selected);
                            } else if (board.highlight[boardRow][boardCol]) {
                                executeHumanMove(board, selected, { boardRow, boardCol }, currentTurn, promoTex, sounds,
                                    undoStack, redoStack, redoMoveHistory, gameSnapshots, gameOver, finalMessage, historyScrollY, drawExtras);
                                reviewPly = -1;
                                if (gameOver) showGameOverModal = true;
                                selected = { -1, -1 };
                                board.clearHighlight();
                                isDragging = false;
                                board.setDraggingPiece({ -1, -1 });
                                botThinkingDelay = 0;
                            } else {
                                if (clickedPiece && clickedPiece->getColor() != currentTurn) {
                                    if (IsSoundValid(sounds.illegal)) PlaySound(sounds.illegal);
                                }
                                selected = { -1, -1 };
                                board.clearHighlight();
                                isDragging = false;
                                board.setDraggingPiece({ -1, -1 });
                            }
                        }
                    } else {
                        selected = { -1, -1 };
                        board.clearHighlight();
                        isDragging = false;
                        board.setDraggingPiece({ -1, -1 });
                    }
                }

                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    if (isDragging) {
                        isDragging = false;
                        board.setDraggingPiece({ -1, -1 });

                        if (mouseOnBoard && (boardRow != dragStart.row || boardCol != dragStart.col)) {
                            if (board.highlight[boardRow][boardCol]) {
                                executeHumanMove(board, dragStart, { boardRow, boardCol }, currentTurn, promoTex, sounds,
                                    undoStack, redoStack, redoMoveHistory, gameSnapshots, gameOver, finalMessage, historyScrollY, drawExtras);
                                reviewPly = -1;
                                if (gameOver) showGameOverModal = true;
                                selected = { -1, -1 };
                                board.clearHighlight();
                                botThinkingDelay = 0;
                            } else {
                                if (IsSoundValid(sounds.illegal)) PlaySound(sounds.illegal);
                            }
                        }
                    }
                }
            }
        }

        // --- 3. Bot Move Execution (if bot turn) ---
        if (vsBot && !gameOver && currentTurn == botColor && !showExitConfirmModal && reviewPly == -1) {
            if (botThinkingDelay < 2) {
                botThinkingDelay++;
            } else {
                undoStack.push_back(board.snapshot(currentTurn));
                redoStack.clear();
                redoMoveHistory.clear();
                playBotMove(board, botColor, botProfile, promoTex, sounds, currentTurn, gameSnapshots, gameOver, finalMessage, historyScrollY, drawExtras);
                reviewPly = -1;
                if (gameOver) showGameOverModal = true;
                botThinkingDelay = 0;
            }
        } else {
            botThinkingDelay = 0;
        }

        // --- 4. Main Single-Pass Virtual Screen Render ---
        BeginVirtualScreen();
        ClearBackground(Color{ 22, 21, 18, 255 });

        // Draw Center Chess Board
        if (reviewPly == -1) {
            board.drawBoard(selected, true, Position{ -1,-1 }, Position{ -1,-1 }, BOARD_X, BOARD_Y, BOXSIZE);
        } else {
            if (reviewPly != activeReviewLoadedPly && reviewPly < (int)gameSnapshots.size()) {
                COLOR dummyTurn;
                reviewBoard.restoreSnapshot(gameSnapshots[reviewPly], dummyTurn);
                activeReviewLoadedPly = reviewPly;
            }
            reviewBoard.flipped = board.flipped;
            reviewBoard.currentTheme = board.currentTheme;
            reviewBoard.showLastMoveArrow = board.showLastMoveArrow;
            reviewBoard.drawBoard(Position{ -1,-1 }, false, Position{ -1,-1 }, Position{ -1,-1 }, BOARD_X, BOARD_Y, BOXSIZE);

            // Review mode header banner above board
            float bannerW = 8 * BOXSIZE;
            Rectangle revBanner{ (float)BOARD_X, (float)(BOARD_Y - 34), bannerW, 28.0f };
            bool revHovered = CheckCollisionPointRec(mouse, revBanner);
            DrawRectangleRounded(revBanner, 0.3f, 6, revHovered ? Color{ 38, 68, 102, 250 } : Color{ 28, 48, 72, 240 });
            DrawRectangleRoundedLinesEx(revBanner, 0.3f, 6, 1.5f, revHovered ? Color{ 140, 205, 255, 255 } : Color{ 85, 160, 245, 255 });
            const char* revText = (reviewPly == 0)
                ? "◄ REVIEWING START OF GAME • Click here or [>|] to Resume Live Play ►"
                : TextFormat("◄ REVIEWING MOVE %d (%s) • Click here or [>|] to Resume Live Play ►",
                    reviewPly, board.getMoveHistory()[reviewPly - 1].san.c_str());
            int rtw = MeasureText(revText, 13);
            DrawText(revText, (int)(BOARD_X + (bannerW - rtw) / 2.0f), (int)(BOARD_Y - 27), 13, Color{ 220, 240, 255, 255 });
            if (revHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                reviewPly = -1;
            }
        }

        // Live right-click arrow preview while dragging
        if (rightClickStart.row >= 0 && mouseOnBoard && (boardRow != rightClickStart.row || boardCol != rightClickStart.col)) {
            int fdr = board.flipped ? 7 - rightClickStart.row : rightClickStart.row;
            int fdc = board.flipped ? 7 - rightClickStart.col : rightClickStart.col;
            int tdr = board.flipped ? 7 - boardRow : boardRow;
            int tdc = board.flipped ? 7 - boardCol : boardCol;
            Vector2 p1{ BOARD_X + fdc * BOXSIZE + BOXSIZE / 2.0f, BOARD_Y + fdr * BOXSIZE + BOXSIZE / 2.0f };
            Vector2 p2{ BOARD_X + tdc * BOXSIZE + BOXSIZE / 2.0f, BOARD_Y + tdr * BOXSIZE + BOXSIZE / 2.0f };
            DrawChessArrow(p1, p2, Color{ 255, 170, 0, 150 });
        }

        // Live piece drag follower with shadow
        if (isDragging && dragStart.row >= 0) {
            Piece* dp = board.getPiece(dragStart);
            if (dp) {
                DrawCircle((int)mouse.x, (int)mouse.y + 18, 34, Color{ 0, 0, 0, 80 });
                dp->drawAtPixel(mouse.x - 50, mouse.y - 58);
            }
        }

        // Draw Evaluation Bar & Right Sidebar with active button interaction
        drawUI(true);

        // --- 5. Chess.com Game Over Modal ---
        if (gameOver && showGameOverModal) {
            DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, Color{ 0, 0, 0, 185 });

            float mw = 440, mh = 310;
            float mx = (VIRTUAL_WIDTH - mw) / 2.0f;
            float my = (VIRTUAL_HEIGHT - mh) / 2.0f;

            DrawRectangleRounded(Rectangle{ mx, my, mw, mh }, 0.08f, 8, Color{ 38, 36, 34, 255 });
            DrawRectangleRoundedLinesEx(Rectangle{ mx, my, mw, mh }, 0.08f, 8, 2.0f, Color{ 70, 68, 65, 255 });

            bool isCheckmate = (finalMessage && strstr(finalMessage, "CHECKMATE"));
            Color bannerColor = isCheckmate ? Color{ 129, 182, 76, 255 } : GOLD;

            int tw = MeasureText(finalMessage, 24);
            DrawText(finalMessage, (int)(mx + (mw - tw) / 2), (int)(my + 30), 24, bannerColor);

            const char* sub = isCheckmate ? "Victory by Checkmate!" : "Game Over";
            int sw = MeasureText(sub, 16);
            DrawText(sub, (int)(mx + (mw - sw) / 2), (int)(my + 66), 16, LIGHTGRAY);

            Rectangle modalNewBtn   { mx + 50, my + 115, 340, 46 };
            Rectangle modalReviewBtn{ mx + 50, my + 172, 340, 40 };
            Rectangle modalMenuBtn  { mx + 50, my + 224, 340, 40 };

            if (DrawButton(modalNewBtn, "Play Again", 20, hoverModalNew, Color{ 129, 182, 76, 255 }, Color{ 145, 202, 85, 255 })) {
                board.initillize();
                board.clearMoveHistory();
                undoStack.clear();
                redoStack.clear();
                redoMoveHistory.clear();
                currentTurn = PWHITE;
                gameSnapshots.clear();
                gameSnapshots.push_back(board.snapshot(currentTurn));
                reviewPly = -1;
                activeReviewLoadedPly = -999;
                gameOver = false;
                showGameOverModal = false;
                whiteWarned10s = false;
                blackWarned10s = false;
                botThinkingDelay = 0;
                if (IsSoundValid(sounds.gameStart)) PlaySound(sounds.gameStart);
                if (!untimedGame) { whiteTime = blackTime = 600.0f; }
                if (vsBot && botProfile.engine == ENGINE_STOCKFISH) {
                    GetStockfishEngine().newGame();
                }
            }
            if (DrawButton(modalReviewBtn, "Review Board", 17, hoverModalReview, Color{ 54, 52, 49, 255 }, Color{ 75, 73, 70, 255 })) {
                showGameOverModal = false;
            }
            if (DrawButton(modalMenuBtn, "Back to Main Menu", 17, hoverModalMenu, Color{ 48, 64, 90, 255 }, Color{ 65, 88, 125, 255 }, RAYWHITE)) {
                returnToMenu = true;
            }
        }

        // --- 6. Exit Confirmation Modal ---
        if (showExitConfirmModal && !gameOver) {
            DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, Color{ 0, 0, 0, 185 });

            float mw = 440, mh = 265;
            float mx = (VIRTUAL_WIDTH - mw) / 2.0f;
            float my = (VIRTUAL_HEIGHT - mh) / 2.0f;

            DrawRectangleRounded(Rectangle{ mx, my, mw, mh }, 0.08f, 8, Color{ 38, 36, 34, 255 });
            DrawRectangleRoundedLinesEx(Rectangle{ mx, my, mw, mh }, 0.08f, 8, 2.0f, Color{ 70, 68, 65, 255 });

            int tw = MeasureText("Exit to Main Menu?", 22);
            DrawText("Exit to Main Menu?", (int)(mx + (mw - tw) / 2), (int)(my + 28), 22, RAYWHITE);

            const char* sub = "Would you like to save your game before leaving?";
            int sw = MeasureText(sub, 14);
            DrawText(sub, (int)(mx + (mw - sw) / 2), (int)(my + 60), 14, LIGHTGRAY);

            Rectangle saveExitBtn{ mx + 45, my + 100, 350, 40 };
            Rectangle exitNoSaveBtn{ mx + 45, my + 148, 350, 40 };
            Rectangle cancelBtn{ mx + 45, my + 196, 350, 36 };

            if (DrawButton(saveExitBtn, "Save & Return to Menu", 16, hoverExitSave, Color{ 120, 160, 90, 255 }, Color{ 140, 185, 105, 255 }, WHITE)) {
                board.save(currentTurn);
                showExitConfirmModal = false;
                returnToMenu = true;
            }
            if (DrawButton(exitNoSaveBtn, "Exit without Saving", 16, hoverExitNoSave, Color{ 150, 60, 60, 255 }, Color{ 180, 75, 75, 255 }, WHITE)) {
                showExitConfirmModal = false;
                returnToMenu = true;
            }
            if (DrawButton(cancelBtn, "Cancel (Keep Playing)", 15, hoverExitCancel, Color{ 54, 52, 49, 255 }, Color{ 75, 73, 70, 255 }, RAYWHITE)) {
                showExitConfirmModal = false;
            }
        }

        // --- 7. Temporary Notification Banner ---
        if (notifyBannerTimer > 0.0f) {
            notifyBannerTimer -= GetFrameTime();
            int bw = MeasureText(notifyBannerText.c_str(), 16) + 36;
            float bx = (VIRTUAL_WIDTH - bw) / 2.0f;
            float by = 16.0f;
            DrawRectangleRounded(Rectangle{ bx, by, (float)bw, 36.0f }, 0.35f, 6, Color{ 30, 28, 26, 235 });
            DrawRectangleRoundedLinesEx(Rectangle{ bx, by, (float)bw, 36.0f }, 0.35f, 6, 1.5f, GOLD);
            DrawText(notifyBannerText.c_str(), (int)bx + 18, (int)by + 10, 16, RAYWHITE);
        }

        EndVirtualScreen();
    }
    }

    GetStockfishEngine().stop();
    unloadGameSounds(sounds);
    unloadPromotionTextures(promoTex);
    Board::unloadTextures();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
