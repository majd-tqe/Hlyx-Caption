# Development Workflow

<p class="kicker">From clone to <code>wininet.dll</code> beside <code>hlvr.exe</code></p>

## Prerequisites

- **VS2022** (v143), **Windows SDK 10**, **x64 only**, C++17 `/utf-8` Unicode
- **vcpkg** triplet `x64-windows-static-md`, configured through `VCPKG_ROOT` and `vcpkg.json` — `harfbuzz`, `freetype`(+d), `fribidi`, deps `brotlicommon/libdec`, `bz2`, `libpng16`, `zlib`
- Link: `/NODEFAULTLIB:brotlicommon.lib brotlidec.lib`, plus `d3d11 dxgi ntdll` and `Ultralight{,Core,AppCore}.lib`
- Ultralight SDK supplied outside Git through `ULTRALIGHT_SDK_ROOT`; it is proprietary and must be licensed separately from this project

## Build

```bash
msbuild HlyxCaption.sln -p:Configuration=Release -p:Platform=x64
# output: HlyxCaption/x64/Release/wininet.dll (TargetName=wininet, ModuleDefinitionFile=wininet.def)
# PCH disabled (pch.h/cpp vestigial), MASM props imported but no .asm
```

## Deploy

```powershell
$dst = "C:\Program Files (x86)\Steam\steamapps\common\Half-Life Alyx\game\bin\win64"
Copy-Item HlyxCaption\x64\Release\wininet.dll $dst
Copy-Item -Recurse HlyxCaption\dist\* $dst
# expected beside hlvr.exe: wininet.dll + Ultralight*.dll + WebCore.dll + AppCore.dll
# + Cairo-Regular.ttf + resources/settings.ini + resources/icudt67l.dat + resources/cacert.pem
# + any user *.ttf|otf|woff2 in resources/
```

Logs: `wininet_hook.log` next to host exe. `_DEBUG` also `AllocConsole`.

## Versioning

- The Windows DLL version is defined in `HlyxCaption/version.rc` (`0.9.5.0`). Keep the `package.json` and `vcpkg.json` package versions (`0.9.5`) synchronized when updating the project version.

## Editing the Settings UI

```powershell
# After ANY edit to settings.html / settings.css / bindings.js / banner.png / fonts/*.woff2:
pwsh HlyxCaption/ui_ultralight/generate_embedded_ui.ps1
# inlines CSS→<style>, JS→<script>, banner.png→base64 {{BANNER_URI}},
# emits ≤7000-byte R"Cn(…)Cn" chunks (MSVC C2026) + regex round-trip check (exit 1 on mismatch)
```
Banner-only tweaks → just edit `banner_config.h` (runtime JS injection, no regen).

## Testing

No standalone rig ships with the repo, so verification happens in game. Deploy the DLL beside `hlvr.exe`, launch through Steam, confirm captions and the F10 panel behave, then read `wininet_hook.log` next to the host exe for boot and hook status.

## Debugging

- `wininet_hook.log` is the first stop — crash handlers log faulting module+offset via `EnumProcessModules`.
- Sig-scan failure (captions missing after update) → the single pattern in `hooks.cpp` (`FindSignature` on `client.dll`): `F3 0F 11 5C 24 ? …` — first suspect.
- Panel stuck: `OnPanelOpened()` replays the slide-in; suspect `window.__panelHidden` / `__bannerDone` state.
- `Renderer::m_CS` is the single lock for queue+shaper (shaper has no mutex). `UltralightManager` has its own `mutex`+`deque<PendingEvent>`.

## Code Style & PR Checklist

- C++17, `/utf-8`, keep diffs minimal (existing style), no drive-by reformats.
- Verify `generate_embedded_ui.ps1` round-trip if touching UI assets.
- Check `wininet_hook.log` after every in-game test run.
- INI key → HTML id must match (`font_size`, `outline_thickness`, …) — JS bridge depends on it.
- Keep `arabic_fallback_data.h` generated — don’t hand-edit.
