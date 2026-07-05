from .fake_provider import FakeProvider
from .ollama_provider import OllamaProvider


def create_provider(name: str, model: str):
    selected = (name or "fake").strip().lower()

    if selected == "ollama":
        return OllamaProvider(model)

    if selected == "fake":
        return FakeProvider(model)

    print(f"[provider] unknown provider '{name}', using fake")
    return FakeProvider(model)
