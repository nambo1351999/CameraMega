# Camera Mega — Image Assets Brief (GPT Image 2/2.5)

> **Skill:** `.cursor/skills/cameramega-image-assets/SKILL.md`  
> **Engine:** [GPT-Image2-Skill](https://github.com/wuyoscar/GPT-Image2-Skill) — `.cursor/skills/gpt-image/`  
> **Branding:** CameraMega only — **never** Super Camera Zoom / SuperCamera  
> **Updated:** 2026-09-13

---

## How to use

### Python (recommended — CameraMega)

```bash
cd tools/image-gen && pip install -e .
export OPENAI_API_KEY='sk-...'
cameramega-image-gen gen --p0          # toàn bộ P0
cameramega-image-gen gen IMG-001       # một asset
cameramega-image-gen gen --dry-run --p0
```

Xem [`tools/image-gen/README.md`](tools/image-gen/README.md).

### GPT-Image2 CLI (upstream)

1. `uvx --from git+https://github.com/wuyoscar/gpt_image_2_skill gpt-image --help`
2. Set `OPENAI_API_KEY` in environment (never commit).
3. Pick asset ID below → copy **Positive** + **Negative** → run **CLI** block.
4. Post-process (resize, WebP compress) → drop in `res/` → wire code → mark **Done**.

Agent shortcut:

```text
Generate CameraMega IMG-020 using cameramega-image-assets skill and gpt-image-2.5-flare.
```

---

## Brand system (freeze in every prompt)

| Token | Hex | Use |
|-------|-----|-----|
| Ink background | `#0D0D0D` | Splash, icon bg, card scrim base |
| Surface | `#1A1A1A` | Cards, panels |
| Accent orange | `#FF6B35` | Zoom ring, CTA, badges, record glow |
| Flare purple | `#7E6BF0` | HW-dependent features, Phantom PiP |
| Text primary | `#FFFFFF` | Titles on dark |

**Style anchor:** premium mobile camera app, cinematic product photography, realistic (not cartoon), high contrast, subtle film grain optional, **no readable text/watermarks/logos** unless asset spec says otherwise.

**Code refs:** `ui/theme/Color.kt`, `ui/flow/`

---

## Priority overview

| P | Group | IDs | Output |
|---|-------|-----|--------|
| P0 | Icon + Splash | IMG-001, IMG-002 | PNG adaptive + splash |
| P0 | Onboarding | IMG-010..012 | WebP 800² circle crop |
| P0 | Home cards | IMG-020..025 | WebP 512² ≤60 KB |
| P1 | Film banner | IMG-030 | WebP 1024×512 ≤80 KB |
| P2 | Play Store | TBD | 6–10 marketing frames |

---

## P0 — IMG-001 · Adaptive launcher icon

| Field | Value |
|-------|-------|
| **Use** | Launcher, `Theme.CameraMega.Splash`, shortcuts |
| **Model** | `gpt-image-2.5-flare` |
| **Size** | `square` (1024×1024) |
| **Quality** | `high` |
| **Format** | PNG, `--background opaque` or separate foreground PNG |
| **Export** | Foreground 432×432 safe zone; bg `#0D0D0D` |
| **Done** | [ ] |

### Positive prompt

```text
Create a square premium Android adaptive app icon for a fictional pro mobile camera app called "Camera Mega". Center a stylized camera lens symbol: concentric glass rings, subtle inner reflections, and a bold orange (#FF6B35) focus/zoom ring as the hero accent. Add a restrained purple (#7E6BF0) lens flare streak at the upper-right edge only. Background: flat deep ink (#0D0D0D) filling the entire canvas. Style: flat-vector meets subtle 3D product render, crisp edges, high contrast, minimal geometry, no text, no letters, no watermark, no Apple/Samsung logos. Composition: perfectly centered, generous padding inside the Android adaptive icon safe zone (central 66% circle), icon reads clearly at 48dp. Mood: professional, cinematic, trustworthy, modern photography tool.
```

### Negative prompt

```text
text, letters, wordmark, watermark, screenshot, UI mockup, cartoon mascot, low poly, noisy gradient, rainbow colors, busy background, human face, brand logos, Samsung, Apple, Google camera icon clone, blurry, jpeg artifacts, off-center crop
```

### CLI

```bash
gpt-image --model gpt-image-2.5-flare \
  -p "$(cat <<'EOF'
Create a square premium Android adaptive app icon for a fictional pro mobile camera app called "Camera Mega". Center a stylized camera lens symbol: concentric glass rings, subtle inner reflections, and a bold orange (#FF6B35) focus/zoom ring as the hero accent. Add a restrained purple (#7E6BF0) lens flare streak at the upper-right edge only. Background: flat deep ink (#0D0D0D) filling the entire canvas. Style: flat-vector meets subtle 3D product render, crisp edges, high contrast, minimal geometry, no text, no letters, no watermark, no Apple/Samsung logos. Composition: perfectly centered, generous padding inside the Android adaptive icon safe zone (central 66% circle), icon reads clearly at 48dp. Mood: professional, cinematic, trustworthy, modern photography tool.
EOF
)" \
  --size square --quality high --format png --background opaque \
  -f assets/gen/img-001-launcher.png
```

### Wire

Replace `ic_launcher_foreground` / update `mipmap-anydpi-v26/ic_launcher.xml`.

---

## P0 — IMG-002 · Splash logo mark

| Field | Value |
|-------|-------|
| **Use** | Optional overlay in `SplashScreen.kt` |
| **Model** | `gpt-image-2.5-flare` |
| **Size** | `square` 512×512 |
| **Quality** | `high` |
| **Format** | PNG `--background transparent` |
| **Done** | [ ] |

### Positive prompt

```text
Create a square transparent-background splash logo mark for "Camera Mega" mobile camera app. Subject: abstract aperture/lens glyph built from 6–8 geometric blades forming a hexagonal opening, with an orange (#FF6B35) outer ring and cool gray inner glass. Subtle purple (#7E6BF0) specular highlight on one blade only. No wordmark, no letters, no text — symbol only. Style: minimal vector-like mark with soft 3D bevel, crisp silhouette, works on #0D0D0D background. Centered, equal padding on all sides, high legibility at 120dp display size.
```

### Negative prompt

```text
text, typography, camera mega wording, watermark, photo background, realistic camera product photo, clutter, gradient backdrop, cartoon, low resolution, uneven padding
```

### CLI

```bash
gpt-image --model gpt-image-2.5-flare \
  -p "..." \
  --size square --quality high --format png --background transparent \
  -f app/src/main/res/drawable-nodpi/splash_logo.png
```

---

## P0 — Onboarding illustrations (IMG-010..012)

**Display:** circle 200dp in `OnboardingScreen.kt`  
**Master size:** 800×800 px → export WebP ≤120 KB

---

### IMG-010 · Pro RAW Photography

| Done | [ ] |
|------|-----|
| **File** | `drawable-nodpi/onboarding_raw_pro.webp` |
| **Model** | `gpt-image-2.5-flare` · `square` · `high` |

**Positive**

```text
Create a square cinematic product-illustration for a pro mobile RAW photography feature. Scene: night city rooftop with distant skyline bokeh; in the foreground a floating semi-transparent DNG file tile and a wireframe histogram overlay suggesting unprocessed sensor data. Show rich shadow detail in building windows and a single orange (#FF6B35) UI accent glow along the bottom edge like a camera app capture bar. Background deep ink (#0D0D0D) vignette. Style: photorealistic base with subtle HUD elements, premium fintech-meets-camera aesthetic, high dynamic range, crisp micro-contrast, no readable text, no brand logos, no identifiable faces. Composition: circular crop friendly — keep hero detail in center 70%, softer edges for circular mask.
```

**Negative**

```text
cartoon, anime, oversaturated HDR, readable text, watermark, Apple/Samsung UI clone, daylight flat lighting, low detail, muddy blacks, human portrait, lens brand logo
```

---

### IMG-011 · JPG Max & Multi-Frame

| Done | [ ] |
|------|-----|
| **File** | `drawable-nodpi/onboarding_jpg_max.webp` |

**Positive**

```text
Create a square conceptual illustration for computational multi-frame photography. Scene: alpine mountain ridge at golden hour duplicated as 5 slightly offset transparent ghost layers merging into one ultra-sharp final peak. Visual metaphor: alignment guides, subtle motion arrows, and an orange (#FF6B35) progress arc suggesting frame stacking complete. Dark ink (#0D0D0D) sky gradient, cool blue shadows, warm sun rim on ridgeline. Style: realistic landscape photography with clean diagrammatic overlays, ghost-free merge aesthetic, premium tech editorial, no text, no logos. Circular-safe center composition.
```

**Negative**

```text
blurry final image, misaligned collage mess, text labels, watermark, cartoon mountains, neon colors, people, camera body brand
```

---

### IMG-012 · Film LUTs & Creative Tools

| Done | [ ] |
|------|-----|
| **File** | `drawable-nodpi/onboarding_film_lut.webp` |

**Positive**

```text
Create a square cinematic color-grading illustration for a mobile film-LUT camera app. Scene: same wet urban street split by a soft vertical blend — left neutral/log flat, right warm Kodak-like amber/teal cinematic grade. Include subtle floating 3D LUT cube wireframe and a curved film-strip ribbon with purple (#7E6BF0) edge light. Background #0D0D0D with gentle fog. Style: photoreal street photography + tasteful UI metaphors, high contrast, orange (#FF6B35) accent dot on a slider shape (no readable labels). No text, no logos, circular crop safe.
```

**Negative**

```text
readable UI text, Adobe logo, Lightroom screenshot, cartoon, oversaturated teal orange cliché, faces, watermark, busy typography
```

---

## P0 — Home capture mode cards (IMG-020..025)

**Display:** 1:1 cards in `HomeModeCard.kt` with bottom scrim for title  
**Export:** 512×512 WebP quality 80–85%, **≤60 KB**

---

### IMG-020 · Pro Photo

**Positive**

```text
Square premium smartphone photography hero image for a "Pro Photo" mode card. Subject: shallow depth-of-field still life — ceramic coffee cup on slate table beside a folded linen napkin, soft window light from left, natural color, realistic contact shadows. Lower third intentionally darker (#0D0D0D gradient) for white title overlay. Tiny orange (#FF6B35) reflection hint on cup rim. Style: editorial product photo, 35mm lens look, subtle grain, no text, no logos, no faces. High micro-contrast, clean negative space top-right.
```

**Negative:** `text, watermark, HDR halos, cartoon, oversharpening, brand logo, human, busy background`

---

### IMG-021 · RAW Max

**Positive**

```text
Square RAW photography hero for "RAW Max" mode. Subject: unprocessed-looking forest interior — deep moss, fern detail, bright sky gap with recoverable highlight roll-off, rich shadow texture in tree trunks. Slight desaturated log-like color before grade, suggesting sensor latitude. Lower third dark gradient for title. Orange (#FF6B35) tiny focus peaking dot motif on a leaf edge (abstract, not UI text). Style: photoreal, medium format clarity, no text, no logos.
```

**Negative:** `overgraded instagram look, text, watermark, cartoon, people, camera product placement`

---

### IMG-022 · JPG Max

**Positive**

```text
Square ultra-sharp stacked landscape for "JPG Max" multi-frame mode. Subject: snow mountain peak and alpine lake reflection, extreme detail in rock texture and cloud edges, computational clarity without halos. Subtle orange (#FF6B35) sun glint on peak. Bottom 35% darker for title scrim. Style: premium travel photography, crisp but natural, no text, no logos.
```

**Negative:** `oversharpened crunchy HDR, text, watermark, drone watermark, people, cartoon`

---

### IMG-023 · Video Pro

**Positive**

```text
Square cinematic video mode hero. Subject: silhouette of filmmaker holding smartphone on gimbal against sunset sky, record-state glow as orange (#FF6B35) ring around record button area (abstract glow, no UI text). Warm amber sky gradient to ink (#0D0D0D) bottom. Lens flare controlled, widescreen energy in square crop. Style: photoreal lifestyle, premium mobile video branding, no readable UI, no logos, no identifiable face.
```

**Negative:** `readable UI, timeline screenshot, text, watermark, cartoon, duplicate record icons`

---

### IMG-024 · Phantom PiP

**Positive**

```text
Square dual-camera PiP concept for "Phantom PiP" mode. Main view: scenic coastal cliff and ocean golden hour. Corner inset: smaller rounded-rectangle selfie silhouette on tripod (no identifiable face — back/side angle only). Purple (#7E6BF0) inset border glow, orange (#FF6B35) tiny capture indicator dot. Dark ink corners, premium app marketing style, no text, no logos, no Samsung/Apple UI clone.
```

**Negative:** `identifiable face, readable UI text, watermark, cartoon, messy collage, brand logos`

---

### IMG-025 · Film & LUT (card)

**Positive**

```text
Square film emulation hero for LUT mode card. Subject: rainy neon street at night with warm film grain, amber highlights, teal shadows, subtle halation around lights. Floating translucent color cube and film canister shapes as abstract props (no brand names). Orange (#FF6B35) and purple (#7E6BF0) accent lights. Bottom darker scrim. Style: cinematic still, photoreal, no text, no logos.
```

**Negative:** `readable neon signs, Kodak logo, text, watermark, cartoon, flat illustration`

---

## P1 — IMG-030 · Film/LUT wide banner

| Field | Value |
|-------|-------|
| **File** | `drawable-nodpi/home_banner_film_lut.webp` |
| **Size** | `landscape` 1024×512 |
| **Quality** | `high` → WebP ≤80 KB |

**Positive**

```text
Create a 2:1 landscape banner for a mobile film LUT library feature. Scene: dark studio tabletop with vintage film canisters (generic labels), scattered 3D LUT cubes, color swatch chips, and a backlit strip of 35mm film curling across frame. Key light warm amber from left, purple (#7E6BF0) rim from right, orange (#FF6B35) accent line under swatches. Background #0D0D0D fading to #1A1A1A. Style: premium product still life, shallow depth of field, photoreal, no readable text on cans, no Kodak/Fuji logos, no watermark. Leave left third slightly darker for title overlay.
```

**Negative:** `readable brand names, text, watermark, cartoon, cluttered mess, low light noise`

---

## Batch CLI helper

Generate all home cards (after reviewing one sample):

```bash
ASSETS=(pro_photo raw_max jpg_max video_pro phantom film_lut)
IDS=(020 021 022 023 024 025)
for i in "${!ASSETS[@]}"; do
  echo "Generate IMG-${IDS[$i]} — paste prompt from brief for ${ASSETS[$i]}"
  # gpt-image --model gpt-image-2.5-flare -p "..." --size square --quality medium \
  #   -f "app/src/main/res/drawable-nodpi/home_card_${ASSETS[$i]}.png"
done
```

Post-process to WebP:

```bash
# Example with cwebp if installed
cwebp -q 82 input.png -o app/src/main/res/drawable-nodpi/home_card_pro_photo.webp
```

---

## Wire code (after assets exist)

1. **Onboarding** — add `imageRes` to `OnboardingPage` in `OnboardingScreen.kt`, replace gradient `Box` with `Image(painterResource(...))`.
2. **Home cards** — add optional `backgroundRes` to `HomeMode` enum; load in `HomeModeCard.kt` with `AsyncImage` or `Image` + scrim.
3. **Launcher** — update adaptive icon XML layers.
4. **Splash** — optional `Image` in `SplashScreen.kt` using IMG-002.

---

## QA checklist (per asset)

- [ ] No readable text / watermarks / third-party logos
- [ ] Orange accent present but not overwhelming
- [ ] Bottom/third dark enough for white title overlay (cards)
- [ ] File size within budget (WebP)
- [ ] Looks correct in circular crop (onboarding)
- [ ] Brand says **CameraMega**, not SuperCamera

---

## Completion tracker

- [ ] IMG-001 Launcher
- [ ] IMG-002 Splash logo
- [ ] IMG-010..012 Onboarding
- [ ] IMG-020..025 Home cards
- [ ] IMG-030 Film banner

---

## References

- [GPT-Image2-Skill](https://github.com/wuyoscar/GPT-Image2-Skill) — prompt gallery + CLI
- `.cursor/skills/gpt-image/SKILL.md` — agent runbook
- `.cursor/skills/cameramega-image-assets/SKILL.md` — CameraMega-specific workflow
