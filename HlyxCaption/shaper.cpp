/*
 * Hlyx Caption — original project source
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Hlyx Caption Contributors
 */

#include "shaper.h"
#include "arabic_fallback.h"
#ifdef _DEBUG
#include <iostream>
#endif
#include <algorithm>
#include <cstdint>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include FT_SYNTHESIS_H
#include <fribidi/fribidi.h>

// -------------------------------------------------------------------
// UTF-8 helpers
// -------------------------------------------------------------------
static size_t UTF8ByteLen(unsigned char c) {
    if (c < 0x80) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

static uint32_t UTF8Decode(const std::string& s, size_t& i) {
    if (i >= s.size()) return 0;
    unsigned char c = (unsigned char)s[i];
    if (c < 0x80) { i += 1; return c; }
    if ((c & 0xE0) == 0xC0 && i + 1 < s.size()) {
        uint32_t cp = ((c & 0x1F) << 6) | (s[i+1] & 0x3F);
        i += 2; return cp;
    }
    if ((c & 0xF0) == 0xE0 && i + 2 < s.size()) {
        uint32_t cp = ((c & 0x0F) << 12) | ((s[i+1] & 0x3F) << 6) | (s[i+2] & 0x3F);
        i += 3; return cp;
    }
    if ((c & 0xF8) == 0xF0 && i + 3 < s.size()) {
        uint32_t cp = ((c & 0x07) << 18) | ((s[i+1] & 0x3F) << 12) | ((s[i+2] & 0x3F) << 6) | (s[i+3] & 0x3F);
        i += 4; return cp;
    }
    i += 1;
    return 0;
}

// -------------------------------------------------------------------
// Glyph-level font fallback ("view" construction)
//
// Previously an RTL run was handed ENTIRELY to the fallback font whenever
// any of its characters was missing from the primary font, switching the
// whole sentence. Instead, every run is re-encoded into a "view" string:
//   * chars present in the primary font are copied verbatim;
//   * chars missing from it are re-encoded via
//     arabic_fallback::NormalizeForFace against the FALLBACK face; they
//     become fallback spans only when that encoding is fully covered by
//     the fallback cmap, otherwise they remain verbatim inside the primary
//     span (same .notdef rendering as before, minimal span boundaries).
// Each maximal same-font span is then shaped OUT OF THE FULL VIEW through
// hb_buffer_add_utf8's item_offset/item_length, so HarfBuzz sees the
// neighbouring characters as pre/post-context and Arabic cursive joining
// stays correct across span boundaries.
// outByteToChar[i] = index of the original character that produced view
// byte i (used to map HarfBuzz clusters back to original text offsets).
// -------------------------------------------------------------------
struct RunChars {
    std::vector<uint32_t> cps;     // decoded codepoints of the run
    std::vector<size_t>   byteOff; // byte offset of each char within run text
};

static RunChars DecodeRunChars(const std::string& text) {
    RunChars rc;
    size_t p = 0;
    while (p < text.size()) {
        rc.byteOff.push_back(p);
        size_t oldP = p;
        uint32_t cp = UTF8Decode(text, p);
        if (p == oldP) p += UTF8ByteLen((unsigned char)text[p]);
        rc.cps.push_back(cp);
    }
    return rc;
}

struct FontSpan { size_t viewStart, viewEnd; bool useFB; };

static void BuildFontSpans(const std::string& runText,
                           const RunChars& rc,
                           FT_Face primaryFace,
                           FT_Face fbFace,
                           std::string& outViewText,
                           std::vector<uint32_t>& outByteToChar,
                           std::vector<FontSpan>& outSpans) {
    outViewText.clear();
    outByteToChar.clear();
    outSpans.clear();

    size_t spanViewStart = 0;
    bool spanUseFB = false;
    bool haveOpenSpan = false;

    for (size_t ci = 0; ci < rc.cps.size(); ++ci) {
        size_t b0 = rc.byteOff[ci];
        size_t b1 = (ci + 1 < rc.cps.size()) ? rc.byteOff[ci + 1] : runText.size();
        std::string piece = runText.substr(b0, b1 - b0);
        bool useFB = false;

        if (fbFace && primaryFace &&
            FT_Get_Char_Index(primaryFace, rc.cps[ci]) == 0) {
            // Missing from the primary face: try to find an encoding fully
            // covered by the fallback face (presentation-form -> base/sibling
            // substitution). Chars that cannot be resolved anywhere stay in
            // the primary span.
            std::string sub = arabic_fallback::NormalizeForFace(piece, fbFace);
            bool allCovered = !sub.empty();
            size_t q = 0;
            while (q < sub.size() && allCovered) {
                size_t oldQ = q;
                uint32_t c = UTF8Decode(sub, q);
                if (q == oldQ) q += UTF8ByteLen((unsigned char)sub[q]);
                if (FT_Get_Char_Index(fbFace, c) == 0) allCovered = false;
            }
            if (allCovered) {
                piece = std::move(sub);
                useFB = true;
            }
        }

        if (haveOpenSpan && useFB != spanUseFB) {
            outSpans.push_back({ spanViewStart, outViewText.size(), spanUseFB });
            haveOpenSpan = false;
        }
        if (!haveOpenSpan) {
            spanViewStart = outViewText.size();
            spanUseFB = useFB;
            haveOpenSpan = true;
        }

        outViewText.append(piece);
        for (size_t k = 0; k < piece.size(); ++k)
            outByteToChar.push_back((uint32_t)ci);
    }
    if (haveOpenSpan)
        outSpans.push_back({ spanViewStart, outViewText.size(), spanUseFB });
}

// -------------------------------------------------------------------
// FriBidi-based directional run splitting
// Returns runs in VISUAL order (left to right)
// -------------------------------------------------------------------
std::vector<TextShaper::DirectionalRun> TextShaper::SplitDirectionalRuns(const std::string& utf8Text) const {
    std::vector<DirectionalRun> runs;
    if (utf8Text.empty()) return runs;

    // Decode UTF-8 ├ÿ UTF-32, tracking byte positions
    std::vector<uint32_t> logical;
    std::vector<size_t> bytePos;
    size_t i = 0;
    while (i < utf8Text.size()) {
        bytePos.push_back(i);
        size_t oldI = i;
        uint32_t cp = UTF8Decode(utf8Text, i);
        logical.push_back(cp);
        if (i == oldI) i++; // safety
    }
    if (logical.empty()) return runs;
    size_t charCount = logical.size();

    // Get bidi types and embedding levels
    std::vector<FriBidiCharType> bidiTypes(charCount);
    std::vector<FriBidiLevel> embedLevels(charCount);
    FriBidiParType baseDir = FRIBIDI_PAR_ON;
    fribidi_get_bidi_types((FriBidiChar*)logical.data(), (FriBidiStrIndex)charCount, bidiTypes.data());
    fribidi_get_par_embedding_levels(bidiTypes.data(), (FriBidiStrIndex)charCount, &baseDir, embedLevels.data());

    // Build logical runs (consecutive same-parity chars)
    struct LogRun { size_t start, end; bool isRTL; };
    std::vector<LogRun> logRuns;
    size_t runStart = 0;
    bool prevRTL = (embedLevels[0] % 2) == 1;
    for (size_t ci = 1; ci < charCount; ci++) {
        bool isRTL = (embedLevels[ci] % 2) == 1;
        if (isRTL != prevRTL) {
            logRuns.push_back({ runStart, ci, prevRTL });
            runStart = ci;
            prevRTL = isRTL;
        }
    }
    logRuns.push_back({ runStart, charCount, prevRTL });

    if (logRuns.empty()) return runs;

    // Get visual order mapping (must init map to identity per FriBidi docs)
    std::vector<FriBidiStrIndex> visualMap(charCount);
    for (FriBidiStrIndex i = 0; i < (FriBidiStrIndex)charCount; i++)
        visualMap[i] = i;
    std::vector<FriBidiChar> visualStr(charCount, 0);
    fribidi_reorder_line(FRIBIDI_FLAGS_DEFAULT, bidiTypes.data(), (FriBidiStrIndex)charCount,
                         0, baseDir, embedLevels.data(), visualStr.data(), visualMap.data());

    // Invert: logical index → first visual position
    std::vector<size_t> logToVis(charCount, (size_t)-1);
    for (FriBidiStrIndex v = 0; v < (FriBidiStrIndex)charCount; v++) {
        FriBidiStrIndex li = visualMap[v];
        if (logToVis[li] == (size_t)-1)
            logToVis[li] = (size_t)v;
    }

    // Sort logical runs by visual order (leftmost first)
    struct VisRun { size_t visPos; size_t start, end; bool isRTL; };
    std::vector<VisRun> visRuns;
    for (auto& lr : logRuns) {
        size_t minV = (size_t)-1;
        for (size_t ci = lr.start; ci < lr.end; ci++)
            if (logToVis[ci] < minV) minV = logToVis[ci];
        visRuns.push_back({ minV, lr.start, lr.end, lr.isRTL });
    }
    std::sort(visRuns.begin(), visRuns.end(),
              [](const VisRun& a, const VisRun& b) { return a.visPos < b.visPos; });

    // Extract UTF-8 substrings in visual order
    for (auto& vr : visRuns) {
        size_t byteStart = bytePos[vr.start];
        size_t byteEnd = (vr.end < charCount) ? bytePos[vr.end] : utf8Text.size();
        runs.push_back({ utf8Text.substr(byteStart, byteEnd - byteStart), vr.isRTL, byteStart });
    }

    return runs;
}

// -------------------------------------------------------------------
// Font initialization
// -------------------------------------------------------------------
bool TextShaper::Initialize(const std::string& fontPath, float fontSize) {
    Shutdown();

    FT_Library ft;
    if (FT_Init_FreeType(&ft) != 0) {
#ifdef _DEBUG
        std::cout << "[-] FreeType init failed.\n";
#endif
        return false;
    }

    FT_Face face;
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face) != 0) {
#ifdef _DEBUG
        std::cout << "[-] FreeType failed to load font: " << fontPath << "\n";
#endif
        FT_Done_FreeType(ft);
        return false;
    }

    FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    FT_Set_Pixel_Sizes(face, 0, (FT_UInt)fontSize);

    hb_font_t* hbFont = hb_ft_font_create(face, nullptr);
    hb_buffer_t* hbBuffer = hb_buffer_create();

    m_FT = ft;
    m_Face = face;
    m_HBFont = hbFont;
    m_HBBuffer = hbBuffer;
    m_FontSize = fontSize;
    m_FontPath = fontPath;

