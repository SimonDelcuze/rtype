import socket
import struct
import time
import argparse
import sys
from enum import IntFlag

DEFAULT_SERVER_IP = "127.0.0.1"
DEFAULT_SERVER_PORT = 12345
DEFAULT_PLAYER_ID = 1

class MessageType:
    CLIENT_TO_SERVER = 0x01
    SERVER_TO_CLIENT = 0x02

class InputFlag(IntFlag):
    NONE = 0x0000
    MOVE_UP = 0x0001
    MOVE_DOWN = 0x0002
    MOVE_LEFT = 0x0004
    MOVE_RIGHT = 0x0008
    FIRE = 0x0010

def crc32(data):
    crc = 0xFFFFFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            lsb = crc & 1
            crc >>= 1
            if lsb:
                crc ^= 0xEDB88320
    return (~crc) & 0xFFFFFFFF

class RTypeTestClient:
    def __init__(self, server_ip, server_port, player_id):
        self.server_ip = server_ip
        self.server_port = server_port
        self.player_id = player_id
        self.sequence_id = 0
        self.tick_id = 0
        self.x = 0.0
        self.y = 0.0
        self.angle = 0.0
        
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.settimeout(1.0)
        print(f"\n[INIT] Client initialized (playerId={player_id}, server={server_ip}:{server_port})")
    
    def _encode_header(self, message_type, payload_size):
        header = bytearray()
        header.extend(struct.pack('4B', 0xA3, 0x5F, 0xC8, 0x1D))
        header.append(1)
        header.append(MessageType.CLIENT_TO_SERVER)
        header.append(message_type)
        header.extend(struct.pack('>H', self.sequence_id & 0xFFFF))
        header.extend(struct.pack('>I', self.tick_id & 0xFFFFFFFF))
        header.extend(struct.pack('>H', payload_size & 0xFFFF))
        return bytes(header)
    
    def _encode_input_payload(self, flags):
        payload = bytearray()
        payload.extend(struct.pack('>I', self.player_id & 0xFFFFFFFF))
        payload.extend(struct.pack('>H', int(flags) & 0xFFFF))
        payload.extend(struct.pack('>f', self.x))
        payload.extend(struct.pack('>f', self.y))
        payload.extend(struct.pack('>f', self.angle))
        return bytes(payload)
    
    def send_input(self, flags, description=""):
        MESSAGE_TYPE_INPUT = 0x05
        payload = self._encode_input_payload(flags)
        header = self._encode_header(MESSAGE_TYPE_INPUT, len(payload))
        
        packet_without_crc = header + payload
        crc = crc32(packet_without_crc)
        crc_bytes = struct.pack('>I', crc)
        
        full_packet = packet_without_crc + crc_bytes
        
        self.socket.sendto(full_packet, (self.server_ip, self.server_port))
        print(f"[SEND] seq={self.sequence_id} tick={self.tick_id} flags=0x{int(flags):04x} {description}")
        
        self.sequence_id += 1
        self.tick_id += 1
    
    def receive_packet(self, timeout=0.5):
        old_timeout = self.socket.gettimeout()
        self.socket.settimeout(timeout)
        try:
            data, _ = self.socket.recvfrom(4096)
            self.socket.settimeout(old_timeout)
            return data
        except socket.timeout:
            self.socket.settimeout(old_timeout)
            return None
    
    def decode_snapshot(self, data):
        if len(data) < 19:
            print(f"[RECV] Invalid packet (too short: {len(data)} bytes)")
            return
        
        print(f"[RECV] SNAPSHOT packet ({len(data)} bytes)")
    
    def close(self):
        self.socket.close()
        print("\n[CLOSE] Client closed")

def test_basic_movement(client):
    print("═" * 70)
    print("TEST 1: Basic movement")
    print("═" * 70)
    print()
    
    movements = [
        (InputFlag.MOVE_LEFT, "→ Move left", 1.0),
        (InputFlag.MOVE_RIGHT, "→ Move right", 1.0),
        (InputFlag.MOVE_UP, "↑ Move up", 1.0),
        (InputFlag.MOVE_DOWN, "↓ Move down", 1.0),
    ]
    
    for flags, description, duration in movements:
        client.send_input(flags, description)
        
        response = client.receive_packet()
        if response:
            client.decode_snapshot(response)
        
        print()
        time.sleep(duration)

def test_diagonal_movement(client):
    print("═" * 70)
    print("TEST 2: Diagonal movement")
    print("═" * 70)
    print()
    
    diagonals = [
        (InputFlag.MOVE_UP | InputFlag.MOVE_RIGHT, "↗ Up-Right"),
        (InputFlag.MOVE_UP | InputFlag.MOVE_LEFT, "↖ Up-Left"),
        (InputFlag.MOVE_DOWN | InputFlag.MOVE_RIGHT, "↘ Down-Right"),
        (InputFlag.MOVE_DOWN | InputFlag.MOVE_LEFT, "↙ Down-Left"),
    ]
    
    for flags, description in diagonals:
        client.send_input(flags, description)
        response = client.receive_packet()
        if response:
            client.decode_snapshot(response)
        print()
        time.sleep(0.5)

