/*
 * Hlyx Caption — original project source
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Hlyx Caption Contributors
 */

#pragma once
#include <stdint.h>
#include <string>

struct OverlayConfig {
    // --- Text Color ---
    uint8_t text_r = 255;
    uint8_t text_g = 255;
    uint8_t text_b = 255;
    uint8_t text_a = 255;

    // --- Shadow ---
    bool shadow_enabled = false;
    uint8_t shadow_r = 0;
    uint8_t shadow_g = 0;
    uint8_t shadow_b = 0;
    uint8_t shadow_a = 128;
    float shadow_offset_x = 4.0f;
    float shadow_offset_y = 4.0f;

    // --- Outline ---
    bool outline_enabled = true;
    uint8_t outline_r = 0;
    uint8_t outline_g = 0;
    uint8_t outline_b = 0;
    uint8_t outline_a = 128;
    float outline_thickness = 3.0f;

    // --- Position & Alignment ---
    float pos_x = 0.50f;
    float pos_y = 0.80f;
    enum Alignment { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT };
    Alignment text_alignment = ALIGN_CENTER;

    // --- Timing ---
    float fade_in_time = 0.20f;
    float fade_out_time = 0.50f;
    float extra_display_time = 1.0f; // Extra seconds added to original caption duration

    // --- Typography ---
    float font_size = 34.0f;
    float font_size_reference_height = 1080.0f; // Screen height at which font_size is exact
    float line_spacing = 1.3f;
    std::string custom_font_path = "Cairo-Regular.ttf"; // OFL font in {ModDir}/resources; resolved at load
    std::string fallback_font_path = "Cairo-Regular.ttf"; // Same resolution; used by FriBidi for RTL runs

    // --- Word Wrap ---
    float max_line_width_percent = 0.75f;  // 75% of screen width before wrap

    // --- Mouse ---
    float mouse_sensitivity = 1.0f;  // Multiplier for software cursor speed
    bool mouse_acceleration = false; // Enable mouse acceleration
    float mouse_accel_factor = 2.0f; // Acceleration multiplier (how much speed affects distance)

    // --- Background Box ---
    bool background_enabled = false;
    uint8_t bg_r = 0;
    uint8_t bg_g = 0;
    uint8_t bg_b = 0;
    uint8_t bg_a = 150;
    float bg_padding_x = 10.0f;
    float bg_padding_y = 5.0f;
    float bg_border_radius = 5.0f;

    // --- UI / Settings Window ---
    int   ui_language = 2;             // 1=EN, 2=AR (default Arabic)
    int   ui_direction_override = 0;   // 0=Auto, 1=LTR, 2=RTL
    float ui_scale = 1.0f;             // 100%-200%
    bool  ui_animations = true;        // enable all UI animations
};

void LoadConfig(const wchar_t* iniPath);
void SaveConfig(const wchar_t* iniPath);
void ResetConfig();

// Path to settings.ini (resolved as {ModDir}\resources\settings.ini in hooks.cpp MainThread).
// Declared here so both the legacy renderer and the Ultralight settings panel
// can persist settings to the exact file LoadConfig() reads at startup.
extern wchar_t g_IniPath[];

// Default-initialize g_Config explicitly to guarantee default member initializers
// are applied even if the linker zero-initializes .bss before C++ static init runs.
inline OverlayConfig g_Config{};
inline const OverlayConfig g_DefaultConfig{};
