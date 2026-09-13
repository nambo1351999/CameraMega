# CameraMega Image Gen

Python CLI gọi **OpenAI GPT Image 2 / 2.5** để generate asset cho app CameraMega.

Chỉ cần set **`OPENAI_API_KEY`** — không cần file config khác.

## Setup (1 lần)

```bash
cd tools/image-gen
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

## Env

Tạo file **`.env`** (đã gitignore) — copy từ `.env.example`:

```bash
cp .env.example .env
# Sửa .env và dán OPENAI_API_KEY thật
```

```env
OPENAI_API_KEY=sk-...
# OPENAI_IMAGE_MODEL=gpt-image-2.5-flare
```

Không commit key vào `.env.example` — chỉ dùng `.env`.

## Lệnh

```bash
# Xem catalog (IMG-001 … IMG-030)
cameramega-image-gen list
cameramega-image-gen list --group onboarding

# Generate 1 asset → lưu thẳng res/drawable-nodpi/
cameramega-image-gen gen IMG-001

# Toàn bộ P0 (icon, splash, OB, home cards)
cameramega-image-gen gen --p0

# Nhóm
cameramega-image-gen gen --group home

# Thử không gọi API
cameramega-image-gen gen --p0 --dry-run

# Chỉ lưu bản staging (out/) — không ghi vào app/
cameramega-image-gen gen IMG-020 --staging
```

Hoặc không cài package:

```bash
cd tools/image-gen
OPENAI_API_KEY='sk-...' python -m cameramega_image_gen gen IMG-001
```

## Output

| ID | File đích |
|----|-----------|
| IMG-001 | `app/src/main/res/drawable-nodpi/ic_launcher_generated.png` |
| IMG-002 | `app/src/main/res/drawable-nodpi/splash_logo.png` |
| IMG-010..012 | `onboarding_*.webp` |
| IMG-020..025 | `home_card_*.webp` |
| IMG-030 | `home_banner_film_lut.webp` |

Bản raw API luôn được lưu thêm tại `tools/image-gen/out/`.

## Prompt catalog

Prompt chi tiết: [`../../IMAGE_ASSETS_BRIEF.md`](../../IMAGE_ASSETS_BRIEF.md)  
Skill agent: [`.cursor/skills/cameramega-image-assets/`](../../.cursor/skills/cameramega-image-assets/SKILL.md)

Upstream: [GPT-Image2-Skill](https://github.com/wuyoscar/GPT-Image2-Skill)

## Lưu ý

- Mỗi lần gọi API **tốn phí** OpenAI — thử `--dry-run` hoặc `--staging` trước.
- Không commit `OPENAI_API_KEY`.
- Sau khi có ảnh, wire code trong `OnboardingScreen.kt` / `HomeModeCard.kt` (xem brief).
