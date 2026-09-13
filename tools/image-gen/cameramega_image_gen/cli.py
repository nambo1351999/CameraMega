from __future__ import annotations

import argparse
import sys
from datetime import datetime
from pathlib import Path

from .api import default_model, generate_image_bytes, require_api_key
from .env_loader import load_env
from .assets import ASSETS, Group, Priority, get_asset, list_assets
from .postprocess import write_asset_output

PROJECT_ROOT = Path(__file__).resolve().parents[3]
STAGING_DIR = PROJECT_ROOT / "tools" / "image-gen" / "out"


def _resolve_output(spec_output: str, *, staging: bool) -> Path:
    if staging:
        stem = Path(spec_output).stem
        return STAGING_DIR / f"{stem}.png"
    return PROJECT_ROOT / spec_output


def cmd_list(args: argparse.Namespace) -> int:
    assets = list_assets(priority=args.priority, group=args.group)
    for spec in assets:
        print(f"{spec.asset_id:8}  [{spec.priority}] {spec.group:12}  {spec.output_path}")
    print(f"\nTotal: {len(assets)}")
    return 0


def _collect_ids(args: argparse.Namespace) -> list[str]:
    if args.ids:
        return [i.upper() for i in args.ids]

    if args.all:
        return sorted(ASSETS.keys())

    if args.p0:
        return [a.asset_id for a in list_assets(priority="P0")]

    if args.group:
        return [a.asset_id for a in list_assets(group=args.group)]

    raise SystemExit("Specify asset ID(s), --p0, --group, or --all")


def cmd_gen(args: argparse.Namespace) -> int:
    if not args.dry_run:
        try:
            require_api_key()
        except RuntimeError as exc:
            print(f"error: {exc}", file=sys.stderr)
            return 2

    model = args.model or default_model()
    asset_ids = _collect_ids(args)
    failures = 0

    print(f"Model: {model}")
    print(f"Project root: {PROJECT_ROOT}")
    print(f"Assets: {', '.join(asset_ids)}\n")

    for asset_id in asset_ids:
        spec = get_asset(asset_id)
        dest = _resolve_output(spec.output_path, staging=args.staging)
        print(f"▶ {spec.asset_id} — {spec.name}")

        if args.dry_run:
            print(f"  prompt: {spec.full_prompt()[:120]}…")
            print(f"  → {dest}")
            continue

        try:
            raw = generate_image_bytes(spec, model=model)
            staging_path = STAGING_DIR / f"{spec.asset_id}_{datetime.now():%Y%m%d_%H%M%S}.png"
            staging_path.parent.mkdir(parents=True, exist_ok=True)
            staging_path.write_bytes(raw)
            print(f"  staged: {staging_path} ({len(raw):,} bytes)")

            final_path, size = write_asset_output(raw, spec, dest)
            print(f"  saved:  {final_path} ({size:,} bytes)")
            if spec.max_bytes and size > spec.max_bytes:
                print(f"  warn:   exceeds budget {spec.max_bytes:,} bytes")
        except (RuntimeError, OSError, KeyError) as exc:
            print(f"  FAIL: {exc}", file=sys.stderr)
            failures += 1
            if not args.continue_on_error:
                return 1

        print()

    if failures:
        print(f"Completed with {failures} failure(s).", file=sys.stderr)
        return 1
    print("Done.")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="cameramega-image-gen",
        description="Generate CameraMega assets via OpenAI GPT Image 2/2.5. Requires OPENAI_API_KEY.",
    )
    sub = parser.add_subparsers(dest="command", required=True)

    list_p = sub.add_parser("list", help="List catalogued assets")
    list_p.add_argument("--priority", choices=["P0", "P1", "P2"])
    list_p.add_argument("--group", choices=["icon", "splash", "onboarding", "home", "banner"])
    list_p.set_defaults(func=cmd_list)

    gen_p = sub.add_parser("gen", help="Generate one or more assets")
    gen_p.add_argument("ids", nargs="*", help="Asset IDs e.g. IMG-001 IMG-020")
    gen_p.add_argument("--p0", action="store_true", help="All P0 assets")
    gen_p.add_argument("--all", action="store_true", help="Entire catalog")
    gen_p.add_argument("--group", type=str, choices=["icon", "splash", "onboarding", "home", "banner"])
    gen_p.add_argument("--model", help="Override model (default: OPENAI_IMAGE_MODEL or gpt-image-2.5-flare)")
    gen_p.add_argument("--staging", action="store_true", help="Write only to tools/image-gen/out/")
    gen_p.add_argument("--dry-run", action="store_true", help="Print plan without API calls")
    gen_p.add_argument("--continue-on-error", action="store_true", help="Keep going after a failure")
    gen_p.set_defaults(func=cmd_gen)

    return parser


def main(argv: list[str] | None = None) -> int:
    load_env()
    parser = build_parser()
    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