#ifdef _DEBUG
    std::cout << "[Shaper] Initialized: " << fontPath << " (" << fontSize << "px)\n";
#endif
    return true;
}

bool TextShaper::InitializeFallback(const std::string& fontPath, float fontSize) {
    if (fontPath.empty()) return false;
    if (!m_FT) return false;

    FT_Face face;
    if (FT_New_Face((FT_Library)m_FT, fontPath.c_str(), 0, &face) != 0) {
#ifdef _DEBUG
        std::cout << "[-] Fallback font failed to load: " << fontPath << "\n";
#endif
        return false;
    }

    FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    FT_Set_Pixel_Sizes(face, 0, (FT_UInt)fontSize);

    hb_font_t* hbFont = hb_ft_font_create(face, nullptr);

    m_FaceFallback = face;
    m_HBFontFallback = hbFont;
    m_FontPathFallback = fontPath;

#ifdef _DEBUG
    std::cout << "[Shaper] Fallback loaded: " << fontPath << "\n";
#endif
    return true;
}

void TextShaper::Shutdown() {
    if (m_HBBuffer) { hb_buffer_destroy((hb_buffer_t*)m_HBBuffer); m_HBBuffer = nullptr; }
    if (m_HBFont) { hb_font_destroy((hb_font_t*)m_HBFont); m_HBFont = nullptr; }
    if (m_Face) { FT_Done_Face((FT_Face)m_Face); m_Face = nullptr; }
    if (m_HBFontFallback) { hb_font_destroy((hb_font_t*)m_HBFontFallback); m_HBFontFallback = nullptr; }
    if (m_FaceFallback) { FT_Done_Face((FT_Face)m_FaceFallback); m_FaceFallback = nullptr; }
    if (m_FT) { FT_Done_FreeType((FT_Library)m_FT); m_FT = nullptr; }
}

