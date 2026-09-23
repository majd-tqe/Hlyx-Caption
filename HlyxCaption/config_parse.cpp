/*
 * Hlyx Caption — original project source
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Hlyx Caption Contributors
 */

#include "config_parse.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cerrno>
#include <cmath>
#include <algorithm>
#include <sstream>

namespace {

std::string Trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    return (a == std::string::npos) ? "" : s.substr(a, b - a + 1);
}

bool ToBool(const std::string& s) {
    return s == "1" || s == "true" || s == "yes" || s == "on";
}

uint8_t ParseByte(const std::string& s, uint8_t fallback) {
    errno = 0;
    char* end = nullptr;
    const long value = std::strtol(s.c_str(), &end, 10);
    if (errno == ERANGE || end == s.c_str() || (end && *end != '\0'))
        return fallback;
    if (value < 0) return 0;
    if (value > 255) return 255;
    return (uint8_t)value;
}

void SanitizeConfig(OverlayConfig& cfg) {
    const OverlayConfig defaults{};
    auto finiteOr = [](float value, float fallback) {
        return std::isfinite(value) ? value : fallback;
    };
    auto bounded = [&](float& value, float fallback, float minValue, float maxValue) {
        value = finiteOr(value, fallback);
        if (value < minValue) value = minValue;
        if (value > maxValue) value = maxValue;
    };

    bounded(cfg.shadow_offset_x, defaults.shadow_offset_x, -1000.0f, 1000.0f);
    bounded(cfg.shadow_offset_y, defaults.shadow_offset_y, -1000.0f, 1000.0f);
    bounded(cfg.outline_thickness, defaults.outline_thickness, 0.0f, 32.0f);
    bounded(cfg.pos_x, defaults.pos_x, 0.0f, 1.0f);
    bounded(cfg.pos_y, defaults.pos_y, 0.0f, 1.0f);
    bounded(cfg.fade_in_time, defaults.fade_in_time, 0.01f, 10.0f);
    bounded(cfg.fade_out_time, defaults.fade_out_time, 0.01f, 10.0f);
    bounded(cfg.extra_display_time, defaults.extra_display_time, 0.0f, 30.0f);
    bounded(cfg.font_size, defaults.font_size, 4.0f, 256.0f);
    bounded(cfg.font_size_reference_height, defaults.font_size_reference_height, 240.0f, 10000.0f);
    bounded(cfg.line_spacing, defaults.line_spacing, 0.1f, 5.0f);
    bounded(cfg.max_line_width_percent, defaults.max_line_width_percent, 0.05f, 1.0f);
    bounded(cfg.mouse_sensitivity, defaults.mouse_sensitivity, 0.0f, 20.0f);
    bounded(cfg.mouse_accel_factor, defaults.mouse_accel_factor, 0.0f, 20.0f);
    bounded(cfg.bg_padding_x, defaults.bg_padding_x, 0.0f, 200.0f);
    bounded(cfg.bg_padding_y, defaults.bg_padding_y, 0.0f, 200.0f);
    bounded(cfg.bg_border_radius, defaults.bg_border_radius, 0.0f, 200.0f);

    if (cfg.ui_language < 1 || cfg.ui_language > 2) cfg.ui_language = defaults.ui_language;
    if (cfg.ui_direction_override < 0 || cfg.ui_direction_override > 2)
        cfg.ui_direction_override = defaults.ui_direction_override;
    if (cfg.text_alignment < OverlayConfig::ALIGN_LEFT ||
        cfg.text_alignment > OverlayConfig::ALIGN_RIGHT)
        cfg.text_alignment = defaults.text_alignment;

    // Paths are later resolved relative to resources. Reject control characters
    // and unreasonably long values before they reach file APIs or JavaScript.
    auto sanitizePath = [](std::string& path, const std::string& fallback) {
        if (path.size() > 260 ||
            std::any_of(path.begin(), path.end(), [](unsigned char c) { return c < 0x20; })) {
            path = fallback;
        }
    };
    sanitizePath(cfg.custom_font_path, defaults.custom_font_path);
    sanitizePath(cfg.fallback_font_path, defaults.fallback_font_path);
}

