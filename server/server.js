/**
 * Online Multiplayer Chess Server
 * -------------------------------------------------------------
 * Lightweight, zero-dependency, high-concurrency TCP server for
 * real-time online chess matches anywhere in the world.
 *
 * Runs locally or deployed to cloud platforms (Render, Railway, Fly.io, etc.).
 */

const net = require('net');
const http = require('http');

const TCP_PORT = parseInt(process.env.TCP_PORT || process.env.PORT || '4000', 10);
const HTTP_PORT = parseInt(process.env.HTTP_PORT || '8080', 10);

// Active rooms map: roomCode -> Room Object
const rooms = new Map();

// Helper to generate readable 5-character room code (no ambiguous characters)
function generateRoomCode() {
    const chars = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789';
    let code = '';
    for (let i = 0; i < 5; i++) {
        code += chars.charAt(Math.floor(Math.random() * chars.length));
    }
    return code;
}

// Sends a JSON message with newline delimiter to a client
function sendJSON(socket, obj) {
    if (socket && !socket.destroyed && socket.writable) {
        try {
            socket.write(JSON.stringify(obj) + '\n');
        } catch (err) {
            console.error('[SOCKET WRITE ERROR]', err.message);
        }
    }
}

// TCP Server for Game Clients
const tcpServer = net.createServer((socket) => {
    let clientRoomCode = null;
    let clientColor = null;
    let clientName = 'Player';
    let buffer = '';

    console.log(`[CLIENT CONNECTED] ${socket.remoteAddress}:${socket.remotePort}`);

    socket.on('data', (chunk) => {
        buffer += chunk.toString('utf8');

        // Gracefully handle HTTP health checks from Render / Cloudflare / Go-http-client
        if (buffer.startsWith('GET ') || buffer.startsWith('HEAD ') || buffer.startsWith('POST ') || buffer.startsWith('OPTIONS ')) {
            if (buffer.includes('\r\n\r\n') || buffer.includes('\n\n')) {
                const body = JSON.stringify({
                    status: 'online',
                    service: 'Chess-Engine-Multiplayer-Server',
                    activeRooms: rooms.size,
                    uptime: process.uptime()
                });
                const res = `HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: ${Buffer.byteLength(body)}\r\nConnection: close\r\n\r\n${body}`;
                try {
                    socket.write(res);
                    socket.end();
                } catch (e) {}
                buffer = '';
            }
            return;
        }

        let boundaryIndex;
        while ((boundaryIndex = buffer.indexOf('\n')) !== -1) {
            const rawLine = buffer.slice(0, boundaryIndex).trim();
            buffer = buffer.slice(boundaryIndex + 1);
            if (rawLine.length === 0) continue;

            // Ignore HTTP header lines if any health check headers arrived in chunks
            if (rawLine.startsWith('Host:') || rawLine.startsWith('User-Agent:') || rawLine.startsWith('Accept:') || rawLine.startsWith('Connection:')) {
                continue;
            }

            try {
                const msg = JSON.parse(rawLine);
                handleClientMessage(socket, msg);
            } catch (parseErr) {
                console.error('[INVALID JSON RECEIVED]', rawLine, parseErr.message);
                sendJSON(socket, { type: 'ERROR', message: 'Malformed JSON packet' });
            }
        }
    });

    function handleClientMessage(sock, msg) {
        const type = msg.type;

        switch (type) {
            case 'CREATE': {
                // Host creates a room
                let code = generateRoomCode();
                while (rooms.has(code)) code = generateRoomCode();

                clientName = msg.name || 'Host';
                const timeControl = typeof msg.timeControl === 'number' ? msg.timeControl : 600;
                let preferredColor = (msg.color || 'white').toLowerCase();
                if (preferredColor === 'random') {
                    preferredColor = Math.random() < 0.5 ? 'white' : 'black';
                }

                clientRoomCode = code;
                clientColor = preferredColor;

                const room = {
                    code: code,
                    timeControl: timeControl,
                    players: {
                        [clientColor]: { socket: sock, name: clientName, connected: true }
                    },
                    status: 'WAITING',
                    currentTurn: 'white',
                    createdAt: Date.now()
                };

                rooms.set(code, room);
                console.log(`[ROOM CREATED] Code: ${code} by ${clientName} (${clientColor}, ${timeControl}s)`);

                sendJSON(sock, {
                    type: 'ROOM_CREATED',
                    roomCode: code,
                    color: clientColor,
                    timeControl: timeControl
                });
                break;
            }

            case 'JOIN': {
                const reqCode = (msg.roomCode || '').toUpperCase().trim();
                clientName = msg.name || 'Guest';

                if (!rooms.has(reqCode)) {
                    sendJSON(sock, { type: 'ERROR', message: 'Room not found. Check code and try again.' });
                    return;
                }

                const room = rooms.get(reqCode);
                if (room.status !== 'WAITING') {
                    sendJSON(sock, { type: 'ERROR', message: 'Room is already full or game in progress.' });
                    return;
                }

                // Determine guest color
                const guestColor = room.players.white ? 'black' : 'white';
                const hostColor = guestColor === 'white' ? 'black' : 'white';
                const hostPlayer = room.players[hostColor];

                clientRoomCode = reqCode;
                clientColor = guestColor;

                room.players[guestColor] = { socket: sock, name: clientName, connected: true };
                room.status = 'PLAYING';

                console.log(`[ROOM JOINED] Code: ${reqCode} - ${clientName} (${guestColor}) joined against ${hostPlayer.name} (${hostColor})`);

                // Notify guest
                sendJSON(sock, {
                    type: 'ROOM_JOINED',
                    roomCode: reqCode,
                    color: guestColor,
                    timeControl: room.timeControl,
                    opponent: hostPlayer.name
                });

                // Notify host
                sendJSON(hostPlayer.socket, {
                    type: 'OPPONENT_JOINED',
                    opponent: clientName
                });
                break;
            }

            case 'MOVE': {
                if (!clientRoomCode || !rooms.has(clientRoomCode)) return;
                const room = rooms.get(clientRoomCode);
                const oppColor = clientColor === 'white' ? 'black' : 'white';
                const opp = room.players[oppColor];

                if (opp && opp.socket) {
                    sendJSON(opp.socket, {
                        type: 'MOVE',
                        fromRow: msg.fromRow,
                        fromCol: msg.fromCol,
                        toRow: msg.toRow,
                        toCol: msg.toCol,
                        san: msg.san,
                        promo: msg.promo || '',
                        time: msg.time
                    });
                }
                break;
            }

            case 'DRAW_OFFER': {
                if (!clientRoomCode || !rooms.has(clientRoomCode)) return;
                const room = rooms.get(clientRoomCode);
                const oppColor = clientColor === 'white' ? 'black' : 'white';
                const opp = room.players[oppColor];
                if (opp && opp.socket) {
                    sendJSON(opp.socket, { type: 'DRAW_OFFERED' });
                }
                break;
            }

            case 'DRAW_ACCEPT': {
                if (!clientRoomCode || !rooms.has(clientRoomCode)) return;
                const room = rooms.get(clientRoomCode);
                const oppColor = clientColor === 'white' ? 'black' : 'white';
                const opp = room.players[oppColor];
                if (opp && opp.socket) {
                    sendJSON(opp.socket, { type: 'DRAW_ACCEPTED' });
                }
                break;
            }

            case 'DRAW_DECLINE': {
                if (!clientRoomCode || !rooms.has(clientRoomCode)) return;
                const room = rooms.get(clientRoomCode);
                const oppColor = clientColor === 'white' ? 'black' : 'white';
                const opp = room.players[oppColor];
                if (opp && opp.socket) {
                    sendJSON(opp.socket, { type: 'DRAW_DECLINED' });
                }
                break;
            }

            case 'RESIGN': {
                if (!clientRoomCode || !rooms.has(clientRoomCode)) return;
                const room = rooms.get(clientRoomCode);
                const oppColor = clientColor === 'white' ? 'black' : 'white';
                const opp = room.players[oppColor];
                if (opp && opp.socket) {
                    sendJSON(opp.socket, { type: 'OPPONENT_RESIGNED' });
                }
                break;
            }

            case 'REMATCH_OFFER': {
                if (!clientRoomCode || !rooms.has(clientRoomCode)) return;
                const room = rooms.get(clientRoomCode);
                const oppColor = clientColor === 'white' ? 'black' : 'white';
                const opp = room.players[oppColor];
                if (opp && opp.socket) {
                    sendJSON(opp.socket, { type: 'REMATCH_OFFERED' });
                }
                break;
            }

            case 'REMATCH_ACCEPT': {
                if (!clientRoomCode || !rooms.has(clientRoomCode)) return;
                const room = rooms.get(clientRoomCode);
                // Swap colors on rematch
                const whitePlayer = room.players.white;
                const blackPlayer = room.players.black;
                room.players.white = blackPlayer;
                room.players.black = whitePlayer;

                if (whitePlayer && whitePlayer.socket) {
                    sendJSON(whitePlayer.socket, {
                        type: 'REMATCH_STARTED',
                        color: 'black',
                        opponent: blackPlayer ? blackPlayer.name : 'Opponent'
                    });
                }
                if (blackPlayer && blackPlayer.socket) {
                    sendJSON(blackPlayer.socket, {
                        type: 'REMATCH_STARTED',
                        color: 'white',
                        opponent: whitePlayer ? whitePlayer.name : 'Opponent'
                    });
                }
                break;
            }

            case 'CHAT': {
                if (!clientRoomCode || !rooms.has(clientRoomCode)) return;
                const room = rooms.get(clientRoomCode);
                const oppColor = clientColor === 'white' ? 'black' : 'white';
                const opp = room.players[oppColor];
                if (opp && opp.socket) {
                    sendJSON(opp.socket, { type: 'CHAT', text: msg.text, sender: clientName });
                }
                break;
            }

            case 'PING': {
                sendJSON(sock, { type: 'PONG', t: msg.t });
                break;
            }

            default:
                console.log(`[UNKNOWN MESSAGE] ${type}`);
        }
    }

    socket.on('close', () => {
        console.log(`[CLIENT DISCONNECTED] ${clientName} (${clientRoomCode || 'no room'})`);
        if (clientRoomCode && rooms.has(clientRoomCode)) {
            const room = rooms.get(clientRoomCode);
            const oppColor = clientColor === 'white' ? 'black' : 'white';
            const opp = room.players[oppColor];
            if (opp && opp.socket) {
                sendJSON(opp.socket, { type: 'OPPONENT_DISCONNECTED' });
            }
            rooms.delete(clientRoomCode);
        }
    });

    socket.on('error', (err) => {
        if (err.code === 'EPIPE' || err.code === 'ECONNRESET') return;
        console.error(`[SOCKET ERROR] ${err.message}`);
    });
});

tcpServer.listen(TCP_PORT, '0.0.0.0', () => {
    console.log(`====================================================`);
    console.log(`  Chess Engine Multiplayer Server Running!`);
    console.log(`  TCP Game Port: ${TCP_PORT}`);
    console.log(`  Active Rooms : ${rooms.size}`);
    console.log(`====================================================`);
});

// Optional HTTP Health Check Server (needed by cloud platforms like Render / Railway)
if (HTTP_PORT !== TCP_PORT) {
    const httpServer = http.createServer((req, res) => {
        if (req.url === '/health' || req.url === '/') {
            res.writeHead(200, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify({
                status: 'online',
                service: 'Chess-Engine-Multiplayer-Server',
                activeRooms: rooms.size,
                tcpPort: TCP_PORT,
                uptime: process.uptime()
            }));
        } else {
            res.writeHead(404);
            res.end('Not Found');
        }
    });

    httpServer.listen(HTTP_PORT, '0.0.0.0', () => {
        console.log(`  HTTP Health Check: http://localhost:${HTTP_PORT}/health`);
    });
}
