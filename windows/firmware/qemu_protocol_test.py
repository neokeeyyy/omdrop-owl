"""Exercise the OWLR UART endpoint in Espressif QEMU without third-party packages."""

from __future__ import annotations

import argparse
import os
import socket
import struct
import subprocess
import sys
import time
from pathlib import Path

MAGIC = b"OWLR"
VERSION = 1
HEADER = struct.Struct("<4sBBHIII")
HELLO = 1
CAPABILITIES = 2
SET_CHANNEL = 3
START_RX = 4
STOP_RX = 5
TX_FRAME = 7
TX_RESULT = 8
SET_MAC = 9
STATUS = 10
STATE_IDLE = 0
STATE_RX = 1
TX_REJECTED = 1


def frame(message_type: int, sequence: int, payload: bytes = b"") -> bytes:
    crc = crc32(payload)
    return HEADER.pack(MAGIC, VERSION, message_type, 0, sequence, len(payload), crc) + payload


def crc32(data: bytes) -> int:
    crc = 0xFFFFFFFF
    for value in data:
        crc ^= value
        for _ in range(8):
            crc = (crc >> 1) ^ (0xEDB88320 if crc & 1 else 0)
    return (~crc) & 0xFFFFFFFF


def parse_frame(buffer: bytearray) -> tuple[dict, bytes]:
    while len(buffer) >= HEADER.size:
        if buffer[:4] != MAGIC:
            del buffer[0]
            continue
        _, version, message_type, flags, sequence, length, expected_crc = HEADER.unpack(
            buffer[: HEADER.size]
        )
        total = HEADER.size + length
        if length > 4096:
            del buffer[0]
            continue
        if len(buffer) < total:
            raise TimeoutError("incomplete OWLR response")
        payload = bytes(buffer[HEADER.size:total])
        del buffer[:total]
        if version != VERSION or crc32(payload) != expected_crc:
            raise AssertionError("invalid OWLR response")
        return (
            {
                "type": message_type,
                "flags": flags,
                "sequence": sequence,
                "length": length,
            },
            payload,
        )
    raise TimeoutError("no OWLR response")


class QemuUart:
    def __init__(self, host: str, port: int, timeout: float) -> None:
        self.socket = socket.create_connection((host, port), timeout)
        self.socket.settimeout(timeout)
        self.buffer = bytearray()

    def send(self, message_type: int, sequence: int, payload: bytes = b"") -> tuple[dict, bytes]:
        self.socket.sendall(frame(message_type, sequence, payload))
        deadline = time.monotonic() + self.socket.gettimeout()
        while time.monotonic() < deadline:
            try:
                self.buffer.extend(self.socket.recv(4096))
            except socket.timeout:
                break
            try:
                return parse_frame(self.buffer)
            except TimeoutError:
                continue
        raise TimeoutError(f"no response to message type {message_type}")

    def close(self) -> None:
        self.socket.close()


def check_response(response: tuple[dict, bytes], message_type: int, sequence: int) -> bytes:
    header, payload = response
    assert header["type"] == message_type, (
        header,
        message_type,
        payload.hex(),
    )
    assert header["sequence"] == sequence, (header, sequence)
    return payload


