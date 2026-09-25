#pragma once

#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include "Board.h"

enum NetworkState {
    NET_DISCONNECTED = 0,
    NET_CONNECTING,
    NET_CONNECTED,
    NET_WAITING_FOR_OPPONENT,
    NET_PLAYING,
    NET_GAME_OVER,
    NET_ERROR
};

enum NetEventType {
    NET_EVENT_CONNECTED,
    NET_EVENT_ROOM_CREATED,
    NET_EVENT_ROOM_JOINED,
    NET_EVENT_OPPONENT_JOINED,
    NET_EVENT_OPPONENT_MOVE,
    NET_EVENT_DRAW_OFFERED,
    NET_EVENT_DRAW_ACCEPTED,
    NET_EVENT_DRAW_DECLINED,
    NET_EVENT_OPPONENT_RESIGNED,
    NET_EVENT_REMATCH_OFFERED,
    NET_EVENT_REMATCH_STARTED,
    NET_EVENT_OPPONENT_DISCONNECTED,
    NET_EVENT_CHAT,
    NET_EVENT_ERROR
};

struct NetMoveData {
    int fromRow = -1;
    int fromCol = -1;
    int toRow = -1;
    int toCol = -1;
    std::string san;
    char promo = '\0';
    float time = 0.0f;
};

struct NetEvent {
    NetEventType type;
    std::string roomCode;
    std::string message;
    COLOR color = PWHITE;
    float timeControl = 600.0f;
    NetMoveData move;
};

class NetworkManager {
public:
    static NetworkManager& getInstance();

    bool connectToServer(const std::string& host, int port);
    void disconnect();
    bool isConnected() const;

    void createRoom(float timeControl, COLOR preferredColor, const std::string& playerName);
    void joinRoom(const std::string& roomCode, const std::string& playerName);
    void sendMove(int fromRow, int fromCol, int toRow, int toCol, const std::string& san, char promo, float time);
    void sendDrawOffer();
    void sendDrawAccept();
    void sendDrawDecline();
    void sendResign();
    void sendRematchOffer();
    void sendRematchAccept();
    void sendChat(const std::string& text);

    bool pollEvent(NetEvent& outEvent);

    NetworkState getState() const { return currentState; }
    const std::string& getRoomCode() const { return currentRoomCode; }
    const std::string& getOpponentName() const { return opponentName; }
    const std::string& getPlayerName() const { return myName; }
    COLOR getMyColor() const { return myColor; }
    float getTimeControl() const { return roomTimeControl; }
    int getPingMs() const { return pingMs.load(); }
    const std::string& getLastError() const { return lastErrorMessage; }

    std::string serverHost = "127.0.0.1";
    int serverPort = 4000;

    void loadConfig();
    void saveConfig();

private:
    NetworkManager();
    ~NetworkManager();
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;

    void runNetworkLoop();
    void sendRaw(const std::string& rawLine);
    void processLine(const std::string& line);
    void pushEvent(const NetEvent& ev);

    int sockFd = -1;
    std::atomic<bool> isRunning{ false };
    std::thread workerThread;

    std::mutex queueMutex;
    std::queue<NetEvent> eventQueue;

    NetworkState currentState = NET_DISCONNECTED;
    std::string currentRoomCode;
    std::string myName = "Player";
    std::string opponentName = "Opponent";
    COLOR myColor = PWHITE;
    float roomTimeControl = 600.0f;
    std::string lastErrorMessage;
    std::atomic<int> pingMs{ 0 };
    int64_t lastPingSentTime = 0;
};
