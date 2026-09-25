#include "UCIEngine.h"
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <signal.h>
#include <fcntl.h>
#include <cstring>
#include <cctype>

UCIEngine::UCIEngine()
    : inPipe{ -1, -1 }, outPipe{ -1, -1 }, pid(-1), available(false),
      engineName("None"), currentElo(-1), currentSkill(-1), currentLimit(false) {}

UCIEngine::~UCIEngine() {
    stop();
}

bool UCIEngine::isAvailable() const {
    if (!available || pid <= 0) return false;
    // Check if child process is still alive
    if (kill(pid, 0) != 0) return false;
    return true;
}

bool UCIEngine::sendCommand(const std::string& cmd) {
    if (inPipe[1] < 0) return false;
    std::string line = cmd + "\n";
    ssize_t written = write(inPipe[1], line.c_str(), line.size());
    return (written == static_cast<ssize_t>(line.size()));
}

std::string UCIEngine::readLine(int timeoutMs) {
    if (outPipe[0] < 0) return "";
    std::string line;
    char ch = 0;

    while (true) {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(outPipe[0], &set);

        struct timeval tv;
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;

        int rv = select(outPipe[0] + 1, &set, nullptr, nullptr, &tv);
        if (rv <= 0) break; // timeout or interrupt

        ssize_t bytes = read(outPipe[0], &ch, 1);
        if (bytes <= 0) break; // EOF or error
        if (ch == '\r') continue;
        if (ch == '\n') break;
        line += ch;
    }
    return line;
}

std::string UCIEngine::readUntil(const std::string& token, int timeoutMs) {
    int elapsed = 0;
    const int stepMs = 50;

    while (elapsed < timeoutMs) {
        std::string line = readLine(stepMs);
        elapsed += stepMs;
        if (!line.empty()) {
            if (line.find(token) != std::string::npos) {
                return line;
            }
        }
    }
    return "";
}

bool UCIEngine::init(const std::string& customPath) {
    stop();

    std::vector<std::string> candidates;
    if (!customPath.empty()) candidates.push_back(customPath);
    candidates.push_back("./stockfish");
    candidates.push_back("stockfish");
    candidates.push_back("/opt/homebrew/bin/stockfish");
    candidates.push_back("/usr/local/bin/stockfish");
    candidates.push_back("/usr/bin/stockfish");

    std::string chosenPath;
    for (const auto& path : candidates) {
        if (path.find('/') != std::string::npos) {
            if (access(path.c_str(), X_OK) == 0) {
                chosenPath = path;
                break;
            }
        } else {
            // Check in PATH
            std::string checkCmd = "which " + path + " > /dev/null 2>&1";
            if (system(checkCmd.c_str()) == 0) {
                chosenPath = path;
                break;
            }
        }
    }

    if (chosenPath.empty()) {
        std::cerr << "[UCIEngine] No Stockfish binary found. Falling back to internal engine." << std::endl;
        return false;
    }

    if (pipe(inPipe) < 0 || pipe(outPipe) < 0) {
        std::cerr << "[UCIEngine] Failed to create pipes." << std::endl;
        return false;
    }

    pid = fork();
    if (pid < 0) {
        close(inPipe[0]); close(inPipe[1]);
        close(outPipe[0]); close(outPipe[1]);
        inPipe[0] = inPipe[1] = outPipe[0] = outPipe[1] = -1;
        return false;
    }

    if (pid == 0) {
        // Child: route pipes to stdin/stdout
        dup2(inPipe[0], STDIN_FILENO);
        dup2(outPipe[1], STDOUT_FILENO);

        // Silence engine stderr so NNUE/internal logs don't spam terminal
        int devNull = open("/dev/null", O_WRONLY);
        if (devNull >= 0) {
            dup2(devNull, STDERR_FILENO);
            close(devNull);
        }

        close(inPipe[0]); close(inPipe[1]);
        close(outPipe[0]); close(outPipe[1]);

        char* args[] = { const_cast<char*>(chosenPath.c_str()), nullptr };
        execvp(args[0], args);
        _exit(127);
    }

    // Parent
    close(inPipe[0]); inPipe[0] = -1;
    close(outPipe[1]); outPipe[1] = -1;

    // Send UCI handshake
    sendCommand("uci");
    int elapsed = 0;
    while (elapsed < 3000) {
        std::string line = readLine(100);
        elapsed += 100;
        if (line.rfind("id name ", 0) == 0) {
            engineName = line.substr(8);
        }
        if (line == "uciok") {
            break;
        }
    }

    sendCommand("isready");
    std::string ready = readUntil("readyok", 3000);
    if (ready.find("readyok") == std::string::npos) {
        std::cerr << "[UCIEngine] Engine did not respond to isready." << std::endl;
        stop();
        return false;
    }

    available = true;
    std::cout << "[UCIEngine] Connected to open-source engine: " << engineName
              << " (" << chosenPath << ")" << std::endl;
    return true;
}

bool UCIEngine::configure(int targetElo, int skillLevel, bool limitStrength) {
    if (!isAvailable()) return false;

    if (limitStrength && targetElo > 0) {
        // Clamp Elo between 1320 and 3190 (Stockfish supported range)
        int clampedElo = targetElo;
        if (clampedElo < 1320) clampedElo = 1320;
        if (clampedElo > 3190) clampedElo = 3190;

        if (currentElo != clampedElo || !currentLimit) {
            sendCommand("setoption name UCI_LimitStrength value true");
            sendCommand("setoption name UCI_Elo value " + std::to_string(clampedElo));
            currentElo = clampedElo;
            currentLimit = true;
        }
    } else {
        int clampedSkill = skillLevel;
        if (clampedSkill < 0) clampedSkill = 0;
        if (clampedSkill > 20) clampedSkill = 20;

        if (currentSkill != clampedSkill || currentLimit) {
            sendCommand("setoption name UCI_LimitStrength value false");
            sendCommand("setoption name Skill Level value " + std::to_string(clampedSkill));
            currentSkill = clampedSkill;
            currentLimit = false;
        }
    }

    sendCommand("isready");
    readUntil("readyok", 1000);
    return true;
}

