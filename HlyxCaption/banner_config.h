/*
 * Hlyx Caption — original project source
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Hlyx Caption Contributors
 */

// banner_config.h
// ============================================================
//  Welcome Banner settings — the single place to edit them
// ------------------------------------------------------------
//  These values are applied at startup by injecting JS into the
//  Ultralight page (see UltralightManager.cpp :: BuildBannerConfigJS),
//  overriding the defaults in settings.css.
//
//  How to tweak: change the numbers here only, then rebuild the
//  project. No CSS edits and no regeneration of embedded_ui.h is
//  ever needed. If injection fails for any reason, the original
//  CSS defaults keep working as a fallback.
// ============================================================
#pragma once

namespace BannerConfig {

    // Reference height — all px values below are defined @1080p.
    // At runtime they are multiplied by (viewportHeight / kReferenceHeight)
    // so the banner keeps the exact same visual ratio on every resolution
    // (720p, 1080p, 1440p, 4K...). No upper clamp — fully adaptive.
    inline constexpr float kReferenceHeight = 1080.0f;

    // ------------------ Position (% of screen dimensions) ------------------
    inline constexpr float kTopPct  = 8.0f;    // banner distance from top of screen
    inline constexpr float kLeftPct = 50.0f;   // 50 = horizontally centered

    // ------------------ Size (pixels @1080p, scaled at runtime) ------------------
    inline constexpr float kLogoMaxPx   = 76.8f;  // max size of the banner logo @1080p
    inline constexpr float kTitleFontPx = 36.0f;  // main title font size @1080p
    inline constexpr float kHintsFontPx = 16.8f;  // hint strip font (F10/F11) @1080p
    inline constexpr float kKbdFontPx   = 14.4f;  // key-cap chips inside hints @1080p
    inline constexpr float kRowGapPx    = 12.0f;  // gap between logo row and hints @1080p

    // ------------------ Duration (seconds) ------------------
    inline constexpr float kFadeInSec   = 0.5f;    // fade in
    inline constexpr float kHoldSec     = 10.0f;    // fully visible hold time
    inline constexpr float kFadeOutSec  = 1.5f;    // final fade-out (rendering stops after)

    // ------------------ Entrance effect (px @1080p, scaled at runtime) ------------------
    inline constexpr float kStartOffsetYpx = -12.0f;  // slide-in offset (negative = drops from above) @1080p

} // namespace BannerConfig