def run(project_dir: Path, port: int, timeout: float) -> None:
    build_dir = project_dir / "build"
    flash = build_dir / "flash_image.bin"
    efuse = build_dir / "qemu_efuse.bin"
    bootloader = build_dir / "bootloader" / "bootloader.bin"
    application = build_dir / "omdrop_radio_firmware.bin"
    partition_table = build_dir / "partition_table" / "partition-table.bin"
    if not flash.exists():
        required = (bootloader, application, partition_table)
        if not all(path.exists() for path in required):
            raise FileNotFoundError("run `idf.py build` before the QEMU test")
        subprocess.run(
            [
                sys.executable,
                "-m",
                "esptool",
                "--chip",
                "esp32s3",
                "merge_bin",
                "--output",
                str(flash),
                "--fill-flash-size",
                "4MB",
                "--flash_mode",
                "dio",
                "--flash_freq",
                "80m",
                "--flash_size",
                "4MB",
                "0x0",
                str(bootloader),
                "0x10000",
                str(application),
                "0x8000",
                str(partition_table),
            ],
            check=True,
        )
    if not efuse.exists():
        efuse.write_bytes(bytes(256))
    qemu = os.environ.get("QEMU_SYSTEM_XTENSA")
    if not qemu:
        qemu = "qemu-system-xtensa"
    command = [
        qemu,
        "-M",
        "esp32s3",
        "-drive",
        f"file={flash},if=mtd,format=raw",
        "-drive",
        f"file={efuse},if=none,format=raw,id=efuse",
        "-global",
        "driver=nvram.esp32s3.efuse,property=drive,value=efuse",
        "-global",
        "driver=timer.esp32s3.timg,property=wdt_disable,value=true",
        "-nic",
        "none",
        "-nographic",
        "-serial",
        f"tcp:127.0.0.1:{port},server=on,wait=off",
    ]
    process = subprocess.Popen(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    uart = None
    try:
        deadline = time.monotonic() + 15
        while time.monotonic() < deadline:
            try:
                uart = QemuUart("127.0.0.1", port, timeout)
                break
            except OSError:
                time.sleep(0.1)
        if uart is None:
            raise TimeoutError("QEMU UART did not open")
        time.sleep(2)
        uart.socket.settimeout(0.1)
        while True:
            try:
                if not uart.socket.recv(4096):
                    break
            except socket.timeout:
                break
        uart.socket.settimeout(timeout)

        sequence = 1
        payload = check_response(
            uart.send(HELLO, sequence, struct.pack("<IH", 0, 4088)),
            CAPABILITIES,
            1,
        )
        assert len(payload) == 18
        assert struct.unpack_from("<I", payload, 4)[0] == 0

        sequence += 1
        payload = check_response(
            uart.send(SET_CHANNEL, sequence, struct.pack("<HBB", 6, 0, 0)),
            STATUS,
            2,
        )
        assert struct.unpack_from("<BH", payload, 0)[:2] == (STATE_IDLE, 6)

        sequence += 1
        payload = check_response(uart.send(START_RX, sequence), STATUS, 3)
        assert struct.unpack_from("<B", payload)[0] == STATE_RX

        sequence += 1
        payload = check_response(uart.send(STATUS, sequence), STATUS, 4)
        assert struct.unpack_from("<B", payload)[0] == STATE_RX

        sequence += 1
        tx_payload = struct.pack("<HbBHH", 6, -40, 1, 4, 0) + b"test"
        payload = check_response(
            uart.send(TX_FRAME, sequence, tx_payload),
            TX_RESULT,
            5,
        )
        assert payload[:2] == bytes((TX_REJECTED, 0))

        sequence += 1
        payload = check_response(
            uart.send(SET_MAC, sequence, bytes.fromhex("021122334455")),
            STATUS,
            6,
        )
        assert payload[0] == STATE_RX

        sequence += 1
        payload = check_response(uart.send(STOP_RX, sequence), STATUS, 7)
        assert payload[0] == STATE_IDLE

        bad = bytearray(frame(STATUS, sequence + 1))
        bad[-1] ^= 0xFF
        uart.socket.sendall(bad)
        time.sleep(0.2)
        payload = check_response(uart.send(STATUS, sequence + 1), STATUS, 8)
        assert payload[0] == STATE_IDLE
        print("qemu_protocol_test: PASS")
    finally:
        if uart is not None:
            uart.close()
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--project-dir", type=Path, default=Path(__file__).parent)
    parser.add_argument("--port", type=int, default=5556)
    parser.add_argument("--timeout", type=float, default=5)
    args = parser.parse_args()
    run(args.project_dir.resolve(), args.port, args.timeout)
