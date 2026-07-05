from abc import ABC, abstractmethod


class AIProvider(ABC):
    name = "base"

    def __init__(self, model: str):
        self.model = model

    def load_model(self, model: str) -> str:
        if model:
            self.model = model
        return f"Model selected: {self.model}"

    @abstractmethod
    def ask(self, prompt: str) -> str:
        raise NotImplementedError
