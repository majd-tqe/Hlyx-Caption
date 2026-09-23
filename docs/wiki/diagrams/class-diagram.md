# Class Diagram

> Core types in `HlyxCaption/` — fields trimmed to the load-bearing subset. Generics use `~T~` per Mermaid.

```mermaid
classDiagram
    class OverlayConfig {
        +uint8 text_r
        +uint8 text_g
        +uint8 text_b
        +uint8 text_a
        +bool shadow_enabled
        +float shadow_offset_x
        +float shadow_offset_y
        +bool outline_enabled
        +float outline_thickness
        +float pos_x
        +float pos_y
        +Alignment text_alignment
        +float fade_in_time
        +float fade_out_time
        +float extra_display_time
        +float font_size
        +float font_size_reference_height
        +float line_spacing
        +string custom_font_path
        +string fallback_font_path
        +float max_line_width_percent
        +float mouse_sensitivity
        +int ui_language
        +float ui_scale
    }

    class CaptionRun {
        +string text
        +uint8 r
        +uint8 g
        +uint8 b
        +uint8 a
        +bool bold
        +bool italic
    }
    class CaptionLine {
        +vector~CaptionRun~ runs
    }
    class CaptionPhrase {
        +vector~CaptionLine~ lines
        +float delay
        +float duration
    }
    class CaptionEntry {
        +vector~CaptionPhrase~ phrases
        +float duration
        +double startQPC
        +float opacity
        +bool expired
        +bool fromPlayer
        +double visualY
        +double targetY
        +int pixelHeight
        +int pixelWidth
        +ID3D11Texture2D* texture
        +ID3D11ShaderResourceView* srv
        +void* texID
        +bool isPreview
        +int lastActivePhraseSig
        +int lastActivePhraseCount
        +double lastQPC
        +double lastRebuildQPC
        +float renderedFontSize
        +float renderedLineSpacing
        +uint8 renderedTextR
        +uint8 renderedTextG
        +uint8 renderedTextB
        +uint8 renderedTextA
        +float renderedMaxLineWidthPercent
        +Alignment renderedAlignment
        +string renderedCustomFontPath
        +string renderedFallbackFontPath
        +ReleaseTexture() void
    }
    class CaptionFormatState {
        +uint8 r
        +uint8 g
        +uint8 b
        +uint8 a
        +bool bold
        +bool italic
        +vector~tuple~ colorStack
    }

    class Renderer {
        <<static>>
        +Initialize() bool
        +Shutdown() void
        +SetCaptionText(string, float, bool) void
        +InitImGui(swapchain) bool
        +ReloadFont() void
        +ReloadFontPreserveQueue() void
        +DrawCaptions() void
        +ShowPreview(string, float) void
        +ClearPreview() void
        +HasPreview() bool
        +DrawSettingsWindow() void
        +DrawBackgroundBox(dl, pos, size, opacity) void
        +UpdateQueue(dt) void
        -HookPresent() bool
        -WndProc(hwnd, msg, w, l) LRESULT
        -RenderEntryTexture(entry, maxW) void
        -CalcActivePhraseCount(entry) int
    }

    class TextShaper {
        +Initialize(path, size) bool
        +InitializeFallback(path, size) bool
        +Shutdown() void
        +Shape(utf8, size) ShapedTextResult
        +ShapeLine(runs, size) ShapedTextResult
        +IsInitialized() bool
        +HasFallback() bool
        -SplitDirectionalRuns(utf8) vector~DirectionalRun~
    }
    class TextRun {
        +string text
        +uint8 r
        +uint8 g
        +uint8 b
        +uint8 a
        +bool bold
        +bool italic
    }
    class GlyphBitmap {
        +int x
        +int y
        +int w
        +int h
        +int bearingX
        +int bearingY
        +int advance
        +vector~uint8~ bitmap
        +uint8 r
        +uint8 g
        +uint8 b
        +uint8 a
    }
    class ShapedTextResult {
        +vector~GlyphBitmap~ glyphs
        +int totalWidth
        +int totalHeight
        +string fontPath
        +float fontSize
    }
    class DirectionalRun {
        <<private nested in TextShaper>>
        +string text
        +bool isRTL
        +size_t logicalByteOffset
    }

    class UltralightManager {
        <<singleton>>
        +Get() UltralightManager&
        +Initialize(dev, ctx, w, h) bool
        +Render() void
        +ProcessWin32Message(hwnd, msg, w, l) bool
        +Resize(w, h) void
        +Shutdown() void
        +SetOpen(bool) void
        +IsOpen() bool
        +OnPanelOpened() void
        +RequestCaptionVisibleChange(bool) void
        +OnDOMReady(view, frame, main, url) void
    }
    class ultralight_blit {
        <<namespace>>
        +Initialize(dev) bool
        +Shutdown() void
        +EnsureTexture(dev, ctx, w, h) bool
        +GetBitmapSize(w, h) void
        +UpdateBitmapFromUG(ctx, bitmap) bool
        +DrawFullscreenQuad(ctx, dst_w, dst_h, alpha) void
        +EndFrame(ctx) void
    }
    class UltralightPlatform {
        <<CustomLogger + CustomFileSystem + CustomFontLoader>>
        +LogMessage(level, msg) void
        +FileExists(path) bool
        +GetFileMimeType(path) String
        +GetFileCharset(path) String
        +OpenFile(path) Buffer
        +fallback_font_for_characters(chars, weight, italic) String
        +Load(family, weight, italic) FontFile
        +InitializeUltralightPlatform(baseDir) void
    }

    %% Relationships
    CaptionPhrase *-- CaptionLine : contains
    CaptionLine *-- CaptionRun : contains
    CaptionEntry *-- CaptionPhrase : phrases
    CaptionEntry ..> CaptionRun : rendered via
    TextShaper *-- TextRun : ShapeLine input
    Renderer *-- CaptionEntry : m_Queue vector
    Renderer *-- TextShaper : m_Shaper
    Renderer *-- OverlayConfig : reads g_Config snapshot
    Renderer ..> GetScaledFontSize : free function (not a member)
    Renderer --> UltralightManager : drives Render
    TextShaper *-- GlyphBitmap : produces
    TextShaper *-- DirectionalRun : internal
    ShapedTextResult *-- GlyphBitmap : glyphs
    UltralightManager *-- ultralight_blit : uses
    UltralightManager --> UltralightPlatform : FileSystem/Logger
    CaptionFormatState ..> CaptionRun : drives colorStack
```

