---
name: cameramega-image-assets
description: >-
  Generate CameraMega app assets (icon, splash, onboarding, home cards) using
  GPT Image 2/2.5 via gpt-image skill. Use when user asks to generate images
  for CameraMega, IMAGE_ASSETS_BRIEF.md, app icon, splash, onboarding, or home
  card artwork. Brand is CameraMega only — never SuperCamera.
compatibility: Requires gpt-image skill, OPENAI_API_KEY, Python 3.11+ or gpt-image CLI.
---

# CameraMega Image Assets

Generate production assets for **Camera Mega** using the [GPT-Image2-Skill](https://github.com/wuyoscar/GPT-Image2-Skill) workflow vendored at `.cursor/skills/gpt-image/`.

## Before generating

1. Read **`IMAGE_ASSETS_BRIEF.md`** at repo root — pick asset ID (e.g. `IMG-001`).
2. Read **`.cursor/skills/gpt-image/SKILL.md`** for model choice and CLI rules.
3. Confirm brand: **CameraMega** palette `#0D0D0D`, `#FF6B35`, `#7E6BF0` — not SuperCamera amber.

## Model defaults (CameraMega)

| Asset type | Model | Size | Quality | Format |
|------------|-------|------|---------|--------|
| App icon (IMG-001) | `gpt-image-2.5-flare` | `square` / 1024×1024 | `high` | PNG transparent or opaque |
| Splash logo (IMG-002) | `gpt-image-2.5-flare` | `square` | `high` | PNG `--background transparent` |
| Onboarding (IMG-010..012) | `gpt-image-2.5-flare` | `square` | `high` | WebP/PNG 800×800 |
| Home cards (IMG-020..025) | `gpt-image-2.5-flare` | `square` | `medium` → compress WebP ≤60KB | WebP |
| Film banner (IMG-030) | `gpt-image-2.5-flare` | `landscape` / 1024×512 | `high` | WebP |

Use **Sunburst** (`gpt-image-2.5-sunburst`) only when editing from a reference image.

## Operating loop

1. Load the asset block from `IMAGE_ASSETS_BRIEF.md` (Positive + Negative + CLI).
2. Resolve model; pass explicit `--model`.
3. Run via CLI (from repo root):

```bash
# Example — replace PROMPT with the Positive Prompt block (one line or heredoc)
gpt-image --model gpt-image-2.5-flare \
  -p "$(cat <<'EOF'
<PASTE POSITIVE PROMPT FROM BRIEF>
EOF
)" \
  --size square --quality high --format png \
  -f app/src/main/res/drawable-nodpi/home_card_pro_photo.png
```

**Preferred — CameraMega Python tool** (only `OPENAI_API_KEY` required):

```bash
cd tools/image-gen && pip install -r requirements.txt
export OPENAI_API_KEY='sk-...'
./gen.sh gen --p0
./gen.sh gen IMG-001
```

If `gpt-image` upstream CLI is needed:

```bash
uvx --from git+https://github.com/wuyoscar/gpt_image_2_skill gpt-image --model gpt-image-2.5-flare -p "..." -f out.png
```

4. Post-process: resize to spec, export WebP (80–85% for cards), verify ≤ size budget.
5. Wire in code: update `OnboardingScreen.kt`, `HomeModeCard.kt`, `ic_launcher_*`, mark `[x]` Done in brief.

## Output paths

| ID | Target path |
|----|-------------|
| IMG-001 | `app/src/main/res/mipmap-xxxhdpi/ic_launcher.png` + adaptive XML |
| IMG-002 | `app/src/main/res/drawable-nodpi/splash_logo.png` |
| IMG-010..012 | `app/src/main/res/drawable-nodpi/onboarding_*.webp` |
| IMG-020..025 | `app/src/main/res/drawable-nodpi/home_card_*.webp` |
| IMG-030 | `app/src/main/res/drawable-nodpi/home_banner_film_lut.webp` |

## Agent invocation (natural language)

```
Generate CameraMega IMG-021 (RAW Max home card) using cameramega-image-assets skill.
Use gpt-image-2.5-flare, save to drawable-nodpi, then wire HomeModeCard if needed.
```

## Related skills

- `.cursor/skills/gpt-image/` — full GPT Image 2/2.5 runbook + gallery
- `.cursor/skills/get-prompt-from-image/` — reverse-engineer prompt from reference photo
