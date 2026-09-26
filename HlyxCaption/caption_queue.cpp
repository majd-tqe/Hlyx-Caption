/*
 * Hlyx Caption — original project source
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Hlyx Caption Contributors
 */

#include "renderer.h"
#include "config.h"
#include "caption_parser.h"
#include "imgui.h"
#include <algorithm>
#ifdef _DEBUG
#include <iostream>
#endif
#include <cmath>
#include <string>
#include <vector>
#include <cctype>

static int CalcActivePhraseSignature(CaptionEntry& entry) {
    if (entry.phrases.empty()) return 0;
    double now = GetTimeQPC();
    float elapsed = (float)(now - entry.startQPC);
    // Hash the complete active-state vector instead of packing only the first
    // 16 phrases into an int. Long close-caption streams can contain more
    // delayed phrases than that.
    uint32_t sig = 2166136261u;
    for (int i = 0; i < (int)entry.phrases.size(); i++) {
        auto& phrase = entry.phrases[i];
        bool active = elapsed >= phrase.delay &&
            (phrase.duration <= 0 || elapsed < phrase.delay + phrase.duration);
        sig ^= active ? 1u : 0u;
        sig *= 16777619u;
    }
    return (int)(sig ^ (sig >> 16));
}

int Renderer::CalcActivePhraseCount(CaptionEntry& entry) {
    if (entry.phrases.empty()) return 0;
    double now = GetTimeQPC();
    float elapsed = (float)(now - entry.startQPC);
    int count = 0;
    for (auto& phrase : entry.phrases) {
        if (elapsed < phrase.delay) break;
        if (phrase.duration > 0 && elapsed >= phrase.delay + phrase.duration)
            continue;
        count++;
    }
    return count;
}

static size_t CountUtf8CodePoints(const char* data, size_t length) {
    size_t count = 0;
    for (size_t i = 0; i < length; ++i) {
        const unsigned char c = (unsigned char)data[i];
        if ((c & 0xC0) != 0x80) ++count;
    }
    return count;
}

static size_t GetVisibleCharCount(const std::string& part) {
    size_t count = 0;
    size_t pos = 0;
    while (pos < part.size()) {
        size_t tagStart = part.find('<', pos);
        if (tagStart == std::string::npos) {
            count += CountUtf8CodePoints(part.data() + pos, part.size() - pos);
            break;
        }
        count += CountUtf8CodePoints(part.data() + pos, tagStart - pos);
        size_t tagEnd = part.find('>', tagStart);
        if (tagEnd == std::string::npos)
            break;
        pos = tagEnd + 1;
    }
    return count;
}

static double GetScreenWidthSafe() {
    if (Renderer::m_ImguiInitialized) {
        const float width = ImGui::GetIO().DisplaySize.x;
        if (width > 0.0f) return width;
    }
    return (double)GetSystemMetrics(SM_CXSCREEN);
}

static double GetScreenHeightSafe() {
    if (Renderer::m_ImguiInitialized) {
        const float height = ImGui::GetIO().DisplaySize.y;
        if (height > 0.0f) return height;
    }
    return (double)GetSystemMetrics(SM_CYSCREEN);
}

#ifdef _DEBUG
static void SbTimeLogicSelfTest() {
    std::vector<std::string> parts = {
        "<clr:255,0,0>Hey there",
        "This is a long sentence that takes much more screen time than the short part"
    };
    const float duration = 10.0f;
    size_t total = 0;
    for (const auto& p : parts)
        total += GetVisibleCharCount(p);
    std::cout << "[SbTest] totalVisibleChars=" << total << " | duration=" << duration << "s\n";
    std::vector<float> partDurations;
    float cumulativeTime = 0.0f;
    for (size_t i = 0; i < parts.size(); i++) {
        size_t chars = GetVisibleCharCount(parts[i]);
        float partDuration = (total > 0) ? ((float)chars / (float)total) * duration
                                         : duration / (float)parts.size();
        partDurations.push_back(partDuration);
        std::cout << "[SbTest] part" << (i + 1)
                  << " chars=" << chars
                  << " partDuration=" << partDuration << "s"
                  << " startOffset=" << cumulativeTime << "s\n";
        cumulativeTime += partDuration;
    }
    bool pass = false;
    if (partDurations.size() == 2)
        pass = partDurations[1] > partDurations[0] * 3.0f;
    std::cout << (pass ? "[SbTest] PASS: long part gets clearly more time than short part\n"
                       : "[SbTest] FAIL: distribution looks wrong\n");
}
#endif

