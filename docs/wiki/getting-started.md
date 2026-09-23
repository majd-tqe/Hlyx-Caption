# Getting Started

> Source: `C:\path\to\Hlyx_Caption` · `HlyxCaption/HlyxCaption.vcxproj` → `wininet.dll`

## Prerequisites

- **Windows 10/11 x64**
- **Visual Studio 2022** with C++ desktop workload, **Windows SDK 10** (latest), **v143** toolset.
- **vcpkg** with triplet **`x64-windows-static-md`**. Set `VCPKG_ROOT` (or pass `-p:VcpkgRoot=...`) to the vcpkg installation. Required static libs from that triplet:
  `harfbuzz.lib`, `freetype.lib` (and `freetyped.lib` for Debug), `fribidi.lib`, plus transitive deps `brotlicommon`/`brotlidec`, `bz2`/`bzip2d`, `libpng16`/`libpng16d`, `zlib`/`zlibd`. Built with `/NODEFAULTLIB:brotlicommon.lib brotlidec.lib`.
- **Ultralight SDK** obtained separately under a path supplied by `ULTRALIGHT_SDK_ROOT` (or `-p:UltralightSdkRoot=...`). The SDK is proprietary and is intentionally not tracked. Follow its license for any runtime DLLs or resources included in a release.
- No project-owned `.asm` files are present, but the vcxproj still imports `masm.props` and `masm.targets` from the Visual Studio build customizations.

## Installation

```bash
# 1. Configure dependency roots for this shell
$env:VCPKG_ROOT = 'C:\path\to\vcpkg'
$env:ULTRALIGHT_SDK_ROOT = 'C:\path\to\ultralight'

# 2. Install the vcpkg manifest for the required triplet
vcpkg install --triplet x64-windows-static-md --feature-flags=manifests

# 3. Open the solution and build (Release is the deployable one)
msbuild HlyxCaption.sln -p:Configuration=Release -p:Platform=x64
# — or Build → Build Solution in Visual Studio

# Output: HlyxCaption/x64/Release/wininet.dll  (TargetName=wininet, ModuleDefinitionFile=wininet.def)
# Project is C++17 (/std:c++17), /utf-8, Unicode, MaxSpeed+FavorSize, SDLCheck=false, PCH disabled
```

## First Run (Game)

```bash
# Deploy next to the game (next to hlvr.exe)
# Source files: built wininet.dll + everything in HlyxCaption/dist/
xcopy /E /I HlyxCaption\dist\* "C:\Program Files (x86)\Steam\steamapps\common\Half-Life Alyx\game\bin\win64\"
copy  HlyxCaption\x64\Release\wininet.dll "C:\Program Files (x86)\Steam\steamapps\common\Half-Life Alyx\game\bin\win64\wininet.dll"

# Expected layout next to hlvr.exe:
#   wininet.dll                ← the mod
#   Ultralight.dll / UltralightCore.dll / WebCore.dll / AppCore.dll
#   Cairo-Regular.ttf          ← default caption font
#   resources\settings.ini     ← plain default config (copy from dist/resources/)
#   resources\icudt67l.dat / cacert.pem
#   Any user fonts → resources\*.ttf|otf|ttc|otc|woff|woff2

# Launch the game normally (Steam). Check the log next to hlvr.exe:
#   wininet_hook.log — DllMain/MainThread/sig-scan/hook status + crash diagnostics
# _DEBUG builds also AllocConsole.

# In-game hotkeys:
#   F10 — toggle settings panel (WM_KEYDOWN and WM_SYSKEYDOWN both handled — F10 is a system key)
#   F11 — toggle caption overlay visibility
# Settings persist to resources/settings.ini on Save (حفظ) in the panel.
```

## Common Workflows

### Rebuild after editing the settings UI

```bash
# After ANY edit to settings.html / settings.css / bindings.js / banner.png / fonts/*.woff2:
pwsh HlyxCaption\ui_ultralight\generate_embedded_ui.ps1
# Inlines CSS→<style>, JS→<script>, banner.png→base64 data URI ({{BANNER_URI}}),
# emits chunked raw strings (≤7000 bytes, MSVC C2026) into embedded_ui.h,
# then regex round-trip verifies byte-for-byte (exits 1 on mismatch).
msbuild HlyxCaption.sln -p:Configuration=Release -p:Platform=x64
```

### Tweak the welcome banner (no UI regeneration needed)

Edit `HlyxCaption/banner_config.h` (`BannerConfig::kTopPct/kLogoMaxPx/kFadeInSec…` — all `@1080p` and scaled at runtime by `viewportH/1080`), then rebuild. Values are injected as JS at runtime, overriding `settings.css` defaults.

### Add a user font

Drop the file into `{ModDir}\resources\` (`*.ttf|otf|ttc|otc|woff|woff2`), pick it in the settings panel's font dropdowns (populated by enumerating that directory), Save. The render thread detects the path change and calls `ReloadFontPreserveQueue()` (takes `m_CS`, safe to call from render/UL threads).

### Change language

Set `ui_language` to `1`=EN or `2`=AR (default) in the **General** tab of the HTML panel. The change is applied immediately; press **Save** to persist it to `settings.ini`. Manual edits to `[UI] ui_language=…` take effect after restarting the game. The value is parsed in `config_parse.cpp` and applied by the panel JS through `UltralightManager.cpp`. Invalid or legacy `0` values fall back to Arabic. `ui_direction_override` and `ui_scale` are compatibility fields with no current HTML control or runtime consumer.

## Configuration

- **`resources/settings.ini`** — sections `[Text][Shadow][Outline][Position][Timing][Typography][WordWrap][Mouse][Background][UI]`; keys match `OverlayConfig` fields exactly (see [modules/configuration.md](modules/configuration.md)). Fresh files become **UTF-16LE with BOM** after `SaveConfig` (`WritePrivateProfileStringW`); `ReadFileToUtf8` handles both UTF-8 and UTF-16LE on load — don't assume UTF-8.
- **`{ModDir}` token** — string values containing `{ModDir}` are expanded to the DLL's directory at parse time (`g_ModDirA`).
- **`HlyxCaption/banner_config.h`** — startup banner geometry/timing (not part of `OverlayConfig`).
- Env/build: `%VCPKG_ROOT%\installed\x64-windows-static-md` and `%ULTRALIGHT_SDK_ROOT%`.

## Where to Go Next

- Architecture & data flow: [architecture.md](architecture.md)
- Module reference: [index.md#module-map](index.md#module-map)
- Class diagram: [diagrams/class-diagram.md](diagrams/class-diagram.md)
- Sequence diagrams: [diagrams/sequences.md](diagrams/sequences.md)

## Troubleshooting

| Symptom | Cause / Fix |
|---|---|
| Captions never appear after a game update | The byte pattern in `hooks.cpp` (`F3 0F 11 5C 24 ? 48 89 54 24`, single `FindSignature` scan of `client.dll`, retried ~30 s) changed — update it; check `wininet_hook.log` for "signature not found" |
| Settings revert after Save | Old bug was `ReadFileToUtf8` missing UTF-16LE BOM handling — already fixed; if it recurs check `config_parse.cpp` |
| `C2026 string too large` after editing UI | `generate_embedded_ui.ps1` chunk size exceeded — re-run it; don't hand-edit `embedded_ui.h` |
| `cannot open harfbuzz.lib` at link | Set `VCPKG_ROOT`, run `vcpkg install --triplet x64-windows-static-md`, and verify the manifest triplet |
| Panel stuck off-screen / no animation | Check `window.__panelHidden` / `window.__bannerDone` flags; `OnPanelOpened()` replays the slide-in |
