from __future__ import annotations

import io
from pathlib import Path

from PIL import Image

from .assets import AssetSpec


def _save_webp(img: Image.Image, path: Path, quality: int) -> int:
    path.parent.mkdir(parents=True, exist_ok=True)
    img.save(path, format="WEBP", quality=quality, method=6)
    return path.stat().st_size


def _save_png(img: Image.Image, path: Path) -> int:
    path.parent.mkdir(parents=True, exist_ok=True)
    img.save(path, format="PNG", optimize=True)
    return path.stat().st_size


def write_asset_output(raw_bytes: bytes, spec: AssetSpec, dest: Path) -> tuple[Path, int]:
    """Decode API bytes, optionally resize, write final file. Returns path and size."""
    img = Image.open(io.BytesIO(raw_bytes)).convert("RGBA")

    if spec.resize_to:
        img = img.resize(spec.resize_to, Image.Resampling.LANCZOS)

    dest = dest.resolve()
    dest.parent.mkdir(parents=True, exist_ok=True)

    wants_webp = dest.suffix.lower() == ".webp" or spec.webp_quality is not None
    if wants_webp:
        if dest.suffix.lower() != ".webp":
            dest = dest.with_suffix(".webp")
        quality = spec.webp_quality or 85
        size = _save_webp(img, dest, quality)
        if spec.max_bytes and size > spec.max_bytes:
            for q in range(quality - 5, 40, -5):
                size = _save_webp(img, dest, q)
                if size <= spec.max_bytes:
                    break
        return dest, size

    if dest.suffix.lower() in (".jpg", ".jpeg"):
        rgb = Image.new("RGB", img.size, (13, 13, 13))
        rgb.paste(img, mask=img.split()[3] if img.mode == "RGBA" else None)
        rgb.save(dest, format="JPEG", quality=90, optimize=True)
    else:
        _save_png(img, dest)

    return dest, dest.stat().st_size
