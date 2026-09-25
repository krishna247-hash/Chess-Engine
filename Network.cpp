#include "Network.h"
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cerrno>

#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <net/if.h>

NetworkSession::NetworkSession() {}

NetworkSession::~NetworkSession() {
    stop();
}

static bool parseNetLine(const std::string& line, NetMessage& out) {
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;

    if (cmd == "MOVE") {
        char promo = '-';
        if (!(iss >> out.fromRow >> out.fromCol >> out.toRow >> out.toCol >> promo)) return false;
        out.type = NetMsgType::Move;
        out.promo = (promo == '-') ? '\0' : promo;
        return true;
    }
    if (cmd == "RESIGN") { out.type = NetMsgType::Resign; return true; }
    if (cmd == "DRAWOFFER") { out.type = NetMsgType::DrawOffer; return true; }
    if (cmd == "DRAWACCEPT") { out.type = NetMsgType::DrawAccept; return true; }
    if (cmd == "DRAWDECLINE") { out.type = NetMsgType::DrawDecline; return true; }
    return false;
}

bool NetworkSession::sendLine(const std::string& content) {
    if (connFd < 0) return false;
    std::string line = content + "\n";
    size_t total = 0;
    while (total < line.size()) {
        ssize_t n = send(connFd, line.data() + total, line.size() - total, 0);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) continue;
            return false;
        }
        total += (size_t)n;
    }
    return true;
}

// --- Hosting -----------------------------------------------------------

bool NetworkSession::startHost(int port, std::string& outError) {
    listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) {
        outError = "Failed to create socket";
        return false;
    }

    int opt = 1;
    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((uint16_t)port);

    if (bind(listenFd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        outError = "Could not bind port " + std::to_string(port) + " (already in use?)";
        close(listenFd);
        listenFd = -1;
        return false;
    }
    if (listen(listenFd, 1) < 0) {
        outError = "Could not listen on the socket";
        close(listenFd);
        listenFd = -1;
        return false;
    }

    int flags = fcntl(listenFd, F_GETFL, 0);
    fcntl(listenFd, F_SETFL, flags | O_NONBLOCK);
    return true;
}

bool NetworkSession::pollAccept() {
    if (connected.load()) return true;
    if (listenFd < 0) return false;

    sockaddr_in peer{};
    socklen_t len = sizeof(peer);
    int fd = accept(listenFd, (sockaddr*)&peer, &len);
    if (fd < 0) return false;

    int opt = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

    connFd = fd;
    close(listenFd);
    listenFd = -1;
    connected.store(true);
    return true;
}

std::string NetworkSession::getLocalAddressHint() {
    struct ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) != 0) return "";

    std::string result;
    for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
        if (ifa->ifa_flags & IFF_LOOPBACK) continue;
        if (!(ifa->ifa_flags & IFF_UP)) continue;

        char buf[INET_ADDRSTRLEN];
        void* addrPtr = &((sockaddr_in*)ifa->ifa_addr)->sin_addr;
        if (inet_ntop(AF_INET, addrPtr, buf, sizeof(buf))) {
            result = buf;
            break;
        }
    }
    freeifaddrs(ifaddr);
    return result;
}

// --- Joining -------------------------------------------------------------

void NetworkSession::beginConnect(const std::string& host, int port) {
    cancelConnectRequested.store(false);
    connectStatus.store(ConnectStatus::Pending);

    connectThread = std::thread([this, host, port]() {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            std::lock_guard<std::mutex> lk(connectErrorMutex);
            connectError = "Failed to create socket";
            connectStatus.store(ConnectStatus::Failed);
            return;
        }

        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        addrinfo* res = nullptr;
        std::string portStr = std::to_string(port);
        int rc = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res);
        if (rc != 0 || res == nullptr) {
            std::lock_guard<std::mutex> lk(connectErrorMutex);
            connectError = std::string("Could not resolve address: ") + gai_strerror(rc);
            connectStatus.store(ConnectStatus::Failed);
            close(fd);
            return;
        }

        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);

        int cr = connect(fd, res->ai_addr, res->ai_addrlen);
        freeaddrinfo(res);

        bool ok = false;
        if (cr == 0) {
            ok = true;
        } else if (errno == EINPROGRESS) {
            const int timeoutMs = 8000;
            const int stepMs = 100;
            int waited = 0;
            while (waited < timeoutMs) {
                if (cancelConnectRequested.load()) { close(fd); return; }

                fd_set wfds;
                FD_ZERO(&wfds);
                FD_SET(fd, &wfds);
                timeval tv{ 0, stepMs * 1000 };
                int sr = select(fd + 1, nullptr, &wfds, nullptr, &tv);
                if (sr > 0) {
                    int soerr = 0;
                    socklen_t slen = sizeof(soerr);
                    getsockopt(fd, SOL_SOCKET, SO_ERROR, &soerr, &slen);
                    ok = (soerr == 0);
                    break;
                }
                waited += stepMs;
            }
        }

        if (cancelConnectRequested.load()) { close(fd); return; }

        if (!ok) {
            std::lock_guard<std::mutex> lk(connectErrorMutex);
            connectError = "Could not connect (connection refused or timed out)";
            connectStatus.store(ConnectStatus::Failed);
            close(fd);
            return;
        }

        int fflags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, fflags & ~O_NONBLOCK);
        int opt = 1;
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

        connFd = fd;
        connected.store(true);
        connectStatus.store(ConnectStatus::Success);
    });
}

