# FAQ & Troubleshooting

## Hotkeys

| Key | Action | Notes |
|---|---|---|
| **F10** | Toggle settings panel | Checked on `WM_KEYDOWN` **and** `WM_SYSKEYDOWN` (F10 is system key). |
| **F11** | Toggle all caption visibility | Flips `m_OverlayVisible` → `window.hlaBanner.setTranslationVisible`, independent of SFX filtering. |

Startup banner shows once per session; native stops rendering closed overlay after `window.__bannerDone`.

## How do I hide sound-effect captions?

The mod follows the game's live `cc_subtitles` ConVar, not a mod INI or panel option. When its integer value is nonzero, any incoming raw caption containing exact `<sfx>` is discarded **in full** before splitting `<sb>` parts (including mixed SFX/dialogue captions). Otherwise SFX entries appear immediately after their scheduled start and fade out normally; the parser silently strips `<sfx>` as an unknown tag. If the ConVar pointer cannot be found, SFX stays visible. F11 hides all captions regardless of this setting.

## My settings revert after Save

`SaveConfig` writes **UTF-16LE BOM** via `WritePrivateProfileStringW`. Old bug was byte-read losing it — fixed in `ReadFileToUtf8` (BOM sniff to UTF-8). If it recurs, `config_parse.cpp` is the site.

## Captions disappeared after a game update

Byte patterns in `hooks.cpp` are fragile — check `wininet_hook.log` for “signature not found”, then update the caption-function pattern:

```
F3 0F 11 5C 24 ? 48 89 54 24
```

If only SFX filtering stops working, check the separate `cc_subtitles` pointer discovery in `client.dll`'s `Process`; `IsSfxHidden()` reads its integer at `+0x58` on each check and defaults to showing SFX if discovery fails.

## Build fails

| Error | Fix |
|---|---|
| `cannot open harfbuzz.lib / freetype.lib` | Set `VCPKG_ROOT`, run `vcpkg install --triplet x64-windows-static-md`, and verify the manifest |
| `C2026 string too large` | `embedded_ui.h` chunk >8.5 KB — rerun `pwsh generate_embedded_ui.ps1`, don’t hand-edit |

## Panel is blank / stuck off-screen

`OnPanelOpened()` replays the `translateX(100%)` → slide-in; if it never fires, suspect `__panelHidden` / `__bannerDone` state.

## Mouse is stuck or trembles

- Open panel suppresses the currently enabled `SetCursorPos` hook plus Raw Input software cursor. `ClipCursor` and `SetCapture` hooks are created but not explicitly enabled in the current bootstrap order. Never call a hooked export when you mean its trampoline (`renderer.cpp`).

## I changed `settings.html` but nothing happened

You must regen: `pwsh HlyxCaption/ui_ultralight/generate_embedded_ui.ps1` (inlines CSS/JS, base64 banner, ≤7000-byte chunks, regex verify).

## Language doesn’t change / `ui_direction_override` does nothing

There is no C++ `Localizer`; the HTML panel applies its JS translation dictionary. `ui_language` (1=English, 2=Arabic) switches strings and direction immediately and is written to `settings.ini` when Save is pressed. `ui_direction_override` remains a legacy compatibility setting and does not override the selected language.
