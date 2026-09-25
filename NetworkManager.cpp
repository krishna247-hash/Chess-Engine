#include "NetworkManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <chrono>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
    #define CLOSE_SOCKET closesocket
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <poll.h>
    #define CLOSE_SOCKET close
#endif

// Helpers for lightweight JSON parsing without heavy dependencies
static std::string extractJsonString(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\":\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) {
        needle = "\"" + key + "\": \"";
        pos = json.find(needle);
        if (pos == std::string::npos) return "";
    }
    pos += needle.length();
    size_t endPos = json.find("\"", pos);
    if (endPos == std::string::npos) return "";
    return json.substr(pos, endPos - pos);
}

static double extractJsonNumber(const std::string& json, const std::string& key, double defaultVal = 0.0) {
    std::string needle = "\"" + key + "\":";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return defaultVal;
    pos += needle.length();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    size_t endPos = pos;
    while (endPos < json.size() && (isdigit(json[endPos]) || json[endPos] == '.' || json[endPos] == '-')) {
        endPos++;
    }
    if (endPos == pos) return defaultVal;
    try {
        return std::stod(json.substr(pos, endPos - pos));
    } catch (...) {
        return defaultVal;
    }
}

static int64_t getCurrentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

NetworkManager& NetworkManager::getInstance() {
    static NetworkManager instance;
    return instance;
}

NetworkManager::NetworkManager() {
    loadConfig();
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

NetworkManager::~NetworkManager() {
    disconnect();
#ifdef _WIN32
    WSACleanup();
#endif
}

void NetworkManager::loadConfig() {
    std::ifstream in("network.cfg");
    if (in.is_open()) {
        std::string line;
        while (std::getline(in, line)) {
            if (line.rfind("host=", 0) == 0) {
                serverHost = line.substr(5);
            } else if (line.rfind("port=", 0) == 0) {
                try {
                    serverPort = std::stoi(line.substr(5));
                } catch (...) {}
            } else if (line.rfind("name=", 0) == 0) {
                myName = line.substr(5);
            }
        }
    }
}

void NetworkManager::saveConfig() {
    std::ofstream out("network.cfg");
    if (out.is_open()) {
        out << "host=" << serverHost << "\n";
        out << "port=" << serverPort << "\n";
        out << "name=" << myName << "\n";
    }
}

bool NetworkManager::connectToServer(const std::string& host, int port) {
    disconnect();

    serverHost = host;
    serverPort = port;
    saveConfig();

    currentState = NET_CONNECTING;

    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    std::string portStr = std::to_string(port);
    if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res) != 0 || !res) {
        currentState = NET_ERROR;
        lastErrorMessage = "Unable to resolve server host: " + host;
        return false;
    }

    sockFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockFd < 0) {
        freeaddrinfo(res);
        currentState = NET_ERROR;
        lastErrorMessage = "Failed to create network socket";
        return false;
    }

    if (connect(sockFd, res->ai_addr, res->ai_addrlen) < 0) {
        freeaddrinfo(res);
        CLOSE_SOCKET(sockFd);
        sockFd = -1;
        currentState = NET_ERROR;
        lastErrorMessage = "Cannot connect to server at " + host + ":" + portStr;
        return false;
    }
    freeaddrinfo(res);

    currentState = NET_CONNECTED;
    isRunning = true;
    workerThread = std::thread(&NetworkManager::runNetworkLoop, this);

    NetEvent ev;
    ev.type = NET_EVENT_CONNECTED;
    ev.message = "Connected to server!";
    pushEvent(ev);

    return true;
}

void NetworkManager::disconnect() {
    isRunning = false;
    if (workerThread.joinable()) {
        workerThread.join();
    }
    if (sockFd >= 0) {
        CLOSE_SOCKET(sockFd);
        sockFd = -1;
    }
    currentState = NET_DISCONNECTED;
}

bool NetworkManager::isConnected() const {
    return (currentState >= NET_CONNECTED && sockFd >= 0);
}

void NetworkManager::sendRaw(const std::string& rawLine) {
    if (sockFd < 0) return;
    std::string packet = rawLine + "\n";
    send(sockFd, packet.c_str(), (int)packet.length(), 0);
}

void NetworkManager::createRoom(float timeControl, COLOR preferredColor, const std::string& playerName) {
    if (!isConnected()) return;
    myName = playerName.empty() ? "Host" : playerName;
    roomTimeControl = timeControl;
    myColor = preferredColor;
    saveConfig();

    std::string colorStr = (preferredColor == PWHITE) ? "white" : "black";
    std::ostringstream ss;
    ss << "{\"type\":\"CREATE\",\"timeControl\":" << (int)timeControl
       << ",\"color\":\"" << colorStr << "\",\"name\":\"" << myName << "\"}";
    sendRaw(ss.str());
}