void Renderer::UpdateQueue(float dt) {
    double now = GetTimeQPC();

    // Global live font-path check: if any visible entry has stale font path, reload shaper once without clearing queue
    {
        bool needFontReload = false;
        for (auto& e : m_Queue) {
            if (e.expired) continue;
            if (g_Config.custom_font_path != e.renderedCustomFontPath || g_Config.fallback_font_path != e.renderedFallbackFontPath) {
                needFontReload = true;
                break;
            }
        }
        if (needFontReload) {
            Renderer::ReloadFontPreserveQueue();
        }
    }

    // Update each entry's timer, expire logic
    for (auto& e : m_Queue) {
        if (e.expired) continue;

        float elapsed = (float)(now - e.startQPC);
        if (elapsed >= e.duration) {
            e.expired = true;
            continue;
        }

        int newSig = CalcActivePhraseSignature(e);
        float curFontSize = GetScaledFontSize(g_Config.font_size);
        bool fontChanged = (fabs(curFontSize - e.renderedFontSize) > 0.5f);
        bool fontPathChanged = (g_Config.custom_font_path != e.renderedCustomFontPath || g_Config.fallback_font_path != e.renderedFallbackFontPath);
        bool spacingChanged = (fabs(g_Config.line_spacing - e.renderedLineSpacing) > 0.001f);
        bool colorChanged = (g_Config.text_r != e.renderedTextR || g_Config.text_g != e.renderedTextG || g_Config.text_b != e.renderedTextB || g_Config.text_a != e.renderedTextA);
        bool wrapChanged = (fabs(g_Config.max_line_width_percent - e.renderedMaxLineWidthPercent) > 0.001f);
        bool alignChanged = (g_Config.text_alignment != e.renderedAlignment);
        bool needsRebuild = (newSig != e.lastActivePhraseSig || fontChanged || fontPathChanged || spacingChanged || colorChanged || wrapChanged || alignChanged);
        if (needsRebuild) {
            // Throttle only spacing changes to 50ms; color / font / phrase
            // changes are always immediate (recolor without reshape when possible)
            bool isThrottledVisual = (newSig == e.lastActivePhraseSig && !colorChanged &&
                !fontChanged && !fontPathChanged && spacingChanged && !wrapChanged && !alignChanged);
            if (!(isThrottledVisual && e.lastRebuildQPC > 0.0 && (now - e.lastRebuildQPC) < 0.05)) {
                if (colorChanged) {
                    for (auto& phrase : e.phrases) {
                        for (auto& line : phrase.lines) {
                            for (auto& run : line.runs) {
                                run.r = g_Config.text_r;
                                run.g = g_Config.text_g;
                                run.b = g_Config.text_b;
                                run.a = g_Config.text_a;
                            }
                        }
                    }
                }
                e.lastActivePhraseCount = CalcActivePhraseCount(e);
                e.lastActivePhraseSig = newSig;
                double screenW = GetScreenWidthSafe();
                RenderEntryTexture(e, (int)(screenW * g_Config.max_line_width_percent));
                e.renderedFontSize = curFontSize;
                e.renderedLineSpacing = g_Config.line_spacing;
                e.renderedTextR = g_Config.text_r;
                e.renderedTextG = g_Config.text_g;
                e.renderedTextB = g_Config.text_b;
                e.renderedTextA = g_Config.text_a;
                e.renderedMaxLineWidthPercent = g_Config.max_line_width_percent;
                e.renderedAlignment = g_Config.text_alignment;
                e.renderedCustomFontPath = g_Config.custom_font_path;
                e.renderedFallbackFontPath = g_Config.fallback_font_path;
                e.lastRebuildQPC = now;
            }
        }
    }

    // Handle fade-out for expired entries
    for (auto& e : m_Queue) {
        if (!e.expired) continue;
        if (e.opacity <= 0.0f) continue;

        e.opacity -= dt / g_Config.fade_out_time;
        if (e.opacity < 0.0f) e.opacity = 0.0f;

        if (e.opacity <= 0.0f) {
            e.ReleaseTexture();
        }
    }

    // Fade-in for delayed entries (opacity < 1.0 and not expired)
    for (auto& e : m_Queue) {
        if (e.expired) continue;
        // <sb> parts sit in the queue with a future startQPC; fading them in
        // before their window starts would leave them fully opaque at pop-in.
        if (now < e.startQPC) continue;
        if (e.isSfx) {
            e.opacity = 1.0f;
            continue;
        }
        if (e.opacity >= 1.0f) continue;

        e.opacity += dt / g_Config.fade_in_time;
        if (e.opacity > 1.0f) e.opacity = 1.0f;
    }

    // Remove fully finished entries
    m_Queue.erase(
        std::remove_if(m_Queue.begin(), m_Queue.end(),
            [](const CaptionEntry& e) { return e.expired && e.opacity <= 0.0f; }),
        m_Queue.end()
    );

    // Recalculate targetY for all entries so Y is always correct
    {
        double screenH = GetScreenHeightSafe();
        double scaledFontSize = GetScaledFontSize(g_Config.font_size);
        double anchorY = g_Config.pos_y * screenH - (scaledFontSize * 1.3);
        double yPos = anchorY;
        for (auto& e : m_Queue) {
            if (e.expired) {
                yPos += e.pixelHeight + 4.0;
                continue;
            }
            e.targetY = yPos;
            yPos += e.pixelHeight + 4.0;
        }
    }

    // Lerp visualY toward targetY
    for (auto& e : m_Queue) {
        if (e.expired) continue;
        double diff = e.targetY - e.visualY;
        if (fabs(diff) > 0.5) {
            e.visualY += diff * dt * m_AnimSpeed;
        } else {
            e.visualY = e.targetY;
        }
    }
}

