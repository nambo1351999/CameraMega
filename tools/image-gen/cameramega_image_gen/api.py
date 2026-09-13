from __future__ import annotations

import base64
import os
from typing import Any

from openai import APIError, OpenAI

from .assets import AssetSpec
from .env_loader import env_file_path, load_env


def require_api_key() -> str:
    load_env()
    key = os.environ.get("OPENAI_API_KEY", "").strip()
    if not key or key == "sk-your-key-here":
        env_path = env_file_path()
        raise RuntimeError(
            "OPENAI_API_KEY is not set. Add your key to:\n"
            f"  {env_path}\n"
            "Or export: export OPENAI_API_KEY='sk-...'"
        )
    return key


def default_model() -> str:
    return os.environ.get("OPENAI_IMAGE_MODEL", "gpt-image-2.5-flare").strip()


def generate_image_bytes(spec: AssetSpec, *, model: str | None = None) -> bytes:
    require_api_key()
    client = OpenAI(max_retries=0)
    resolved_model = model or spec.model or default_model()

    kwargs: dict[str, Any] = {
        "model": resolved_model,
        "prompt": spec.full_prompt(),
        "size": spec.size,
        "quality": spec.quality,
        "n": 1,
        "output_format": spec.output_format,
    }
    if spec.background:
        kwargs["background"] = spec.background
    if spec.output_format in ("jpeg", "webp"):
        kwargs["output_compression"] = 85

    try:
        result = client.images.generate(**kwargs)
    except APIError as exc:
        raise RuntimeError(f"OpenAI API error: {exc}") from exc

    data = result.data or []
    if not data:
        raise RuntimeError(f"No image data in response: {result!r}")

    item = data[0]
    b64 = getattr(item, "b64_json", None)
    if not b64:
        raise RuntimeError("Response missing b64_json — check model and API version.")

    return base64.b64decode(b64)
