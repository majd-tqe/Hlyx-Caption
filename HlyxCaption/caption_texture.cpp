/*
 * Hlyx Caption — original project source
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Hlyx Caption Contributors
 */

#include "renderer.h"
#include "config.h"
#include "shaper.h"
#include <d3d11.h>
#include <string>
#include <vector>

void Renderer::RenderEntryTexture(CaptionEntry& entry, int maxPixelWidth) {
    if (!m_Shaper || !m_Shaper->IsInitialized() || !m_Device || entry.phrases.empty()) return;
    entry.ReleaseTexture();

    // Determine which phrases are currently active (non-expired)
    double now = GetTimeQPC();
    float elapsed = (float)(now - entry.startQPC);
    std::vector<int> activeIndices;
    for (int pi = 0; pi < (int)entry.phrases.size(); pi++) {
        auto& phrase = entry.phrases[pi];
        if (elapsed < phrase.delay) break;
        if (phrase.duration > 0 && elapsed >= phrase.delay + phrase.duration)
            continue;
        activeIndices.push_back(pi);
    }
    if (activeIndices.empty()) return;

    // Collect all runs from active phrases into lines
    std::vector<std::vector<TextRun>> allLines;
    for (int pi : activeIndices) {
        for (auto& cl : entry.phrases[pi].lines) {
            std::vector<TextRun> runs;
            for (auto& cr : cl.runs) {
                if (!cr.text.empty())
                    runs.push_back({ cr.text, cr.r, cr.g, cr.b, cr.a, cr.bold, cr.italic });
            }
            if (!runs.empty())
                allLines.push_back(std::move(runs));
        }
    }
    if (allLines.empty()) return;

    float fontSize = GetScaledFontSize(g_Config.font_size);

    // Word wrap: split lines that exceed maxPixelWidth at word boundaries
    if (maxPixelWidth > 0) {
        std::vector<std::vector<TextRun>> wrappedLines;
        for (auto& runs : allLines) {
            ShapedTextResult sr = m_Shaper->ShapeLine(runs, fontSize);
            if (sr.glyphs.empty() || sr.totalWidth <= maxPixelWidth) {
                wrappedLines.push_back(runs);
                continue;
            }

            // Extract words preserving run properties
            struct Word { std::string text; uint8_t r, g, b, a; bool bold, italic; int w; };
            std::vector<Word> words;
            for (auto& run : runs) {
                std::string word;
                for (char c : run.text) {
                    if (c == ' ') {
                        if (!word.empty()) {
                            words.push_back({word, run.r, run.g, run.b, run.a, run.bold, run.italic, 0});
                            word.clear();
                        }
                    } else {
                        word += c;
                    }
                }
                if (!word.empty())
                    words.push_back({word, run.r, run.g, run.b, run.a, run.bold, run.italic, 0});
            }
            if (words.empty()) { wrappedLines.push_back(runs); continue; }

            // Measure each word
            for (auto& w : words) {
                std::vector<TextRun> tr = {{w.text, w.r, w.g, w.b, w.a, w.bold, w.italic}};
                w.w = m_Shaper->ShapeLine(tr, fontSize).totalWidth;
            }

            // Measure a space for word separation
            int spaceW = m_Shaper->ShapeLine({{" ", 255, 255, 255, 255, false, false}}, fontSize).totalWidth;

            // Greedy pack words into lines
            std::vector<std::vector<size_t>> packed;
            std::vector<size_t> curLine;
            int curW = 0;
            for (size_t i = 0; i < words.size(); i++) {
                int needed = curW + (curLine.empty() ? 0 : spaceW) + words[i].w;
                if (needed > maxPixelWidth && !curLine.empty()) {
                    packed.push_back(std::move(curLine));
                    curLine.clear();
                    curW = 0;
                }
                if (!curLine.empty()) curW += spaceW;
                curLine.push_back(i);
                curW += words[i].w;
            }
            if (!curLine.empty()) packed.push_back(std::move(curLine));

            // Reconstruct runs from packed lines
            for (auto& line : packed) {
                std::vector<TextRun> lineRuns;
                for (size_t i = 0; i < line.size(); i++) {
                    auto& w = words[line[i]];
                    if (i > 0) {
                        auto& prev = words[line[i - 1]];
                        if (prev.r == w.r && prev.g == w.g && prev.b == w.b &&
                            prev.a == w.a && prev.bold == w.bold && prev.italic == w.italic) {
                            lineRuns.back().text += " " + w.text;
                            continue;
                        }
                        lineRuns.push_back({" ", prev.r, prev.g, prev.b, prev.a, prev.bold, prev.italic});
                    }
                    lineRuns.push_back({w.text, w.r, w.g, w.b, w.a, w.bold, w.italic});
                }
                wrappedLines.push_back(std::move(lineRuns));
            }
        }
        allLines = std::move(wrappedLines);
    }

    float lineSpacing = fontSize * g_Config.line_spacing;
    struct ShapedLine {
        std::vector<GlyphBitmap> glyphs;
        int totalWidth;
    };
    std::vector<ShapedLine> shapedLines;
    int maxWidth = 0;
    int cursorY = 0;

    for (auto& runs : allLines) {
        ShapedTextResult sr = m_Shaper->ShapeLine(runs, fontSize);
        if (sr.glyphs.empty()) continue;

        for (auto& g : sr.glyphs)
            g.y += cursorY;

        if (sr.totalWidth > maxWidth) maxWidth = sr.totalWidth;
        cursorY += (int)lineSpacing;

        shapedLines.push_back({ std::move(sr.glyphs), sr.totalWidth });
    }
    if (shapedLines.empty()) return;

    // Align each line horizontally based on config
    if (shapedLines.size() > 1) {
        for (auto& sl : shapedLines) {
            int offsetX = 0;
            switch (g_Config.text_alignment) {
                case OverlayConfig::ALIGN_LEFT:
                    offsetX = 0;
                    break;
                case OverlayConfig::ALIGN_RIGHT:
                    offsetX = maxWidth - sl.totalWidth;
                    break;
                case OverlayConfig::ALIGN_CENTER:
                default:
                    offsetX = (maxWidth - sl.totalWidth) / 2;
                    break;
            }
            if (offsetX > 0) {
                for (auto& g : sl.glyphs)
                    g.x += offsetX;
            }
        }
    }

    // Calculate bounding box
    int absMinX = 0, absMinY = 0, absMaxX = 0, absMaxY = 0;
    bool first = true;
    for (auto& sl : shapedLines) {
        for (auto& g : sl.glyphs) {
            if (first) {
                absMinX = g.x; absMinY = g.y;
                absMaxX = g.x + g.w; absMaxY = g.y + g.h;
                first = false;
            } else {
                if (g.x < absMinX) absMinX = g.x;
                if (g.y < absMinY) absMinY = g.y;
                if (g.x + g.w > absMaxX) absMaxX = g.x + g.w;
                if (g.y + g.h > absMaxY) absMaxY = g.y + g.h;
            }
        }
    }
    int totalW = absMaxX - absMinX;
    int totalH = absMaxY - absMinY;

    int w = totalW + 8;
    int h = totalH + 8;
    if (w < 1) w = 1;
    if (h < 1) h = 1;

    int offX = 4 - absMinX;
    int offY = 4 - absMinY;

    std::vector<unsigned char> buffer(w * h * 4, 0);

    for (auto& sl : shapedLines) {
        for (auto& g : sl.glyphs) {
            if (g.bitmap.empty()) continue;
            int gxStart = g.x + offX;
            int gyStart = g.y + offY;
            for (int gy = 0; gy < g.h; gy++) {
                for (int gx = 0; gx < g.w; gx++) {
                    int bx = gxStart + gx;
                    int by = gyStart + gy;
                    if (bx < 0 || bx >= w || by < 0 || by >= h) continue;
                    unsigned char alpha = g.bitmap[gy * g.w + gx];
                    if (alpha == 0) continue;
                    int dst = (by * w + bx) * 4;
                    unsigned char ta = (unsigned char)((int)g.a * alpha / 255);
                    if (ta == 0) continue;

                    if (buffer[dst + 3] == 0) {
                        buffer[dst + 0] = g.r;
                        buffer[dst + 1] = g.g;
                        buffer[dst + 2] = g.b;
                        buffer[dst + 3] = ta;
                    }
                    else {
                        unsigned char oldA = buffer[dst + 3];
                        unsigned char newA = ta > oldA ? ta : oldA;
                        buffer[dst + 0] = (uint8_t)((g.r * ta + buffer[dst + 0] * oldA) / (ta + oldA + 1));
                        buffer[dst + 1] = (uint8_t)((g.g * ta + buffer[dst + 1] * oldA) / (ta + oldA + 1));
                        buffer[dst + 2] = (uint8_t)((g.b * ta + buffer[dst + 2] * oldA) / (ta + oldA + 1));
                        buffer[dst + 3] = newA;
                    }
                }
            }
        }
    }

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = w;
    td.Height = h;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = buffer.data();
    initData.SysMemPitch = w * 4;

    if (FAILED(m_Device->CreateTexture2D(&td, &initData, &entry.texture))) return;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = td.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    if (FAILED(m_Device->CreateShaderResourceView(entry.texture, &srvDesc, &entry.srv))) {
        entry.texture->Release();
        entry.texture = nullptr;
        return;
    }

    entry.texID = (void*)entry.srv;
    entry.pixelWidth = w;
    entry.pixelHeight = h;
}
