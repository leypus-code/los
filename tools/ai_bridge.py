#!/usr/bin/env python3
import argparse
import html
import json
import os
import re
import socket
import urllib.parse
import urllib.request

PREFIX_AI_IN = "LOS_AI_REQUEST:"
PREFIX_AI_OUT = "LOS_AI_RESPONSE:"
PREFIX_WEB_IN = "LOS_WEB_REQUEST:"
PREFIX_WEB_OUT = "LOS_WEB_RESPONSE:"

SYSTEM = """You are the intent router for LOS, an AI-native operating system prototype.

Return exactly one line.
Return no explanation.
Return no markdown.
Return no punctuation around the command.
Return only one of the allowed commands.

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
web:<search query>

Routing rules:
- If the user asks to change the screen, workspace, layout, dashboard, coding mode, notes, planning, debug, or UI state, return a normal LOS intent.
- If the user asks a factual question, current information, weather, news, definition, price, person, company, place, or anything that needs external knowledge, return web:<search query>.
- If unsure, return web:<original user query>.
"""

def ascii_safe(text: str) -> str:
    replacements = {
        "°": "C",
        "℃": "C",
        "☁": "cloudy",
        "🌤": "partly cloudy",
        "⛅": "partly cloudy",
        "☀": "sunny",
        "🌧": "rain",
        "🌦": "rain",
        "❄": "snow",
        "💨": "wind",
        "↗": "NE",
        "↘": "SE",
        "↖": "NW",
        "↙": "SW",
        "↑": "up",
        "↓": "down",
        "–": "-",
        "—": "-",
        "“": '"',
        "”": '"',
        "‘": "'",
        "’": "'",
        "…": "...",
        "\xa0": " ",
    }

    for old, new in replacements.items():
        text = text.replace(old, new)

    return text.encode("ascii", "ignore").decode("ascii")


def one_line(text: str, max_len: int = 360) -> str:
    text = ascii_safe(text)
    text = text.replace("\r", " ").replace("\n", " ")
    text = re.sub(r"\s+", " ", text).strip()
    if len(text) > max_len:
        return text[: max_len - 3] + "..."
    return text

def mock_route(prompt: str) -> str:
    p = prompt.lower()

    web_triggers = [
        "what is ", "who is ", "when is ", "where is ",
        "latest", "current", "news", "price", "weather in",
        "search", "google", "internet", "web",
        "что такое", "кто такой", "когда", "где", "новости", "найди", "поищи"
    ]

    for trigger in web_triggers:
        if trigger in p:
            cleaned = prompt.strip()
            cleaned = cleaned.replace('"', '').replace("'", "")
            return "web:" + cleaned

    if "weather" in p or "погод" in p:
        return "add weather"
    if "check" in p or "todo" in p or "чек" in p:
        return "add checklist"
    if "log" in p or "лог" in p:
        return "add logs panel"
    if "code" in p or "coding" in p or "developer" in p or "код" in p:
        return "coding mode"
    if "blank" in p or "empty" in p or "canvas" in p or "пуст" in p:
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

def sanitize_ai_route(answer: str, prompt: str) -> str:
    answer = (answer or "").strip()

    if not answer:
        return mock_route(prompt)

    first = answer.splitlines()[0].strip()
    first = first.strip("`").strip()

    allowed = {
        "home",
        "build dashboard",
        "coding mode",
        "blank canvas",
        "reset home",
        "add weather",
        "remove weather",
        "add checklist",
        "remove checklist",
        "add logs panel",
        "remove logs panel",
        "debug build error",
        "system overview",
        "write notes",
        "plan project",
    }

    if first in allowed:
        return first

    if first.startswith("web:"):
        return first

    return mock_route(prompt)

def extract_openai_text(data: dict) -> str:
    if not isinstance(data, dict):
        return ""

    if isinstance(data.get("output_text"), str):
        return data["output_text"].strip()

    out = data.get("output")
    if isinstance(out, list):
        parts = []

        for item in out:
            if not isinstance(item, dict):
                continue

            content = item.get("content")

            if isinstance(content, list):
                for c in content:
                    if not isinstance(c, dict):
                        continue

                    text = c.get("text")

                    if isinstance(text, str):
                        parts.append(text)

                    if isinstance(text, dict) and isinstance(text.get("value"), str):
                        parts.append(text["value"])

            if isinstance(content, str):
                parts.append(content)

        if parts:
            return "\n".join(parts).strip()

    return ""

