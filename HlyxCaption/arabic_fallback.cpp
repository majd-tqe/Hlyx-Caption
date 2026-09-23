/*
 * Hlyx Caption — original project source
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Hlyx Caption Contributors
 */

#include "arabic_fallback.h"
#include "arabic_fallback_data.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace arabic_fallback {

namespace {

// -------------------------------------------------------------------
// UTF-8 helpers (self-contained; same logic as shaper.cpp)
// -------------------------------------------------------------------
size_t UTF8ByteLen(unsigned char c) {
    if (c < 0x80) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

uint32_t UTF8Decode(const std::string& s, size_t& i) {
    if (i >= s.size()) return 0;
    unsigned char c = (unsigned char)s[i];
    if (c < 0x80) { i += 1; return c; }
    if ((c & 0xE0) == 0xC0 && i + 1 < s.size()) {
        uint32_t cp = ((c & 0x1F) << 6) | (s[i + 1] & 0x3F);
        i += 2; return cp;
    }
    if ((c & 0xF0) == 0xE0 && i + 2 < s.size()) {
        uint32_t cp = ((c & 0x0F) << 12) | ((s[i + 1] & 0x3F) << 6) | (s[i + 2] & 0x3F);
        i += 3; return cp;
    }
    if ((c & 0xF8) == 0xF0 && i + 3 < s.size()) {
        uint32_t cp = ((c & 0x07) << 18) | ((s[i + 1] & 0x3F) << 12) | ((s[i + 2] & 0x3F) << 6) | (s[i + 3] & 0x3F);
        i += 4; return cp;
    }
    i += 1;
    return 0;
}

void UTF8Append(std::string& out, uint32_t cp) {
    if (cp < 0x80) {
        out.push_back((char)cp);
    } else if (cp < 0x800) {
        out.push_back((char)(0xC0 | (cp >> 6)));
        out.push_back((char)(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        out.push_back((char)(0xE0 | (cp >> 12)));
        out.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back((char)(0x80 | (cp & 0x3F)));
    } else {
        out.push_back((char)(0xF0 | (cp >> 18)));
        out.push_back((char)(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back((char)(0x80 | (cp & 0x3F)));
    }
}

// -------------------------------------------------------------------
// Table lookup (kFormEntries is sorted by cp)
// -------------------------------------------------------------------
constexpr size_t kFormCount = sizeof(kFormEntries) / sizeof(kFormEntries[0]);

const FormEntry* FindEntry(uint32_t cp) {
    size_t lo = 0, hi = kFormCount;
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        if (kFormEntries[mid].cp < cp) lo = mid + 1;
        else hi = mid;
    }
    if (lo < kFormCount && kFormEntries[lo].cp == cp) return &kFormEntries[lo];
    return nullptr;
}

inline bool HasGlyph(FT_Face face, uint32_t cp) {
    return face != nullptr && FT_Get_Char_Index(face, cp) != 0;
}

// -------------------------------------------------------------------
// Sibling map: base Arabic letter -> its presentation forms (isolated,
// final, initial, medial...). Built once; table is already cp-sorted so
// forms come out in canonical order (isolated first).
// -------------------------------------------------------------------
const std::unordered_map<uint32_t, std::vector<uint32_t>>& SiblingMap() {
    static std::unordered_map<uint32_t, std::vector<uint32_t>> s_map = [] {
        std::unordered_map<uint32_t, std::vector<uint32_t>> m;
        for (const auto& e : kFormEntries) {
            if (e.baseLen == 1 && e.base[0] >= 0x0600 && e.base[0] <= 0x06FF)
                m[e.base[0]].push_back(e.cp);
        }
        return m;
    }();
    return s_map;
}

// -------------------------------------------------------------------
// Resolve a single presentation-form codepoint against `face`.
// Returns the best substitute sequence (may be empty = keep original).
// -------------------------------------------------------------------
std::vector<uint32_t> Resolve(uint32_t cp, FT_Face face, int depth) {
    if (depth > 4) return {};
    const FormEntry* e = FindEntry(cp);
    if (!e) return {};

    // Unicode attaches a leading spacing U+0020 to the isolated forms of
    // combining marks (e.g. U+FE70 -> "0020 064B") so the mark can stand
    // alone as a spacing character. Drop ONLY that leading spacer; internal
    // spaces in long ligature decompositions (e.g. U+FDFA) are meaningful.
    std::vector<uint32_t> seq;
    for (uint32_t i = 0; i < e->baseLen; i++) {
        bool isLeadingMarkSpacer =
            (i == 0 && e->base[0] == 0x0020 && e->baseLen == 2 &&
             e->base[1] >= 0x0600 && e->base[1] <= 0x06FF);
        if (!isLeadingMarkSpacer)
            seq.push_back(e->base[i]);
    }
    if (seq.empty()) return {};

    if (seq.size() == 1) {
        uint32_t base = seq[0];
        // 1) the base letter/mark itself
        if (HasGlyph(face, base)) return { base };
        // 2) another form of the same letter (only meaningful for letters)
        if (base >= 0x0600 && base <= 0x06FF) {
            const auto& siblings = SiblingMap().find(base);
            if (siblings != SiblingMap().end()) {
                for (uint32_t s : siblings->second) {
                    if (s != cp && HasGlyph(face, s)) return { s };
                }
            }
        }
        // 3) base is itself a presentation form (defensive chain)
        if (base >= 0xFB50 && base <= 0xFEFC) {
            auto r = Resolve(base, face, depth + 1);
            if (!r.empty()) return r;
        }
        return {}; // nothing available -> caller keeps the original
    }

    // Multi-codepoint sequence (lam-alef, tatweel+mark, ligatures):
    // resolve each part independently. TATWEEL is dropped if missing;
    // an unresolvable letter part is kept as-is (better than dropping).
    std::vector<uint32_t> out;
    bool produced = false;
    for (uint32_t part : seq) {
        std::vector<uint32_t> r;
        if (part >= 0xFB50 && part <= 0xFEFC) {
            r = Resolve(part, face, depth + 1);
        } else if (HasGlyph(face, part)) {
            r = { part };
        }
        if (!r.empty()) {
            out.insert(out.end(), r.begin(), r.end());
            produced = true;
        } else if (part == 0x0640) {
            // tatweel missing: drop it, keep the mark
        } else {
            out.push_back(part); // last resort: keep the part as-is
        }
    }
    if (!produced && out.empty()) return {};
    return out;
}

} // namespace

// -------------------------------------------------------------------
// Public entry point
// -------------------------------------------------------------------
std::string NormalizeForFace(const std::string& utf8, FT_Face face) {
    if (utf8.empty() || !face) return utf8;

    std::string out;
    out.reserve(utf8.size() + 16);
    bool changed = false;
    size_t i = 0;
    while (i < utf8.size()) {
        size_t start = i;
        uint32_t cp = UTF8Decode(utf8, i);
        if (i == start) i += UTF8ByteLen((unsigned char)utf8[start]); // invalid byte

        bool isPresentationForm = (cp >= 0xFB50 && cp <= 0xFEFC);
        if (isPresentationForm && !HasGlyph(face, cp)) {
            auto r = Resolve(cp, face, 0);
            if (!r.empty()) {
                for (uint32_t c : r) UTF8Append(out, c);
                changed = true;
                continue;
            }
        }
        out.append(utf8, start, i - start); // keep original bytes
    }
    return changed ? out : utf8;
}

} // namespace arabic_fallback
