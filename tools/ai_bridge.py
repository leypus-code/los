#!/usr/bin/env python3
import os
import socket
import sys
import time

SOCK = "/tmp/los-ai.sock"

def connect_loop():
    while True:
        try:
            s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            s.connect(SOCK)
            return s
        except FileNotFoundError:
            print("[bridge] waiting for /tmp/los-ai.sock ...")
            time.sleep(0.5)
        except ConnectionRefusedError:
            print("[bridge] socket exists but QEMU is not ready yet ...")
            time.sleep(0.5)

def main():
    print("[bridge] LOS AI bridge")
    print("[bridge] start LOS with: make run-ai-bridge")
    print("[bridge] waiting for packets...")

    sock = connect_loop()
    print("[bridge] connected")

    buffer = b""

    while True:
        chunk = sock.recv(1024)
        if not chunk:
            print("[bridge] disconnected")
            break

        buffer += chunk

        while b"\n" in buffer:
            line, buffer = buffer.split(b"\n", 1)
            text = line.decode("utf-8", errors="replace").strip()

            if not text:
                continue

            print(f"[kernel] {text}")

            if text.startswith("LOS_AI|"):
                parts = text.split("|", 2)
                kind = parts[1] if len(parts) > 1 else "unknown"
                payload = parts[2] if len(parts) > 2 else ""

                if kind == "mode":
                    print(f"[ai] provider mode = {payload}")
                elif kind == "load":
                    print(f"[ai] load model requested: {payload}")
                    print("[ai] TODO v24.6: call ollama/llama.cpp here")
                elif kind == "ask":
                    print(f"[ai] prompt: {payload}")
                    print("[ai] TODO v24.6: generate response and send UI patch back")
                else:
                    print(f"[ai] unknown packet kind={kind} payload={payload}")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print()
        print("[bridge] stopped")
        sys.exit(0)