def openai_route(prompt: str, model: str) -> str:
    api_key = os.environ.get("OPENAI_API_KEY", "").strip()

    if not api_key:
        raise RuntimeError("OPENAI_API_KEY is not set")

    payload = {
        "model": model,
        "instructions": SYSTEM,
        "input": prompt,
    }

    req = urllib.request.Request(
        "https://api.openai.com/v1/responses",
        data=json.dumps(payload).encode("utf-8"),
        headers={
            "Content-Type": "application/json",
            "Authorization": "Bearer " + api_key,
        },
        method="POST",
    )

    with urllib.request.urlopen(req, timeout=120) as r:
        data = json.loads(r.read().decode("utf-8"))

    text = extract_openai_text(data)
    return text.strip()


def handle_prompt(prompt: str, mode: str, model: str) -> str:
    if mode == "openai":
        try:
            answer = openai_route(prompt, model)
            return sanitize_ai_route(answer, prompt)
        except Exception as e:
            print(f"[bridge] openai error: {e}; using mock")
            return mock_route(prompt)

    if mode == "ollama":
        try:
            answer = ollama_route(prompt, model)
            return sanitize_ai_route(answer, prompt)
        except Exception as e:
            print(f"[bridge] ollama error: {e}; using mock")
            return mock_route(prompt)

    return mock_route(prompt)

def is_weather_query(query: str) -> str | None:
    q = query.lower().strip()

    prefixes = [
        "weather in ",
        "weather ",
        "погода в ",
        "погода ",
    ]

    for prefix in prefixes:
        if q.startswith(prefix):
            city = query[len(prefix):].strip()
            return city or None

    return None

def strip_html(text: str) -> str:
    text = re.sub(r"<script.*?</script>", " ", text, flags=re.I | re.S)
    text = re.sub(r"<style.*?</style>", " ", text, flags=re.I | re.S)
    text = re.sub(r"<.*?>", " ", text, flags=re.S)
    text = html.unescape(text)
    return one_line(text, 360)

def looks_like_html(text: str) -> bool:
    l = text.lower()
    return "<html" in l or "<!doctype" in l or "<head" in l or "<body" in l

def web_weather(query: str) -> str | None:
    city = is_weather_query(query)

    if not city:
        return None

    city_q = urllib.parse.quote(city)

    urls = [
        "https://wttr.in/" + city_q + "?format=%l:+%t,+%C,+humidity+%h,+wind+%w",
        "https://wttr.in/" + city_q + "?format=3",
    ]

    for url in urls:
        req = urllib.request.Request(
            url,
            headers={
                "User-Agent": "curl/8.0",
                "Accept": "text/plain,*/*",
            },
            method="GET",
        )

        with urllib.request.urlopen(req, timeout=20) as r:
            text = r.read().decode("utf-8", errors="replace")

        if not looks_like_html(text):
            return one_line(text, 220)

    return None

def is_what_is_query(query: str) -> str | None:
    q = query.strip()
    l = q.lower()

    prefixes = [
        "what is ",
        "what are ",
        "who is ",
        "что такое ",
        "кто такой ",
        "кто такая ",
    ]

    for prefix in prefixes:
        if l.startswith(prefix):
            subject = q[len(prefix):].strip(" ?.!'\"")
            return subject or None

    return None

def wikipedia_best_title(subject: str) -> str:
    manual = {
        "docker": "Docker_(software)",
        "linux": "Linux",
        "python": "Python_(programming_language)",
        "javascript": "JavaScript",
        "typescript": "TypeScript",
    }

    key = subject.lower().strip()
    if key in manual:
        return manual[key]

    search_url = (
        "https://en.wikipedia.org/w/api.php?action=query&list=search&format=json&srsearch="
        + urllib.parse.quote(subject)
    )

    req = urllib.request.Request(
        search_url,
        headers={"User-Agent": "LOS-Host-Web-Bridge/0.1"},
        method="GET",
    )

    with urllib.request.urlopen(req, timeout=20) as r:
        data = json.loads(r.read().decode("utf-8", errors="replace"))

    items = data.get("query", {}).get("search", [])

    if items:
        return items[0].get("title", subject).replace(" ", "_")

    return subject.replace(" ", "_")