ConnectStatus NetworkSession::pollConnect(std::string& outError) {
    ConnectStatus s = connectStatus.load();
    if (s == ConnectStatus::Failed) {
        std::lock_guard<std::mutex> lk(connectErrorMutex);
        outError = connectError;
    }
    if (s != ConnectStatus::Pending && s != ConnectStatus::Idle && connectThread.joinable()) {
        connectThread.join();
    }
    return s;
}

void NetworkSession::cancelConnect() {
    cancelConnectRequested.store(true);
    if (connectThread.joinable()) connectThread.join();
    connectStatus.store(ConnectStatus::Idle);
}

// --- Handshake -------------------------------------------------------------

bool NetworkSession::sendHandshake(char assignedColorChar, float timeControlSeconds) {
    std::ostringstream oss;
    oss << "HELLO " << assignedColorChar << " " << timeControlSeconds;
    return sendLine(oss.str());
}

bool NetworkSession::receiveHandshake(char& outAssignedColorChar, float& outTimeControlSeconds, std::string& outError) {
    if (connFd < 0) {
        outError = "Not connected";
        return false;
    }

    timeval tv{ 5, 0 };
    setsockopt(connFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    std::string buf;
    char c;
    bool gotLine = false;
    while (buf.size() < 128) {
        ssize_t n = recv(connFd, &c, 1, 0);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) continue;
            break;
        }
        if (c == '\n') { gotLine = true; break; }
        buf.push_back(c);
    }

    tv = { 0, 0 };
    setsockopt(connFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    if (!gotLine) {
        outError = "Timed out waiting for the host";
        return false;
    }

    std::istringstream iss(buf);
    std::string tag;
    char colorChar = 'W';
    float t = -1.0f;
    if (!(iss >> tag >> colorChar >> t) || tag != "HELLO") {
        outError = "Received an unexpected reply from the host";
        return false;
    }

    outAssignedColorChar = colorChar;
    outTimeControlSeconds = t;
    return true;
}

// --- Gameplay messages -----------------------------------------------------

void NetworkSession::startMessageLoop() {
    if (recvThreadStarted) return;
    recvThreadStarted = true;
    recvThread = std::thread(&NetworkSession::recvLoop, this);
}

void NetworkSession::recvLoop() {
    std::string buf;
    char chunk[512];

    while (true) {
        ssize_t n = recv(connFd, chunk, sizeof(chunk), 0);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) continue;
            NetMessage m;
            m.type = NetMsgType::Disconnected;
            {
                std::lock_guard<std::mutex> lk(queueMutex);
                incoming.push_back(m);
            }
            connected.store(false);
            return;
        }

        buf.append(chunk, (size_t)n);
        size_t pos;
        while ((pos = buf.find('\n')) != std::string::npos) {
            std::string line = buf.substr(0, pos);
            buf.erase(0, pos + 1);
            NetMessage msg;
            if (parseNetLine(line, msg)) {
                std::lock_guard<std::mutex> lk(queueMutex);
                incoming.push_back(msg);
            }
        }
    }
}

void NetworkSession::sendMove(int fromRow, int fromCol, int toRow, int toCol, char promo) {
    char p = promo ? promo : '-';
    std::ostringstream oss;
    oss << "MOVE " << fromRow << " " << fromCol << " " << toRow << " " << toCol << " " << p;
    sendLine(oss.str());
}

void NetworkSession::sendResign() { sendLine("RESIGN"); }
void NetworkSession::sendDrawOffer() { sendLine("DRAWOFFER"); }
void NetworkSession::sendDrawAccept() { sendLine("DRAWACCEPT"); }
void NetworkSession::sendDrawDecline() { sendLine("DRAWDECLINE"); }

bool NetworkSession::poll(NetMessage& out) {
    std::lock_guard<std::mutex> lk(queueMutex);
    if (incoming.empty()) return false;
    out = incoming.front();
    incoming.pop_front();
    return true;
}

// --- Teardown ----------------------------------------------------------

void NetworkSession::stop() {
    cancelConnectRequested.store(true);
    if (connectThread.joinable()) connectThread.join();

    connected.store(false);

    if (connFd >= 0) {
        shutdown(connFd, SHUT_RDWR);
    }
    if (recvThread.joinable()) recvThread.join();
    recvThreadStarted = false;

    if (connFd >= 0) { close(connFd); connFd = -1; }
    if (listenFd >= 0) { close(listenFd); listenFd = -1; }

    std::lock_guard<std::mutex> lk(queueMutex);
    incoming.clear();
}
