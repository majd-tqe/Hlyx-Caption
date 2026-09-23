/*
 * Hlyx Caption — original project source
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Hlyx Caption Contributors
 */

#include "renderer.h"
#include "config.h"
#include "caption_parser.h"
#include <string>
#include <vector>
#include <tuple>
#include <charconv>
#include <cstdlib>

// ---- Tag parsing ----

// Parse color from a clr:r,g,b tag using std::from_chars (safer, no sscanf)
// Returns true on success, false if parsing failed
static bool ParseClrTag(const std::string& tag, uint8_t& outR, uint8_t& outG, uint8_t& outB) {
    if (tag.size() <= 4 || tag.substr(0, 4) != "clr:") return false;
    const char* start = tag.c_str() + 4;
    const char* end = tag.c_str() + tag.size();
    int r = 0, g = 0, b = 0;
    auto res = std::from_chars(start, end, r);
    if (res.ec != std::errc{} || res.ptr >= end || *res.ptr != ',') return false;
    res = std::from_chars(res.ptr + 1, end, g);
    if (res.ec != std::errc{} || res.ptr >= end || *res.ptr != ',') return false;
    res = std::from_chars(res.ptr + 1, end, b);
    if (res.ec != std::errc{}) return false;

    auto clamp = [](int v) { return (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v)); };
    outR = clamp(r); outG = clamp(g); outB = clamp(b);
    return true;
}

static bool ParsePlayerClrTag(const std::string& tag, bool fromPlayer, uint8_t& outR, uint8_t& outG, uint8_t& outB) {
    if (tag.size() <= 10 || tag.substr(0, 10) != "playerclr:") return false;
    const char* start = tag.c_str() + 10;
    const char* end = tag.c_str() + tag.size();
    int pr = 0, pg = 0, pb = 0, nr = 0, ng = 0, nb = 0;
    auto res = std::from_chars(start, end, pr);
    if (res.ec != std::errc{} || res.ptr >= end || *res.ptr != ',') return false;
    res = std::from_chars(res.ptr + 1, end, pg);
    if (res.ec != std::errc{} || res.ptr >= end || *res.ptr != ',') return false;
    res = std::from_chars(res.ptr + 1, end, pb);
    if (res.ec != std::errc{} || res.ptr >= end || *res.ptr != ':') return false;
    res = std::from_chars(res.ptr + 1, end, nr);
    if (res.ec != std::errc{} || res.ptr >= end || *res.ptr != ',') return false;
    res = std::from_chars(res.ptr + 1, end, ng);
    if (res.ec != std::errc{} || res.ptr >= end || *res.ptr != ',') return false;
    res = std::from_chars(res.ptr + 1, end, nb);
    if (res.ec != std::errc{}) return false;

    auto clamp = [](int v) { return (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v)); };
    if (fromPlayer) {
        outR = clamp(pr); outG = clamp(pg); outB = clamp(pb);
    } else {
        outR = clamp(nr); outG = clamp(ng); outB = clamp(nb);
    }
    return true;
}

static void ParseLineRuns(const std::string& text, std::vector<CaptionLine>& lines,
                          uint8_t& curR, uint8_t& curG, uint8_t& curB, uint8_t& curA,
                          bool& curBold, bool& curItalic,
                          std::vector<std::tuple<uint8_t,uint8_t,uint8_t,uint8_t>>& colorStack,
                          bool fromPlayer = false)
{
    if (text.empty()) return;

    CaptionLine line;
    size_t pos = 0;
    while (pos < text.size()) {
        size_t tagStart = text.find('<', pos);
        if (tagStart == std::string::npos) {
            std::string plain = text.substr(pos);
            if (!plain.empty()) {
                if (!line.runs.empty() &&
                    line.runs.back().r == curR &&
                    line.runs.back().g == curG &&
                    line.runs.back().b == curB &&
                    line.runs.back().a == curA &&
                    line.runs.back().bold == curBold &&
                    line.runs.back().italic == curItalic) {
                    line.runs.back().text += plain;
                } else {
                    line.runs.push_back({ plain, curR, curG, curB, curA, curBold, curItalic });
                }
            }
            break;
        }

        if (tagStart > pos) {
            std::string plain = text.substr(pos, tagStart - pos);
            if (!line.runs.empty() &&
                line.runs.back().r == curR &&
                line.runs.back().g == curG &&
                line.runs.back().b == curB &&
                line.runs.back().a == curA &&
                line.runs.back().bold == curBold &&
                line.runs.back().italic == curItalic) {
                line.runs.back().text += plain;
            } else {
                line.runs.push_back({ plain, curR, curG, curB, curA, curBold, curItalic });
            }
        }

        size_t tagEnd = text.find('>', tagStart);
        if (tagEnd == std::string::npos) break;
        std::string tag = text.substr(tagStart + 1, tagEnd - tagStart - 1);
        pos = tagEnd + 1;

        if (tag == "clr") {
            if (!colorStack.empty()) {
                auto& prev = colorStack.back();
                curR = std::get<0>(prev);
                curG = std::get<1>(prev);
                curB = std::get<2>(prev);
                curA = std::get<3>(prev);
                colorStack.pop_back();
            } else {
                curR = g_Config.text_r;
                curG = g_Config.text_g;
                curB = g_Config.text_b;
                curA = g_Config.text_a;
            }
        } else if (tag == "I" || tag == "i") {
            curItalic = !curItalic;
        } else if (tag == "B" || tag == "b") {
            curBold = !curBold;
        } else if (tag.size() > 4 && tag.substr(0, 4) == "clr:") {
            colorStack.push_back({ curR, curG, curB, curA });
            uint8_t nr, ng, nb;
            if (ParseClrTag(tag, nr, ng, nb)) {
                curR = nr; curG = ng; curB = nb;
            } else {
                colorStack.pop_back();
            }
        } else if (tag == "playerclr") {
            if (!colorStack.empty()) {
                auto& prev = colorStack.back();
                curR = std::get<0>(prev);
                curG = std::get<1>(prev);
                curB = std::get<2>(prev);
                curA = std::get<3>(prev);
                colorStack.pop_back();
            } else {
                curR = g_Config.text_r;
                curG = g_Config.text_g;
                curB = g_Config.text_b;
                curA = g_Config.text_a;
            }
        } else if (tag.size() > 10 && tag.substr(0, 10) == "playerclr:") {
            colorStack.push_back({ curR, curG, curB, curA });
            uint8_t nr, ng, nb;
            if (ParsePlayerClrTag(tag, fromPlayer, nr, ng, nb)) {
                curR = nr; curG = ng; curB = nb;
            } else {
                colorStack.pop_back();
            }
        }
    }

    if (!line.runs.empty())
        lines.push_back(std::move(line));
}

