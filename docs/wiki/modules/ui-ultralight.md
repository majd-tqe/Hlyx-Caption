# Module: UI — Ultralight (HTML/CSS/JS)

> **Files:** `ui_ultralight/UltralightManager.{h,cpp}` (126 + 985 lines), `UltralightBlit.{h,cpp}` (40 + 389 lines), `UltralightPlatform.{h,cpp}`, `ui_exports.{h,cpp}`, `UL_Debug.h`, `embedded_ui.h` (2588 lines GENERATED), `generate_embedded_ui.ps1`, `assets/settings.html` + `settings.css` + `bindings.js` + `banner.png`, `assets/fonts/*.woff2`

## Responsibilities

- Host the **settings panel** as an offscreen HTML/CSS/JS `ultralight::View` that renders to a CPU BGRA bitmap → D3D11 texture → fullscreen triangle **after** ImGui and **before** the software cursor, so it sits on top of captions.
- Provide the JS↔C++ bridge (polling `window.*` globals + `EvaluateScript` pushes) and banner lifecycle.

## Key Files

- `UltralightManager.h` — `class UltralightManager : public LoadListener` singleton (`Get()`): `Initialize(dev,ctx,w,h)`, `Render()` (to backbuffer), `ProcessWin32Message`, `Resize`, `Shutdown`, `SetOpen/IsOpen`, `OnPanelOpened` (replays CSS slide-in), `RequestCaptionVisibleChange`, `OnDOMReady` (LoadListener). Queues: `m_EventMutex` + `deque<PendingEvent{msg,wParam,lParam}>` (`m_EventQueue`), `m_WarmupCounter`, `m_BannerFinished` (`window.__bannerDone`), `m_BannerCfgApplied`, `m_PendingOverlayVisible(Value)`, `m_Renderer`/`m_View`, `m_BitmapTexture/SRV/W/H`, `m_Width/Height/Hwnd`.
- `UltralightManager.cpp` — `Initialize`: create `ultralight::Renderer`, pass the on-disk `assetDir` to `InitializeUltralightPlatform`, build a transparent `ViewConfig` (`is_accelerated=false`, Tahoma fallback fonts), then load the embedded page via `LoadHTML(kEmbeddedHTML, kEmbeddedHTMLLen)`. `Render`: drain `m_EventQueue` via `DrainInputQueue` (dispatch through `FireMouseEvent`/`FireKeyEvent`/`FireScrollEvent`), run warmup `Update()` ticks while closed, poll JS flags (`__saveRequested`→ `snapshot()` JSON → `ApplyConfigField*` per key → `SaveConfig`; `__resetRequested`→ `ResetConfig`+`RefreshAllFromConfig`; `__stateDirty`→ live edits; `__preview*`→ `Renderer::ShowPreview/ClearPreview`; `__bannerDone`→ `m_BannerFinished` → zero-cost closed state; `__panelHidden` after slide-out), inject `BannerConfig` JS once via `BuildBannerConfigJS`, push `hlaFonts.setList`/`hlaBanner.setTranslationVisible`/`hlaConfig._setValue` as needed, `View::Update`→ `View::Render`→ copy `Bitmap` BGRA → upload `m_BitmapTexture`, handle `m_PendingOverlayVisible`, and early-out when `!m_Open && m_BannerFinished`. `Resize`, `ProcessWin32Message` (enqueue only), `OnDOMReady` → `RefreshAllFromConfig` (27 `DispatchConfig*` pushes) + font list, `ApplyConfigField/Bool/String`, `RequestSaveConfig/CloseUI`.
- `UltralightBlit.h/.cpp` — free functions in `namespace ultralight_blit`, not a class: `Initialize(device)`, `EnsureTexture(device,ctx,w,h)` (persistent Bitmap→Texture2D staging), `GetBitmapSize`, `UpdateBitmapFromUG(ctx,bitmap)`, `DrawFullscreenQuad(ctx,dst_w,dst_h,alpha_mod)`, `EndFrame(ctx)`. The draw is a fullscreen triangle computed from `SV_VertexID` with a null input layout — there is no vertex buffer and no input layout object. Shaders convert premultiplied BGRA to sRGB with sampler + blend state. The staging globals live in `UltralightManager.cpp`, not as `UltralightManager` members.
- `UltralightPlatform.h/.cpp` — implements `ultralight::FileSystem` as `CustomFileSystem`, disk-backed (base asset dir plus DllDir and resources fallbacks, so `file://` requests resolve from disk), plus `Logger` (`UL_LogToFile`) and `CustomFontLoader` (system fonts plus bundled woff2, Arial fallback). `InitializeUltralightPlatform` registers logger, file system and font loader. Offscreen operation comes from `ViewConfig.is_accelerated=false`; there are no GPU or driver stubs.
- `ui_exports.h/.cpp` — C exports: `UI_Initialize(HWND,ID3D11Device*,ID3D11DeviceContext*)`, `UI_Render()`, `UI_SetOpen(bool)`, `UI_IsOpen()`, `UI_ProcessWin32Message` (returns consumed bool), `UI_Shutdown()`, `UI_IsInitialized()`. There are no resize helpers.
- `UL_Debug.h` — `void UL_LogToFile(const char* fmt, ...)` — appends to `wininet_hook.log` next to the host exe (also used by crash handlers).
- `embedded_ui.h` — **AUTO-GENERATED**: `namespace hlyx_caption { inline const char* kEmbeddedHTML = R"Cn(… )Cn" …; }` — 20 chunks of ≤7000-byte raw strings with unique delimiters (MSVC `C2026` limit ~8.5 KB), length published as `kEmbeddedHTMLLen`, verified by regex round-trip. Do not hand-edit.
- `generate_embedded_ui.ps1` — inlines `settings.css` into `<style>`, `bindings.js` into `<script>`, converts `assets/banner.png` to base64 data URI replacing `{{BANNER_URI}}`, emits chunked raw strings, verifies reconstruction byte-for-byte, exits 1 on mismatch.
- `assets/settings.html` — panel markup (8 visible tabs: text/color/border/shadow/position/timing/reading/general; the current General tab exposes `ui_language`); element IDs == INI keys (`font_size`, `text_r`, `outline_enabled`, `pos_x`, …); localized footer/actions.
- `assets/settings.css` — "Resistance Terminal" theme (HEV hazard-orange on dark), `translateX(100%)` hidden, `hlaPanelOpen()` slides in.
- `assets/bindings.js` — exposes `window.hlaConfig` (`_setValue[key]`, `snapshot()`, `__stateDirty`), `window.hlaFonts.setList`, `window.hlaBanner.setTranslationVisible`, `window.hlaPanelOpen`, plus flags `__saveRequested/__resetRequested/__previewRequested/__previewText/__previewClear/__previewEnabled/__bannerDone/__panelHidden`.