// Append one Unicode scalar value (already computed) as UTF-8.
void AppendCodePoint(std::string& out, uint32_t cp) {
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

// Decode a UTF-16 byte stream (BOM already stripped) to UTF-8.
std::string DecodeUtf16(const char* data, size_t len, bool bigEndian) {
    std::string out;
    size_t i = 0;
    while (i + 1 < len) {
        uint16_t w0 = (uint16_t)(uint8_t)data[i] | ((uint16_t)(uint8_t)data[i + 1] << 8);
        i += 2;
        if (bigEndian) w0 = (uint16_t)((w0 << 8) | (w0 >> 8));

        uint32_t cp = w0;
        if (w0 >= 0xD800 && w0 <= 0xDBFF) {
            // High surrogate: expect a low surrogate next.
            if (i + 1 < len) {
                uint16_t w1 = (uint16_t)(uint8_t)data[i] | ((uint16_t)(uint8_t)data[i + 1] << 8);
                i += 2;
                if (bigEndian) w1 = (uint16_t)((w1 << 8) | (w1 >> 8));
                if (w1 >= 0xDC00 && w1 <= 0xDFFF)
                    cp = 0x10000 + (((uint32_t)(w0 - 0xD800)) << 10) + (w1 - 0xDC00);
                else
                    cp = 0xFFFD; // lone high surrogate
            } else {
                cp = 0xFFFD;
            }
        } else if (w0 >= 0xDC00 && w0 <= 0xDFFF) {
            cp = 0xFFFD; // lone low surrogate
        }
        AppendCodePoint(out, cp);
    }
    return out;
}

}  // namespace

void SanitizeOverlayConfig(OverlayConfig& cfg) {
    SanitizeConfig(cfg);
}

bool ReadFileToUtf8(const wchar_t* path, std::string& outUtf8) {
    if (!path || !path[0]) return false;

    FILE* f = nullptr;
    if (_wfopen_s(&f, path, L"rb") != 0 || !f) return false;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) {
        fclose(f);
        return false;
    }
    // A settings file is tiny by design. Refuse unbounded allocations from a
    // corrupted or malicious file before constructing the std::string below.
    constexpr long kMaxConfigBytes = 1024 * 1024;
    if (sz > kMaxConfigBytes) {
        fclose(f);
        return false;
    }

    std::string bytes((size_t)sz, '\0');
    if (sz > 0) {
        size_t got = fread(&bytes[0], 1, (size_t)sz, f);
        fclose(f);
        if (got != (size_t)sz) return false;
    } else {
        fclose(f);
    }

    // Strip UTF-8 BOM.
    if (bytes.size() >= 3 && (uint8_t)bytes[0] == 0xEF &&
        (uint8_t)bytes[1] == 0xBB && (uint8_t)bytes[2] == 0xBF) {
        outUtf8 = bytes.substr(3);
        return true;
    }

    // UTF-16 BOMs.
    if (bytes.size() >= 2) {
        uint8_t b0 = (uint8_t)bytes[0];
        uint8_t b1 = (uint8_t)bytes[1];
        if (b0 == 0xFF && b1 == 0xFE) {  // UTF-16LE (BOM)
            outUtf8 = DecodeUtf16(bytes.data() + 2, bytes.size() - 2, false);
            return true;
        }
        if (b0 == 0xFE && b1 == 0xFF) {  // UTF-16BE (BOM)
            outUtf8 = DecodeUtf16(bytes.data() + 2, bytes.size() - 2, true);
            return true;
        }
    }

    // No BOM: assume UTF-8/ANSI text (existing behavior).
    outUtf8 = bytes;
    return true;
}

