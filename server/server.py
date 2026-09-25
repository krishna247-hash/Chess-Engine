#!/usr/bin/env python3
"""
Online Multiplayer Chess Server (Python 3 Alternative)
------------------------------------------------------
Zero-dependency, multi-threaded TCP server for online chess matches.
Compatible with the Raylib C++ Chess Client.
"""

import socket
import threading
import json
import random
import os
import sys

TCP_PORT = int(os.environ.get('PORT', os.environ.get('TCP_PORT', 4000)))

# Map of room_code -> room_dict
rooms = {}
rooms_lock = threading.Lock()

def generate_room_code():
    chars = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789'
    return ''.join(random.choice(chars) for _ in range(5))

def send_json(sock, data):
    try:
        payload = (json.dumps(data) + '\n').encode('utf-8')
        sock.sendall(payload)
    except Exception as e:
        print(f"[SEND ERROR] {e}")

def handle_client(sock, addr):
    print(f"[CLIENT CONNECTED] {addr}")
    client_room_code = None
    client_color = None
    client_name = "Player"
    buffer = ""

    try:
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            buffer += chunk.decode('utf-8', errors='replace')
            while '\n' in buffer:
                line, buffer = buffer.split('\n', 1)
                line = line.strip()
                if not line:
                    continue
                try:
                    msg = json.loads(line)
                except Exception as e:
                    print(f"[PARSE ERROR] {e}")
                    continue

                msg_type = msg.get('type')

                if msg_type == 'CREATE':
                    with rooms_lock:
                        code = generate_room_code()
                        while code in rooms:
                            code = generate_room_code()

                        client_name = msg.get('name', 'Host')
                        time_control = msg.get('timeControl', 600)
                        pref_color = msg.get('color', 'white').lower()
                        if pref_color == 'random':
                            pref_color = 'white' if random.random() < 0.5 else 'black'

                        client_room_code = code
                        client_color = pref_color

                        rooms[code] = {
                            'code': code,
                            'timeControl': time_control,
                            'players': {
                                client_color: {'socket': sock, 'name': client_name}
                            },
                            'status': 'WAITING'
                        }

                    print(f"[ROOM CREATED] Code: {code} by {client_name} ({client_color})")
                    send_json(sock, {
                        'type': 'ROOM_CREATED',
                        'roomCode': code,
                        'color': client_color,
                        'timeControl': time_control
                    })

                elif msg_type == 'JOIN':
                    req_code = msg.get('roomCode', '').strip().upper()
                    client_name = msg.get('name', 'Guest')

                    with rooms_lock:
                        if req_code not in rooms:
                            send_json(sock, {'type': 'ERROR', 'message': 'Room not found. Check code.'})
                            continue

                        room = rooms[req_code]
                        if room['status'] != 'WAITING':
                            send_json(sock, {'type': 'ERROR', 'message': 'Room is full or game in progress.'})
                            continue

                        guest_color = 'black' if 'white' in room['players'] else 'white'
                        host_color = 'white' if guest_color == 'black' else 'black'
                        host_player = room['players'][host_color]

                        client_room_code = req_code
                        client_color = guest_color

                        room['players'][guest_color] = {'socket': sock, 'name': client_name}
                        room['status'] = 'PLAYING'

                    print(f"[ROOM JOINED] Code: {req_code} - {client_name} joined vs {host_player['name']}")
                    send_json(sock, {
                        'type': 'ROOM_JOINED',
                        'roomCode': req_code,
                        'color': guest_color,
                        'timeControl': room['timeControl'],
                        'opponent': host_player['name']
                    })
                    send_json(host_player['socket'], {
                        'type': 'OPPONENT_JOINED',
                        'opponent': client_name
                    })

                elif msg_type == 'MOVE':
                    if client_room_code and client_room_code in rooms:
                        room = rooms[client_room_code]
                        opp_color = 'black' if client_color == 'white' else 'white'
                        opp = room['players'].get(opp_color)
                        if opp and opp.get('socket'):
                            send_json(opp['socket'], {
                                'type': 'MOVE',
                                'fromRow': msg.get('fromRow'),
                                'fromCol': msg.get('fromCol'),
                                'toRow': msg.get('toRow'),
                                'toCol': msg.get('toCol'),
                                'san': msg.get('san'),
                                'promo': msg.get('promo', ''),
                                'time': msg.get('time')
                            })

                elif msg_type in ('DRAW_OFFER', 'DRAW_ACCEPT', 'DRAW_DECLINE', 'RESIGN', 'REMATCH_OFFER', 'CHAT', 'PING'):
                    if client_room_code and client_room_code in rooms:
                        room = rooms[client_room_code]
                        opp_color = 'black' if client_color == 'white' else 'white'
                        opp = room['players'].get(opp_color)
                        if msg_type == 'PING':
                            send_json(sock, {'type': 'PONG', 't': msg.get('t')})
                        elif opp and opp.get('socket'):
                            fwd = {'DRAW_OFFER': 'DRAW_OFFERED', 'DRAW_ACCEPT': 'DRAW_ACCEPTED',
                                   'DRAW_DECLINE': 'DRAW_DECLINED', 'RESIGN': 'OPPONENT_RESIGNED',
                                   'REMATCH_OFFER': 'REMATCH_OFFERED'}
                            if msg_type == 'CHAT':
                                send_json(opp['socket'], {'type': 'CHAT', 'text': msg.get('text'), 'sender': client_name})
                            elif msg_type in fwd:
                                send_json(opp['socket'], {'type': fwd[msg_type]})

                elif msg_type == 'REMATCH_ACCEPT':
                    if client_room_code and client_room_code in rooms:
                        room = rooms[client_room_code]
                        w_p = room['players'].get('white')
                        b_p = room['players'].get('black')
                        room['players']['white'] = b_p
                        room['players']['black'] = w_p
                        if w_p and w_p.get('socket'):
                            send_json(w_p['socket'], {'type': 'REMATCH_STARTED', 'color': 'black', 'opponent': b_p['name'] if b_p else 'Opponent'})
                        if b_p and b_p.get('socket'):
                            send_json(b_p['socket'], {'type': 'REMATCH_STARTED', 'color': 'white', 'opponent': w_p['name'] if w_p else 'Opponent'})

    except Exception as e:
        print(f"[CLIENT HANDLER ERROR] {e}")
    finally:
        print(f"[CLIENT DISCONNECTED] {client_name} ({client_room_code or 'none'})")
        if client_room_code and client_room_code in rooms:
            with rooms_lock:
                room = rooms.get(client_room_code)
                if room:
                    opp_color = 'black' if client_color == 'white' else 'white'
                    opp = room['players'].get(opp_color)
                    if opp and opp.get('socket'):
                        send_json(opp['socket'], {'type': 'OPPONENT_DISCONNECTED'})
                    del rooms[client_room_code]
        try:
            sock.close()
        except:
            pass

def main():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(('0.0.0.0', TCP_PORT))
    server.listen(128)
    print("====================================================")
    print("  Python Multiplayer Chess Server Running!")
    print(f"  TCP Game Port: {TCP_PORT}")
    print("====================================================")
    while True:
        try:
            client_sock, client_addr = server.accept()
            t = threading.Thread(target=handle_client, args=(client_sock, client_addr), daemon=True)
            t.start()
        except KeyboardInterrupt:
            break
        except Exception as e:
            print(f"[ACCEPT ERROR] {e}")

if __name__ == '__main__':
    main()
