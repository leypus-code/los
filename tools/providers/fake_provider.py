from .base_provider import AIProvider


class FakeProvider(AIProvider):
    name = "fake"

    def ask(self, prompt: str) -> str:
        low = prompt.lower()

        if "workspace" in low or "space" in low or "рабоч" in low:
            return "Workspace created. Ready for generated widgets."

        if "terminal" in low or "терминал" in low:
            return "Terminal workspace requested. Shell widget will be created next."

        if "editor" in low or "редактор" in low:
            return "Editor workspace requested. Code surface will be prepared."

        if "code" in low or "код" in low:
            return "Coding surface requested. Editor and terminal layout will be prepared."

        return "Fake provider is alive. AI bridge pipeline works."
