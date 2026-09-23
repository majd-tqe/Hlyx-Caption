/*
 * Hlyx Caption — original project source
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Hlyx Caption Contributors
 */

#pragma once
#include <string>

// Forward declaration of FreeType's FT_Face (avoid pulling freetype headers here).
struct FT_FaceRec_;
typedef struct FT_FaceRec_* FT_Face;

namespace arabic_fallback {

// ============================================================
// Normalize Arabic presentation-form codepoints that are missing
// from `face` to equivalent codepoints the face DOES contain:
//   - the base letter in the Arabic block (U+0600-U+06FF),
//   - another form (isolated/final/initial/medial) of the same letter,
//   - the decomposed sequence (lam-alef, tatweel+mark, ligatures).
// Codepoints the font already has are left untouched.
// Returns a copy of the input with substitutions applied.
// ============================================================
std::string NormalizeForFace(const std::string& utf8, FT_Face face);

} // namespace arabic_fallback
