# Localization & Translation

<p class="kicker">HTML panel i18n · JS-side · <code>ui_language</code> dropdown</p>

> All user-facing UI translations live in the Ultralight panel assets: labels in
> `settings.html` and language-specific strings in `bindings.js`.

## How It Works

- The panel starts with Arabic markup but switches its `lang`/`dir` attributes at runtime.
- The visible General tab exposes `ui_language` (1=EN, 2=AR) as an `.fdrop`
  dropdown (`wireDrop("ui_language")` in `bindings.js`); the value round-trips
  through `window.hlaConfig.snapshot()` into `g_Config.ui_language` and persists
  in `settings.ini` when Save is pressed. The selected language is applied immediately to all static
  and dynamic panel strings; invalid or legacy `0` values fall back to Arabic.
- `ui_direction_override`, `ui_scale`, and `ui_animations` are currently compatibility fields without HTML controls.
- Caption shaping (Arabic presentation forms, bidi) is unrelated to panel language —
  see [Language Support](language-support.md).

## Translation Guide

1. **Panel labels** — edit `assets/settings.html` directly (labels are literal).
   If layout depends on string length, adjust `assets/settings.css`.
2. **JS bridge keys** — if a renamed control has a `window.hlaConfig._setValue` /
   `snapshot()` binding in `assets/bindings.js`, keep the element id in sync
   (ids double as INI keys via `ApplyConfigField`).
3. **Rebuild the embedded page:**
   ```powershell
   pwsh HlyxCaption/ui_ultralight/generate_embedded_ui.ps1
   ```
4. **Test** — launch the game, open the panel with F10, change a value, Save, and confirm it persists in `settings.ini`.

## Adding a New Language

1. Translate `assets/settings.html` labels (and any `bindings.js` user-visible strings).
2. If the language needs a new script, ensure `custom_font_path` / `fallback_font_path`
   cover it (see [Language Support](language-support.md)).
3. Rebuild `embedded_ui.h` with the script above and document the new `ui_language`
   value in [Configuration](configuration.md).
