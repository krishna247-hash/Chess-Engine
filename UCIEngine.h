#pragma once
#include <string>
#include <vector>
#include "utility.h"

// UCIEngine: communicates with open-source UCI chess engines (Stockfish)
// via bidirectional POSIX standard I/O pipes.
class UCIEngine {
public:
    UCIEngine();
    ~UCIEngine();

    // Locates engine binary, starts process, and runs UCI handshake (uci / isready).
    // Checks candidate paths: customPath, "stockfish", /opt/homebrew/bin/stockfish, etc.
    bool init(const std::string& customPath = "");

    // Returns true if engine process is running and responsive
    bool isAvailable() const;

    // Returns engine name reported by 'id name' (e.g. "Stockfish 19")
    const std::string& getEngineName() const { return engineName; }

    // Configures strength:
    // If limitStrength is true and targetElo > 0:
    //   sets UCI_LimitStrength=true, UCI_Elo=targetElo (1320..3190)
    // Else:
    //   sets UCI_LimitStrength=false, Skill Level=skillLevel (0..20)
    bool configure(int targetElo, int skillLevel, bool limitStrength = true);

    // Resets engine state for a new game
    bool newGame();

    // Requests best move from FEN position
    // movetimeMs: search duration in milliseconds
    // Returns true on success, populating outMove and optional outPromo ('q','r','b','n')
    bool getBestMove(const std::string& fen, int movetimeMs, Move& outMove, char& outPromo);

    // Live Evaluation (White-relative advantage in pawns and display text)
    float getWhiteAdvantagePawns() const { return evalPawns; }
    std::string getEvalText() const { return evalString; }
    bool isMateScore() const { return mateScore; }
    int getMateMoves() const { return mateMoves; }

    // Evaluates current position quickly for live evaluation bar
    bool evaluatePosition(const std::string& fen, int movetimeMs = 120);

    // Shuts down engine process
    void stop();

private:
    int inPipe[2];
    int outPipe[2];
    pid_t pid;
    bool available;
    std::string engineName;
    int currentElo;
    int currentSkill;
    bool currentLimit;

    float evalPawns = 0.0f;
    bool mateScore = false;
    int mateMoves = 0;
    std::string evalString = "0.0";

    void parseInfoScore(const std::string& line, bool isWhiteTurn);
    bool sendCommand(const std::string& cmd);
    std::string readLine(int timeoutMs = 3000);
    std::string readUntil(const std::string& token, int timeoutMs = 5000);
};

// Global singleton accessor for Stockfish engine instance
UCIEngine& GetStockfishEngine();
