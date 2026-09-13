# CameraMega — Agent Context

## Project

**Camera Mega** (`com.mega.superx.filter.camera`) — professional Android camera app.

- **Stack:** Kotlin, Jetpack Compose, Camera2/CameraX, DataStore, native C++ (CMake)
- **Package:** `com.mega.superx.filter.camera`
- **Branding:** Always **CameraMega** / **Camera Mega** — never rename to SuperCamera

## Navigation flow (2026-09)

```
Splash → Language (first time) → Onboarding (first launch) → Home → Camera / Gallery / …
```

Key paths:
- Flow UI: `app/src/main/java/.../ui/flow/`
- Main nav: `MainActivity.kt` (`Routes`, `NavigationHost`)
- Image generation brief: `IMAGE_ASSETS_BRIEF.md`

## CodeLocal

This workspace includes [CodeLocal](https://github.com/codelocal-cloud/codelocal) at `../codelocal/`.

Install CLI (Node 20+):

```bash
npm i -g codelocal
cd /path/to/CameraMega
codelocal .
codelocal
```

MCP endpoint: `https://codelocal.cloud/mcp`

Typical prompt:

```
@CodeLocal inspect project_info first, understand CameraMega, then make the requested change and run relevant checks.
```

## Conventions

- Dark theme default; accent orange `#FF6B35`
- No Hilt — ViewModels via `activity.viewModels()`
- User prefs: `UserPreferencesRepository` / DataStore
- Minimize diff scope; match existing naming and patterns

## Asset generation (GPT Image 2/2.5)

Skills vendored from [GPT-Image2-Skill](https://github.com/wuyoscar/GPT-Image2-Skill):

| Skill | Path |
|-------|------|
| CameraMega assets | `.cursor/skills/cameramega-image-assets/SKILL.md` |
| GPT Image CLI | `.cursor/skills/gpt-image/SKILL.md` |
| Image → prompt | `.cursor/skills/get-prompt-from-image/SKILL.md` |

Prompt library: **`IMAGE_ASSETS_BRIEF.md`** — each asset has Positive/Negative prompt + CLI block.  
Brand palette: `#0D0D0D`, `#FF6B35`, `#7E6BF0`. **Never** SuperCamera branding.

```bash
# CameraMega Python tool (requires OPENAI_API_KEY only)
cd tools/image-gen && pip install -e .
export OPENAI_API_KEY='sk-...'
cameramega-image-gen gen --p0

# Upstream gpt-image CLI
uvx --from git+https://github.com/wuyoscar/gpt_image_2_skill gpt-image \
  --model gpt-image-2.5-flare -p "..." --size square --quality high -f out.png
```