static void ParsePhraseText(const std::string& text, CaptionPhrase& phrase,
                            CaptionFormatState* state = nullptr, bool fromPlayer = false) {
    uint8_t curR, curG, curB, curA;
    bool curBold, curItalic;
    std::vector<std::tuple<uint8_t,uint8_t,uint8_t,uint8_t>> colorStack;

    if (state) {
        curR = state->r; curG = state->g; curB = state->b; curA = state->a;
        curBold = state->bold; curItalic = state->italic;
        colorStack = state->colorStack;
    } else {
        curR = g_Config.text_r; curG = g_Config.text_g; curB = g_Config.text_b; curA = g_Config.text_a;
        curBold = false; curItalic = false;
    }

    size_t pos = 0;
    while (pos < text.size()) {
        size_t crPos = text.find("<cr>", pos);
        std::string lineText;
        if (crPos == std::string::npos) {
            lineText = text.substr(pos);
            pos = text.size();
        } else {
            lineText = text.substr(pos, crPos - pos);
            pos = crPos + 4;
        }
        ParseLineRuns(lineText, phrase.lines, curR, curG, curB, curA, curBold, curItalic, colorStack, fromPlayer);
        if (crPos == std::string::npos) break;
    }

    if (state) {
        state->r = curR; state->g = curG; state->b = curB; state->a = curA;
        state->bold = curBold; state->italic = curItalic;
        state->colorStack = colorStack;
    }
}

std::vector<CaptionPhrase> ParseCaptionText(const std::string& raw, bool fromPlayer) {
    std::vector<CaptionPhrase> phrases;
    size_t pos = 0;
    float nextDelay = 0.0f;
    std::string buffer;

    while (pos < raw.size()) {
        size_t tagPos = raw.find("<delay:", pos);
        if (tagPos == std::string::npos) {
            buffer += raw.substr(pos);
            if (!buffer.empty()) {
                CaptionPhrase p;
                ParsePhraseText(buffer, p, nullptr, fromPlayer);
                p.delay = nextDelay;
                phrases.push_back(std::move(p));
            }
            break;
        }

        buffer += raw.substr(pos, tagPos - pos);
        if (!buffer.empty()) {
            CaptionPhrase p;
            ParsePhraseText(buffer, p, nullptr, fromPlayer);
            p.delay = nextDelay;
            phrases.push_back(std::move(p));
        }
        buffer.clear();

        pos = tagPos + 7;
        size_t endPos = raw.find('>', pos);
        if (endPos == std::string::npos) break;
        nextDelay = (float)atof(raw.substr(pos, endPos - pos).c_str());
        pos = endPos + 1;
    }

    if (phrases.empty()) {
        CaptionPhrase p;
        p.delay = 0;
        phrases.push_back(std::move(p));
    }

    return phrases;
}

std::vector<CaptionPhrase> ParseCaptionText(const std::string& raw, CaptionFormatState& state, bool fromPlayer) {
    std::vector<CaptionPhrase> phrases;
    size_t pos = 0;
    float nextDelay = 0.0f;
    std::string buffer;

    while (pos < raw.size()) {
        size_t tagPos = raw.find("<delay:", pos);
        if (tagPos == std::string::npos) {
            buffer += raw.substr(pos);
            if (!buffer.empty()) {
                CaptionPhrase p;
                ParsePhraseText(buffer, p, &state, fromPlayer);
                p.delay = nextDelay;
                phrases.push_back(std::move(p));
            }
            break;
        }

        buffer += raw.substr(pos, tagPos - pos);
        if (!buffer.empty()) {
            CaptionPhrase p;
            ParsePhraseText(buffer, p, &state, fromPlayer);
            p.delay = nextDelay;
            phrases.push_back(std::move(p));
        }
        buffer.clear();

        pos = tagPos + 7;
        size_t endPos = raw.find('>', pos);
        if (endPos == std::string::npos) break;
        nextDelay = (float)atof(raw.substr(pos, endPos - pos).c_str());
        pos = endPos + 1;
    }

    if (phrases.empty()) {
        CaptionPhrase p;
        p.delay = 0;
        phrases.push_back(std::move(p));
    }

    return phrases;
}
