from __future__ import annotations

import os
from pathlib import Path

from dotenv import load_dotenv

_TOOL_ROOT = Path(__file__).resolve().parents[1]
_LOADED = False


def load_env() -> None:
    """Load tools/image-gen/.env without overriding existing shell env."""
    global _LOADED
    if _LOADED:
        return
    load_dotenv(_TOOL_ROOT / ".env", override=False)
    _LOADED = True


def env_file_path() -> Path:
    return _TOOL_ROOT / ".env"
