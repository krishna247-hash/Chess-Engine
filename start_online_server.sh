#!/bin/bash
# ========================================================
# Chess Engine - 1-Click Worldwide Multiplayer Launcher
# Works across DIFFERENT WI-FI anywhere in the world!
# ========================================================

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

python3 server/launch_worldwide.py