void NetworkManager::joinRoom(const std::string& roomCode, const std::string& playerName) {
    if (!isConnected()) return;
    myName = playerName.empty() ? "Guest" : playerName;
    saveConfig();

    std::ostringstream ss;
    ss << "{\"type\":\"JOIN\",\"roomCode\":\"" << roomCode << "\",\"name\":\"" << myName << "\"}";
    sendRaw(ss.str());
}

void NetworkManager::sendMove(int fromRow, int fromCol, int toRow, int toCol,
    const std::string& san, char promo, float time) {
    if (!isConnected()) return;

    std::ostringstream ss;
    ss << "{\"type\":\"MOVE\",\"fromRow\":" << fromRow
       << ",\"fromCol\":" << fromCol
       << ",\"toRow\":" << toRow
       << ",\"toCol\":" << toCol
       << ",\"san\":\"" << san << "\"";
    if (promo != '\0') ss << ",\"promo\":\"" << promo << "\"";
    ss << ",\"time\":" << time << "}";
    sendRaw(ss.str());
}

void NetworkManager::sendDrawOffer() {
    sendRaw("{\"type\":\"DRAW_OFFER\"}");
}

void NetworkManager::sendDrawAccept() {
    sendRaw("{\"type\":\"DRAW_ACCEPT\"}");
}

void NetworkManager::sendDrawDecline() {
    sendRaw("{\"type\":\"DRAW_DECLINE\"}");
}

void NetworkManager::sendResign() {
    sendRaw("{\"type\":\"RESIGN\"}");
}

void NetworkManager::sendRematchOffer() {
    sendRaw("{\"type\":\"REMATCH_OFFER\"}");
}

void NetworkManager::sendRematchAccept() {
    sendRaw("{\"type\":\"REMATCH_ACCEPT\"}");
}

void NetworkManager::sendChat(const std::string& text) {
    std::ostringstream ss;
    ss << "{\"type\":\"CHAT\",\"text\":\"" << text << "\"}";
    sendRaw(ss.str());
}

void NetworkManager::pushEvent(const NetEvent& ev) {
    std::lock_guard<std::mutex> lock(queueMutex);
    eventQueue.push(ev);
}

bool NetworkManager::pollEvent(NetEvent& outEvent) {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (eventQueue.empty()) return false;
    outEvent = eventQueue.front();
    eventQueue.pop();
    return true;
}

void NetworkManager::runNetworkLoop() {
    char buf[4096];
    std::string streamBuffer;
    lastPingSentTime = getCurrentTimeMs();

    while (isRunning) {
#ifndef _WIN32
        struct pollfd pfd;
        pfd.fd = sockFd;
        pfd.events = POLLIN;
        int pollRes = poll(&pfd, 1, 100);
        if (pollRes < 0) break;
        if (pollRes == 0) {
            // Check ping timer
            int64_t now = getCurrentTimeMs();
            if (now - lastPingSentTime > 3000) {
                lastPingSentTime = now;
                std::ostringstream pingMsg;
                pingMsg << "{\"type\":\"PING\",\"t\":" << now << "}";
                sendRaw(pingMsg.str());
            }
            continue;
        }
#endif

        int bytes = (int)recv(sockFd, buf, sizeof(buf) - 1, 0);
        if (bytes <= 0) {
            // Disconnected
            break;
        }
        buf[bytes] = '\0';
        streamBuffer.append(buf, bytes);

        size_t newlinePos;
        while ((newlinePos = streamBuffer.find('\n')) != std::string::npos) {
            std::string line = streamBuffer.substr(0, newlinePos);
            streamBuffer.erase(0, newlinePos + 1);

            // Trim CR if CRLF
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!line.empty()) {
                processLine(line);
            }
        }

        // Periodic ping
        int64_t now = getCurrentTimeMs();
        if (now - lastPingSentTime > 3000) {
            lastPingSentTime = now;
            std::ostringstream pingMsg;
            pingMsg << "{\"type\":\"PING\",\"t\":" << now << "}";
            sendRaw(pingMsg.str());
        }
    }

    if (currentState != NET_DISCONNECTED) {
        currentState = NET_DISCONNECTED;
        NetEvent ev;
        ev.type = NET_EVENT_OPPONENT_DISCONNECTED;
        ev.message = "Disconnected from server";
        pushEvent(ev);
    }
}

