# 🌐 Chess Engine Multiplayer Server (Worldwide Online Play)

A lightweight, zero-dependency server that connects players anywhere in the world with room codes, live move synchronization, clocks, and chat.

---

## ⚡ Quick Start (Local / Same Wi-Fi)

### Run with Node.js (Recommended)
```bash
cd server
node server.js
```

### Run with Python 3 (Alternative)
```bash
python3 server/server.py
```

Default port: **`4000`**.

---

## 🌍 Play Anywhere in the World (3 Free Options)

### Option 1: Instant Free Cloud Tunnel (Fastest - 30 seconds!)

You can run the server on your Mac and expose it to the entire world instantly with **no port forwarding**:

1. Start your server:
   ```bash
   node server/server.js
   ```
2. In a second terminal, run **ngrok**:
   ```bash
   brew install ngrok
   ngrok tcp 4000
   ```
3. Ngrok will print an address like:
   `tcp://0.tcp.ngrok.io:14923`
4. In your Chess Game, set the server host to `0.tcp.ngrok.io` and port to `14923`.
5. Anyone in the world can now connect, create rooms, and play!

---

### Option 2: 100% Free 24/7 Cloud Hosting (Render / Railway)

You can host this server 24/7 on the internet for free so your friends can play anytime without your Mac needing to be on:

#### Deploy to Render.com:
1. Fork or push this repository to GitHub.
2. Go to [Render.com](https://render.com) and create a **Free Web Service** (or Background Worker).
3. Connect your repository.
4. Set **Build Command**: `cd server && npm install`
5. Set **Start Command**: `node server/server.js`
6. Click **Deploy**! Render will give you a public host.

#### Deploy to Railway.app:
1. Go to [Railway.app](https://railway.app).
2. Click **New Project** -> **Deploy from GitHub repo**.
3. Select your `Chess-Engine` repository and root directory `/server`.
4. Add a TCP Proxy port in Railway Settings for port `4000`.

---

### Option 3: Free Cloud VPS (Oracle Cloud / AWS Free Tier)
Run on an Ubuntu VPS:
```bash
git clone https://github.com/krishna247-hash/Chess-Engine.git
cd Chess-Engine/server
nohup node server.js > server.log 2>&1 &
```
Open port 4000 in your cloud firewall/security list.
Share your VPS IP address with your friends!

---

## 📡 Network Protocol Specification

Communication is line-delimited JSON (`\n`) over TCP:

| Direction | Packet | Description |
| :--- | :--- | :--- |
| Client -> Server | `{"type":"CREATE","timeControl":600,"name":"Alice","color":"white"}` | Create room |
| Server -> Client | `{"type":"ROOM_CREATED","roomCode":"W8K9F","timeControl":600,"color":"white"}` | Room created |
| Client -> Server | `{"type":"JOIN","roomCode":"W8K9F","name":"Bob"}` | Join room |
| Server -> Client | `{"type":"ROOM_JOINED","roomCode":"W8K9F","timeControl":600,"color":"black","opponent":"Alice"}` | Room joined |
| Server -> Host | `{"type":"OPPONENT_JOINED","opponent":"Bob"}` | Match starts! |
| Client -> Server | `{"type":"MOVE","fromRow":6,"fromCol":4,"toRow":4,"toCol":4,"san":"e4","promo":"Q","time":598.5}` | Broadcast move |
| Client <-> Server| `{"type":"PING","t":12345678}` / `{"type":"PONG","t":12345678}` | Measure latency / keepalive |
| Client -> Server | `{"type":"DRAW_OFFER"}` / `{"type":"RESIGN"}` / `{"type":"REMATCH_OFFER"}` | Game controls |
