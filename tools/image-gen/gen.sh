#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

if [[ ! -d .venv ]]; then
  python3 -m venv .venv
  .venv/bin/pip install -r requirements.txt -q
fi

# Key from shell env or tools/image-gen/.env (loaded by Python)
exec .venv/bin/python -m cameramega_image_gen "$@"