int ParseConfigText(const std::string& text, OverlayConfig& cfg,
                    const std::string& modDirUtf8) {
    // {ModDir} is the DLL directory, e.g. "C:\...\bin\win64\". Strip any
    // trailing separator so "{ModDir}\Cairo.ttf" expands to a clean single
    // backslash instead of a double one.
    std::string modDir = modDirUtf8;
    while (!modDir.empty() && (modDir.back() == '\\' || modDir.back() == '/'))
        modDir.pop_back();

    int loaded = 0;
    std::istringstream file(text);
    std::string line;
    while (std::getline(file, line)) {
        size_t hash = line.find('#');
        if (hash != std::string::npos) line = line.substr(0, hash);
        size_t semi = line.find(';');
        if (semi != std::string::npos) line = line.substr(0, semi);
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = Trim(line.substr(0, eq));
        std::string val = Trim(line.substr(eq + 1));

        // --- Text Color ---
        if (key == "text_r") cfg.text_r = ParseByte(val, cfg.text_r);
        else if (key == "text_g") cfg.text_g = ParseByte(val, cfg.text_g);
        else if (key == "text_b") cfg.text_b = ParseByte(val, cfg.text_b);
        else if (key == "text_a") cfg.text_a = ParseByte(val, cfg.text_a);

        // --- Shadow ---
        else if (key == "shadow_enabled") cfg.shadow_enabled = ToBool(val);
        else if (key == "shadow_r") cfg.shadow_r = ParseByte(val, cfg.shadow_r);
        else if (key == "shadow_g") cfg.shadow_g = ParseByte(val, cfg.shadow_g);
        else if (key == "shadow_b") cfg.shadow_b = ParseByte(val, cfg.shadow_b);
        else if (key == "shadow_a") cfg.shadow_a = ParseByte(val, cfg.shadow_a);
        else if (key == "shadow_offset_x") cfg.shadow_offset_x = (float)atof(val.c_str());
        else if (key == "shadow_offset_y") cfg.shadow_offset_y = (float)atof(val.c_str());

        // --- Outline ---
        else if (key == "outline_enabled") cfg.outline_enabled = ToBool(val);
        else if (key == "outline_r") cfg.outline_r = ParseByte(val, cfg.outline_r);
        else if (key == "outline_g") cfg.outline_g = ParseByte(val, cfg.outline_g);
        else if (key == "outline_b") cfg.outline_b = ParseByte(val, cfg.outline_b);
        else if (key == "outline_a") cfg.outline_a = ParseByte(val, cfg.outline_a);
        else if (key == "outline_thickness") cfg.outline_thickness = (float)atof(val.c_str());

        // --- Position & Alignment ---
        else if (key == "pos_x") cfg.pos_x = (float)atof(val.c_str());
        else if (key == "pos_y") cfg.pos_y = (float)atof(val.c_str());
        else if (key == "text_alignment") {
            std::string lower = val;
            for (auto& c : lower) c = (char)tolower((unsigned char)c);
            if (lower == "left") cfg.text_alignment = OverlayConfig::ALIGN_LEFT;
            else if (lower == "right") cfg.text_alignment = OverlayConfig::ALIGN_RIGHT;
            else cfg.text_alignment = OverlayConfig::ALIGN_CENTER;
        }

        // --- Timing ---
        else if (key == "fade_in_time") cfg.fade_in_time = (float)atof(val.c_str());
        else if (key == "fade_out_time") cfg.fade_out_time = (float)atof(val.c_str());
        else if (key == "extra_display_time") cfg.extra_display_time = (float)atof(val.c_str());

        // --- Typography ---
        else if (key == "font_size") cfg.font_size = (float)atof(val.c_str());
        else if (key == "font_size_reference_height")
            cfg.font_size_reference_height = (float)atof(val.c_str());
        else if (key == "line_spacing") cfg.line_spacing = (float)atof(val.c_str());
        else if (key == "custom_font_path") {
            size_t p = val.find("{ModDir}");
            if (p != std::string::npos) val.replace(p, 8, modDir);
            cfg.custom_font_path = val;
        } else if (key == "fallback_font_path") {
            size_t p = val.find("{ModDir}");
            if (p != std::string::npos) val.replace(p, 8, modDir);
            cfg.fallback_font_path = val;
        }

        // --- Word Wrap ---
        else if (key == "max_line_width_percent") cfg.max_line_width_percent = (float)atof(val.c_str());

        // --- Mouse ---
        else if (key == "mouse_sensitivity") cfg.mouse_sensitivity = (float)atof(val.c_str());
        else if (key == "mouse_acceleration") cfg.mouse_acceleration = ToBool(val);
        else if (key == "mouse_accel_factor") cfg.mouse_accel_factor = (float)atof(val.c_str());

        // --- Background Box ---
        else if (key == "background_enabled") cfg.background_enabled = ToBool(val);
        else if (key == "bg_r") cfg.bg_r = ParseByte(val, cfg.bg_r);
        else if (key == "bg_g") cfg.bg_g = ParseByte(val, cfg.bg_g);
        else if (key == "bg_b") cfg.bg_b = ParseByte(val, cfg.bg_b);
        else if (key == "bg_a") cfg.bg_a = ParseByte(val, cfg.bg_a);
        else if (key == "bg_padding_x") cfg.bg_padding_x = (float)atof(val.c_str());
        else if (key == "bg_padding_y") cfg.bg_padding_y = (float)atof(val.c_str());
        else if (key == "bg_border_radius") cfg.bg_border_radius = (float)atof(val.c_str());

        // --- UI ---
        else if (key == "ui_language") cfg.ui_language = atoi(val.c_str());
        else if (key == "ui_direction_override") cfg.ui_direction_override = atoi(val.c_str());
        else if (key == "ui_scale") cfg.ui_scale = (float)atof(val.c_str());
        else if (key == "ui_animations") cfg.ui_animations = ToBool(val);
        else
            continue;  // unknown key — not counted

        loaded++;
    }
    SanitizeConfig(cfg);
    return loaded;
}
