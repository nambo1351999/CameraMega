from __future__ import annotations

from dataclasses import dataclass
from typing import Literal

Quality = Literal["low", "medium", "high", "auto"]
Priority = Literal["P0", "P1", "P2"]
Group = Literal["icon", "splash", "onboarding", "home", "banner"]


@dataclass(frozen=True)
class AssetSpec:
    asset_id: str
    name: str
    group: Group
    priority: Priority
    positive: str
    negative: str
    output_path: str
    model: str = "gpt-image-2.5-flare"
    size: str = "1024x1024"
    quality: Quality = "high"
    output_format: str = "png"
    background: str | None = "opaque"
    resize_to: tuple[int, int] | None = None
    webp_quality: int | None = None
    max_bytes: int | None = None

    def full_prompt(self) -> str:
        return f"{self.positive.strip()}\n\nAvoid: {self.negative.strip()}"


def _p(positive: str, negative: str) -> tuple[str, str]:
    return positive.strip(), negative.strip()


ASSETS: dict[str, AssetSpec] = {}

def _register(spec: AssetSpec) -> None:
    ASSETS[spec.asset_id.upper()] = spec


# ── P0 Icon & Splash ─────────────────────────────────────────────────────────

_pos, _neg = _p(
    """Create a square premium Android adaptive app icon for a fictional pro mobile camera app called "Camera Mega". Center a stylized camera lens symbol: concentric glass rings, subtle inner reflections, and a bold orange (#FF6B35) focus/zoom ring as the hero accent. Add a restrained purple (#7E6BF0) lens flare streak at the upper-right edge only. Background: flat deep ink (#0D0D0D) filling the entire canvas. Style: flat-vector meets subtle 3D product render, crisp edges, high contrast, minimal geometry, no text, no letters, no watermark, no Apple/Samsung logos. Composition: perfectly centered, generous padding inside the Android adaptive icon safe zone (central 66% circle), icon reads clearly at 48dp. Mood: professional, cinematic, trustworthy, modern photography tool.""",
    "text, letters, wordmark, watermark, screenshot, UI mockup, cartoon mascot, low poly, noisy gradient, rainbow colors, busy background, human face, brand logos, Samsung, Apple, Google camera icon clone, blurry, jpeg artifacts, off-center crop",
)
_register(AssetSpec(
    asset_id="IMG-001",
    name="Adaptive launcher icon",
    group="icon",
    priority="P0",
    positive=_pos,
    negative=_neg,
    output_path="app/src/main/res/drawable-nodpi/ic_launcher_generated.png",
    size="1024x1024",
    quality="high",
    output_format="png",
    background="opaque",
))

_pos, _neg = _p(
    """Create a square transparent-background splash logo mark for "Camera Mega" mobile camera app. Subject: abstract aperture/lens glyph built from 6–8 geometric blades forming a hexagonal opening, with an orange (#FF6B35) outer ring and cool gray inner glass. Subtle purple (#7E6BF0) specular highlight on one blade only. No wordmark, no letters, no text — symbol only. Style: minimal vector-like mark with soft 3D bevel, crisp silhouette, works on #0D0D0D background. Centered, equal padding on all sides, high legibility at 120dp display size.""",
    "text, typography, camera mega wording, watermark, photo background, realistic camera product photo, clutter, gradient backdrop, cartoon, low resolution, uneven padding",
)
_register(AssetSpec(
    asset_id="IMG-002",
    name="Splash logo mark",
    group="splash",
    priority="P0",
    positive=_pos,
    negative=_neg,
    output_path="app/src/main/res/drawable-nodpi/splash_logo.png",
    size="1024x1024",
    quality="high",
    output_format="png",
    background="transparent",
    resize_to=(512, 512),
))

# ── P0 Onboarding ────────────────────────────────────────────────────────────

