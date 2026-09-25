#pragma once
#include <string>
#include <deque>
#include <mutex>
#include <thread>
#include <atomic>

// Default TCP port used for online play if the user doesn't specify one.
#define ONLINE_DEFAULT_PORT 5455

// Deliberately independent of the game's own types (Position/Move/COLOR) so
// this file has no dependency on raylib and can be built/tested in
// isolation. Source.cpp converts to/from Position when calling in.
enum class NetMsgType { Move, Resign, DrawOffer, DrawAccept, DrawDecline, Disconnected };

struct NetMessage {
    NetMsgType type = NetMsgType::Move;
    int fromRow = -1, fromCol = -1, toRow = -1, toCol = -1;
    char promo = '\0';
};

enum class ConnectStatus { Idle, Pending, Success, Failed };

// A single peer-to-peer TCP connection used for online chess play. One side
// hosts (startHost + pollAccept), the other joins (beginConnect +
// pollConnect). Once the TCP link is up, a one-line handshake assigns the
// joining player's color and syncs the clock, then both sides exchange
// moves/resign/draw messages as newline-delimited text over the socket.
class NetworkSession {
public:
    NetworkSession();
    ~NetworkSession();

    NetworkSession(const NetworkSession&) = delete;
    NetworkSession& operator=(const NetworkSession&) = delete;

    // --- Hosting ---
    // Opens a non-blocking listening socket on the given port.
    bool startHost(int port, std::string& outError);
    // Call once per frame; returns true the frame a peer connects.
    bool pollAccept();
    // Best-effort local LAN IPv4 address to show the host (empty if unknown).
    static std::string getLocalAddressHint();

    // --- Joining ---
    // Starts an asynchronous connect to host:port (resolves hostnames too).
    void beginConnect(const std::string& host, int port);
    // Call once per frame to check on an async connect started above.
    ConnectStatus pollConnect(std::string& outError);
    // Cancels a pending beginConnect() and cleans up.
    void cancelConnect();

    // --- Handshake --- (call once, right after the TCP connection is up)
    // Host: tells the peer which color they've been assigned and the clock.
    bool sendHandshake(char assignedColorChar, float timeControlSeconds);
    // Join: blocks briefly (a few seconds, bounded) to receive it.
    bool receiveHandshake(char& outAssignedColorChar, float& outTimeControlSeconds, std::string& outError);

    // Starts the background thread that receives gameplay messages. Call
    // once, after the handshake completes.
    void startMessageLoop();

    bool isConnected() const { return connected.load(); }

    // --- Sending gameplay messages ---
    void sendMove(int fromRow, int fromCol, int toRow, int toCol, char promo);
    void sendResign();
    void sendDrawOffer();
    void sendDrawAccept();
    void sendDrawDecline();

    // Non-blocking pop of the next received message. Returns false if none
    // is queued right now.
    bool poll(NetMessage& out);

    // Tears down the connection and stops all threads/sockets. Safe to call
    // more than once.
    void stop();

private:
    int listenFd = -1;
    int connFd = -1;
    std::atomic<bool> connected{ false };

    // Async connect (join side) state
    std::thread connectThread;
    std::atomic<ConnectStatus> connectStatus{ ConnectStatus::Idle };
    std::string connectError;
    std::mutex connectErrorMutex;
    std::atomic<bool> cancelConnectRequested{ false };

    // Background receive thread (gameplay messages, started after handshake)
    std::thread recvThread;
    bool recvThreadStarted = false;
    std::mutex queueMutex;
    std::deque<NetMessage> incoming;

    bool sendLine(const std::string& content);
    void recvLoop();
};
