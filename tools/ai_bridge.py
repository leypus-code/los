#!/usr/bin/env python3
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

def send_ui(sock, kind, payload):
    line = f"LOS_UI|{kind}|{payload}\n"
    sock.sendall(line.encode("utf-8"))
    print(f"[host -> los] {line.strip()}")

def fake_ai_response(sock, prompt):
    print(f"[ai] fake response for: {prompt}")

    send_ui(sock, "state", "thinking")
    time.sleep(0.25)

    send_ui(sock, "state", "drawing")
    time.sleep(0.25)

    if "workspace" in prompt.lower() or "space" in prompt.lower() or "рабоч" in prompt.lower():
        send_ui(sock, "surface", "workspace")
    else:
        send_ui(sock, "surface", "ai")

    time.sleep(0.15)
    send_ui(sock, "state", "ready")

def main():
    print("[bridge] LOS AI bridge v24.6")
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
            raw, buffer = buffer.split(b"\n", 1)
            text = raw.decode("utf-8", errors="replace").strip()

            if not text:
                continue

            print(f"[los -> host] {text}")

            if not text.startswith("LOS_AI|"):
                continue

            parts = text.split("|", 2)
            kind = parts[1] if len(parts) > 1 else "unknown"
            payload = parts[2] if len(parts) > 2 else ""

            if kind == "mode":
                print(f"[ai] provider mode = {payload}")
                if payload == "offline":
                    send_ui(sock, "state", "idle")
                else:
                    send_ui(sock, "state", "ready")

            elif kind == "load":
                print(f"[ai] load model requested: {payload}")
                send_ui(sock, "state", "loading")
                time.sleep(0.5)
                send_ui(sock, "state", "ready")

            elif kind == "ask":
                print(f"[ai] prompt: {payload}")
                fake_ai_response(sock, payload)

            else:
                print(f"[ai] unknown packet kind={kind} payload={payload}")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print()
        print("[bridge] stopped")
        sys.exit(0)