def web_wikipedia_summary(query: str) -> str | None:
    subject = is_what_is_query(query)

    if not subject:
        return None

    title = wikipedia_best_title(subject)
    url = "https://en.wikipedia.org/api/rest_v1/page/summary/" + urllib.parse.quote(title)

    req = urllib.request.Request(
        url,
        headers={"User-Agent": "LOS-Host-Web-Bridge/0.1"},
        method="GET",
    )

    with urllib.request.urlopen(req, timeout=20) as r:
        data = json.loads(r.read().decode("utf-8", errors="replace"))

    extract = data.get("extract") or ""
    if not extract:
        return None

    return one_line(extract, 340)


def web_search_ddg(query: str) -> str:
    urls = [
        "https://html.duckduckgo.com/html/?q=" + urllib.parse.quote_plus(query),
        "https://lite.duckduckgo.com/lite/?q=" + urllib.parse.quote_plus(query),
    ]

    last_page = ""

    for url in urls:
        req = urllib.request.Request(
            url,
            headers={
                "User-Agent": "Mozilla/5.0 LOS-Host-Web-Bridge/0.1",
            },
            method="GET",
        )

        with urllib.request.urlopen(req, timeout=20) as r:
            page = r.read().decode("utf-8", errors="replace")

        last_page = page
        results = []

        patterns = [
            r'<a[^>]+class="result__a"[^>]*>(.*?)</a>',
            r'<a[^>]+class="result-link"[^>]*>(.*?)</a>',
            r'<a[^>]+rel="nofollow"[^>]*>(.*?)</a>',
        ]

        for pattern in patterns:
            for m in re.finditer(pattern, page, flags=re.I | re.S):
                title = re.sub(r"<.*?>", "", m.group(1))
                title = html.unescape(title)
                title = one_line(title, 160)

                if title and title not in results and "DuckDuckGo" not in title:
                    results.append(title)

                if len(results) >= 3:
                    break

            if results:
                break

        snippets = []
        for m in re.finditer(r'<a[^>]+class="result__snippet"[^>]*>(.*?)</a>|<td[^>]+class="result-snippet"[^>]*>(.*?)</td>', page, flags=re.I | re.S):
            raw = m.group(1) or m.group(2) or ""
            snip = re.sub(r"<.*?>", "", raw)
            snip = html.unescape(snip)
            snip = one_line(snip, 220)
            if snip:
                snippets.append(snip)
            if len(snippets) >= 1:
                break

        if snippets:
            return one_line(snippets[0], 340)

        if results:
            return one_line(" | ".join(results), 340)

    m = re.search(r"<title>(.*?)</title>", last_page, flags=re.I | re.S)
    if m:
        title = html.unescape(re.sub(r"<.*?>", "", m.group(1)))
        return one_line("Search result page: " + title, 300)

    return "No web results found."

def web_handle(query: str, mode: str) -> str:
    if mode == "off":
        return "Web bridge is off. Start bridge with --web ddg."

    if mode == "mock":
        return f"Mock web result for: {query}"

    try:
        weather = web_weather(query)
        if weather:
            return weather
    except Exception as e:
        print(f"[bridge] weather error: {e}")

    try:
        wiki = web_wikipedia_summary(query)
        if wiki:
            return wiki
    except Exception as e:
        print(f"[bridge] wikipedia error: {e}")

    try:
        result = web_search_ddg(query)
        if looks_like_html(result):
            return strip_html(result)
        return result
    except Exception as e:
        return one_line(f"Web error: {e}")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=7777)
    parser.add_argument("--mode", choices=["mock", "ollama", "openai"], default="mock")
    parser.add_argument("--model", default=os.environ.get("OPENAI_MODEL", "gpt-5.5"))
    parser.add_argument("--web", choices=["off", "mock", "ddg"], default="mock")
    args = parser.parse_args()

    print(f"[bridge] connecting to {args.host}:{args.port}, mode={args.mode}, model={args.model}, web={args.web}")

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

            if line.startswith(PREFIX_AI_IN):
                prompt = line[len(PREFIX_AI_IN):].strip()
                answer = handle_prompt(prompt, args.mode, args.model)
                response = f"{PREFIX_AI_OUT}{one_line(answer, 240)}\n"
                print(f"[bridge] => {response.strip()}")
                f.write(response.encode("utf-8"))
                continue

            if line.startswith(PREFIX_WEB_IN):
                query = line[len(PREFIX_WEB_IN):].strip()
                answer = web_handle(query, args.web)
                response = f"{PREFIX_WEB_OUT}{one_line(answer, 360)}\n"
                print(f"[bridge] => {response.strip()}")
                f.write(response.encode("utf-8"))
                continue

if __name__ == "__main__":
    main()