## Notes

- `Renderer` is **static-everything** — no instances; `m_CS` (`CRITICAL_SECTION`) guards `m_Queue` and the `TextShaper` it owns. `UltralightManager` is a Meyers singleton with its own `mutex` + `deque<PendingEvent>` so the `View` is single-threaded to the render thread.
- Caption layering: `CaptionPhrase{lines{runs{text,rgba,bold,italic}}}` — `CaptionFormatState` is the transient parser stack that builds `CaptionRun`s; `caption_texture` rebuilds `TextRun` vectors from those runs for `ShapeLine`. `CaptionEntry` additionally stores a full `OverlayConfig` snapshot (`rendered*`) plus `lastActivePhraseSig`/`lastQPC` to detect when a texture needs rebuilding.
- `TextShaper` exposes only `Shape`/`ShapeLine` (FriBidi→HarfBuzz→FreeType → `ShapedTextResult`); `DirectionalRun` is a private nested struct, and `BuildFontSpans` is a static free function in `shaper.cpp`, not a member. `GetScaledFontSize` is likewise a free function in `renderer.h`, not a `Renderer` member.
- Vendored `imgui` and `minhook-detours-src` types are omitted — they appear only as `ImDrawList`/`ImGuiContext` consumers and `MH_*` trampoline storage.
