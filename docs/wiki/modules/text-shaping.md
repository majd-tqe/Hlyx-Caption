# Module: Text Shaping

> **Files:** `shaper.h` (66 lines) + `shaper.cpp` (676 lines), `arabic_fallback.{h,cpp}`, `arabic_fallback_data.h` (731-entry GENERATED table)

## Responsibilities

- Shape UTF-8 captions into positioned, rasterized glyph bitmaps with Arabic joining, bidi reordering, and fallback-font coverage — optimized for Arabic/Latin mixed text (`caption_texture.cpp` via `Renderer`).
- Repair Arabic Presentation Forms (U+FB50–FEFC) that a font lacks by substituting equivalent base/sibling/decomposed codepoints before shaping.

## Key Files

- `shaper.h` — `GlyphBitmap{x,y,w,h,bearingX/Y,advance,bitmap,rgba}`, `ShapedTextResult{glyphs,totalWidth/Height,fontPath/Size}`, `TextRun{text,rgba,bold,italic}`, `class TextShaper` (see API). Private: `DirectionalRun{text,isRTL,logicalByteOffset}`, `SplitDirectionalRuns`, `FT`/`Face`/`HBFont`/`HBBuffer`, `m_FontSize`/`m_FontPath`/`m_FontPathFallback`, fallback `Face`/`HBFont`, plus `m_FTFallback` (declared, never assigned — `InitializeFallback` reuses `m_FT`).
- `shaper.cpp` — `Initialize(path,size)` (`FT_New_Face` + `hb_ft_font_create` + `hb_buffer_create`; `InitializeFallback(path,size)` only creates a second `hb_font` on the existing `m_FT`, no second buffer), `Shutdown`, `Shape(utf8,size)` (standalone single-string path — it never calls `ShapeLine`, emits white glyphs, and ignores `bold`/`italic` entirely), `ShapeLine(vector<TextRun>,size)` full pipeline (see below), `SplitDirectionalRuns` (`fribidi_get_bidi_types` + `fribidi_get_par_embedding_levels` with `FRIBIDI_PAR_ON` → `MaxLevel` parity grouping → `fribidi_reorder_line` → visual-LTR `DirectionalRun`s with `logicalByteOffset`), `BuildFontSpans` (per-char `FT_Get_Char_Index` cmap probe; fallback span only if fallback **fully** covers the missing run; maximal same-font spans), per-span HarfBuzz over the **whole re-encoded view string** with `item_offset/item_length` so joining context crosses span boundaries; RTL spans visited reversed; `direction/script` **hard-coded** (`RTL→HB_SCRIPT_ARABIC/"ar"`, `LTR→HB_SCRIPT_LATIN/"en"`), glyph load per path (`Shape`: `FT_LOAD_RENDER`; `ShapeLine`: `FT_LOAD_DEFAULT` + `Oblique`/`Embolden` + `FT_Render_Glyph`) with 26.6→px conversion, bbox normalize to (0,0), cluster→byte-offset mapping back to styled `TextRun`s, fake bold (`FT_GlyphSlot_Embolden`) / italic (`FT_GlyphSlot_Oblique`) in `ShapeLine` only.
- `arabic_fallback.h` — `namespace arabic_fallback { string NormalizeForFace(const string& utf8, FT_Face face); }` — returns modified copy only if changed.
- `arabic_fallback.cpp` — `NormalizeForFace`: decode UTF-8 → per-codepoint `FT_Get_Char_Index` check; for each missing presentation form (FB50–FEFC) try: (1) base letter 0600–06FF if present, (2) sibling contextual form via lazily-built `SiblingMap` (scans `arabic_fallback_data.h`), (3) compatibility decomposition (lam-alef, tatweel+mark); strip leading `U+0020` on isolated mark forms; drop missing tatweel; recursive resolve depth ≤4; re-encode. `kFormEntries` sorted by `cp` for binary search (`FindEntry`).
- `arabic_fallback_data.h` — GENERATED — `struct FormEntry{uint32_t cp; uint16_t baseLen; uint32_t base[18];}` × **731** entries from `UnicodeData.txt`; regeneration script `gen_arabic_data.ps1` is **not in repo**.

## Public API

```cpp
class TextShaper {
  bool Initialize(const string& fontPath, float fontSize);
  bool InitializeFallback(const string& fontPath, float fontSize);
  void Shutdown();
  ShapedTextResult Shape(const string& utf8, float fontSize=0);
  ShapedTextResult ShapeLine(const vector<TextRun>& runs, float fontSize=0);
  bool IsInitialized() const;
  bool HasFallback() const;
};

namespace arabic_fallback {
  string NormalizeForFace(const string& utf8, FT_Face face);
}
```

## Internal Structure

```
ShapeLine(runs, size):
  for each TextRun: NormalizeForFace(text, Face) — the fallback face is only tried inside BuildFontSpans, for a missing char candidate substitution
  SplitDirectionalRuns(concatenated logical text) → visual-LTR DirectionalRuns
  BuildFontSpans(per DirectionalRun) → FontSpans (cmap coverage, fallback only if fully covers)
  for each DirectionalRun in visual order
    for each FontSpan (same direction) — RTL spans in reverse
      hb_buffer_clear + add whole re-encoded view string (item_offset/length scoping)
      hb_buffer_set_direction/script/language (hard-coded AR/LATIN)
      hb_shape(hbFont, buffer)
      for each hb_glyph: FT_Load_Glyph(FT_LOAD_DEFAULT) + Oblique/Embolden + FT_Render_Glyph → 26.6→px → GlyphBitmap (ShapeLine path; Shape uses FT_LOAD_RENDER and skips bold/italic)
      map cluster → original byte offset → attach run's rgba/bold/italic
  bbox normalization to (0,0); totalWidth/Height
```

## Dependencies

- **Uses:** FreeType (`FT_New_Face`, `FT_Select_Charmap`, `FT_Set_Pixel_Sizes`, `FT_Load_Glyph`, `FT_Render_Glyph`, `FT_GlyphSlot_Embolden/Oblique`), HarfBuzz (`hb_ft_font_create`, `hb_ft_font_changed`, `hb_buffer_*`, `hb_shape`), FriBidi (`fribidi_get_bidi_types`, `fribidi_get_par_embedding_levels`, `fribidi_reorder_line`), `arabic_fallback_data.h`.
- **Used by:** `Renderer` (`RenderEntryTexture`), `renderer.cpp:ReloadFont`/`ReloadFontPreserveQueue` (re-`Initialize` on font change), `caption_texture.cpp`.

## Notable Patterns / Gotchas

- **No internal mutex** — `TextShaper` relies on `Renderer::m_CS` externally; calling `Shape` off the `m_CS` is a race.
- Script hard-coding: RTL always `HB_SCRIPT_ARABIC`, LTR always `LATIN` — Hebrew/other RTL scripts will mis-shape.
- `BuildFontSpans` fallback rule is all-or-nothing per missing run — if fallback doesn't **fully** cover a run, the primary face's `FT_Get_Char_Index==0` (missing glyph box) is kept.
- `arabic_fallback_data.h` is generated — edits must go through the generator; hand-patching will be clobbered.
- Fake bold/italic are FreeType post-effects (`Embolden`/`Oblique`) applied after shaping in `ShapeLine` only — `Shape` ignores both flags. Neither is a true font weight.