def test_firing(client):
    print("═" * 70)
    print("TEST 3: Missile firing")
    print("═" * 70)
    print()
    
    import math
    
    angles = [
        (0.0, "→ Right (0°)"),
        (math.pi / 4, "↗ Up-Right (45°)"),
        (math.pi / 2, "↑ Up (90°)"),
        (math.pi, "← Left (180°)"),
    ]
    
    for angle, description in angles:
        client.angle = angle
        client.send_input(InputFlag.FIRE, f"Fire {description}")
        response = client.receive_packet()
        if response:
            client.decode_snapshot(response)
        print()
        time.sleep(0.3)

def test_movement_and_fire(client):
    print("═" * 70)
    print("TEST 4: Movement + Fire")
    print("═" * 70)
    print()
    
    import math
    
    for i in range(5):
        client.angle = math.pi / 4
        flags = InputFlag.MOVE_RIGHT | InputFlag.FIRE
        client.send_input(flags, f"Movement + Fire #{i+1}")
        response = client.receive_packet()
        if response:
            client.decode_snapshot(response)
        print()
        time.sleep(0.2)

def test_rapid_fire(client):
    print("═" * 70)
    print("TEST 5: Rapid fire (10 packets/second)")
    print("═" * 70)
    print()
    
    for i in range(20):
        client.send_input(InputFlag.FIRE, f"Rapid fire #{i+1}")
        time.sleep(0.1)
    
    print()
    print("Waiting for responses...")
    
    for _ in range(5):
        response = client.receive_packet()
        if response:
            client.decode_snapshot(response)

def test_idle(client):
    print("═" * 70)
    print("TEST 6: Idle (timeout test)")
    print("═" * 70)
    print()
    
    print("Waiting 5 seconds without sending packets...")
    print("(Server should detect timeout after 10s by default)")
    
    for i in range(5):
        print(f"  {i+1}/5 seconds elapsed...")
        time.sleep(1.0)
    
    print()
    print("Sending packet to reactivate connection...")
    client.send_input(InputFlag.NONE, "Reactivation")
    response = client.receive_packet()
    if response:
        client.decode_snapshot(response)

def interactive_mode(client):
    print("═" * 70)
    print("INTERACTIVE MODE")
    print("═" * 70)
    print()
    print("Available commands:")
    print("  z/w      Up")
    print("  s        Down")
    print("  q/a      Left")
    print("  d        Right")
    print("  space    Fire")
    print("  combo    Example: 'zd' = up + right")
    print("  quit     Exit")
    print()
    
    while True:
        try:
            cmd = input("Command > ").strip().lower()
            
            if cmd in ['quit', 'q', 'exit']:
                break
            
            flags = InputFlag.NONE
            
            if 'z' in cmd or 'w' in cmd:
                flags |= InputFlag.MOVE_UP
            if 's' in cmd:
                flags |= InputFlag.MOVE_DOWN
            if 'q' in cmd or 'a' in cmd:
                flags |= InputFlag.MOVE_LEFT
            if 'd' in cmd:
                flags |= InputFlag.MOVE_RIGHT
            if ' ' in cmd or 'space' in cmd or 'fire' in cmd:
                flags |= InputFlag.FIRE
            
            if flags != InputFlag.NONE:
                client.send_input(flags, f"Command: {cmd}")
                response = client.receive_packet()
                if response:
                    client.decode_snapshot(response)
                print()
            else:
                print("Invalid command")
                
        except KeyboardInterrupt:
            print("\nInterruption (Ctrl+C)")
            break
        except EOFError:
            break

def main():
    parser = argparse.ArgumentParser(
        description='R-Type Server Test Client',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Usage examples:
  %(prog)s
  %(prog)s --server 192.168.1.100
  %(prog)s --port 54321 --player-id 42
  %(prog)s --interactive
"""
    )
    
    parser.add_argument('--server', default=DEFAULT_SERVER_IP, help=f'Server IP address (default: {DEFAULT_SERVER_IP})')
    parser.add_argument('--port', type=int, default=DEFAULT_SERVER_PORT, help=f'Server UDP port (default: {DEFAULT_SERVER_PORT})')
    parser.add_argument('--player-id', type=int, default=DEFAULT_PLAYER_ID, help=f'Player ID (default: {DEFAULT_PLAYER_ID})')
    parser.add_argument('--interactive', action='store_true', help='Interactive mode')
    
    args = parser.parse_args()
    
    client = RTypeTestClient(args.server, args.port, args.player_id)
    
    try:
        if args.interactive:
            interactive_mode(client)
        else:
            test_basic_movement(client)
            test_diagonal_movement(client)
            test_firing(client)
            test_movement_and_fire(client)
            test_rapid_fire(client)
            test_idle(client)
    except KeyboardInterrupt:
        print("\n\nInterruption (Ctrl+C)")
    finally:
        client.close()

if __name__ == '__main__':
    main()
