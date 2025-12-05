#!/usr/bin/env python3
import socket
import struct
import time
import zlib
import json
from pathlib import Path
import random
import struct


def packet_header(payload_size, sequence, tick):
    return bytearray([
        0xA3, 0x5F, 0xC8, 0x1D,
        0x01,
        0x02,
        0x14,
        (sequence >> 8) & 0xFF,
        sequence & 0xFF,
        (tick >> 24) & 0xFF,
        (tick >> 16) & 0xFF,
        (tick >> 8) & 0xFF,
        tick & 0xFF,
        (payload_size >> 8) & 0xFF,
        payload_size & 0xFF,
    ])


def build_snapshot(sequence, tick, entity_id, entity_type, x, y, vx, vy):
    payload = bytearray()
    payload.extend(struct.pack(">H", 1))
    update_mask = 0x1F
    payload.extend(struct.pack(">I", entity_id))
    payload.extend(struct.pack(">H", update_mask))
    payload.append(entity_type)
    payload.extend(struct.pack(">f", x))
    payload.extend(struct.pack(">f", y))
    payload.extend(struct.pack(">f", vx))
    payload.extend(struct.pack(">f", vy))
    header = packet_header(len(payload), sequence, tick)
    crc = zlib.crc32(header + payload) & 0xFFFFFFFF
    pkt = header + payload
    pkt.extend([
        (crc >> 24) & 0xFF,
        (crc >> 16) & 0xFF,
        (crc >> 8) & 0xFF,
        crc & 0xFF,
    ])
    return bytes(pkt)


def parse_header(data: bytes):
    if len(data) < 15:
        return None
    if data[0:4] != bytes([0xA3, 0x5F, 0xC8, 0x1D]):
        return None
    version = data[4]
    if version != 1:
        return None
    return {
        "packetType": data[5],
        "messageType": data[6],
        "sequenceId": (data[7] << 8) | data[8],
        "tickId": (data[9] << 24) | (data[10] << 16) | (data[11] << 8) | data[12],
        "payloadSize": (data[13] << 8) | data[14],
    }


def encode_string(val: str) -> bytes:
    data = val.encode("utf-8")
    return bytes([len(data)]) + data


def crc32(data: bytes) -> int:
    return zlib.crc32(data) & 0xFFFFFFFF


def build_level_init(sequence: int, level_id: int, seed: int, background: str, music: str, archetypes: list) -> bytes:
    payload = bytearray()
    payload.extend(struct.pack(">H", level_id))
    payload.extend(struct.pack(">I", seed))
    payload.extend(encode_string(background))
    payload.extend(encode_string(music))
    payload.append(len(archetypes))
    for entry in archetypes:
        type_id, sprite_id, anim_id, layer = entry
        payload.extend(struct.pack(">H", type_id))
        payload.extend(encode_string(sprite_id))
        payload.extend(encode_string(anim_id))
        payload.append(layer & 0xFF)

    header = packet_header(len(payload), sequence, 0)
    header[6] = 0x30
    hdr_bytes = bytes(header)
    crc_val = crc32(hdr_bytes + payload)
    pkt = bytearray()
    pkt.extend(hdr_bytes)
    pkt.extend(payload)
    pkt.extend([
        (crc_val >> 24) & 0xFF,
        (crc_val >> 16) & 0xFF,
        (crc_val >> 8) & 0xFF,
        crc_val & 0xFF,
    ])
    return bytes(pkt)


MESSAGE = {
    "ClientHello": 0x01,
    "ClientJoinRequest": 0x02,
    "ClientReady": 0x03,
    "ClientPing": 0x04,
    "Snapshot": 0x14,
    "LevelInit": 0x30,
}
PKT_CLIENT = 0x01
PKT_SERVER = 0x02


def main():
    listen_port = 50010
    manifest_path = Path(__file__).resolve().parent.parent / "client" / "assets" / "assets.json"
    type_map = {}
    try:
        with manifest_path.open("r", encoding="utf-8") as f:
            manifest = json.load(f)
            sprites = [tex["id"] for tex in manifest.get("textures", []) if tex.get("type") == "sprite"]
            for idx, sid in enumerate(sprites, start=1):
                type_map[sid] = idx
    except Exception as e:
        print(f"[mock] failed to load manifest: {e}, using default type id 1")
        type_map = {"player_ship": 1, "enemy_ship": 2, "bullet": 3}

    player_type = type_map.get("player_ship") or next(iter(type_map.values()), 1)
    dst = ("127.0.0.1", 50000)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        try:
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
        except Exception:
            pass
        sock.bind(("0.0.0.0", listen_port))
    except OSError as e:
        print(f"[ERROR] bind 0.0.0.0:{listen_port} failed: {e}")
        sock.close()
        return
    print(f"[mock] listening on {sock.getsockname()} sending to {dst} using type {player_type}")
    sock.settimeout(0.5)

    print("[INFO] waiting for client hello/join/ready ...")
    seen = {"hello": False, "join": False, "ready": False}
    while True:
        try:
            data, addr = sock.recvfrom(2048)
        except socket.timeout:
            continue
        hdr = parse_header(data)
        if not hdr:
            continue
        if hdr["packetType"] != PKT_CLIENT:
            continue
        mtype = hdr["messageType"]
        if mtype == MESSAGE["ClientHello"]:
            seen["hello"] = True
            print(f"[RECV] ClientHello {addr} seq={hdr['sequenceId']}")
        elif mtype == MESSAGE["ClientJoinRequest"]:
            seen["join"] = True
            print(f"[RECV] ClientJoinRequest {addr} seq={hdr['sequenceId']}")
        elif mtype == MESSAGE["ClientReady"]:
            seen["ready"] = True
            print(f"[RECV] ClientReady {addr} seq={hdr['sequenceId']}")
        elif mtype == MESSAGE["ClientPing"]:
            pong_hdr          = packet_header(0, hdr["sequenceId"], hdr["tickId"])
            pong_hdr[5]       = PKT_SERVER
            pong_hdr[6]       = MESSAGE["ClientPing"]
            crc_val           = crc32(pong_hdr)
            pong              = bytearray(pong_hdr)
            pong.append((crc_val >> 24) & 0xFF)
            pong.append((crc_val >> 16) & 0xFF)
            pong.append((crc_val >> 8) & 0xFF)
            pong.append(crc_val & 0xFF)
            sock.sendto(bytes(pong), addr)
            print(f"[SEND] Pong -> {addr} seq={hdr['sequenceId']}")
        if all(seen.values()):
            print("[INFO] handshake complete")
            break

    player_variant = 1
    level_seed = random.randint(0, 0xFFFFFFFF)
    level_init = build_level_init(
        sequence=0,
        level_id=1,
        seed=level_seed,
        background="space_background",
        music="theme_music",
        archetypes=[(player_type, "player_ship", f"player{player_variant}", 0)],
    )
    sock.sendto(level_init, dst)

    sequence = 1
    tick = 0
    x = 100.0
    y = 200.0
    vx = 30.0
    vy = 0.0
    while True:
        tick += 1
        x += vx * 0.016
        pkt = build_snapshot(sequence, tick, 1, player_type, x, y, vx, vy)
        sock.sendto(pkt, dst)
        sequence = (sequence + 1) & 0xFFFF
        time.sleep(0.016)


if __name__ == "__main__":
    main()
