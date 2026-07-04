#!/usr/bin/env python3
import argparse
import json
import socket
import urllib.request

PREFIX_IN = "LOS_AI_REQUEST:"
PREFIX_OUT = "LOS_AI_RESPONSE:"

SYSTEM = """You are the intent router for LOS, an AI-native operating system prototype.
Return exactly one short LOS intent command, no explanation.

Allowed outputs:
home
build dashboard
coding mode
blank canvas
reset home
add weather
remove weather
add checklist
remove checklist
add logs panel
remove logs panel
debug build error
system overview
write notes
plan project

Choose the closest output for the user request.
"""

def mock_route(prompt: str) -> str:
    p = prompt.lower()

    if "weather" in p or "погод" in p:
        return "add weather"
    if "check" in p or "todo" in p or "чек" in p:
        return "add checklist"
    if "log" in p or "лог" in p:
        return "add logs panel"
    if "code" in p or "coding" in p or "developer" in p or "код" in p:
        return "coding mode"
    if "blank" in p or "empty" in p or "пуст" in p:
        return "blank canvas"
    if "debug" in p or "build" in p or "ошиб" in p:
        return "debug build error"
    if "dashboard" in p or "даш" in p:
        return "build dashboard"
    if "note" in p or "замет" in p:
        return "write notes"
    if "plan" in p or "план" in p:
        return "plan project"

    return "build dashboard"

def ollama_route(prompt: str, model: str) -> str:
    payload = {
        "model": model,
        "system": SYSTEM,
        "prompt": prompt,
        "stream": False,
    }

    req = urllib.request.Request(
        "http://127.0.0.1:11434/api/generate",
        data=json.dumps(payload).encode("utf-8"),
        headers={"Content-Type": "application/json"},
        method="POST",
    )

    with urllib.request.urlopen(req, timeout=120) as r:
        data = json.loads(r.read().decode("utf-8"))

    return data.get("response", "").strip().splitlines()[0].strip()

def handle_prompt(prompt: str, mode: str, model: str) -> str:
    if mode == "ollama":
        try:
            answer = ollama_route(prompt, model)
            return answer or mock_route(prompt)
        except Exception as e:
            print(f"[bridge] ollama error: {e}; using mock")
            return mock_route(prompt)

    return mock_route(prompt)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=7777)
    parser.add_argument("--mode", choices=["mock", "ollama"], default="mock")
    parser.add_argument("--model", default="llama3.2")
    args = parser.parse_args()

    print(f"[bridge] connecting to {args.host}:{args.port}, mode={args.mode}, model={args.model}")

    with socket.create_connection((args.host, args.port)) as sock:
        f = sock.makefile("rwb", buffering=0)
        print("[bridge] connected")

        while True:
            raw = f.readline()
            if not raw:
                print("[bridge] disconnected")
                break

            line = raw.decode("utf-8", errors="replace").strip()
            print(f"[bridge] <= {line}")

            if not line.startswith(PREFIX_IN):
                continue

            prompt = line[len(PREFIX_IN):].strip()
            answer = handle_prompt(prompt, args.mode, args.model)
            response = f"{PREFIX_OUT}{answer}\n"

            print(f"[bridge] => {response.strip()}")
            f.write(response.encode("utf-8"))

if __name__ == "__main__":
    main()
