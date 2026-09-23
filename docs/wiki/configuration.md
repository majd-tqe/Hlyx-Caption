# Configuration & Settings Reference

<p class="kicker">Single source: <code>OverlayConfig</code> in <code>config.h</code> ↔ <code>resources/settings.ini</code></p>

> Edit through the F10 HTML panel, or by hand in `resources/settings.ini` inside `{ModDir}\resources`. The folder is created on first run, and a legacy `{ModDir}\HLAMod.ini` is migrated there once when present. When a key repeats, the last value wins. `{ModDir}` expands only in `custom_font_path` and `fallback_font_path`, first occurrence only.

## Quick Example (`resources/settings.ini`)

```ini
[Text]
text_r=255
text_g=255
text_b=255
text_a=255
[Shadow]
shadow_enabled=0
shadow_offset_x=4.00
shadow_offset_y=4.00
[Outline]
outline_enabled=1
outline_thickness=3.00
[Position]
pos_x=0.50
pos_y=0.80
text_alignment=center
[Typography]
font_size=34.00
font_size_reference_height=1080.00
line_spacing=1.30
custom_font_path=Cairo-Regular.ttf
fallback_font_path=Cairo-Regular.ttf
```

Deploy copy: `HlyxCaption/dist/resources/settings.ini` contains the plain default values without comments. Unknown keys are skipped silently.

## Full Key Table

| Section | Key | Type | Default | Description |
|---|---|---|---|---|
| `[Text]` | `text_r/g/b/a` | uint8 | 255/255/255/255 | Caption fill color. Alpha blends per-entry. |
| `[Shadow]` | `shadow_enabled` | bool 0/1 | 0 | Draw shadow copies behind each glyph. |
| | `shadow_r/g/b/a` | uint8 | 0/0/0/128 | Shadow color. |
| | `shadow_offset_x/y` | float | 4 / 4 | Shadow offset in raw pixels. |
| `[Outline]` | `outline_enabled` | bool | 1 | Tinted outline ring. |
| | `outline_r/g/b/a` | uint8 | 0/0/0/128 | Outline color. |
| | `outline_thickness` | float | 3.0 | Ring radius in px. |
| `[Position]` | `pos_x` | 0–1 | 0.50 | Horizontal anchor (0 left → 1 right). |
| | `pos_y` | 0–1 | 0.80 | Vertical anchor of the stack. |
| | `text_alignment` | `left\|center\|right` | `center` | Text alignment inside each entry. Anything else falls back to `center` silently; the comparison is case-insensitive. |
| `[Timing]` | `fade_in_time` | seconds | 0.20 | Fade-in on appear, applied to every queued entry. |
| | `fade_out_time` | seconds | 0.50 | Fade-out on expire. |
| | `extra_display_time` | seconds | 1.00 | Extra hold added to game duration. |
| `[Typography]` | `font_size` | px | 34 | Base size at `font_size_reference_height`. |
| | `font_size_reference_height` | px | 1080 | Resolution that `font_size` is exact at. Scaled `base * (screenH / refH)`. |
| | `line_spacing` | factor | 1.30 | Line gap (rebuild throttled 50 ms). |
| | `custom_font_path` | string | `Cairo-Regular.ttf` | Primary font. Bare name → `{ModDir}\resources\name`. |
| | `fallback_font_path` | string | `Cairo-Regular.ttf` | Fallback for missing glyphs (FriBidi RTL). |
| `[WordWrap]` | `max_line_width_percent` | 0–1 | 0.75 | Wrap threshold = `percent * screenW`. ASCII `' '` only. |
| `[Mouse]` | `mouse_sensitivity` | float | 1.0 | Software cursor multiplier (Raw Input). |
| | `mouse_acceleration` | bool | 0 | Enable accel. |
| | `mouse_accel_factor` | float | 2.0 | Accel multiplier. |
| `[Background]` | `background_enabled` | bool | 0 | Rounded box behind each entry. |
| | `bg_r/g/b/a` | uint8 | 0/0/0/150 | Box color. |
| | `bg_padding_x/y` | float | 10 / 5 | Box padding. |
| | `bg_border_radius` | float | 5 | Corner radius. |
| `[UI]` | `ui_language` | 1/2 | 2 (AR) | 1=EN, 2=AR. The value is read, saved, and applied to the panel by C++/JS. Invalid or legacy `0` values fall back to Arabic. |
| | `ui_direction_override` | 0/1/2 | 0 | Parsed and saved in C++; no current HTML control or runtime consumer. |
| | `ui_scale` | float | 1.0 | Parsed and saved for compatibility; no current HTML control or runtime consumer. |
| | `ui_animations` | bool | 1 | Parsed, saved, and forwarded to JS; no current HTML control, so configure it in the INI. It controls panel animation suppression. |

> **Known HTML gaps:** no text-alpha slider, no `shadow_offset_y` control, no shadow/outline color pickers, and no background section in the current HTML. `ui_direction_override` and `ui_scale` are file-only compatibility fields; `ui_animations` is file-only but still controls panel animation suppression.

Bool keys accept `1`, `true`, `yes`, `on`. Anything else reads as `false` without a warning.

## Encoding & Persistence

```mermaid
flowchart LR
    A[resources/settings.ini] --> B[ReadFileToUtf8 BOM sniff]
    B --> C[ParseConfigText last-wins + {ModDir}]
    C --> D[g_Config]
    D --> E[SaveConfig WritePrivateProfileStringW]
    E --> F[UTF-16LE BOM file]
    F --> B
```

- **Save** uses `WritePrivateProfileStringW` → fresh files become **UTF-16LE with BOM**. `ReadFileToUtf8` normalizes UTF-8/UTF-16LE/BE → UTF-8, so a just-saved file never loses settings (prior bug was plain byte read).
- **Comments:** `;` or `#` to end of line. Lines without `=` are skipped (sections `[...]`).
- **Token:** `{ModDir}` expands only in `custom_font_path` and `fallback_font_path`, first occurrence only. The replacement is `g_ModDirA` (UTF-8 DLL dir, trailing separators stripped).

## Banner Config (`banner_config.h`)

Not part of INI — edit constants, rebuild, no `embedded_ui.h` regen needed (injected as JS at runtime).

| Constant | Default (@1080p) | Meaning |
|---|---|---|
| `kReferenceHeight` | 1080 | Baseline; all px scaled by `viewportH / kReferenceHeight` |
| `kTopPct / kLeftPct` | 8% / 50% | Banner anchor |
| `kLogoMaxPx` | 76.8 | Logo max |
| `kTitleFontPx / kHintsFontPx / kKbdFontPx` | 36 / 16.8 / 14.4 | Fonts |
| `kRowGapPx` | 12 | Gap logo→hints |
| `kFadeInSec / kHoldSec / kFadeOutSec` | 0.5 / 10 / 1.5 | Lifecycle |
| `kStartOffsetYpx` | -12 | Slide-in offset |

Fallback: CSS defaults in `settings.css` if JS injection fails.

## Live Apply Rules

- Most exposed keys: `UpdateQueue` detects snapshot diff → `RenderEntryTexture` on the next frame. The current implementation rebuilds and reshapes the entry texture; there is no separate recolor-only fast path.
- `line_spacing` only: 50 ms throttle.
- Font path drift → `ReloadFontPreserveQueue()` (takes `m_CS`).