// -------------------------------------------------------------------
// Shape a single string (uses directional run splitting + FriBidi)
// -------------------------------------------------------------------
ShapedTextResult TextShaper::Shape(const std::string& utf8Text, float fontSize) {
    ShapedTextResult result;
    if (!m_FT || !m_Face || !m_HBFont) return result;
    if (fontSize <= 0) fontSize = m_FontSize;

    // Normalize Arabic presentation forms against the primary face BEFORE
    // directional splitting and font-coverage checks, so every check below
    // runs on the text that will actually be rendered. Missing presentation
    // forms are replaced by base letters / other forms / decompositions the
    // font does contain (see arabic_fallback.cpp).
    std::string normText = utf8Text;
    if (m_Face)
        normText = arabic_fallback::NormalizeForFace(utf8Text, (FT_Face)m_Face);

    auto dirRuns = SplitDirectionalRuns(normText);
    if (dirRuns.empty()) return result;



    FT_Face primaryFace = (FT_Face)m_Face;
    hb_font_t* primaryHbFont = (hb_font_t*)m_HBFont;
    hb_buffer_t* buf = (hb_buffer_t*)m_HBBuffer;
    FT_Face fbFace = (FT_Face)m_FaceFallback;
    hb_font_t* fbHbFont = (hb_font_t*)m_HBFontFallback;

    int baselineRef = static_cast<int>(primaryFace->size->metrics.ascender >> 6);
    int minX = 0, maxX = 0, minY = 0, maxY = 0;
    int cursorX = 0;
    bool firstGlyph = true;

    for (auto& dirRun : dirRuns) {
        // --- Glyph-level font selection (joining-safe) ---
        // The run is re-encoded into a "view" where only characters missing
        // from the primary font take their encoding from the fallback font.
        // Every span is shaped out of the full view so HarfBuzz uses the
        // neighbouring characters as pre/post-context and Arabic cursive
        // forms stay correct across font-span boundaries.
        RunChars rc = DecodeRunChars(dirRun.text);
        std::string viewText;
        std::vector<uint32_t> viewByteToChar;
        std::vector<FontSpan> spans;
        BuildFontSpans(dirRun.text, rc, primaryFace, fbFace,
                       viewText, viewByteToChar, spans);
        if (spans.empty()) continue;

        // Lay the spans out along the pen cursor. HarfBuzz returns each
        // span's glyphs already in visual order INTERNALLY, but cross-span
        // placement follows iteration order:
        //   * LTR run: logical order == visual order -> iterate forward.
        //   * RTL run: logically-first span must end up RIGHTMOST, so the
        //     spans are visited in REVERSE order (otherwise a fallback span
        //     in the middle lands on the wrong side of the sentence).
        size_t spanCount = spans.size();
        for (size_t si = 0; si < spanCount; ++si) {
            auto& sp = spans[dirRun.isRTL ? (spanCount - 1 - si) : si];
            bool useFB = sp.useFB && fbFace;
            FT_Face face = useFB ? fbFace : primaryFace;
            hb_font_t* hbFont = useFB ? fbHbFont : primaryHbFont;

            FT_Set_Pixel_Sizes(face, 0, (FT_UInt)fontSize);
            hb_ft_font_changed(hbFont);

            hb_buffer_clear_contents(buf);
            hb_buffer_set_direction(buf, dirRun.isRTL ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
            hb_buffer_set_script(buf, dirRun.isRTL ? HB_SCRIPT_ARABIC : HB_SCRIPT_LATIN);
            hb_buffer_set_language(buf, dirRun.isRTL ?
                hb_language_from_string("ar", -1) :
                hb_language_from_string("en", -1));
            // Shape only [viewStart, viewEnd); the rest of the view supplies
            // implicit pre/post-context. Cluster values come back absolute
            // w.r.t. viewText (mapped back via viewByteToChar where needed).
            hb_buffer_add_utf8(buf, viewText.data(), (int)viewText.size(),
                               (unsigned int)sp.viewStart,
                               (int)(sp.viewEnd - sp.viewStart));

            hb_shape(hbFont, buf, nullptr, 0);

            unsigned int glyphCount = 0;
            hb_glyph_info_t* glyphInfo = hb_buffer_get_glyph_infos(buf, &glyphCount);
            hb_glyph_position_t* glyphPos = hb_buffer_get_glyph_positions(buf, &glyphCount);
            if (glyphCount == 0) continue;

            size_t glyphStartIdx = result.glyphs.size();
            int runCursorX = 0;
            int runMinX = 0, runMaxX = 0;
            bool firstInRun = true;

            for (unsigned int gi = 0; gi < glyphCount; gi++) {
                FT_Load_Glyph(face, glyphInfo[gi].codepoint, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL);
                FT_Bitmap& bmp = face->glyph->bitmap;

                auto hb_to_px = [](int v) { return (v + (v >= 0 ? 32 : -32)) / 64; };
                int penX = runCursorX + hb_to_px(glyphPos[gi].x_offset);
                int penY = -hb_to_px(glyphPos[gi].y_offset);
                int localX = penX + face->glyph->bitmap_left;
                int gy = penY - face->glyph->bitmap_top + baselineRef;
                GlyphBitmap gb;
                gb.x = localX;
                gb.y = gy;
                gb.w = bmp.width;
                gb.h = bmp.rows;
                gb.bearingX = face->glyph->bitmap_left;
                gb.bearingY = face->glyph->bitmap_top;
                gb.advance = (int)(face->glyph->advance.x / 64);

                if (bmp.width > 0 && bmp.rows > 0 && bmp.buffer) {
                    gb.bitmap.resize(bmp.width * bmp.rows);
                    memcpy(gb.bitmap.data(), bmp.buffer, bmp.width * bmp.rows);
                }

                if (firstInRun) {
                    runMinX = localX;
                    runMaxX = localX + bmp.width;
                    firstInRun = false;
                } else {
                    if (localX < runMinX) runMinX = localX;
                    if (localX + (int)bmp.width > runMaxX) runMaxX = localX + (int)bmp.width;
                }

                result.glyphs.push_back(gb);
                runCursorX += (int)(glyphPos[gi].x_advance / 64);
            }

            int runPixelWidth = runMaxX - runMinX;
            int shift = cursorX - runMinX;

            for (size_t i = glyphStartIdx; i < result.glyphs.size(); i++) {
                int newX = result.glyphs[i].x + shift;
                auto& gb = result.glyphs[i];
                gb.x = newX;
                if (firstGlyph) {
                    minX = newX; maxX = newX + gb.w;
                    minY = gb.y; maxY = gb.y + gb.h;
                    firstGlyph = false;
                } else {
                    if (newX < minX) minX = newX;
                    if (newX + (int)gb.w > maxX) maxX = newX + gb.w;
                    if (gb.y < minY) minY = gb.y;
                    if (gb.y + (int)gb.h > maxY) maxY = gb.y + gb.h;
                }
            }

            cursorX += runCursorX;
        }
    }

    result.totalWidth = maxX - minX;
    result.totalHeight = maxY - minY;

    if (minX != 0 || minY != 0) {
        for (auto& g : result.glyphs) {
            g.x -= minX;
            g.y -= minY;
        }
    }

    result.fontSize = fontSize;
    result.fontPath = m_FontPath;
    return result;
}

// -------------------------------------------------------------------
// Shape a line (multiple TextRuns with color/style)
// -------------------------------------------------------------------
ShapedTextResult TextShaper::ShapeLine(const std::vector<TextRun>& runs, float fontSize) {
    ShapedTextResult result;
    if (!m_FT || !m_Face || !m_HBFont || runs.empty()) return result;
    if (fontSize <= 0) fontSize = m_FontSize;

    // Concatenate all runs and track boundaries.
    // Runs are normalized individually (substitution is per-codepoint, so
    // normalizing the concatenation == concatenating normalizations); this
    // keeps the boundary offsets aligned with the normalized fullText.
    std::string fullText;
    struct RunBound { size_t end; uint8_t r, g, b, a; bool bold, italic; };
    std::vector<RunBound> boundaries;
    FT_Face normFace = (FT_Face)m_Face;
    for (auto& run : runs) {
        if (run.text.empty()) continue;
        std::string normRun = run.text;
        if (normFace)
            normRun = arabic_fallback::NormalizeForFace(run.text, normFace);
        fullText += normRun;
        boundaries.push_back({ fullText.size(), run.r, run.g, run.b, run.a, run.bold, run.italic });
    }
    if (fullText.empty()) return result;

    // Split by direction
    auto dirRuns = SplitDirectionalRuns(fullText);
    if (dirRuns.empty()) return result;



    FT_Face primaryFace = (FT_Face)m_Face;
    hb_font_t* primaryHbFont = (hb_font_t*)m_HBFont;
    hb_buffer_t* buf = (hb_buffer_t*)m_HBBuffer;
    FT_Face fbFace = (FT_Face)m_FaceFallback;
    hb_font_t* fbHbFont = (hb_font_t*)m_HBFontFallback;

    int baselineRef = static_cast<int>(primaryFace->size->metrics.ascender >> 6);
    int minX = 0, maxX = 0, minY = 0, maxY = 0;
    int cursorX = 0;
    bool firstGlyph = true;

    for (auto& dirRun : dirRuns) {
        // --- Glyph-level font selection (joining-safe) ---
        // The run is re-encoded into a "view" where only characters missing
        // from the primary font take their encoding from the fallback font.
        // Every span is shaped out of the full view so HarfBuzz uses the
        // neighbouring characters as pre/post-context and Arabic cursive
        // forms stay correct across font-span boundaries.
        RunChars rc = DecodeRunChars(dirRun.text);
        std::string viewText;
        std::vector<uint32_t> viewByteToChar;
        std::vector<FontSpan> spans;
        BuildFontSpans(dirRun.text, rc, primaryFace, fbFace,
                       viewText, viewByteToChar, spans);
        if (spans.empty()) continue;

        // Lay the spans out along the pen cursor. HarfBuzz returns each
        // span's glyphs already in visual order INTERNALLY, but cross-span
        // placement follows iteration order:
        //   * LTR run: logical order == visual order -> iterate forward.
        //   * RTL run: logically-first span must end up RIGHTMOST, so the
        //     spans are visited in REVERSE order (otherwise a fallback span
        //     in the middle lands on the wrong side of the sentence).
        size_t spanCount = spans.size();
        for (size_t si = 0; si < spanCount; ++si) {
            auto& sp = spans[dirRun.isRTL ? (spanCount - 1 - si) : si];
            bool useFB = sp.useFB && fbFace;
            FT_Face face = useFB ? fbFace : primaryFace;
            hb_font_t* hbFont = useFB ? fbHbFont : primaryHbFont;

            FT_Set_Pixel_Sizes(face, 0, (FT_UInt)fontSize);
            hb_ft_font_changed(hbFont);

            hb_buffer_clear_contents(buf);
            hb_buffer_set_direction(buf, dirRun.isRTL ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
            hb_buffer_set_script(buf, dirRun.isRTL ? HB_SCRIPT_ARABIC : HB_SCRIPT_LATIN);
            hb_buffer_set_language(buf, dirRun.isRTL ?
                hb_language_from_string("ar", -1) :
                hb_language_from_string("en", -1));
            // Shape only [viewStart, viewEnd); the rest of the view supplies
            // implicit pre/post-context. Cluster values come back absolute
            // w.r.t. viewText and are mapped back to original fullText bytes
            // through viewByteToChar below.
            hb_buffer_add_utf8(buf, viewText.data(), (int)viewText.size(),
                               (unsigned int)sp.viewStart,
                               (int)(sp.viewEnd - sp.viewStart));

            hb_shape(hbFont, buf, nullptr, 0);

            unsigned int glyphCount = 0;
            hb_glyph_info_t* glyphInfo = hb_buffer_get_glyph_infos(buf, &glyphCount);
            hb_glyph_position_t* glyphPos = hb_buffer_get_glyph_positions(buf, &glyphCount);
            if (glyphCount == 0) continue;

            size_t glyphStartIdx = result.glyphs.size();
            int runCursorX = 0;
            int runMinX = 0, runMaxX = 0;
            bool firstInRun = true;

            for (unsigned int gi = 0; gi < glyphCount; gi++) {
                // Map the glyph's cluster (absolute within view bytes) back
                // to its source character, then to the original fullText byte
                // offset used by the TextRun boundary lookup below.
                unsigned int cluster = 0;
                if (!viewByteToChar.empty()) {
                    uint32_t vc = glyphInfo[gi].cluster;
                    if (vc >= viewByteToChar.size())
                        vc = (uint32_t)viewByteToChar.size() - 1;
                    cluster = (unsigned int)(dirRun.logicalByteOffset +
                                             rc.byteOff[viewByteToChar[vc]]);
                }

                // Find which TextRun this glyph belongs to
                uint8_t cr = 255, cg = 255, cb = 255, ca = 255;
                bool cBold = false, cItalic = false;
                for (auto& bnd : boundaries) {
                    if (cluster < bnd.end) {
                        cr = bnd.r; cg = bnd.g; cb = bnd.b; ca = bnd.a;
                        cBold = bnd.bold; cItalic = bnd.italic;
                        break;
                    }
                }

                FT_Load_Glyph(face, glyphInfo[gi].codepoint, FT_LOAD_DEFAULT);
                if (cItalic) FT_GlyphSlot_Oblique(face->glyph);
                if (cBold) FT_GlyphSlot_Embolden(face->glyph);
                FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
                FT_Bitmap& bmp = face->glyph->bitmap;

                auto hb_to_px = [](int v) { return (v + (v >= 0 ? 32 : -32)) / 64; };
                int penX = runCursorX + hb_to_px(glyphPos[gi].x_offset);
                int penY = -hb_to_px(glyphPos[gi].y_offset);
                int localX = penX + face->glyph->bitmap_left;
                int gy = penY - face->glyph->bitmap_top + baselineRef;
                GlyphBitmap gb;
                gb.x = localX; gb.y = gy;
                gb.w = bmp.width; gb.h = bmp.rows;
                gb.bearingX = face->glyph->bitmap_left;
                gb.bearingY = face->glyph->bitmap_top;
                gb.advance = (int)(face->glyph->advance.x / 64);
                gb.r = cr; gb.g = cg; gb.b = cb; gb.a = ca;

                if (bmp.width > 0 && bmp.rows > 0 && bmp.buffer) {
                    gb.bitmap.resize(bmp.width * bmp.rows);
                    memcpy(gb.bitmap.data(), bmp.buffer, bmp.width * bmp.rows);
                }

                if (firstInRun) {
                    runMinX = localX;
                    runMaxX = localX + bmp.width;
                    firstInRun = false;
                } else {
                    if (localX < runMinX) runMinX = localX;
                    if (localX + (int)bmp.width > runMaxX) runMaxX = localX + (int)bmp.width;
                }

                result.glyphs.push_back(gb);
                runCursorX += (int)(glyphPos[gi].x_advance / 64);
            }

            int runPixelWidth = runMaxX - runMinX;
            int shift = cursorX - runMinX;

            for (size_t i = glyphStartIdx; i < result.glyphs.size(); i++) {
                int newX = result.glyphs[i].x + shift;
                auto& gb = result.glyphs[i];
                gb.x = newX;
                if (firstGlyph) {
                    minX = newX; maxX = newX + gb.w;
                    minY = gb.y; maxY = gb.y + gb.h;
                    firstGlyph = false;
                } else {
                    if (newX < minX) minX = newX;
                    if (newX + (int)gb.w > maxX) maxX = newX + gb.w;
                    if (gb.y < minY) minY = gb.y;
                    if (gb.y + (int)gb.h > maxY) maxY = gb.y + gb.h;
                }
            }

            cursorX += runCursorX;
        }
    }

    result.totalWidth = maxX - minX;
    result.totalHeight = maxY - minY;

    if (minX != 0 || minY != 0) {
        for (auto& g : result.glyphs) {
            g.x -= minX;
            g.y -= minY;
        }
    }

    result.fontSize = fontSize;
    result.fontPath = m_FontPath;
    return result;
}

