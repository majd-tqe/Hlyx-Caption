# Module: Configuration

> **Files:** `config.h`, `config_parse.{h,cpp}`, save path in `hooks.cpp` (`SaveConfig`), resource `dist/resources/settings.ini`

## Responsibilities

- Declare every tunable setting with defaults (`OverlayConfig`), expose globals `g_Config`/`g_DefaultConfig` and `g_IniPath`.
- Load settings from `resources/settings.ini` handling **any** encoding the file may have (UTF-8/BOM, UTF-16LE/BE BOM) and expand `{ModDir}` tokens.
- Persist settings via Win32 `WritePrivateProfileStringW` (which re-encodes fresh files as UTF-16LE).

## Key Files

- `config.h` — `struct OverlayConfig` inline-defaulted fields:

  | Group | Fields |
  |---|---|
  | Text color | `text_r/g/b/a` (255,255,255,255) |
  | Shadow | `shadow_enabled` false, `shadow_r/g/b/a`, `shadow_offset_x/y` 4,4 |
  | Outline | `outline_enabled` true, `outline_r/g/b/a` (0,0,0,128), `outline_thickness` 3 |
  | Position | `pos_x` 0.50, `pos_y` 0.80, `Alignment{LEFT,CENTER,RIGHT}` `ALIGN_CENTER` |
  | Timing | `fade_in_time` 0.20, `fade_out_time` 0.50, `extra_display_time` 1.0 |
  | Typography | `font_size` 34, `font_size_reference_height` 1080, `line_spacing` 1.3, `custom_font_path` `"Cairo-Regular.ttf"`, `fallback_font_path` `"Cairo-Regular.ttf"` |
  | Word wrap | `max_line_width_percent` 0.75 |
  | Mouse | `mouse_sensitivity` 1.0, `mouse_acceleration` false, `mouse_accel_factor` 2.0 |
  | Background box | `background_enabled` false, `bg_r/g/b/a`, `bg_padding_x/y`, `bg_border_radius` |
  | UI | `ui_language` 2 (AR default, 1=EN), `ui_direction_override` 0, `ui_scale` 1.0, `ui_animations` true; the latter fields have no current HTML controls, while `ui_animations` still suppresses panel animation when false |
- `config_parse.h` — `bool ReadFileToUtf8(const wchar_t* path, string& outUtf8); int ParseConfigText(const string& text, OverlayConfig& cfg, const string& modDirUtf8);`
- `config_parse.cpp` — `ReadFileToUtf8`: read raw bytes, detect BOM (`EF BB BF`→UTF-8, `FF FE`→UTF-16LE, `FE FF`→UTF-16BE) else assume UTF-8/ANSI; decode UTF-16 manually (`DecodeUtf16`) into `out`, no Win32 calls; `ParseConfigText`: strip `;`/`#` comments, skip lines without `=`, trim, `last-key-wins`, `{ModDir}` expansion only in `custom_font_path`/`fallback_font_path` (first occurrence), `strcmp` dispatch for floats/ints/bools/strings into `cfg`; returns recognized-key count. **Unit-testable** — no Win32 hook deps.
- Write path (`hooks.cpp:SaveConfig`) — `WritePrivateProfileStringW` per-field into sections `[Text][Shadow][Outline][Position][Timing][Typography][WordWrap][Mouse][Background][UI]` on `g_IniPath`.

## Public API

```cpp
struct OverlayConfig { /* see table above */ };
inline OverlayConfig g_Config{};            // live config
inline const OverlayConfig g_DefaultConfig{}; // defaults snapshot
extern wchar_t g_IniPath[]; // {ModDir}\resources\settings.ini (set in MainThread)

void LoadConfig(const wchar_t* iniPath); // ReadFileToUtf8 + ParseConfigText
void SaveConfig(const wchar_t* iniPath); // WritePrivateProfileStringW per field
void ResetConfig();                       // g_Config = g_DefaultConfig

// config_parse.h — the testable core
bool ReadFileToUtf8(const wchar_t* path, std::string& outUtf8);
int  ParseConfigText(const std::string& text, OverlayConfig& cfg,
                     const std::string& modDirUtf8);
```

## Internal Structure

```
MainThread: g_IniPath = ModDir + L"\\resources\\settings.ini"
LoadConfig(iniPath):
  ReadFileToUtf8(iniPath) — BOM sniff → UTF-8 string
  ParseConfigText(text, g_Config, g_ModDirA) — line loop, comment strip, key dispatch

Ultralight save path (Render thread inside UltralightManager::Render):
  JS flag window.__saveRequested set → snapshot = window.hlaConfig.snapshot() (JSON-ish)
  for each key: ApplyConfigField*(key, value) → g_Config
  SaveConfig(g_IniPath) — WritePrivateProfileStringW → file becomes UTF-16LE on first save
  next LoadConfig transparently handles UTF-16LE BOM via ReadFileToUtf8
```

`{ModDir}` expansion: only `custom_font_path` and `fallback_font_path` are expanded, first `{ModDir}` occurrence only. The replacement is `g_ModDirA` (UTF-8 DLL directory, trailing separators stripped) at parse time; font paths `"Cairo-Regular.ttf"` etc. are bare filenames resolved later in `renderer.cpp` to `{ModDir}\resources\<name>` if not absolute.

## Dependencies

- **Uses:** pure C++ STL in `config_parse.cpp` (manual UTF-16 decode, no Win32 calls); Win32 `WritePrivateProfileStringW` plus `MultiByteToWideChar` on the `hooks.cpp` save path; `banner_config.h` is independent (not part of `OverlayConfig`).
- **Used by:** `hooks.cpp` (`MainThread`, `SaveConfig`), `renderer.cpp` (reads `g_Config` for `DrawCaptions`/`UpdateQueue`/`ReloadFontPreserveQueue` + `GetScaledFontSize`), `UltralightManager` (`ApplyConfigField*`, `RequestSaveConfig`, `RefreshAllFromConfig`), and `caption_queue` (snapshots).

## Notable Patterns / Gotchas

- **UTF-16LE trap:** `SaveConfig` via `WritePrivateProfileStringW` silently converts a fresh INI to UTF-16LE+BOM; if `ReadFileToUtf8` didn't handle that, every setting would reset after first save — this was a shipped bug, now fixed.
- **`ui_direction_override` has no effect** — parsed and saved, but not exposed by the current HTML and never applied to anything. There is no `Localizer` in the code.
- `ui_language` is stored as a plain number (1=EN, 2=AR), handed to the panel JS, and applied immediately; persistence happens on an explicit Save request. Invalid or legacy `0` values fall back to Arabic.
- `last-key-wins` — repeated keys in the INI: the last occurrence silently wins (INI convention).
- `OverlayConfig` is a plain struct with inline initializers; `inline OverlayConfig g_Config{}` guarantees zero-init safety even if the linker zeroed `.bss` before C++ static init.
