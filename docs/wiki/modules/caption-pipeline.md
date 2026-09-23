# Module: Caption Pipeline

> **Files:** `renderer.h` (119 lines), `renderer.cpp` (1208 lines), `caption_parser.{h,cpp}`, `caption_queue.cpp`, `caption_texture.cpp` — plus `banner_config.h`

## Responsibilities

- Define caption data types (`CaptionRun`/`CaptionLine`/`CaptionPhrase`/`CaptionEntry`) and the static `Renderer` facade.
- Parse Valve closecaption tag grammar into structured phrases (`caption_parser`).
- Manage entry lifecycle: timing, fades, stacking, rebuild throttling, font-path drift (`caption_queue`).
- Word-wrap, shape, composite and upload one RGBA texture per entry (`caption_texture` + `shaper`).

## Key Files

- `renderer.h` — `CaptionRun{text,rgba,bold,italic}`, `CaptionLine{vector<Run>}`, `CaptionPhrase{vector<Line>,delay,duration}`, `CaptionEntry{phrases,duration,startQPC,opacity,expired,fromPlayer,targetY/visualY,pixelWidth/pixelHeight,texture/srv/texID,lastActivePhraseCount/lastActivePhraseSig,lastQPC/lastRebuildQPC,cached render snapshots (fontSize/lineSpacing/rgba/maxLineWidth/alignment/fontPaths),isPreview}`; `class Renderer` static facade (`Initialize/Shutdown/SetCaptionText/InitImGui/ReloadFont/ReloadFontPreserveQueue/DrawCaptions/ShowPreview/ClearPreview/DrawSettingsWindow(no-op)/DrawBackgroundBox`), `m_Queue` (`vector<CaptionEntry>`), `m_CS` (`CRITICAL_SECTION`), `m_Device/m_Context/m_BackBufferRTV/m_Window/m_OriginalWndProc`, `m_SettingsOpen` (`atomic<bool>` — F10 toggles it), and `m_OverlayVisible`.
- `renderer.cpp` — `HookPresent` (temp `CreateWindow`+`D3D11CreateDeviceAndSwapChain` → vtable[8]=Present, vtable[13]=Resize hooks via MinHook), `InitImGui` (ImGui context `NoIni/NoMouseCursorChange`, `ImGui_ImplWin32/DX11_Init`, RTV, WndProc subclass, 1×1 blank cursor `CreateCursor`, `UltralightManager::Initialize`, `RegisterRawInputDevices`, load `IDC_ARROW` → `GetIconInfo` → `GetDIBits` forced top-down into D3D11 cursor texture), `WndProc` (F10 on `WM_KEYDOWN`+`WM_SYSKEYDOWN`, F11, Raw Input accumulation into `g_SoftMouseX/Y` with `mouse_sensitivity`/`mouse_acceleration`, Ultralight forwarding via its queue), `hkPresent` (see sequence in architecture.md), `DrawCaptions` (per-entry `AddImage` with outline ring + shadow copies, background box, stacking `targetY` from `pos_y·H − fontSize·1.3`, `visualY` eased @8.0, `texID` alpha from opacity), `GetScaledFontSize` (`base * (screenH / font_size_reference_height)`), `DrawBackgroundBox`, and `DrawRealCursor`.
- `caption_parser.cpp` — `ParseCaptionText(raw, fromPlayer)` → `vector<CaptionPhrase>`. Grammar: `<clr:r,g,b>` pushes a color and `<clr>` pops back to the previous one (`</clr>` parses as `"/clr"`, matches no branch, and is dropped silently), `<playerclr:pr,pg,pb:nr,ng,nb>` (palette by `fromPlayer`), `<I>/<i>`/`<B>/<b>` (italic/bold), `<cr>` (line break), `<delay:N>` (sets `CaptionPhrase::delay`), `<sb>` (phrase separator — outer split in `SetCaptionText`, format state persists across parts), unknown tags silently dropped; `CaptionFormatState{rgba,bold,italic,colorStack}` is carried across `<sb>` parts.
- `caption_queue.cpp` — `SetCaptionText(raw,dur,fromPlayer)` under `m_CS`: split on `<sb>`, `ParseCaptionText`, `GetVisibleCharCount` (UTF-8 **code-point** count, ignoring markup tags), distribute `dur` across parts ∝ char count, push `CaptionEntry` with config snapshot; `UpdateQueue(dt)`: `CalcActivePhraseCount` (phrase becomes active when `elapsed ≥ delay`, duration 0 = sticky; the count loop stops at the first future-delay phrase with `break`, while the rebuild-signature loop skips it with `continue`), expire when `elapsed ≥ duration` (fades run afterward by stepping `opacity`), rebuild when the phrase signature (first 16 phrases only — later ones stay out of the signature, so they can miss a texture update, while expiry is unaffected) or the config snapshot changes (a color change rewrites run colors in place, then `RenderEntryTexture` still runs — reshape and upload happen either way, there is no reshape-skipping fast path; 50 ms throttle for `line_spacing` alone), stack `targetY` downward +4 px gaps, ease `visualY`, font-path drift → `ReloadFontPreserveQueue`; `CalcActivePhraseCount` helper.
- `caption_texture.cpp` — `RenderEntryTexture(entry,maxPixelWidth)` (8 px margin): gather active phrases' lines, greedy word-wrap (split **ASCII space only**, measure by `TextShaper::ShapeLine` widths, merge same-format runs), `TextShaper::ShapeLine` per line, composite into CPU RGBA buffer with alpha-weighted overlap blend, upload `ID3D11Texture2D` `R8G8B8A8_UNORM_SRGB` `DEFAULT` + `SRV` into `entry.texID`. `SetCaptionText` is a static member of `Renderer` (declared in `renderer.h`), not a free function (there is still no `caption_queue.h`).
- `banner_config.h` — `BannerConfig` constants (`kReferenceHeight=1080`, `kTopPct/kLeftPct`, logo/title/hints/kbd font px @1080p, `kFadeIn/Hold/FadeOutSec`, `kStartOffsetYpx`) — scaled at runtime by `viewportH / kReferenceHeight`, injected into Ultralight JS via `BuildBannerConfigJS`.

