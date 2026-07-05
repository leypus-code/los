from .base_provider import AIProvider


class FakeProvider(AIProvider):
    name = "fake"

    def ask(self, prompt: str) -> str:
        low = prompt.lower()

        vague_screen = ("screen" in low or "surface" in low or "interface" in low) and not (
            "coding" in low or "code" in low or "debug" in low or "terminal" in low
            or "editor" in low or "files" in low or "logs" in low or "docker" in low
            or "ssh" in low or "browser" in low or "workspace" in low
        )

        if vague_screen:
            return "\n".join([
                "ASK What kind of LOS screen do you need: coding, debug, files, or terminal?",
                "SURFACE ai",
                "STATE ready",
            ])

        if "workspace" in low or "coding" in low or "code" in low:
            return "\n".join([
                "CHAT Understood, creating a coding workspace.",
                "STATE drawing",
                "SURFACE workspace",
                "LAYOUT coding",
                "WIDGET editor",
                "WIDGET terminal",
                "NOTE Coding workspace prepared",
                "STATE ready",
            ])

        if "debug" in low:
            return "\n".join([
                "CHAT Understood, creating a debug workspace.",
                "STATE drawing",
                "SURFACE workspace",
                "LAYOUT debug",
                "WIDGET logs",
                "WIDGET terminal",
                "WIDGET docker",
                "NOTE Debug workspace prepared",
                "STATE ready",
            ])

        return "\n".join([
            "CHAT I am listening. Tell me what LOS interface you want to build.",
            "SURFACE ai",
            "STATE ready",
        ])