bool UCIEngine::newGame() {
    if (!isAvailable()) return false;
    sendCommand("ucinewgame");
    sendCommand("isready");
    return (readUntil("readyok", 2000).find("readyok") != std::string::npos);
}

void UCIEngine::parseInfoScore(const std::string& line, bool isWhiteTurn) {
    // Check for "score mate "
    size_t matePos = line.find("score mate ");
    if (matePos != std::string::npos) {
        int m = 0;
        if (sscanf(line.c_str() + matePos, "score mate %d", &m) == 1) {
            mateScore = true;
            mateMoves = isWhiteTurn ? m : -m;
            evalPawns = (mateMoves > 0) ? 999.0f : -999.0f;
            evalString = (mateMoves > 0 ? "M" : "-M") + std::to_string(std::abs(mateMoves));
            return;
        }
    }

    // Check for "score cp "
    size_t cpPos = line.find("score cp ");
    if (cpPos != std::string::npos) {
        int cp = 0;
        if (sscanf(line.c_str() + cpPos, "score cp %d", &cp) == 1) {
            mateScore = false;
            mateMoves = 0;
            float whitePawns = (isWhiteTurn ? (float)cp : -(float)cp) / 100.0f;
            evalPawns = whitePawns;

            char buf[32];
            if (fabs(whitePawns) < 0.05f) {
                snprintf(buf, sizeof(buf), "0.0");
            } else if (whitePawns > 0.0f) {
                snprintf(buf, sizeof(buf), "+%.1f", whitePawns);
            } else {
                snprintf(buf, sizeof(buf), "%.1f", whitePawns);
            }
            evalString = buf;
        }
    }
}

bool UCIEngine::evaluatePosition(const std::string& fen, int movetimeMs) {
    if (!isAvailable()) return false;
    sendCommand("position fen " + fen);
    sendCommand("go movetime " + std::to_string(movetimeMs));

    bool isWhiteTurn = (fen.find(" w ") != std::string::npos);
    int timeout = movetimeMs + 2500;
    int elapsed = 0;
    const int stepMs = 30;

    while (elapsed < timeout) {
        std::string line = readLine(stepMs);
        elapsed += stepMs;
        if (!line.empty()) {
            if (line.rfind("info ", 0) == 0) {
                parseInfoScore(line, isWhiteTurn);
            }
            if (line.find("bestmove") != std::string::npos) {
                break;
            }
        }
    }
    return true;
}

bool UCIEngine::getBestMove(const std::string& fen, int movetimeMs, Move& outMove, char& outPromo) {
    if (!isAvailable()) return false;
    outPromo = '\0';

    sendCommand("position fen " + fen);
    sendCommand("go movetime " + std::to_string(movetimeMs));

    bool isWhiteTurn = (fen.find(" w ") != std::string::npos);
    int timeout = movetimeMs + 4000;
    int elapsed = 0;
    const int stepMs = 30;
    std::string bestMoveLine = "";

    while (elapsed < timeout) {
        std::string line = readLine(stepMs);
        elapsed += stepMs;
        if (!line.empty()) {
            if (line.rfind("info ", 0) == 0) {
                parseInfoScore(line, isWhiteTurn);
            }
            if (line.find("bestmove") != std::string::npos) {
                bestMoveLine = line;
                break;
            }
        }
    }

    if (bestMoveLine.empty()) {
        std::cerr << "[UCIEngine] Timeout waiting for bestmove." << std::endl;
        return false;
    }

    std::istringstream iss(bestMoveLine);
    std::string token, moveStr;
    iss >> token >> moveStr;

    if (token != "bestmove" || moveStr.size() < 4) {
        return false;
    }
    if (moveStr == "(none)" || moveStr == "NULL") {
        return false;
    }

    outMove.from.col = moveStr[0] - 'a';
    outMove.from.row = 8 - (moveStr[1] - '0');
    outMove.to.col = moveStr[2] - 'a';
    outMove.to.row = 8 - (moveStr[3] - '0');

    if (moveStr.size() >= 5) {
        outPromo = static_cast<char>(tolower(moveStr[4]));
    }

    return true;
}

void UCIEngine::stop() {
    if (pid > 0) {
        sendCommand("quit");
        if (inPipe[1] >= 0) { close(inPipe[1]); inPipe[1] = -1; }
        if (outPipe[0] >= 0) { close(outPipe[0]); outPipe[0] = -1; }

        int status = 0;
        int waited = 0;
        while (waited < 10 && waitpid(pid, &status, WNOHANG) == 0) {
            usleep(50000); // 50ms
            waited++;
        }
        if (waited >= 10) {
            kill(pid, SIGKILL);
            waitpid(pid, nullptr, 0);
        }
        pid = -1;
    }

    if (inPipe[0] >= 0) { close(inPipe[0]); inPipe[0] = -1; }
    if (inPipe[1] >= 0) { close(inPipe[1]); inPipe[1] = -1; }
    if (outPipe[0] >= 0) { close(outPipe[0]); outPipe[0] = -1; }
    if (outPipe[1] >= 0) { close(outPipe[1]); outPipe[1] = -1; }

    available = false;
    engineName = "None";
    currentElo = -1;
    currentSkill = -1;
    currentLimit = false;
}

UCIEngine& GetStockfishEngine() {
    static UCIEngine sInstance;
    return sInstance;
}