void Renderer::SetCaptionText(const std::string& text, float duration, bool fromPlayer) {
    if (m_ShuttingDown.load(std::memory_order_acquire)) return;
    EnterCriticalSection(&m_CS);
    if (m_ShuttingDown.load(std::memory_order_acquire)) {
        LeaveCriticalSection(&m_CS);
        return;
    }

#ifdef _DEBUG
    static bool sbSelfTestDone = false;
    if (!sbSelfTestDone) {
        sbSelfTestDone = true;
        SbTimeLogicSelfTest();
    }
#endif

    std::vector<std::string> sbParts;
    size_t pos = 0;
    size_t sbPos;
    while ((sbPos = text.find("<sb>", pos)) != std::string::npos) {
        sbParts.push_back(text.substr(pos, sbPos - pos));
        pos = sbPos + 4;
    }
    sbParts.push_back(text.substr(pos));

    if (sbParts.size() == 1) {
        CaptionEntry entry;
        entry.fromPlayer = fromPlayer;
        entry.opacity = 0.0f;
        entry.isSfx = text.find("<sfx>") != std::string::npos;
        entry.phrases = ParseCaptionText(text, fromPlayer);
        entry.duration = duration + g_Config.extra_display_time;
        entry.startQPC = GetTimeQPC();
        entry.lastQPC = entry.startQPC;

        entry.lastActivePhraseSig = CalcActivePhraseSignature(entry);
        entry.lastActivePhraseCount = CalcActivePhraseCount(entry);
        if (Renderer::m_ImguiInitialized && entry.lastActivePhraseCount > 0) {
            double screenW = GetScreenWidthSafe();
            RenderEntryTexture(entry, (int)(screenW * g_Config.max_line_width_percent));
            entry.renderedFontSize = GetScaledFontSize(g_Config.font_size);
            entry.renderedLineSpacing = g_Config.line_spacing;
            entry.renderedTextR = g_Config.text_r;
            entry.renderedTextG = g_Config.text_g;
            entry.renderedTextB = g_Config.text_b;
            entry.renderedTextA = g_Config.text_a;
            entry.renderedMaxLineWidthPercent = g_Config.max_line_width_percent;
            entry.renderedAlignment = g_Config.text_alignment;
            entry.renderedCustomFontPath = g_Config.custom_font_path;
            entry.renderedFallbackFontPath = g_Config.fallback_font_path;
            entry.lastRebuildQPC = GetTimeQPC();
        } else {
            // Texture not built yet (no active phrase at creation, e.g. <delay>).
            // Snapshot the current config anyway so the activation rebuild doesn't
            // misread zero defaults as a color/settings change and clobber run colors.
            entry.renderedFontSize = GetScaledFontSize(g_Config.font_size);
            entry.renderedLineSpacing = g_Config.line_spacing;
            entry.renderedTextR = g_Config.text_r;
            entry.renderedTextG = g_Config.text_g;
            entry.renderedTextB = g_Config.text_b;
            entry.renderedTextA = g_Config.text_a;
            entry.renderedMaxLineWidthPercent = g_Config.max_line_width_percent;
            entry.renderedAlignment = g_Config.text_alignment;
            entry.renderedCustomFontPath = g_Config.custom_font_path;
            entry.renderedFallbackFontPath = g_Config.fallback_font_path;
        }

        {
            double screenH = GetScreenHeightSafe();
            double scaledFontSize = GetScaledFontSize(g_Config.font_size);
            double bottomY = g_Config.pos_y * screenH - (scaledFontSize * 1.3);
            for (auto& e : m_Queue) {
                if (e.expired && e.opacity <= 0.0f) continue;
                double b = e.targetY + e.pixelHeight + 4.0;
                if (b > bottomY) bottomY = b;
            }
            entry.visualY = bottomY;
            entry.targetY = entry.visualY;
        }

#ifdef _DEBUG
        std::cout << "[Caption] Queue=" << (m_Queue.size() + 1) << " | Duration=" << duration << "s | "
                  << "Text=\"" << text << "\"\n";
#endif

        m_Queue.push_back(std::move(entry));
    } else {
        double baseQPC = GetTimeQPC();
        size_t totalVisibleChars = 0;
        for (const auto& part : sbParts)
            totalVisibleChars += GetVisibleCharCount(part);
        CaptionFormatState fmtState;
        fmtState.r = g_Config.text_r;
        fmtState.g = g_Config.text_g;
        fmtState.b = g_Config.text_b;
        fmtState.a = g_Config.text_a;
        float cumulativeTime = 0.0f;
        for (int i = 0; i < (int)sbParts.size(); i++) {
            float partDuration;
            if (totalVisibleChars > 0) {
                float partChars = (float)GetVisibleCharCount(sbParts[i]);
                partDuration = (partChars / (float)totalVisibleChars) * duration;
            } else {
                partDuration = duration / (float)sbParts.size();
            }
            CaptionEntry entry;
            entry.fromPlayer = fromPlayer;
            entry.opacity = 0.0f;
            entry.isSfx = sbParts[i].find("<sfx>") != std::string::npos;
            entry.phrases = ParseCaptionText(sbParts[i], fmtState, fromPlayer);
            entry.duration = partDuration + g_Config.extra_display_time;
            entry.startQPC = baseQPC + cumulativeTime;
            entry.lastQPC = entry.startQPC;
            entry.lastActivePhraseSig = CalcActivePhraseSignature(entry);
            entry.lastActivePhraseCount = CalcActivePhraseCount(entry);
            if (Renderer::m_ImguiInitialized && entry.lastActivePhraseCount > 0) {
                double screenW = GetScreenWidthSafe();
                RenderEntryTexture(entry, (int)(screenW * g_Config.max_line_width_percent));
                entry.renderedFontSize = GetScaledFontSize(g_Config.font_size);
                entry.renderedLineSpacing = g_Config.line_spacing;
                entry.renderedTextR = g_Config.text_r;
                entry.renderedTextG = g_Config.text_g;
                entry.renderedTextB = g_Config.text_b;
                entry.renderedTextA = g_Config.text_a;
                entry.renderedMaxLineWidthPercent = g_Config.max_line_width_percent;
                entry.renderedAlignment = g_Config.text_alignment;
                entry.renderedCustomFontPath = g_Config.custom_font_path;
                entry.renderedFallbackFontPath = g_Config.fallback_font_path;
                entry.lastRebuildQPC = GetTimeQPC();
            } else {
                // Future <sb> part: texture is built lazily on activation. Snapshot the
                // current config so that rebuild doesn't treat zero defaults as a settings
                // change and force-recolor runs over their parsed <clr>/playerclr colors.
                entry.renderedFontSize = GetScaledFontSize(g_Config.font_size);
                entry.renderedLineSpacing = g_Config.line_spacing;
                entry.renderedTextR = g_Config.text_r;
                entry.renderedTextG = g_Config.text_g;
                entry.renderedTextB = g_Config.text_b;
                entry.renderedTextA = g_Config.text_a;
                entry.renderedMaxLineWidthPercent = g_Config.max_line_width_percent;
                entry.renderedAlignment = g_Config.text_alignment;
                entry.renderedCustomFontPath = g_Config.custom_font_path;
                entry.renderedFallbackFontPath = g_Config.fallback_font_path;
            }
            {
                double screenH = GetScreenHeightSafe();
                double scaledFontSize = GetScaledFontSize(g_Config.font_size);
            double bottomY = g_Config.pos_y * screenH - (scaledFontSize * 1.3);
                for (auto& e : m_Queue) {
                    if (e.expired && e.opacity <= 0.0f) continue;
                    double b = e.targetY + e.pixelHeight + 4.0;
                    if (b > bottomY) bottomY = b;
                }
                entry.visualY = bottomY;
                entry.targetY = entry.visualY;
            }
#ifdef _DEBUG
            std::cout << "[Caption] Part" << (i + 1) << "/" << sbParts.size()
                      << " Queue=" << (m_Queue.size() + 1) << " | Duration=" << duration << "s | "
                      << "Text=\"" << sbParts[i] << "\"\n";
#endif
            m_Queue.push_back(std::move(entry));
            cumulativeTime += partDuration;
        }
    }

    LeaveCriticalSection(&m_CS);
}
void Renderer::ShowPreview(const std::string& text, float duration) {
    if (text.empty()) return;
    ClearPreview();
    size_t before = 0;
    EnterCriticalSection(&m_CS);
    before = m_Queue.size();
    LeaveCriticalSection(&m_CS);
    SetCaptionText(text, duration, false);
    EnterCriticalSection(&m_CS);
    for (size_t i = before; i < m_Queue.size(); ++i) {
        m_Queue[i].isPreview = true;
    }
    LeaveCriticalSection(&m_CS);
}

void Renderer::ClearPreview() {
    EnterCriticalSection(&m_CS);
    for (auto& e : m_Queue) if (e.isPreview) e.ReleaseTexture();
    m_Queue.erase(std::remove_if(m_Queue.begin(), m_Queue.end(),
        [](const CaptionEntry& e) { return e.isPreview; }), m_Queue.end());
    LeaveCriticalSection(&m_CS);
}

bool Renderer::HasPreview() {
    EnterCriticalSection(&m_CS);
    bool has = false;
    for (auto& e : m_Queue) if (e.isPreview && !e.expired && e.opacity > 0.0f) { has = true; break; }
    LeaveCriticalSection(&m_CS);
    return has;
}
