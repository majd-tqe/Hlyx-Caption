# Contributing — Adding Features & Improvements

<p class="kicker">Small, tested, single-concern PRs</p>

## Adding a New Setting

1. **Config:** add field + default to `OverlayConfig` in `config.h`.
2. **Parse/Save:** extend `ParseConfigText` (config_parse.cpp, `key ==` on `std::string`) and `hooks.cpp:SaveConfig` (`WritePrivateProfileStringW` per section). Keep INI key == HTML id.
3. **Renderer:** snapshot it in `CaptionEntry` (`rendered*`) if it affects shaping/texture; handle in `UpdateQueue` rebuild logic (`caption_queue.cpp`).
4. **Shaping/UI:** if typography, wire through `TextShaper` (`shaper.h`).
5. **HTML panel:** add control to `assets/settings.html` (id == INI key), style in `settings.css`, bridge in `bindings.js` (`_setValue[key]` + `snapshot()` + flag handling). Handle gaps: alpha, shadow colors, `bg_*` currently stubbed.
6. **Bridge:** `UltralightManager::ApplyConfigField*` (`strcmp` dispatch) + `RefreshAllFromConfig` push.
7. **Regen + build:**
   ```powershell
   pwsh HlyxCaption/ui_ultralight/generate_embedded_ui.ps1
   msbuild HlyxCaption.sln -p:Configuration=Release -p:Platform=x64
   ```
8. **Docs:** update [Configuration](configuration.md) table and `dist/resources/settings.ini` defaults (raw values, no comment lines).

## Adding a New Caption Tag

- Grammar lives in `caption_parser.cpp:ParseCaptionText` + `CaptionFormatState`. Follow `<clr:r,g,b>`, `<I>/<B>`, `<cr>`, `<delay:N>` pattern — unknown tags are silently dropped. Persist state across `<sb>` parts, then verify against live game captions.

## Adding a UI Tab / Control

- Tabs are hard-coded in `settings.html` (8 visible, including General). Add section markup + `settings.css` + `bindings.js` entry, then regenerate `embedded_ui.h`. Keep `dir="rtl"` hard-code in mind — see [Localization](localization.md).

## Adding a Language

The old C++ side (`lang/` headers, `Localizer`) is gone — deleted with the legacy panel. Translation now means editing the panel assets directly. See [Localization → Adding a New Language](localization.md#adding-a-new-language): translate `settings.html` labels, cover the script with fonts, regen `embedded_ui.h`, document the `ui_language` value.

## Good PRs

- One feature per PR, and update the relevant files under `docs/wiki` when documentation changes.
- Include `wininet_hook.log` snippet and before/after screenshots for visual changes.
- Keep `arabic_fallback_data.h` generated — don't hand-edit.
