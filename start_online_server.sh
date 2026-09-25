#!/bin/bash
# ========================================================
# Chess Engine - 1-Click Worldwide Multiplayer Launcher
# ========================================================

echo "===================================================="
echo "  Starting Chess Multiplayer Cloud Server..."
echo "===================================================="

# Kill any existing server on port 4000
lsof -ti:4000 | xargs kill -9 2>/dev/null

# Start local server in background
node server/server.js &
SERVER_PID=$!
sleep 1

# Start public TCP tunnel
echo ""
echo "Connecting to Global Game Network (bore.pub)..."
echo "===================================================="
echo "  Share these Server Settings with your friend:"
echo "===================================================="

bore local 4000 --to bore.pub