void NetworkManager::processLine(const std::string& line) {
    std::string type = extractJsonString(line, "type");
    if (type.empty()) return;

    if (type == "ROOM_CREATED") {
        currentRoomCode = extractJsonString(line, "roomCode");
        std::string col = extractJsonString(line, "color");
        myColor = (col == "black") ? PBLACK : PWHITE;
        roomTimeControl = (float)extractJsonNumber(line, "timeControl", 600.0);
        currentState = NET_WAITING_FOR_OPPONENT;

        NetEvent ev;
        ev.type = NET_EVENT_ROOM_CREATED;
        ev.roomCode = currentRoomCode;
        ev.color = myColor;
        ev.timeControl = roomTimeControl;
        pushEvent(ev);
    }
    else if (type == "ROOM_JOINED") {
        currentRoomCode = extractJsonString(line, "roomCode");
        std::string col = extractJsonString(line, "color");
        myColor = (col == "black") ? PBLACK : PWHITE;
        roomTimeControl = (float)extractJsonNumber(line, "timeControl", 600.0);
        opponentName = extractJsonString(line, "opponent");
        if (opponentName.empty()) opponentName = "Host";
        currentState = NET_PLAYING;

        NetEvent ev;
        ev.type = NET_EVENT_ROOM_JOINED;
        ev.roomCode = currentRoomCode;
        ev.color = myColor;
        ev.timeControl = roomTimeControl;
        ev.message = opponentName;
        pushEvent(ev);
    }
    else if (type == "OPPONENT_JOINED") {
        opponentName = extractJsonString(line, "opponent");
        if (opponentName.empty()) opponentName = "Guest";
        currentState = NET_PLAYING;

        NetEvent ev;
        ev.type = NET_EVENT_OPPONENT_JOINED;
        ev.message = opponentName;
        pushEvent(ev);
    }
    else if (type == "MOVE") {
        NetEvent ev;
        ev.type = NET_EVENT_OPPONENT_MOVE;
        ev.move.fromRow = (int)extractJsonNumber(line, "fromRow", -1);
        ev.move.fromCol = (int)extractJsonNumber(line, "fromCol", -1);
        ev.move.toRow = (int)extractJsonNumber(line, "toRow", -1);
        ev.move.toCol = (int)extractJsonNumber(line, "toCol", -1);
        ev.move.san = extractJsonString(line, "san");
        std::string pStr = extractJsonString(line, "promo");
        ev.move.promo = pStr.empty() ? '\0' : pStr[0];
        ev.move.time = (float)extractJsonNumber(line, "time", 0.0);
        pushEvent(ev);
    }
    else if (type == "DRAW_OFFERED") {
        NetEvent ev;
        ev.type = NET_EVENT_DRAW_OFFERED;
        pushEvent(ev);
    }
    else if (type == "DRAW_ACCEPTED") {
        NetEvent ev;
        ev.type = NET_EVENT_DRAW_ACCEPTED;
        pushEvent(ev);
    }
    else if (type == "DRAW_DECLINED") {
        NetEvent ev;
        ev.type = NET_EVENT_DRAW_DECLINED;
        pushEvent(ev);
    }
    else if (type == "OPPONENT_RESIGNED") {
        NetEvent ev;
        ev.type = NET_EVENT_OPPONENT_RESIGNED;
        pushEvent(ev);
    }
    else if (type == "REMATCH_OFFERED") {
        NetEvent ev;
        ev.type = NET_EVENT_REMATCH_OFFERED;
        pushEvent(ev);
    }
    else if (type == "REMATCH_STARTED") {
        std::string col = extractJsonString(line, "color");
        myColor = (col == "black") ? PBLACK : PWHITE;
        opponentName = extractJsonString(line, "opponent");
        currentState = NET_PLAYING;

        NetEvent ev;
        ev.type = NET_EVENT_REMATCH_STARTED;
        ev.color = myColor;
        ev.message = opponentName;
        pushEvent(ev);
    }
    else if (type == "OPPONENT_DISCONNECTED") {
        NetEvent ev;
        ev.type = NET_EVENT_OPPONENT_DISCONNECTED;
        pushEvent(ev);
    }
    else if (type == "CHAT") {
        NetEvent ev;
        ev.type = NET_EVENT_CHAT;
        ev.message = extractJsonString(line, "text");
        pushEvent(ev);
    }
    else if (type == "PONG") {
        int64_t sentT = (int64_t)extractJsonNumber(line, "t", 0);
        if (sentT > 0) {
            int64_t diff = getCurrentTimeMs() - sentT;
            if (diff >= 0 && diff < 5000) {
                pingMs = (int)diff;
            }
        }
    }
    else if (type == "ERROR") {
        lastErrorMessage = extractJsonString(line, "message");
        NetEvent ev;
        ev.type = NET_EVENT_ERROR;
        ev.message = lastErrorMessage;
        pushEvent(ev);
    }
}
