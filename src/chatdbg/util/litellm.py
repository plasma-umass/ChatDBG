import requests

# LiteLLM is licensed under MIT.
model_data = requests.get(
    "https://raw.githubusercontent.com/BerriAI/litellm/refs/heads/main/model_prices_and_context_window.json"
).json()