_onboarding = [
    (
        "IMG-010",
        "onboarding_raw_pro.webp",
        """Create a square cinematic product-illustration for a pro mobile RAW photography feature. Scene: night city rooftop with distant skyline bokeh; in the foreground a floating semi-transparent DNG file tile and a wireframe histogram overlay suggesting unprocessed sensor data. Show rich shadow detail in building windows and a single orange (#FF6B35) UI accent glow along the bottom edge like a camera app capture bar. Background deep ink (#0D0D0D) vignette. Style: photorealistic base with subtle HUD elements, premium fintech-meets-camera aesthetic, high dynamic range, crisp micro-contrast, no readable text, no brand logos, no identifiable faces. Composition: circular crop friendly — keep hero detail in center 70%, softer edges for circular mask.""",
        "cartoon, anime, oversaturated HDR, readable text, watermark, Apple/Samsung UI clone, daylight flat lighting, low detail, muddy blacks, human portrait, lens brand logo",
    ),
    (
        "IMG-011",
        "onboarding_jpg_max.webp",
        """Create a square conceptual illustration for computational multi-frame photography. Scene: alpine mountain ridge at golden hour duplicated as 5 slightly offset transparent ghost layers merging into one ultra-sharp final peak. Visual metaphor: alignment guides, subtle motion arrows, and an orange (#FF6B35) progress arc suggesting frame stacking complete. Dark ink (#0D0D0D) sky gradient, cool blue shadows, warm sun rim on ridgeline. Style: realistic landscape photography with clean diagrammatic overlays, ghost-free merge aesthetic, premium tech editorial, no text, no logos. Circular-safe center composition.""",
        "blurry final image, misaligned collage mess, text labels, watermark, cartoon mountains, neon colors, people, camera body brand",
    ),
    (
        "IMG-012",
        "onboarding_film_lut.webp",
        """Create a square cinematic color-grading illustration for a mobile film-LUT camera app. Scene: same wet urban street split by a soft vertical blend — left neutral/log flat, right warm Kodak-like amber/teal cinematic grade. Include subtle floating 3D LUT cube wireframe and a curved film-strip ribbon with purple (#7E6BF0) edge light. Background #0D0D0D with gentle fog. Style: photoreal street photography + tasteful UI metaphors, high contrast, orange (#FF6B35) accent dot on a slider shape (no readable labels). No text, no logos, circular crop safe.""",
        "readable UI text, Adobe logo, Lightroom screenshot, cartoon, oversaturated teal orange cliché, faces, watermark, busy typography",
    ),
]
for asset_id, filename, pos, neg in _onboarding:
    _register(AssetSpec(
        asset_id=asset_id,
        name=filename.replace("_", " ").replace(".webp", "").title(),
        group="onboarding",
        priority="P0",
        positive=pos,
        negative=neg,
        output_path=f"app/src/main/res/drawable-nodpi/{filename}",
        quality="high",
        output_format="png",
        resize_to=(800, 800),
        webp_quality=85,
        max_bytes=120_000,
    ))

# ── P0 Home cards ────────────────────────────────────────────────────────────

_home_cards = [
    (
        "IMG-020",
        "home_card_pro_photo.webp",
        """Square premium smartphone photography hero image for a "Pro Photo" mode card. Subject: shallow depth-of-field still life — ceramic coffee cup on slate table beside a folded linen napkin, soft window light from left, natural color, realistic contact shadows. Lower third intentionally darker (#0D0D0D gradient) for white title overlay. Tiny orange (#FF6B35) reflection hint on cup rim. Style: editorial product photo, 35mm lens look, subtle grain, no text, no logos, no faces. High micro-contrast, clean negative space top-right.""",
        "text, watermark, HDR halos, cartoon, oversharpening, brand logo, human, busy background",
    ),
    (
        "IMG-021",
        "home_card_raw_max.webp",
        """Square RAW photography hero for "RAW Max" mode. Subject: unprocessed-looking forest interior — deep moss, fern detail, bright sky gap with recoverable highlight roll-off, rich shadow texture in tree trunks. Slight desaturated log-like color before grade, suggesting sensor latitude. Lower third dark gradient for title. Orange (#FF6B35) tiny focus peaking dot motif on a leaf edge (abstract, not UI text). Style: photoreal, medium format clarity, no text, no logos.""",
        "overgraded instagram look, text, watermark, cartoon, people, camera product placement",
    ),
    (
        "IMG-022",
        "home_card_jpg_max.webp",
        """Square ultra-sharp stacked landscape for "JPG Max" multi-frame mode. Subject: snow mountain peak and alpine lake reflection, extreme detail in rock texture and cloud edges, computational clarity without halos. Subtle orange (#FF6B35) sun glint on peak. Bottom 35% darker for title scrim. Style: premium travel photography, crisp but natural, no text, no logos.""",
        "oversharpened crunchy HDR, text, watermark, drone watermark, people, cartoon",
    ),
    (
        "IMG-023",
        "home_card_video_pro.webp",
        """Square cinematic video mode hero. Subject: silhouette of filmmaker holding smartphone on gimbal against sunset sky, record-state glow as orange (#FF6B35) ring around record button area (abstract glow, no UI text). Warm amber sky gradient to ink (#0D0D0D) bottom. Lens flare controlled, widescreen energy in square crop. Style: photoreal lifestyle, premium mobile video branding, no readable UI, no logos, no identifiable face.""",
        "readable UI, timeline screenshot, text, watermark, cartoon, duplicate record icons",
    ),
    (
        "IMG-024",
        "home_card_phantom.webp",
        """Square dual-camera PiP concept for "Phantom PiP" mode. Main view: scenic coastal cliff and ocean golden hour. Corner inset: smaller rounded-rectangle selfie silhouette on tripod (no identifiable face — back/side angle only). Purple (#7E6BF0) inset border glow, orange (#FF6B35) tiny capture indicator dot. Dark ink corners, premium app marketing style, no text, no logos, no Samsung/Apple UI clone.""",
        "identifiable face, readable UI text, watermark, cartoon, messy collage, brand logos",
    ),
    (
        "IMG-025",
        "home_card_film_lut.webp",
        """Square film emulation hero for LUT mode card. Subject: rainy neon street at night with warm film grain, amber highlights, teal shadows, subtle halation around lights. Floating translucent color cube and film canister shapes as abstract props (no brand names). Orange (#FF6B35) and purple (#7E6BF0) accent lights. Bottom darker scrim. Style: cinematic still, photoreal, no text, no logos.""",
        "readable neon signs, Kodak logo, text, watermark, cartoon, flat illustration",
    ),
]
for asset_id, filename, pos, neg in _home_cards:
    _register(AssetSpec(
        asset_id=asset_id,
        name=filename.replace("home_card_", "").replace(".webp", "").replace("_", " ").title(),
        group="home",
        priority="P0",
        positive=pos,
        negative=neg,
        output_path=f"app/src/main/res/drawable-nodpi/{filename}",
        quality="medium",
        output_format="png",
        resize_to=(512, 512),
        webp_quality=82,
        max_bytes=60_000,
    ))

