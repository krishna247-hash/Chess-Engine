#!/usr/bin/env python3
import os
import sys
import time
import subprocess
import re
import signal
import socket

def is_port_in_use(port):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        return s.connect_ex(('127.0.0.1', port)) == 0

def kill_process_on_port(port):
    try:
        out = subprocess.check_output(['lsof', '-ti', f':{port}'], text=True).strip()
        for pid_str in out.split():
            try:
                os.kill(int(pid_str), signal.SIGKILL)
            except Exception:
                pass
    except Exception:
        pass

def main():
    print("\033[1;36m" + "=" * 65)
    print("   ♟️  STARTING WORLDWIDE CHESS MULTIPLAYER SERVER (DIFFERENT WI-FI)")
    print("=" * 65 + "\033[0m")

    # 1. Start node server.js if not already running
    node_proc = None
    if not is_port_in_use(4000):
        print("▶ Starting Chess Game Server on port 4000...")
        node_script = os.path.join(os.path.dirname(__file__), "server.js")
        node_proc = subprocess.Popen(["node", node_script], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        time.sleep(1)
        if not is_port_in_use(4000):
            print("\033[1;31mError: Could not start local server on port 4000.\033[0m")
            sys.exit(1)
    else:
        print("✔ Local game server is already active on port 4000.")

    # 2. Check bore
    bore_path = None
    for p in ["/opt/homebrew/bin/bore", "/usr/local/bin/bore", "bore"]:
        try:
            res = subprocess.run([p, "--version"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            if res.returncode == 0:
                bore_path = p
                break
        except Exception:
            continue

    if not bore_path:
        print("\033[1;31mError: 'bore' is not installed. Install via: brew install bore-cli\033[0m")
        if node_proc:
            node_proc.terminate()
        sys.exit(1)

    print("▶ Connecting to global network (bore.pub)...")
    bore_proc = subprocess.Popen([bore_path, "local", "4000", "--to", "bore.pub"],
                                 stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    remote_port = None
    start_t = time.time()
    while time.time() - start_t < 10:
        line = bore_proc.stdout.readline()
        if not line:
            break
        m = re.search(r'listening at bore\.pub:(\d+)', line)
        if m:
            remote_port = m.group(1)
            break

    if not remote_port:
        print("\033[1;31mError: Failed to obtain public tunnel from bore.pub.\033[0m")
        bore_proc.terminate()
        if node_proc:
            node_proc.terminate()
        sys.exit(1)

    print("\n\033[1;32m" + "━" * 65)
    print("   🌐 WORLDWIDE SERVER IS READY FOR PLAYERS ON DIFFERENT WI-FI!")
    print("━" * 65 + "\033[0m")
    print(f"\n  Give these settings to your friend to enter in \033[1m'Server Settings'\033[0m:")
    print(f"  ┌──────────────────────────────────────────────────────────┐")
    print(f"  │  \033[1;33mSERVER HOST\033[0m : \033[1;37mbore.pub\033[0m                                  │")
    print(f"  │  \033[1;33mPORT\033[0m        : \033[1;37m{remote_port:<42}\033[0m│")
    print(f"  └──────────────────────────────────────────────────────────┘")
    print("\n  \033[1mHow to play together:\033[0m")
    print(f"  1. Friend opens ChessGame ➔ 'Play Online' ➔ 'Server Settings' tab.")
    print(f"     ➔ Host: bore.pub  |  Port: {remote_port}")
    print(f"     ➔ Click '⚡ Test Server Connection' (turns green SUCCESS).")
    print(f"  2. You click 'CREATE GAME ROOM' and get a 5-letter code (e.g. P5GUG).")
    print(f"  3. Friend goes to 'Join Room' tab, pastes your 5-letter code, and clicks JOIN MATCH!")
    print("\033[1;32m" + "━" * 65 + "\033[0m")
    print("  \033[90m(Keep this terminal window open while playing. Press Ctrl+C to stop)\033[0m\n")

    def handle_sig(sig, frame):
        print("\nStopping worldwide server...")
        bore_proc.terminate()
        if node_proc:
            node_proc.terminate()
        sys.exit(0)

    signal.signal(signal.SIGINT, handle_sig)
    signal.signal(signal.SIGTERM, handle_sig)

    try:
        bore_proc.wait()
    except KeyboardInterrupt:
        handle_sig(None, None)

if __name__ == "__main__":
    main()