## Public API

```cpp
// renderer.h — all static
class Renderer {
  static bool Initialize();            // HookPresent
  static void Shutdown();
  static void SetCaptionText(const string& raw, float dur, bool fromPlayer=false);
  static bool InitImGui(IDXGISwapChain* sc);
  static void ReloadFont();
  static void ReloadFontPreserveQueue(); // takes m_CS, safe from render/UL threads
  static void DrawCaptions();          // hkPresent, under m_CS
  static void ShowPreview(const string& text, float dur=9999);
  static void ClearPreview();
  static bool HasPreview();
  static void DrawSettingsWindow();    // no-op (legacy panel deleted)
  static void DrawBackgroundBox(ImDrawList*, ImVec2 pos, ImVec2 size, float opacity);
  static void UpdateQueue(float dt);
  static double GetTimeQPC();
  static float  GetScaledFontSize(float base);
};

// caption_parser.h
vector<CaptionPhrase> ParseCaptionText(const string& raw, bool fromPlayer=false);
vector<CaptionPhrase> ParseCaptionText(const string& raw, CaptionFormatState& state, bool fromPlayer);
```

## Internal Structure

```
hkProcess → SetCaptionText (m_CS)
  split raw on "<sb>" → ParseCaptionText per part (shared CaptionFormatState)
  distribute duration ∝ GetVisibleCharCount (UTF-8 code-point count) → push CaptionEntry (+ config snapshot)

hkPresent (render thread, m_CS):
  UpdateQueue(dt)
    expire old entries
    rebuild signature/config check → RenderEntryTexture if needed
    stack targetY; ease visualY; ReloadFontPreserveQueue if font paths drifted
  DrawCaptions
    for each entry: foreground draw-list AddImage(texID) + outline ring + shadow + bg box
  UltralightManager::Render + DrawRealCursor

RenderEntryTexture(entry, maxW = max_line_width_percent * screenW):
  active phrases → lines → if width>maxW: greedy wrap on ' ' by measured width
  for each line: TextShaper::ShapeLine(runs, scaledFontSize) → glyph bitmaps
  composite (+8 px margin, alpha overlap) → upload ID3D11Texture2D+SRV
```

## Dependencies

- **Uses:** `TextShaper` (`shaper.h`), `caption_parser.h`, `config.h`, `arabic_fallback`, `UltralightManager`/`UltralightBlit`, `imgui` (+ `imgui_impl_dx11/win32`), D3D11/DXGI, `MinHook`.
- **Used by:** `hooks.cpp` (`hkProcess` → `SetCaptionText`; `MainThread` → `Initialize`), `config` save path, and `UltralightManager` (`ShowPreview`).

## Notable Patterns / Gotchas

- `GetVisibleCharCount` counts UTF-8 **code points**, not bytes or glyphs; markup tags are excluded from the count.
- Past 16 phrases per entry, the extras fall outside the rebuild signature, so their activation can miss a texture rebuild. Expiry does not use the signature and is unaffected.
- Word-wrap is ASCII `' '` only — no NBSP/tab/CJK handling.
- Parse+shape+upload all happen **under `m_CS` on the game's caption thread** — can stall the render thread.