# ── P0 Home quick actions ────────────────────────────────────────────────────

_quick_actions = [
    (
        "IMG-026",
        "home_quick_capture.webp",
        """Create a 2:1 landscape hero thumbnail for a mobile camera app "Capture Now" quick-action card. Subject: close-up of hands holding a smartphone in landscape orientation, about to tap shutter — warm orange (#FF6B35) glow emanates from the shutter area (abstract, no readable UI). Background: deep ink (#0D0D0D) with soft bokeh city lights. Left third slightly darker for white title overlay. Style: photoreal lifestyle, premium mobile photography, shallow depth of field, subtle grain, no text, no logos, no identifiable face (hands only).""",
        "readable UI text, watermark, cartoon, identifiable face, brand logos, Samsung/Apple UI clone, oversaturated HDR",
    ),
    (
        "IMG-027",
        "home_quick_gallery.webp",
        """Create a 2:1 landscape hero thumbnail for a mobile camera app "Open Gallery" quick-action card. Subject: elegant floating mosaic of 6–8 rounded photo tiles showing abstract landscapes and still-life (no faces), arranged in a staggered grid with purple (#7E6BF0) rim light on tile edges. Dark ink (#0D0D0D) background, subtle orange (#FF6B35) accent dot on one tile corner. Left third darker for title overlay. Style: photoreal gallery metaphor, premium app marketing, clean composition, no text, no logos, no watermark.""",
        "readable filenames, text labels, watermark, cartoon, identifiable faces, Google Photos UI clone, messy collage",
    ),
]
for asset_id, filename, pos, neg in _quick_actions:
    _register(AssetSpec(
        asset_id=asset_id,
        name=filename.replace("home_quick_", "").replace(".webp", "").replace("_", " ").title(),
        group="home",
        priority="P0",
        positive=pos,
        negative=neg,
        output_path=f"app/src/main/res/drawable-nodpi/{filename}",
        size="1536x768",
        quality="medium",
        output_format="png",
        resize_to=(640, 320),
        webp_quality=82,
        max_bytes=50_000,
    ))

# ── P1 Banner ───────────────────────────────────────────────────────────────

_pos, _neg = _p(
    """Create a 2:1 landscape banner for a mobile film LUT library feature. Scene: dark studio tabletop with vintage film canisters (generic labels), scattered 3D LUT cubes, color swatch chips, and a backlit strip of 35mm film curling across frame. Key light warm amber from left, purple (#7E6BF0) rim from right, orange (#FF6B35) accent line under swatches. Background #0D0D0D fading to #1A1A1A. Style: premium product still life, shallow depth of field, photoreal, no readable text on cans, no Kodak/Fuji logos, no watermark. Leave left third slightly darker for title overlay.""",
    "readable brand names, text, watermark, cartoon, cluttered mess, low light noise",
)
_register(AssetSpec(
    asset_id="IMG-030",
    name="Film LUT banner",
    group="banner",
    priority="P1",
    positive=_pos,
    negative=_neg,
    output_path="app/src/main/res/drawable-nodpi/home_banner_film_lut.webp",
    size="1536x768",
    quality="high",
    output_format="png",
    resize_to=(1024, 512),
    webp_quality=85,
    max_bytes=80_000,
))


def list_assets(
    *,
    priority: Priority | None = None,
    group: Group | None = None,
) -> list[AssetSpec]:
    items = list(ASSETS.values())
    if priority:
        items = [a for a in items if a.priority == priority]
    if group:
        items = [a for a in items if a.group == group]
    return sorted(items, key=lambda a: a.asset_id)


def get_asset(asset_id: str) -> AssetSpec:
    key = asset_id.upper()
    if key not in ASSETS:
        known = ", ".join(sorted(ASSETS))
        raise KeyError(f"Unknown asset {asset_id!r}. Known: {known}")
    return ASSETS[key]
