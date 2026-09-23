/*
 * Hlyx Caption — original project source
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Hlyx Caption Contributors
 */

#pragma once
#include <string>
#include <vector>
#include <d3d11.h>
#include <cstdint>

struct GlyphBitmap {
    int x, y, w, h;
    int bearingX, bearingY;
    int advance;
    std::vector<unsigned char> bitmap;
    uint8_t r = 255, g = 255, b = 255, a = 255;
};

struct ShapedTextResult {
    std::vector<GlyphBitmap> glyphs;
    int totalWidth = 0;
    int totalHeight = 0;
    std::string fontPath;
    float fontSize = 0;
};

struct TextRun {
    std::string text;
    uint8_t r = 255, g = 255, b = 255, a = 255;
    bool bold = false;
    bool italic = false;
};

class TextShaper {
public:
    bool Initialize(const std::string& fontPath, float fontSize);
    bool InitializeFallback(const std::string& fontPath, float fontSize);
    void Shutdown();
    ShapedTextResult Shape(const std::string& utf8Text, float fontSize = 0);
    ShapedTextResult ShapeLine(const std::vector<TextRun>& runs, float fontSize = 0);
    bool IsInitialized() const { return m_FT != nullptr; }
    bool HasFallback() const { return m_FaceFallback != nullptr; }

private:
    struct DirectionalRun {
        std::string text;
        bool isRTL;
        size_t logicalByteOffset = 0;
    };

    std::vector<DirectionalRun> SplitDirectionalRuns(const std::string& utf8Text) const;

    void* m_FT = nullptr;
    void* m_Face = nullptr;
    void* m_HBFont = nullptr;
    void* m_HBBuffer = nullptr;
    float m_FontSize = 0;
    std::string m_FontPath;

    void* m_FTFallback = nullptr;
    void* m_FaceFallback = nullptr;
    void* m_HBFontFallback = nullptr;
    std::string m_FontPathFallback;
};
