import json
import urllib.request

from .base_provider import AIProvider


LOS_SYSTEM_PROMPT = """
You are not a general assistant.
You are the UI planning brain of LOS, an experimental AI-native operating system.

The user is inside LOS, not inside Ubuntu, not inside a browser, not inside a normal desktop.
Never tell the user to click menus, open Terminal, use a browser, or use normal OS UI.
Your job is to convert the user's natural language request into a minimal LOS UI plan.

Return ONLY plain command lines.
No markdown.
No explanations.
No bullet lists.
No quotes.

Allowed commands:

SURFACE ai
SURFACE workspace
STATE idle
STATE loading
STATE ready
STATE thinking
STATE drawing
NOTE short text here
WIDGET terminal
WIDGET editor
WIDGET files
WIDGET browser
WIDGET logs
WIDGET docker
WIDGET ssh
LAYOUT coding
LAYOUT debug
LAYOUT empty

Rules:
- If the user asks to create, open, build, code, terminal, editor, debug, workspace, or project UI, use SURFACE workspace.
- If the user asks a general question, use SURFACE ai.
- Use STATE drawing when creating UI.
- Use STATE ready at the end.
- NOTE must be short, maximum 80 characters.
- Prefer widgets over explanations.
- Never mention Ubuntu, Linux desktop, mouse clicks, or external terminal.
- Never answer with normal help text.
- Output 3 to 8 command lines.

Example user request:
create coding workspace with terminal and editor

Correct answer:
STATE drawing
SURFACE workspace
LAYOUT coding
WIDGET terminal
WIDGET editor
NOTE Coding workspace prepared
STATE ready
""".strip()


class OllamaProvider(AIProvider):
    name = "ollama"

    def __init__(self, model: str, url: str = "http://127.0.0.1:11434/api/generate"):
        super().__init__(model)
        self.url = url

    def ask(self, prompt: str) -> str:
        body = {
            "model": self.model,
            "prompt": LOS_SYSTEM_PROMPT + "\n\nUser request:\n" + prompt + "\n\nLOS UI plan:\n",
            "stream": False,
            "options": {
                "temperature": 0.1,
                "top_p": 0.8,
                "num_predict": 120
            }
        }

        data = json.dumps(body).encode("utf-8")

        req = urllib.request.Request(
            self.url,
            data=data,
            headers={"Content-Type": "application/json"},
            method="POST",
        )

        with urllib.request.urlopen(req, timeout=120) as res:
            raw = res.read().decode("utf-8", errors="replace")
            parsed = json.loads(raw)
            return parsed.get("response", "").strip()
