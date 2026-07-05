#!/usr/bin/env python3
import os
import socket
import sys
import time

from providers.factory import create_provider

SOCK = "/tmp/los-ai.sock"

DEFAULT_PROVIDER = os.environ.get("LOS_AI_PROVIDER", "ollama")
DEFAULT_MODEL = os.environ.get("LOS_AI_MODEL", "qwen2.5:0.5b")


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
    payload = str(payload or "")
    payload = payload.replace("\n", " ").replace("\r", " ")
    line = f"LOS_UI|{kind}|{payload}\n"
    sock.sendall(line.encode("utf-8"))
    print(f"[host -> los] {line.strip()}")


def apply_model_plan(sock, prompt, answer):
    surface_sent = False
    note_sent = False

    lines = []
    for raw in str(answer or "").splitlines():
        line = raw.strip()
        if line:
            lines.append(line)

    if not lines:
        send_ui(sock, "note", "No UI plan returned.")
        send_ui(sock, "state", "ready")
        return

    for line in lines:
        upper = line.upper()

        if upper.startswith("STATE "):
            value = line[6:].strip().lower()
            send_ui(sock, "state", value)
            continue

        if upper.startswith("SURFACE "):
            value = line[8:].strip().lower()
            send_ui(sock, "surface", value)
            surface_sent = True
            continue

        if upper.startswith("NOTE "):
            value = line[5:].strip()
            send_ui(sock, "note", value[:120])
            note_sent = True
            continue

        if upper.startswith("WIDGET "):
            value = line[7:].strip().lower()
            print(f"[plan] widget requested: {value}")
            continue

        if upper.startswith("LAYOUT "):
            value = line[7:].strip().lower()
            print(f"[plan] layout requested: {value}")
            continue

    if not surface_sent:
        if looks_like_workspace_request(prompt, answer):
            send_ui(sock, "surface", "workspace")
        else:
            send_ui(sock, "surface", "ai")

    if not note_sent:
        send_ui(sock, "note", "UI plan applied.")

    send_ui(sock, "state", "ready")



def looks_like_workspace_request(prompt, answer):
    low = (prompt + " " + answer).lower()

    return (
        "workspace" in low
        or "space" in low
        or "coding" in low
        or "terminal" in low
        or "editor" in low
        or "рабоч" in low
        or "терминал" in low
        or "редактор" in low
        or "код" in low
    )


def ai_response(sock, provider, prompt):
    print(f"[ai] provider={provider.name} model={provider.model}")
    print(f"[ai] prompt: {prompt}")

    send_ui(sock, "state", "thinking")
    time.sleep(0.15)

    try:
        answer = provider.ask(prompt)
        if not answer:
            raise RuntimeError("empty provider answer")

        print(f"[{provider.name}] {answer}")

    except Exception as e:
        print(f"[{provider.name}] error: {e}")
        print("[provider] falling back to fake response")

        fallback = create_provider("fake", provider.model)
        answer = fallback.ask(prompt)

    send_ui(sock, "state", "drawing")
    time.sleep(0.15)

    apply_model_plan(sock, prompt, answer)


def main():
    provider_name = DEFAULT_PROVIDER
    model_name = DEFAULT_MODEL

    provider = create_provider(provider_name, model_name)

    print("[bridge] LOS AI bridge v24.8")
    print("[bridge] provider abstraction enabled")
    print("[bridge] start LOS with: make run-ai-bridge")
    print(f"[bridge] provider: {provider.name}")
    print(f"[bridge] model: {provider.model}")

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
                    send_ui(sock, "note", f"Host AI ready. Provider: {provider.name}. Model: {provider.model}")

            elif kind == "load":
                requested = payload.strip()

                if requested:
                    model_name = requested

                print(f"[ai] load model requested: {model_name}")

                send_ui(sock, "state", "loading")
                send_ui(sock, "note", f"Loading model: {model_name}")

                try:
                    message = provider.load_model(model_name)
                except Exception as e:
                    print(f"[provider] load error: {e}")
                    provider = create_provider("fake", model_name)
                    message = provider.load_model(model_name)

                time.sleep(0.35)

                send_ui(sock, "state", "ready")
                send_ui(sock, "note", message)

            elif kind == "ask":
                ai_response(sock, provider, payload)

            else:
                print(f"[ai] unknown packet kind={kind} payload={payload}")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print()
        print("[bridge] stopped")
        sys.exit(0)