## Public API

```cpp
class UltralightManager : public LoadListener {
  static UltralightManager& Get();
  bool Initialize(ID3D11Device*, ID3D11DeviceContext*, int w, int h);
  void Render();                          // backbuffer already bound
  bool ProcessWin32Message(HWND, UINT, WPARAM, LPARAM); // enqueues
  void Resize(int w, int h);
  void Shutdown();
  void SetOpen(bool); bool IsOpen() const;
  void OnPanelOpened(); // replay CSS slide-in
  void RequestCaptionVisibleChange(bool);
  void OnDOMReady(View*, uint64_t, bool, const String& url) override;
};

// C exports
extern "C" bool UI_Initialize(HWND, ID3D11Device*, ID3D11DeviceContext*);
extern "C" void UI_Render();
extern "C" void UI_SetOpen(bool);
extern "C" bool UI_IsOpen();
extern "C" bool UI_ProcessWin32Message(HWND, UINT, WPARAM, LPARAM);
extern "C" void UI_Shutdown();
extern "C" bool UI_IsInitialized();
```

## Bridge Contract

| Direction | Mechanism |
|---|---|
| C++ → JS value push | `EvaluateScript("window.hlaConfig._setValue['key'](value)")` per field; `RefreshAllFromConfig` pushes 27 fields on `OnDOMReady`/reset |
| C++ → JS fonts | `window.hlaFonts.setList(['a.ttf',…])` enumerated from `{ModDir}/resources` |
| C++ → JS banner | `window.hlaBanner.setTranslationVisible(bool)` + `BannerConfig` JS injection |
| C++ → JS panel anim | `window.hlaPanelOpen()` only on real F10 open (`OnPanelOpened`) |
| JS → C++ save/reset | flags `__saveRequested`/`__resetRequested`; values read via `snapshot()` JSON → `ApplyConfigField*` → `SaveConfig` |
| JS → C++ live edits | `__stateDirty` + snapshot → per-field `ApplyConfigField*` |
| JS → C++ preview | `__previewRequested/__previewText/__previewClear/__previewEnabled` → `ShowPreview`/`ClearPreview` |
| Handshakes | `__bannerDone` (banner fade-out complete → `m_BannerFinished` → zero-cost closed), `__panelHidden` (slide-out complete) |

## Dependencies

- **Uses:** Ultralight SDK (`Ultralight/Renderer`, `View`, `Bitmap`, `RefPtr`, `Listener`), D3D11 (bitmap texture + `ultralight_blit`), Win32, STL, `config.h` (`SaveConfig`/`ResetConfig`), `renderer.h` (`ShowPreview`/`ClearPreview`/`HasPreview`), `banner_config.h`.
- **Used by:** `renderer.cpp` (`InitImGui` → `Initialize`; `hkPresent` → `Render`; `WndProc` → `ProcessWin32Message`).

## Notable Patterns / Gotchas

- **Threading:** `ProcessWin32Message` only **enqueues**; the `View` is touched solely on the render thread inside `Render` via `DrainInputQueue` + polled JS flags — guarded by `m_EventMutex`.
- **Zero-cost closed:** after `__bannerDone`, `Render` early-outs without `View::Update/Render` or texture upload; warmup frames before that keep Ultralight's scheduler alive.
- **Polling, not bindings:** no `JSObject` bound functions — JS sets flags + `snapshot()` JSON, C++ polls each `Render` tick. Simple and avoids Ultralight GC lifetime bugs.
- **Regeneration is mandatory** after editing any asset: `pwsh ui_ultralight/generate_embedded_ui.ps1` — otherwise `embedded_ui.h` is stale and changes won't ship.
- **Runtime localization:** `bindings.js` applies the English or Arabic dictionary and toggles `lang`/`dir` when `ui_language` changes; no C++ `Localizer` is required.
- **Known JS stub gap:** `shadow_offset_y` has no current DOM control; `ui_direction_override`, `ui_scale`, and `ui_animations` are compatibility fields without current HTML controls.
