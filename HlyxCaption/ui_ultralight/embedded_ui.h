// Hlyx Caption original source — GPL-3.0-or-later.
// Copyright (C) 2026 Hlyx Caption Contributors
// Auto-generated header. Do not edit by hand.
// Generated from ui_ultralight/assets/settings.html (settings.css + bindings.js inlined) at compile time.
// Rebuild with: pwsh ui_ultralight\generate_embedded_ui.ps1

#pragma once

#include <cstddef>

namespace hlyx_caption {

// Large string stored as concatenation of raw string chunks
// (MSVC imposes a limit on single raw string literal size).
static const char kEmbeddedHTML[] =
    R"C0(<!-- Hlyx Caption original UI source — SPDX-License-Identifier: GPL-3.0-or-later
     Copyright (C) 2026 Hlyx Caption Contributors -->
<!DOCTYPE html>
<!--
HLA Mod Settings Panel — "Resistance Terminal" redesign.
Ultralight-based HTML/CSS/JS UI.
Theme: HEV hazard-orange on dark, notched cards, reticle wordmark, Arabic/English.
Layout: hazard cap + top bar + header (title/reticle) + scrolling main
        (settings column | divider | tab list) + footer stripe + action bar.
Panel: 36vw of the screen, anchored to the language side, slides in/out.
-->
<html lang="ar" dir="rtl">
<head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title data-i18n="page.title">Hlyx Caption — إعدادات الترجمة</title>
    <style>
/* Hlyx Caption original UI source — SPDX-License-Identifier: GPL-3.0-or-later
   Copyright (C) 2026 Hlyx Caption Contributors
   =========================================================== */
/* ===========================================================
   settings.css - HLA Mod Settings Panel
   Theme: "Resistance Terminal" — HEV hazard-orange on dark,
   notched HUD cards, reticle wordmark, Arabic/English.
   Panel: 36vw of the screen, anchored to the language side, slides in/out.
   =========================================================== */

:root {
    /* Base surfaces */
    --bg:        #07090b;
    --app-bg:    #0d1013;
    --panel-bg:  #12161a;
    --border:        #242b2c;
    --border-soft:   #1a1f21;
    --text:      #eef1f2;
    --text-dim:  #8a9394;
    --text-faint:#333c3d;

    /* HEV hazard orange */
    --hazard:        #e2812f;
    --hazard-bright: #f39a49;
    --hazard-dim:    #7a4a20;
    --hazard-glow:   rgba(226, 129, 47, 0.35);

    /* HEV teal + rose */
    --hev:        #4bb9a8;
    --hev-bright: #6fd8c4;
    --rose:       #d6636d;

    /* Fonts (offline-safe stacks; Cairo/Oxanium/JetBrains used if installed) */
    --mono:   "JetBrains Mono", "Consolas", "Courier New", monospace;
    --tech:   "Oxanium", "Segoe UI", Tahoma, sans-serif;
    --arabic: "Cairo", "Segoe UI", Tahoma, sans-serif;

    --transition: 160ms cubic-bezier(0.22, 1, 0.36, 1);
}

/* ============== Font self-hosting (no Google Fonts in-game) ==============
   Bundled files in assets/fonts/:
     - Cairo-Latin.woff2      (covers Latin + Latin-Ext + Vietnamese subsets)
     - Cairo-Arabic.woff2     (covers the Arabic subset — used for the panel text)
     - JetBrainsMono-Latin.woff2
     - Oxanium-Latin.woff2
   Each file is a variable font, so a single file covers all weights (400/600/700/900).
   If the file is missing the @font-face fails silently and we fall back to the
   system stack declared on :root (Tahoma on Windows). */
@font-face {
    font-family: "Cairo";
    font-style: normal;
    font-weight: 100 900;
    font-display: swap;
    src: url("fonts/Cairo-Latin.woff2") format("woff2");
    unicode-range: U+0000-00FF, U+0131, U+0152-0153, U+02BB-02BC, U+02C6, U+02DA, U+02DC,
                   U+0304, U+0308, U+0329, U+2000-206F, U+20AC, U+2122, U+2191, U+2193,
                   U+2212, U+2215, U+FEFF, U+FFFD,
                   U+0100-02BA, U+02BD-02C5, U+02C7-02CC, U+02CE-02D7, U+02DD-02FF,
                   U+0304, U+0308, U+0329, U+1D00-1DBF, U+1E00-1E9F, U+1EF2-1EFF,
                   U+2020, U+20A0-20AB, U+20AD-20C0, U+2113, U+2C60-2C7F, U+A720-A7FF;
}
@font-face {
    font-family: "Cairo";
    font-style: normal;
    font-weight: 100 900;
    font-display: swap;
    src: url("fonts/Cairo-Arabic.woff2") format("woff2");
    unicode-range: U+0600-06FF, U+0750-077F, U+0870-088E, U+0890-0891, U+0897-08E1,
                   U+08E3-08FF, U+200C-200E, U+2010-2011, U+204F, U+2E41, U+FB50-FDFF,
                   U+FE70-FE74, U+FE76-FEFC, U+102E0-102FB, U+10E60-10E7E, U+10EC2-10EC4,
                   U+10EFC-10EFF, U+1EE00-1EE03, U+1EE05-1EE1F, U+1EE21-1EE22, U+1EE24,
                   U+1EE27, U+1EE29-1EE32, U+1EE34-1EE37, U+1EE39, U+1EE3B, U+1EE42,
                   U+1EE47, U+1EE49, U+1EE4B, U+1EE4D-1EE4F, U+1EE51-1EE52, U+1EE54,
                   U+1EE57, U+1EE59, U+1EE5B, U+1EE5D, U+1EE5F, U+1EE61-1EE62, U+1EE64,
                   U+1EE67-1EE6A, U+1EE6C-1EE72, U+1EE74-1EE77, U+1EE79-1EE7C, U+1EE7E,
                   U+1EE80-1EE89, U+1EE8B-1EE9B, U+1EEA1-1EEA3, U+1EEA5-1EEA9,
                   U+1EEAB-1EEBB, U+1EEF0-1EEF1;
}
@font-face {
    font-family: "JetBrains Mono";
    font-style: normal;
    font-weight: 100 900;
    font-display: swap;
    src: url("fonts/JetBrainsMono-Latin.woff2") format("woff2");
    unicode-range: U+0000-00FF, U+0131, U+0152-0153, U+02BB-02BC, U+02C6, U+02DA, U+02DC,
                   U+0304, U+0308, U+0329, U+2000-206F, U+20AC, U+2122, U+2191, U+2193,
                   U+2212, U+2215, U+FEFF, U+FFFD,
                   U+0100-02BA, U+02BD-02C5, U+02C7-02CC, U+02CE-02D7, U+02DD-02FF,
                   U+0304, U+0308, U+0329, U+1D00-1DBF, U+1E00-1E9F, U+1EF2-1EFF,
                   U+2020, U+20A0-20AB, U+20AD-20C0, U+2113, U+2C60-2C7F, U+A720-A7FF;
}
@font-face {
    font-family: "Oxanium";
    font-style: normal;
    font-weight: 100 900;
    font-display: swap;
    src: url("fonts/Oxanium-Latin.woff2") format("woff2");
    unicode-range: U+0000-00FF, U+0131, U+0152-0153, U+02BB-02BC, U+02C6, U+02DA, U+02DC,
                   U+0304, U+0308, U+0329, U+2000-206F, U+20AC, U+2122, U+2191, U+2193,
                   U+2212, U+2215, U+FEFF, U+FFFD,
                   U+0100-02BA, U+02BD-02C5, U+02C7-02CC, U+02CE-02D7, U+02DD-02FF,
                   U+0304, U+0308, U+0329, U+1D00-1DBF, U+1E00-1E9F, U+1EF2-1EFF,
                   U+2020, U+20A0-20AB, U+20AD-20C0, U+2113, U+2C60-2C7F, U+A720-A7FF;
}

/* ============== Reset & Base ============== */

*, *::before, *::after { box-sizing: border-box; }

html, body {
    margin: 0;
    padding: 0;
    height: 100%;
    overflow: hidden;
    font-family: var(--arabic);
    font-size: 13px;
    line-height: 1.5;
    color: var(--text);
    /* Page itself is transparent — only the anchored .app panel
       paints over the game behind it. The panel's bg-color is set on
       .app so it stays fully opaque regardless of any gradient layer. */
    background: transparent;
    user-select: none;
    -webkit-user-select: none;
    direction: rtl;
    -webkit-font-smoothing: antialiased;
    min-height: 100vh;
}

/* English uses the same panel geometry but switches content flow and text
   alignment to LTR. The native language value toggles this class at runtime. */
body.ui-ltr { direction: ltr; }
body.ui-ltr .header-title { text-align: left; }
body.ui-ltr .header-sub { text-align: left; }
body.ui-ltr .panel-col {
    direction: ltr;
    text-align: left;
}
body.ui-ltr .text-input-row .slider-label { text-align: left; }
body.ui-ltr .tabs-col {
    direction: ltr;
    text-align: left;
}
body.ui-ltr .tab {
    display: grid;
    grid-template-columns: 6px 20px minmax(0, 1fr);
)C0"
    R"C1(    justify-content: initial;
    column-gap: 10px;
    text-align: left;
}
body.ui-ltr .main {
    grid-template-columns: 170px 1px minmax(0, 1fr);
}
body.ui-ltr .tabs-col {
    grid-column: 1;
    grid-row: 1;
    border-left: none;
    border-right: 1px solid var(--border-soft);
    overflow-x: hidden;
}
body.ui-ltr .divider { grid-column: 2; grid-row: 1; }
body.ui-ltr .panel-col { grid-column: 3; grid-row: 1; }
body.ui-ltr .tabs-col::before {
    left: auto;
    right: -1px;
    border-left: none;
    border-right: 2px solid var(--hazard);
}
body.ui-ltr .tabs-col::after {
    right: auto;
    left: -1px;
    border-right: none;
    border-left: 2px solid var(--hazard);
}
body.ui-ltr .tab {
    border-right: none;
    border-left: 2px solid transparent;
}
body.ui-ltr .tab.active {
    border-right: none;
    border-left: 2px solid var(--hazard);
}
body.ui-ltr #text_alignment .seg-buttons {
    /* The source order is Right, Center, Left for Arabic. */
    flex-direction: row-reverse;
}
body.ui-ltr .slider-info {
    flex-basis: 182px;
    justify-content: space-between;
}
body.ui-ltr .slider-value { text-align: right; }
body.ui-ltr .footer {
    display: grid;
    grid-template-columns: max-content minmax(0, 1fr);
}
body.ui-ltr .footer-center {
    justify-content: flex-end;
}
body.ui-ltr .footer-spacer { display: none; }

/* Subtle blueprint grid on the whole page — only while the settings panel is
   open (body.panel-open toggled by bindings.js). Hidden otherwise so the
   startup banner never shows it over the whole screen. */
body.panel-open::before {
    content: "";
    position: fixed;
    inset: 0;
    background-image:
        repeating-linear-gradient(0deg, rgba(255, 255, 255, 0.025) 0 1px, transparent 1px 48px),
        repeating-linear-gradient(90deg, rgba(255, 255, 255, 0.025) 0 1px, transparent 1px 48px);
    pointer-events: none;
    z-index: 0;
}

button {
    font-family: inherit;
    font-size: inherit;
    color: inherit;
    background: none;
    border: none;
    cursor: pointer;
    outline: none;
}

input, select {
    font-family: inherit;
    font-size: inherit;
    color: inherit;
}

::-webkit-scrollbar { width: 6px; height: 6px; }
::-webkit-scrollbar-thumb { background: rgba(226, 129, 47, 0.25); border-radius: 3px; }
::-webkit-scrollbar-thumb:hover { background: rgba(226, 129, 47, 0.5); }
::-webkit-scrollbar-track { background: transparent; }

/* ============== Startup Banner (top-center) ==============
   Mod branding shown on game start: logo image + F10/F11 hints.
   Fully adaptive: all sizes scale as (viewportHeight / 1080) via
   BannerConfig (banner_config.h) injected by UltralightManager.
   The CSS below is a responsive fallback (before JS injection) using
   viewport units so the shape stays identical on 720p/1080p/4K.
   When animation ends, bindings.js sets window.__bannerDone and the
   native side stops rendering the closed overlay entirely. */
#hud-banner {
    position: fixed;
    top: 8vh; /* overridden by BannerConfig::kTopPct via JS */
    left: 50%;
    transform: translateX(-50%);
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: clamp(8px, 1.1vh, 14px);
    text-align: center;
    pointer-events: none;
    z-index: 1;
    animation: banner-lifecycle 6s ease forwards;
    filter: drop-shadow(0 4px 24px rgba(0, 0, 0, 0.65));
    /* adaptive container — no fixed max-width, shrinks/grows with viewport */
    width: auto;
    max-width: 92vw;
}

/* Logo + title side by side — gap scales with viewport height */
.banner-row {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: clamp(10px, 1.3vh, 16px);
}

#banner-logo {
    /* fallback adaptive sizes — JS will override with exact (76.8*scale) */
    max-width: clamp(48px, 7vh, 96px);
    max-height: clamp(48px, 7vh, 96px);
    width: auto;
    height: auto;
    object-fit: contain;
    /* Match the wiki logo's SVG corner radius (rx=22 on a 100px canvas). */
    border-radius: 22%;
}

.banner-title {
    font-family: var(--arabic);
    /* fallback: ~3.3vh at 1080p = 36px — fully adaptive */
    font-size: clamp(20px, 3.3vh, 48px);
    font-weight: 800;
    line-height: 1.1;
    color: var(--hazard-bright);
    text-shadow:
        0 0 18px rgba(226, 129, 47, 0.45),
        0 2px 10px rgba(0, 0, 0, 0.9);
    white-space: nowrap;
}

.banner-hints {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: clamp(8px, 1.25vh, 16px);
    font-family: var(--arabic);
    font-size: clamp(11px, 1.55vh, 20px);
    font-weight: 600;
    color: var(--text-dim);
    background: rgba(7, 9, 11, 0.72);
    border: 1px solid rgba(226, 129, 47, 0.28);
    border-radius: 999px;
    padding: clamp(4px, 0.55vh, 8px) clamp(12px, 1.65vh, 22px);
    white-space: nowrap;
}

.banner-hints .hint {
    display: inline-flex;
    align-items: center;
    gap: clamp(4px, 0.6vh, 8px);
}

.banner-hints .kbd {
    font-family: var(--mono);
    font-size: clamp(10px, 1.33vh, 17px);
    font-weight: 700;
    color: var(--hazard-bright);
    background: rgba(226, 129, 47, 0.12);
    border: 1px solid rgba(226, 129, 47, 0.45);
    border-bottom-width: 2px;
    border-radius: 5px;
    padding: 1px 8px;
}

.banner-hints .hint-sep {
    color: var(--hazard-dim);
    font-size: clamp(10px, 1.1vh, 14px);
}

#banner-translation {
    color: var(--hev-bright);
}

@keyframes banner-lifecycle {
    0%   { opacity: 0; transform: translate(-50%, -12px); }
    8%   { opacity: 1; transform: translate(-50%, 0); }
    75%  { opacity: 1; }
    100% { opacity: 0; }
}

/* ============== App Panel (36% width, language-side anchored) ============== */

.app {
    position: fixed;
    top: 0;
    right: 0;               /* Arabic is anchored to the RIGHT edge */
    width: 36vw;            /* 36% of the screen width — wider to fit the
                               inline label|value|slider layout from the
                               reference design without cramping */
    max-width: 720px;       /* but never wider than the reference card */
    min-width: 420px;       /* and never narrower than the compact layout */
    height: 100vh;
    display: grid;
    grid-template-rows: 7px auto auto 1fr 4px auto;
    overflow: hidden;
    /* Hidden by default (off-screen to the language-side edge). The page now renders even
       while the overlay is "closed" (startup banner), so without this the
       settings panel would be visible at game start. panelOpen() (F10) slides
       it in via .open-anim; closePanel() slides it back out. */
    transform: translateX(100%);
    /* Solid bg first, then the orange tint overlay. Two separate properties
       so the underlying color is guaranteed opaque (otherwise the radial
       gradient layer can let the game show through). */
    background-color: var(--app-bg);
    background-image:
        radial-gradient(1200px 400px at 80% -10%, rgba(226, 129, 47, 0.10), transparent 60%);
    box-shadow:
)C1"
    R"C2(        -30px 0 90px -30px rgba(0, 0, 0, 0.85),
        -1px 0 0 var(--border);
    z-index: 1;
}

/* Faint scanlines over the whole panel. */
.app::after {
    content: "";
    position: absolute;
    inset: 0;
    background: repeating-linear-gradient(0deg, rgba(255, 255, 255, 0.012) 0 1px, transparent 1px 3px);
    pointer-events: none;
    mix-blend-mode: overlay;
    z-index: 2;
}

/* Glowing accent line on the panel's screen-facing edge. */
.app::before {
    content: "";
    position: absolute;
    top: 0; left: 0;
    width: 2px;
    height: 100%;
    background: linear-gradient(180deg, transparent, var(--hev) 25%, var(--hev) 75%, transparent);
    opacity: 0.55;
    box-shadow: 0 0 14px rgba(75, 185, 168, 0.45);
    pointer-events: none;
    z-index: 3;
}

/* Slide-in from the Arabic side (replayed by JS each time the panel opens). */
.app.open-anim {
    animation: panel-slide-in 320ms cubic-bezier(0.22, 1, 0.36, 1) both;
}

/* Slide-out to the Arabic side (played before the native side hides the overlay). */
.app.closing {
    animation: none;
    transition: transform 280ms cubic-bezier(0.55, 0, 0.55, 0.2);
    transform: translateX(100%);
}

/* Animations disabled (ui_animations = off in the settings). */
.app.no-anim {
    animation: none !important;
    transition: none !important;
}

@keyframes panel-slide-in {
    from { transform: translateX(100%); }
    to   { transform: translateX(0); }
}

/* English mirrors the panel: it is anchored on the LEFT and enters/exits
   from that edge, while the settings column moves to the RIGHT of the tabs. */
body.ui-ltr .app {
    left: 0;
    right: auto;
    transform: translateX(-100%);
    box-shadow:
        30px 0 90px -30px rgba(0, 0, 0, 0.85),
        1px 0 0 var(--border);
    background-image:
        radial-gradient(1200px 400px at 20% -10%, rgba(226, 129, 47, 0.10), transparent 60%);
}
body.ui-ltr .app::before {
    left: auto;
    right: 0;
}
body.ui-ltr .app.open-anim {
    animation-name: panel-slide-in-left;
}
body.ui-ltr .app.closing {
    transform: translateX(-100%);
}
@keyframes panel-slide-in-left {
    from { transform: translateX(-100%); }
    to   { transform: translateX(0); }
}

/* ============== Hazard Stripe Cap ============== */

.hazard-bar {
    height: 7px;
    background: repeating-linear-gradient(
        -45deg,
        var(--hazard) 0 10px,
        #14100a 10px 20px
    );
    opacity: 0.85;
}

/* ============== Top Bar ============== */

.topbar {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 8px;
    padding: 14px 24px;
    border-bottom: 1px solid var(--border-soft);
    background: linear-gradient(180deg, rgba(255, 255, 255, 0.015), transparent);
    position: relative;
    z-index: 3;
}

.topbar-right { display: flex; align-items: center; gap: 10px; }
.topbar-left  { display: flex; align-items: center; gap: 10px; }

.brand {
    font-family: var(--tech);
    font-weight: 700;
    font-size: 16px;
    letter-spacing: 1.5px;
    color: #dfe3e0;
}

.brand-badge {
    font-family: var(--arabic);
    font-weight: 800;
    font-size: 13px;
    color: #140d05;
    background: linear-gradient(180deg, var(--hazard-bright), var(--hazard));
    padding: 6px 12px;
    letter-spacing: 1px;
    clip-path: polygon(6px 0, 100% 0, 100% calc(100% - 6px), calc(100% - 6px) 100%, 0 100%, 0 6px);
}

/* F-key pill buttons */
.key-btn {
    display: flex;
    align-items: center;
    gap: 8px;
    background: #0f1214;
    border: 1px solid var(--border);
    color: #adb6b3;
    font-family: var(--arabic);
    font-size: 13px;
    padding: 6px 10px 6px 8px;
    cursor: pointer;
    transition: border-color var(--transition), color var(--transition);
    clip-path: polygon(5px 0, 100% 0, 100% calc(100% - 5px), calc(100% - 5px) 100%, 0 100%, 0 5px);
}

.key-btn:hover { border-color: var(--hev); color: #fff; }

/* "on" state (toggled by the تبديل button — cosmetic). */
.key-btn.on {
    border-color: var(--hev);
    color: #fff;
    box-shadow: 0 0 10px rgba(75, 185, 168, 0.25);
}

.key-btn .key {
    font-family: var(--mono);
    font-size: 11px;
    color: var(--hev-bright);
    border: 1px solid #24403c;
    background: #0a1211;
    padding: 2px 6px;
    line-height: 1.4;
}

/* ============== Header / Title ============== */

.header {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 12px;
    padding: 28px 24px 22px;
    border-bottom: 1px solid var(--border-soft);
    position: relative;
    z-index: 3;
}

.header-title { text-align: right; min-width: 0; }

.eyebrow {
    display: inline-flex;
    align-items: center;
    gap: 8px;
    font-family: var(--mono);
    font-size: 11px;
    letter-spacing: 3px;
    color: var(--hev-bright);
    text-transform: uppercase;
    margin-bottom: 10px;
}

.eyebrow::before {
    content: "";
    width: 16px;
    height: 1px;
    background: var(--hev-bright);
}

.header-title h1 {
    margin: 0 0 10px;
    font-size: 29px;
    font-weight: 900;
    color: #f4f1ea;
    letter-spacing: 0.5px;
    text-shadow: 0 0 24px rgba(226, 129, 47, 0.15);
    animation: flicker-in .9s ease-out;
}

@keyframes flicker-in {
    0%   { opacity: 0; }
    8%   { opacity: 1; }
    12%  { opacity: 0.2; }
    20%  { opacity: 1; }
    28%  { opacity: 0.4; }
    36%  { opacity: 1; }
    100% { opacity: 1; }
}

.header-sub {
    margin: 0;
    font-family: var(--mono);
    font-size: 12.5px;
    color: var(--hazard);
    letter-spacing: 0.3px;
    direction: ltr;
    text-align: right;
    unicode-bidi: plaintext;
}

.header-sub b { font-weight: 600; color: var(--hazard-bright); }

.stat-sep { color: var(--text-dim); opacity: 0.6; }

/* Targeting-reticle frame around the wordmark */
.reticle {
    position: relative;
    width: 120px;
    height: 74px;
    display: flex;
    align-items: center;
    justify-content: center;
    flex: 0 0 auto;
}

.reticle .corner {
    position: absolute;
    width: 16px;
    height: 16px;
    border: 2px solid var(--hev);
    opacity: 0.8;
}

.reticle .corner.tl { top: 0; left: 0; border-right: none; border-bottom: none; }
.reticle .corner.tr { top: 0; right: 0; border-left: none; border-bottom: none; }
.reticle .corner.bl { bottom: 0; left: 0; border-right: none; border-top: none; }
.reticle .corner.br { bottom: 0; right: 0; border-left: none; border-top: none; }

.watermark {
    font-family: var(--tech);
    font-weight: 700;
    font-size: 26px;
    line-height: 1.15;
    color: var(--text-faint);
    letter-spacing: 6px;
    user-select: none;
    text-align: center;
}

/* ============== Main (settings column | divider | tabs) ============== */

.main {
    /* LTR grid order like the reference design: settings column first (LEFT),
       tabs column second (RIGHT). Inner columns stay RTL.
       Tabs are narrower than the settings side. */
    direction: ltr;
    display: grid;
    grid-template-columns: minmax(0, 1fr) 1px 170px;
    min-height: 0;
)C2"
    R"C3(    position: relative;
    z-index: 3;
}

.divider { background: var(--border-soft); }

.panel-col {
    direction: rtl;
    text-align: right;
    padding: 24px 24px 32px;
    overflow-y: auto;
    overflow-x: hidden;
    min-width: 0;
    display: flex;
    flex-direction: column;
    /* gap is applied between sections (when more than one is visible);
       the section itself stacks its own cards with a 26px gap. */
    gap: 26px;
    background-color: var(--app-bg);
}

/* Active section switching (driven by bindings.js).
   The section is a flex column so the cards inside get a vertical gap.
   Otherwise the parent .panel-col's gap only applies between sibling
   sections (of which only one is .active at a time), so the cards would
   visually touch each other. */
.panel-col section {
    display: none;
    flex-direction: column;
    gap: 26px;
    animation: section-in 240ms cubic-bezier(0.22, 1, 0.36, 1);
}

.panel-col section.active {
    display: flex;
}

@keyframes section-in {
    from { opacity: 0; transform: translateY(8px); }
    to   { opacity: 1; transform: translateY(0); }
}

/* ============== Notched Cards ============== */

.card.notched {
    position: relative;
    background-color: var(--panel-bg);
    border: 1px solid var(--border);
    padding: 20px 22px;
    /* margin-bottom handled by parent's flex gap */
}

/* HUD corner brackets (top-left / bottom-right) */
.card.notched::before,
.card.notched::after {
    content: "";
    position: absolute;
    width: 14px;
    height: 14px;
    border-color: var(--hazard-dim);
    z-index: 2;
}

.card.notched::before {
    top: -1px; left: -1px;
    border-top: 2px solid var(--hazard);
    border-left: 2px solid var(--hazard);
}

.card.notched::after {
    bottom: -1px; right: -1px;
    border-bottom: 2px solid var(--hazard);
    border-right: 2px solid var(--hazard);
}

.card h2 {
    display: flex;
    align-items: center;
    gap: 8px;
    margin: 0 0 14px;
    font-size: 15.5px;
    font-weight: 700;
    color: #eceef1;
    text-transform: uppercase;
    letter-spacing: 0.5px;
}

.card h2::before {
    content: "»";
    color: var(--hazard);
    font-family: var(--tech);
    font-weight: 700;
}

.card-body { padding: 0; }

/* ============== Sliders ============== */

/* Inline slider-row layout (matches reference design):
   [label] [value] [slider fills remaining space].
   The slider-info wrapper is rendered as a single flex row instead of the
   older stacked layout (label+value above, slider below). */
.slider-row {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 10px 0;
}
.slider-row + .slider-row {
    border-top: 1px solid var(--border-soft);
}
.slider-row:last-child { padding-bottom: 0; }
.slider-row:first-child { padding-top: 0; }

.slider-info {
    display: flex;
    align-items: center;
    gap: 10px;
    flex: 0 0 auto;
    min-width: 0;
}

.slider-label {
    color: #dfe3e0;
    font-size: 14px;
    font-weight: 500;
    white-space: nowrap;
}

.slider-value {
    font-family: var(--mono);
    font-size: 14px;
    font-weight: 600;
    color: var(--hazard-bright);
    min-width: 64px;
    direction: ltr;
    text-align: left;
}

.slider-row > input[type="range"] {
    flex: 1 1 auto;
    min-width: 0;
}

/* Range input — diamond hazard thumb, filled track (painted by JS).
   NOTE: the track pseudo is transparent so the input's own gradient
   background shows through as the fill. */
input[type="range"] {
    -webkit-appearance: none;
    appearance: none;
    width: 100%;
    height: 3px;
    background: linear-gradient(90deg, var(--hazard) 0%, var(--hazard-bright) 50%, #262d2e 50%, #262d2e 100%);
    border: none;
    border-radius: 0;
    outline: none;
    cursor: pointer;
    margin: 0;
    padding: 0;
    direction: ltr;
}

input[type="range"]::-webkit-slider-runnable-track {
    height: 3px;
    background: transparent;
}

input[type="range"]::-webkit-slider-thumb {
    -webkit-appearance: none;
    appearance: none;
    width: 15px;
    height: 15px;
    border-radius: 2px;
    background: #171b1c;
    border: 2px solid var(--hazard);
    transform: rotate(45deg);
    cursor: pointer;
    box-shadow: 0 0 10px var(--hazard-glow);
    margin-top: -6px;
}

input[type="range"]::-webkit-slider-thumb:hover {
    border-color: var(--hazard-bright);
    box-shadow: 0 0 14px var(--hazard-glow);
}

input[type="range"]::-moz-range-track {
    height: 3px;
    background: #262d2e;
}

input[type="range"]::-moz-range-progress {
    height: 3px;
    background: var(--hazard);
}

input[type="range"]::-moz-range-thumb {
    width: 13px;
    height: 13px;
    border-radius: 2px;
    background: #171b1c;
    border: 2px solid var(--hazard);
    transform: rotate(45deg);
    cursor: pointer;
}

/* Color sliders — static channel gradients (JS skips these). */
.color-r { background: linear-gradient(90deg, #000, #f00) !important; }
.color-g { background: linear-gradient(90deg, #000, #0f0) !important; }
.color-b { background: linear-gradient(90deg, #000, #00f) !important; }

/* Tight slider (used inside a parent row) */
.slider-row.tight {
    display: flex;
    gap: 10px;
    align-items: center;
    margin-bottom: 0;
}

.slider-row.tight input[type="range"] { flex: 1; }
.slider-row.tight .slider-value { min-width: 50px; text-align: center; }

/* ============== Toggle Switch ============== */

.toggle-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin-bottom: 14px;
    padding: 4px 0;
    gap: 12px;
}

.toggle-row:last-child { margin-bottom: 0; }

.switch {
    position: relative;
    display: inline-block;
    width: 42px;
    height: 22px;
    cursor: pointer;
    flex: 0 0 auto;
}

.switch input {
    opacity: 0;
    width: 0;
    height: 0;
}

.switch-track {
    position: absolute;
    inset: 0;
    background: #202627;
    border: 1px solid var(--border);
    border-radius: 12px;
    transition: var(--transition);
}

.switch-knob {
    position: absolute;
    top: 3px;
    right: 3px;
    width: 14px;
    height: 14px;
    background: var(--text-dim);
    border-radius: 50%;
    transition: var(--transition);
}

.switch input:checked + .switch-track {
    background: rgba(226, 129, 47, 0.18);
    border-color: var(--hazard);
    box-shadow: 0 0 10px var(--hazard-glow);
}

.switch input:checked + .switch-track .switch-knob {
    background: var(--hazard);
    right: 23px;
    box-shadow: 0 0 8px var(--hazard-glow);
}

/* ============== Text Input / Select ============== */

.text-input-row {
    display: flex;
    align-items: center;
    gap: 12px;
    margin-bottom: 12px;
    padding: 6px 0;
}

.text-input-row:last-child { margin-bottom: 0; }

.text-input-row .slider-label {
    flex: 0 0 auto;
    min-width: 96px;
    text-align: right;
}

.text-input {
    flex: 1;
    min-width: 0;
    padding: 8px 12px;
    background: #0f1214;
    border: 1px solid var(--border);
    color: var(--text);
)C3"
    R"C4(    font-family: var(--mono);
    font-size: 12.5px;
    direction: ltr;
    text-align: left;
    transition: border-color var(--transition);
}

.text-input:focus {
    border-color: var(--hazard);
    box-shadow: 0 0 8px rgba(226, 129, 47, 0.18);
    outline: none;
}

/* ============== Custom HTML dropdowns (.fdrop) ==============
   Replaces native <select> elements: the WebKit popup of a native select
   requires view focus that the embedded view loses in-game, so the FIRST
   click never opened its list. These are plain HTML button + list — they
   open on the first click no matter what (no WebKit popup involved).
   Unified with Resistance Terminal dark theme: clipped corners, hazard glow,
   hev accents, fixed positioning to escape overflow clipping. */

.fdrop {
    position: relative;
    flex: 1;
    min-width: 0;
}

.fdrop-btn {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 8px;
    width: 100%;
    padding: 8px 12px;
    background: #0f1214;
    color: var(--text);
    border: 1px solid var(--border);
    cursor: pointer;
    text-align: left;
    font-family: var(--arabic);
    font-size: 13px;
    clip-path: polygon(6px 0, 100% 0, 100% calc(100% - 6px), calc(100% - 6px) 100%, 0 100%, 0 6px);
    transition: border-color var(--transition), box-shadow var(--transition), background var(--transition), color var(--transition);
}

.fdrop-btn:hover,
.fdrop.open .fdrop-btn {
    border-color: var(--hazard);
    box-shadow: 0 0 10px var(--hazard-glow);
    background: #111517;
}

.fdrop-btn:focus-visible {
    outline: 2px solid var(--hazard);
    outline-offset: 2px;
}

.fdrop-value {
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}

/* Chevron arrow (pure CSS, no images). */
.fdrop-arrow {
    flex: 0 0 auto;
    width: 0;
    height: 0;
    border-left: 4px solid transparent;
    border-right: 4px solid transparent;
    border-top: 5px solid var(--text-dim);
    transition: transform var(--transition), border-top-color var(--transition);
}

.fdrop.open .fdrop-arrow {
    transform: rotate(180deg);
    border-top-color: var(--hazard-bright);
}

.fdrop-list {
    display: none;
    position: absolute;
    top: calc(100% + 4px);
    left: 0;
    right: 0;
    margin: 0;
    padding: 4px 0;
    list-style: none;
    background: #0f1214;
    border: 1px solid var(--border);
    border-top-color: var(--hazard-dim);
    box-shadow: 0 12px 32px rgba(0, 0, 0, 0.65), 0 0 0 1px rgba(226, 129, 47, 0.12);
    max-height: 220px;
    overflow-y: auto;
    z-index: 60;
}

.fdrop.open .fdrop-list {
    display: block;
}

.fdrop-list::-webkit-scrollbar { width: 6px; }
.fdrop-list::-webkit-scrollbar-thumb { background: rgba(226, 129, 47, 0.25); border-radius: 3px; }
.fdrop-list::-webkit-scrollbar-thumb:hover { background: rgba(226, 129, 47, 0.5); }
.fdrop-list::-webkit-scrollbar-track { background: transparent; }

.fdrop-item {
    padding: 7px 12px;
    color: #adb6b3;
    background-color: transparent;
    cursor: pointer;
    font-family: var(--arabic);
    font-size: 13px;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
    transition: background var(--transition), color var(--transition), border-color var(--transition);
    border-right: 2px solid transparent;
}

.fdrop-item:hover {
    background: rgba(226, 129, 47, 0.12);
    color: #fff;
    border-right-color: var(--hazard);
}

.fdrop-item.selected {
    background: linear-gradient(180deg, var(--hazard-bright), var(--hazard-dim));
    color: #140d05;
    font-weight: 700;
    box-shadow: 0 0 12px rgba(226, 129, 47, 0.25);
    border-right-color: var(--hazard);
}

.fdrop-item.selected:hover {
    filter: brightness(1.05);
}

/* Font-file pickers: file names render in mono, LTR. */
.fdrop.font-select .fdrop-btn,
.fdrop.font-select .fdrop-item {
    font-family: var(--mono);
    font-size: 12.5px;
    direction: ltr;
    text-align: left;
}

/* Language picker follows the selected interface language. */
#ui_language .fdrop-btn,
#ui_language .fdrop-item {
    font-family: var(--arabic);
    direction: rtl;
    text-align: right;
}
body.ui-ltr #ui_language .fdrop-btn,
body.ui-ltr #ui_language .fdrop-item {
    direction: ltr;
    text-align: left;
}

@media (prefers-reduced-motion: reduce) {
    .fdrop-btn, .fdrop-item, .fdrop-arrow { transition: none; }
}

.app.compact .fdrop-btn { padding: 7px 10px; font-size: 12px; }
.app.compact .fdrop-item { padding: 6px 10px; font-size: 12px; }

/* ============== Segmented Control (clip-path buttons) ============== */

.seg-control {
    display: flex;
    align-items: center;
    gap: 16px;
}

.seg-label {
    color: #dfe3e0;
    font-size: 14px;
    font-weight: 500;
    white-space: nowrap;
}

.seg-buttons {
    display: flex;
    gap: 8px;
    flex: 1;
    min-width: 0;
}

.seg-btn {
    flex: 0 0 auto;
    font-family: var(--arabic);
    font-size: 13px;
    color: #adb6b3;
    background: #0f1214;
    border: 1px solid var(--border);
    padding: 9px 22px;
    cursor: pointer;
    transition: all var(--transition);
    clip-path: polygon(6px 0, 100% 0, 100% calc(100% - 6px), calc(100% - 6px) 100%, 0 100%, 0 6px);
}

.seg-btn:hover {
    border-color: var(--hev);
    color: #fff;
}

.seg-btn.selected {
    background: linear-gradient(180deg, var(--hazard-bright), var(--hazard-dim));
    border-color: var(--hazard);
    color: #140d05;
    font-weight: 700;
    box-shadow: 0 0 14px rgba(226, 129, 47, 0.3);
}

/* ============== Color Preview Large ============== */

.color-preview-large {
    width: 100%;
    height: 68px;
    margin-bottom: 14px;
    border: 1px solid var(--border);
    background: rgb(255, 255, 255);
    color: rgba(0, 0, 0, 0.7);
    font-weight: 800;
    font-size: 32px;
    display: flex;
    align-items: center;
    justify-content: center;
    text-shadow: 1px 1px 0 rgba(255, 255, 255, 0.4);
    transition: background 120ms linear;
}

/* ============== Tab List ============== */

.tabs-col {
    direction: rtl;
    text-align: right;
    background: linear-gradient(180deg, #0b0e11 0%, #0d1013 100%);
    border-left: 1px solid var(--border-soft);
    padding: 18px 16px;
    overflow-y: auto;
    overflow-x: hidden;
    display: flex;
    flex-direction: column;
    gap: 2px;
    position: relative;
    min-width: 0;
}

/* HUD corner brackets on the tab column (top-left / bottom-right). */
.tabs-col::before,
.tabs-col::after {
    content: "";
    position: absolute;
    width: 14px;
    height: 14px;
    border-color: var(--hazard-dim);
    z-index: 2;
    pointer-events: none;
}

.tabs-col::before {
    top: -1px; left: -1px;
    border-top: 2px solid var(--hazard);
    border-left: 2px solid var(--hazard);
}

.tabs-col::after {
    bottom: -1px; right: -1px;
    border-bottom: 2px solid var(--hazard);
    border-right: 2px solid var(--hazard);
}

.tab {
    width: 100%;
    display: flex;
)C4"
    R"C5(    flex-direction: row-reverse;  /* RTL: Arabic label on the RIGHT, icon/dot on the LEFT */
    align-items: center;
    justify-content: space-between;
    gap: 6px;
    background: transparent;
    border: none;
    border-right: 2px solid transparent;
    padding: 12px 10px;
    margin-bottom: 2px;
    font-family: var(--arabic);
    font-size: 13.5px;
    color: #b7bfbc;
    text-align: right;
    cursor: pointer;
    white-space: nowrap;
    transition: background var(--transition), color var(--transition), border-color var(--transition);
}

.tab:hover { background: #12171a; color: #fff; }

.tab .tab-icon {
    font-family: var(--mono);
    font-size: 13px;
    color: var(--hev);
    min-width: 16px;
    text-align: center;
}

.tab .dot {
    width: 6px;
    height: 6px;
    flex: 0 0 auto;
}

.tab.active {
    background: #180f06;
    border-right: 2px solid var(--hazard);
    color: #f3e9dc;
    font-weight: 700;
}

.tab.active .tab-icon { color: var(--hazard-bright); font-weight: 700; }

.tab.active .dot {
    background: var(--hazard-bright);
    box-shadow: 0 0 8px rgba(243, 154, 73, 0.9);
}

/* ============== Footer Stripe + Action Bar ============== */

.footer-stripe {
    height: 4px;
    background: repeating-linear-gradient(-45deg, var(--hev) 0 8px, #0a1211 8px 16px);
    opacity: 0.6;
}

.footer {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 18px 22px 22px;
    background: linear-gradient(180deg, #0b0e11 0%, var(--app-bg) 100%);
    position: relative;
    z-index: 3;
}
#btn-preview {
    flex: 0 0 auto;
}
.footer-center {
    display: flex;
    gap: 12px;
    align-items: center;
    justify-content: center;
    flex: 1;
}
.footer-spacer {
    flex: 1;
}

.btn {
    font-family: var(--arabic);
    font-size: 14px;
    font-weight: 700;
    padding: 11px 22px;
    cursor: pointer;
    background: transparent;
    transition: all var(--transition);
    clip-path: polygon(7px 0, 100% 0, 100% calc(100% - 7px), calc(100% - 7px) 100%, 0 100%, 0 7px);
    white-space: nowrap;
}

.btn-red {
    border: 1.5px solid var(--rose);
    color: var(--rose);
}

.btn-red:hover { background: rgba(214, 99, 109, 0.1); }

.btn-orange {
    border: 1.5px solid var(--hazard);
    background: linear-gradient(180deg, var(--hazard-bright), var(--hazard));
    color: #ffffff;
    box-shadow: 0 0 18px rgba(226, 129, 47, 0.25);
    /* White label with a thin dark rim so the word pops on the orange fill. */
    -webkit-text-stroke: 0.5px rgba(20, 13, 5, 0.5);
    font-weight: 800;
}

.btn-orange:hover { filter: brightness(1.08); }

.btn-hev {
    border: 1.5px solid var(--hev);
    color: var(--hev);
}
.btn-hev:hover { background: rgba(75,185,168,0.1); border-color: var(--hev-bright); color: var(--hev-bright); }
.btn-hev.on {
    background: var(--hev);
    border-color: var(--hev);
    color: #0a1211;
    font-weight: 700;
}

/* Brief confirmation flash after Save / Reset (toggled by bindings.js). */
.btn.flash {
    animation: btn-flash 300ms ease-out;
}

@keyframes btn-flash {
    0%   { filter: brightness(1.6); }
    100% { filter: brightness(1); }
}

/* ============== Compact mode (narrow panel) ============== */

.app.compact .main { grid-template-columns: minmax(0, 1fr) 1px 140px; }
body.ui-ltr .app.compact .main { grid-template-columns: 140px 1px minmax(0, 1fr); }
.app.compact .reticle { display: none; }
.app.compact .header { padding: 18px 18px 14px; gap: 8px; }
.app.compact .header-title h1 { font-size: 21px; }
.app.compact .header-sub { font-size: 11px; }
.app.compact .eyebrow { font-size: 9.5px; letter-spacing: 2px; margin-bottom: 6px; }
.app.compact .panel-col { padding: 14px 16px 22px; gap: 14px; }
.app.compact .card.notched { padding: 14px 16px; }
.app.compact .card h2 { font-size: 14px; margin-bottom: 10px; }
.app.compact .slider-label { font-size: 13px; }
.app.compact .slider-value { font-size: 13px; min-width: 56px; }
.app.compact .slider-row { padding: 8px 0; gap: 8px; }
.app.compact .slider-info { gap: 8px; }
body.ui-ltr .app.compact .slider-info { flex-basis: 160px; }
.app.compact .tabs-col { padding: 12px 10px; }
.app.compact .tab { padding: 10px 10px; font-size: 13px; gap: 8px; }
.app.compact .footer { gap: 8px; padding: 14px 14px 16px; }
.app.compact .btn { padding: 9px 14px; font-size: 12.5px; }
.app.compact .key-btn { font-size: 11.5px; padding: 5px 8px 5px 6px; }
.app.compact .brand { font-size: 14px; }
.app.compact .brand-badge { font-size: 11px; padding: 5px 9px; }
.app.compact .seg-btn { padding: 7px 10px; font-size: 12px; }
.app.compact .topbar { padding: 10px 16px; }

</style>
</head>
<body>
    <!-- ===================== STARTUP BANNER ===================== -->
    <!-- Mod branding shown at the top-center of the screen on game start.
         Fades in, holds ~5s, then fades out (see banner-lifecycle in CSS).
         pointer-events:none so it never intercepts mouse input. -->
    <div id="hud-banner">
        <div class="banner-row">
)C5"
    R"C6(            <img id="banner-logo" alt="" src="data:image/png;base64,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)C6"
    R"C7(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" />
            <span class="banner-title" data-i18n="brand.title">Hlyx Caption</span>
        </div>
)C7"
    R"C8(        <div class="banner-hints">
            <span class="hint"><span class="kbd">F10</span> <span data-i18n="hints.settings">قائمة الإعدادات</span></span>
            <span class="hint-sep">&#8226;</span>
            <span class="hint"><span class="kbd">F11</span> <span id="banner-translation" data-i18n="translation.enabled">الترجمة مفعلة</span></span>
        </div>
    </div>

    <div class="app" id="app">

        <!-- ===================== HAZARD STRIPE CAP ===================== -->
        <div class="hazard-bar" aria-hidden="true"></div>

        <!-- ===================== TOP BAR ===================== -->
        <header class="topbar">
            <div class="topbar-right">
                <span class="brand-badge" data-i18n="brand.badge">الإعدادات</span>
                <span class="brand">Hlyx Caption</span>
            </div>
            <div class="topbar-left">
                <button class="key-btn" id="btn-close">
                    <span data-i18n="actions.close">إغلاق</span><span class="key">F10</span>
                </button>
                <button class="key-btn on" id="btn-hl-toggle" type="button" aria-pressed="true">
                    <span data-i18n="translation.enabled">الترجمة مفعلة</span><span class="key">F11</span>
                </button>
            </div>
        </header>

        <!-- ===================== HEADER / TITLE ===================== -->
        <div class="header">
            <div class="header-title">
                <span class="eyebrow">RESISTANCE TERMINAL</span>
                <h1 data-i18n="brand.title">Hlyx Caption</h1>
                <p class="header-sub">
                    <span data-i18n="stats.font_size">حجم الخط:</span> <b id="stat-font-size">34px</b>
                    <span class="stat-sep">•</span>
                    <span data-i18n="stats.spacing">تباعد:</span> <b id="stat-line-spacing">1.30</b>
                    <span class="stat-sep">•</span>
                    <span data-i18n="stats.reference">مقياس:</span> <b id="stat-reference-height">1080px</b>
                </p>
            </div>
            <div class="reticle" aria-hidden="true">
                <span class="corner tl"></span>
                <span class="corner tr"></span>
                <span class="corner bl"></span>
                <span class="corner br"></span>
                <div class="watermark">HLA<br>LYX</div>
            </div>
        </div>

        <!-- ===================== MAIN (settings + tabs) ===================== -->
        <div class="main">

            <div class="panel-col content">

                <!-- ============ النص (Text / Typography) ============ -->
                <section id="text" class="active">
                    <div class="card notched">
                        <h2 data-i18n="sections.typography">الطباعة</h2>
                        <div class="card-body">
                            <div class="slider-row">
                                <div class="slider-info">
                                    <span class="slider-label" data-i18n="labels.font_size">حجم الخط</span>
                                    <span class="slider-value" id="font_size_out">34.0 px</span>
                                </div>
                                <input type="range" id="font_size" min="14" max="64" step="1" value="34" />
                            </div>

                            <div class="slider-row">
                                <div class="slider-info">
                                    <span class="slider-label" data-i18n="labels.line_spacing">تباعد الأسطر</span>
                                    <span class="slider-value" id="line_spacing_out">1.30</span>
                                </div>
                                <input type="range" id="line_spacing" min="0.8" max="2.5" step="0.05" value="1.3" />
                            </div>

                            <div class="slider-row">
                                <div class="slider-info">
                                    <span class="slider-label" data-i18n="labels.reference_height">الهامش الرئيسي</span>
                                    <span class="slider-value" id="font_size_reference_height_out">1080 px</span>
                                </div>
                                <input type="range" id="font_size_reference_height" min="600" max="2160" step="10" value="1080" />
                            </div>
                        </div>
                    </div>

                    <div class="card notched">
                        <h2 data-i18n="sections.alignment">المحاذاة</h2>
                        <div class="card-body">
                            <div class="seg-control" id="text_alignment">
                                <span class="seg-label" data-i18n="labels.alignment">المحاذاة</span>
                                <div class="seg-buttons">
                                    <button class="seg-btn" data-value="2" data-i18n="values.right">يمين</button>
                                    <button class="seg-btn selected" data-value="1" data-i18n="values.center">وسط</button>
                                    <button class="seg-btn" data-value="0" data-i18n="values.left">يسار</button>
                                </div>
                            </div>
                        </div>
                    </div>
                </section>

                <!-- ============ اللون (Color) ============ -->
                <section id="color">
                    <div class="card notched">
                        <h2 data-i18n="sections.text_color">لون النص</h2>
                        <div class="card-body">
                            <div class="color-preview-large" id="text-preview">Aa</div>

                            <div class="slider-row">
                                <div class="slider-info">
                                    <span class="slider-label" data-i18n="labels.red">أحمر (R)</span>
                                    <span class="slider-value" id="text_r_out">255</span>
                                </div>
                                <input type="range" class="color-r" id="text_r" min="0" max="255" value="255" />
                            </div>

                            <div class="slider-row">
                                <div class="slider-info">
                                    <span class="slider-label" data-i18n="labels.green">أخضر (G)</span>
                                    <span class="slider-value" id="text_g_out">255</span>
                                </div>
                                <input type="range" class="color-g" id="text_g" min="0" max="255" value="255" />
                            </div>

                            <div class="slider-row">
)C8"
    R"C9(                                <div class="slider-info">
                                    <span class="slider-label" data-i18n="labels.blue">أزرق (B)</span>
                                    <span class="slider-value" id="text_b_out">255</span>
                                </div>
                                <input type="range" class="color-b" id="text_b" min="0" max="255" value="255" />
                            </div>
                        </div>
                    </div>
                </section>

                <!-- ============ الحدود (Border / Outline) ============ -->
                <section id="border">
                    <div class="card notched">
                        <h2 data-i18n="sections.outline">الحدود (Outline)</h2>
                        <div class="card-body">
                            <div class="toggle-row">
                                <span class="slider-label" data-i18n="labels.outline_enabled">تفعيل الحدود</span>
                                <label class="switch">
                                    <input type="checkbox" id="outline_enabled" checked />
                                    <span class="switch-track"><span class="switch-knob"></span></span>
                                </label>
                            </div>

                            <div class="slider-row">
                                <div class="slider-info">
                                    <span class="slider-label" data-i18n="labels.outline_thickness">سمك الحدود</span>
                                    <span class="slider-value" id="outline_thickness_out">3.0 px</span>
                                </div>
                                <input type="range" id="outline_thickness" min="0" max="10" step="0.5" value="3.0" />
                            </div>
                        </div>
                    </div>
                </section>

                <!-- ============ الظل (Shadow) ============ -->
                <section id="shadow">
                    <div class="card notched">
                        <h2 data-i18n="sections.shadow">الظل</h2>
                        <div class="card-body">
                            <div class="toggle-row">
                                <span class="slider-label" data-i18n="labels.shadow_enabled">إضافة ظل</span>
                                <label class="switch">
                                    <input type="checkbox" id="shadow_enabled" />
                                    <span class="switch-track"><span class="switch-knob"></span></span>
                                </label>
                            </div>

                            <div class="slider-row">
                                <div class="slider-info">
                                    <span class="slider-label" data-i18n="labels.shadow_offset_x">سمك الظل (offset X)</span>
                                    <span class="slider-value" id="shadow_offset_x_out">4.0 px</span>
                                </div>
                                <input type="range" id="shadow_offset_x" min="-10" max="20" step="0.5" value="4.0" />
                            </div>
                        </div>
                    </div>
                </section>

                <!-- ============ الموقع (Position) ============ -->
                <section id="position">
                    <div class="card notched">
                        <h2 data-i18n="sections.position">موضع النصوص</h2>
                        <div class="card-body">
                            <div class="slider-row">
                                <div class="slider-info">
                                    <span class="slider-label" data-i18n="labels.position_x">الموضع الأفقي (X)</span>
                                    <span class="slider-value" id="pos_x_out">50%</span>
                                </div>
                                <input type="range" id="pos_x" min="0.0" max="1.0" step="0.01" value="0.50" />
                            </div>

                            <div class="slider-row">
                                <div class="slider-info">
                                    <span class="slider-label" data-i18n="labels.position_y">الموضع العمودي (Y)</span>
                                    <span class="slider-value" id="pos_y_out">80%</span>
                                </div>
                                <input type="range" id="pos_y" min="0.0" max="1.0" step="0.01" value="0.80" />
                            </div>
                        </div>
                    </div>
                </section>

                <!-- ============ التوقيت (Timing) ============ -->
                <section id="timing">
                    <div class="card notched">
                        <h2 data-i18n="sections.timing">التوقيت</h2>
                        <div class="card-body">
                            <div class="slider-row">
                                <div class="slider-info">
                                    <span class="slider-label" data-i18n="labels.fade_in">الظهور (fade-in)</span>
                                    <span class="slider-value" id="fade_in_time_out">0.20s</span>
                                </div>
                                <input type="range" id="fade_in_time" min="0.05" max="1.5" step="0.05" value="0.20" />
                            </div>

                            <div class="slider-row">
                                <div class="slider-info">
                                    <span class="slider-label" data-i18n="labels.fade_out">الاختفاء (fade-out)</span>
                                    <span class="slider-value" id="fade_out_time_out">0.50s</span>
                                </div>
                                <input type="range" id="fade_out_time" min="0.05" max="2.0" step="0.05" value="0.50" />
                            </div>

                            <div class="slider-row">
                                <div class="slider-info">
                                    <span class="slider-label" data-i18n="labels.extra_time">الوقت الإضافي للجملة</span>
                                    <span class="slider-value" id="extra_display_time_out">1.00s</span>
                                </div>
                                <input type="range" id="extra_display_time" min="0.0" max="3.0" step="0.05" value="1.0" />
                            </div>
                        </div>
                    </div>
                </section>

                <!-- ============ الخط (Font) ============ -->
                <section id="reading">
                    <div class="card notched">
                        <h2 data-i18n="sections.font">الخط</h2>
                        <div class="card-body">
)C9"
    R"C10(                            <!-- Custom HTML dropdown (no native <select> popup:
                                 the WebKit popup needs focus that the embedded
                                 view loses in-game, so the FIRST click on a
                                 native select never opens its list). Options are
                                 built by hlaFonts.setList from the font files in
                                 {ModDir}/resources (ttf/otf/ttc/otc/woff/woff2). -->
                            <div class="text-input-row">
                                <span class="slider-label" data-i18n="labels.custom_font">الخط المخصص</span>
                                <div class="fdrop font-select" id="custom_font_path">
                                    <button type="button" class="fdrop-btn" tabindex="0">
                                        <span class="fdrop-value">Cairo-Regular.ttf</span>
                                        <span class="fdrop-arrow" aria-hidden="true"></span>
                                    </button>
                                    <ul class="fdrop-list" role="listbox"></ul>
                                </div>
                            </div>

                            <div class="text-input-row">
                                <span class="slider-label" data-i18n="labels.fallback_font">خط احتياطي</span>
                                <div class="fdrop font-select" id="fallback_font_path">
                                    <button type="button" class="fdrop-btn" tabindex="0">
                                        <span class="fdrop-value">Cairo-Regular.ttf</span>
                                        <span class="fdrop-arrow" aria-hidden="true"></span>
                                    </button>
                                    <ul class="fdrop-list" role="listbox"></ul>
                                </div>
                            </div>
                        </div>
                    </div>
                </section>

                <!-- ============ عام (General: interface language) ============ -->
                <section id="general">
                    <div class="card notched">
                        <h2 data-i18n="sections.language">اللغة</h2>
                        <div class="card-body">
                            <div class="text-input-row">
                                <span class="slider-label" data-i18n="labels.ui_language">لغة الواجهة</span>
                                <div class="fdrop" id="ui_language">
                                    <button type="button" class="fdrop-btn" tabindex="0">
                                        <span class="fdrop-value" data-i18n="languages.arabic">العربية</span>
                                        <span class="fdrop-arrow" aria-hidden="true"></span>
                                    </button>
                                    <ul class="fdrop-list" role="listbox">
                                        <li class="fdrop-item" data-value="1" data-i18n="languages.english">English</li>
                                        <li class="fdrop-item selected" data-value="2" data-i18n="languages.arabic">العربية</li>
                                    </ul>
                                </div>
                            </div>

                        </div>
                    </div>
                </section>

            </div>

            <div class="divider" aria-hidden="true"></div>

            <!-- ===================== TAB LIST ===================== -->
            <nav class="tabs-col" id="tabGroup">
                <button class="tab nav-item active" data-tab="text">
                    <span class="dot"></span><span class="tab-icon">Aa</span><span data-i18n="tabs.text">النص</span>
                </button>
                <button class="tab nav-item" data-tab="color">
                    <span></span><span class="tab-icon">#</span><span data-i18n="tabs.color">اللون</span>
                </button>
                <button class="tab nav-item" data-tab="border">
                    <span></span><span class="tab-icon">O</span><span data-i18n="tabs.outline">الحدود</span>
                </button>
                <button class="tab nav-item" data-tab="shadow">
                    <span></span><span class="tab-icon">S</span><span data-i18n="tabs.shadow">الظل</span>
                </button>
                <button class="tab nav-item" data-tab="position">
                    <span></span><span class="tab-icon">P</span><span data-i18n="tabs.position">الموقع</span>
                </button>
                <button class="tab nav-item" data-tab="timing">
                    <span></span><span class="tab-icon">T</span><span data-i18n="tabs.timing">التوقيت</span>
                </button>
                <button class="tab nav-item" data-tab="reading">
                    <span></span><span class="tab-icon">M</span><span data-i18n="tabs.font">الخط</span>
                </button>
                <button class="tab nav-item" data-tab="general">
                    <span></span><span class="tab-icon">G</span><span data-i18n="tabs.general">عام</span>
                </button>
            </nav>
        </div>

        <!-- ===================== FOOTER ===================== -->
        <div class="footer-stripe" aria-hidden="true"></div>
        <footer class="footer">
            <button id="btn-preview" class="btn btn-hev" data-i18n="actions.preview">معاينة</button>
            <div class="footer-center">
                <button id="btn-apply" class="btn btn-orange" data-i18n="actions.save">حفظ</button>
                <button id="btn-reset" class="btn btn-red" data-i18n="actions.reset">استعادة الافتراضي</button>
            </div>
            <div class="footer-spacer" aria-hidden="true"></div>
        </footer>

    </div>

    <script>
// Hlyx Caption original UI source — SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Hlyx Caption Contributors
// bindings.js - UL settings panel
// Stores all current values in a single global object (window.__state).
// On Apply, native polls this object via EvaluateScript("__getConfigState()").
// On Reset, native refreshes DOM via EvaluateScript calls.

(function () {
    "use strict";

    // The panel has two explicit languages.  ui_language=1 is English and
    // ui_language=2 is Arabic; old/invalid values fall back to Arabic.
    const TRANSLATIONS = {
        ar: {
            "page.title": "Hlyx Caption — إعدادات الترجمة",
            "brand.title": "Hlyx Caption",
            "brand.badge": "الإعدادات",
            "hints.settings": "قائمة الإعدادات",
            "translation.enabled": "الترجمة مفعلة",
            "translation.disabled": "الترجمة موقوفة",
)C10"
    R"C11(            "actions.close": "إغلاق",
            "actions.preview": "معاينة",
            "actions.save": "حفظ",
            "actions.reset": "استعادة الافتراضي",
            "stats.font_size": "حجم الخط:",
            "stats.spacing": "تباعد:",
            "stats.reference": "مقياس:",
            "sections.typography": "الطباعة",
            "sections.alignment": "المحاذاة",
            "sections.text_color": "لون النص",
            "sections.outline": "الحدود (Outline)",
            "sections.shadow": "الظل",
            "sections.position": "موضع النصوص",
            "sections.timing": "التوقيت",
            "sections.font": "الخط",
            "sections.language": "اللغة",
            "labels.font_size": "حجم الخط",
            "labels.line_spacing": "تباعد الأسطر",
            "labels.reference_height": "الهامش الرئيسي",
            "labels.alignment": "المحاذاة",
            "labels.red": "أحمر (R)",
            "labels.green": "أخضر (G)",
            "labels.blue": "أزرق (B)",
            "labels.outline_enabled": "تفعيل الحدود",
            "labels.outline_thickness": "سمك الحدود",
            "labels.shadow_enabled": "إضافة ظل",
            "labels.shadow_offset_x": "سمك الظل (offset X)",
            "labels.position_x": "الموضع الأفقي (X)",
            "labels.position_y": "الموضع العمودي (Y)",
            "labels.fade_in": "الظهور (fade-in)",
            "labels.fade_out": "الاختفاء (fade-out)",
            "labels.extra_time": "الوقت الإضافي للجملة",
            "labels.custom_font": "الخط المخصص",
            "labels.fallback_font": "خط احتياطي",
            "labels.ui_language": "لغة الواجهة",
            "values.right": "يمين",
            "values.center": "وسط",
            "values.left": "يسار",
            "languages.english": "English",
            "languages.arabic": "العربية",
            "tabs.text": "النص",
            "tabs.color": "اللون",
            "tabs.outline": "الحدود",
            "tabs.shadow": "الظل",
            "tabs.position": "الموقع",
            "tabs.timing": "التوقيت",
            "tabs.font": "الخط",
            "tabs.general": "عام"
        },
        en: {
            "page.title": "Hlyx Caption — Caption Settings",
            "brand.title": "Hlyx Caption",
            "brand.badge": "SETTINGS",
            "hints.settings": "Settings",
            "translation.enabled": "Caption enabled",
            "translation.disabled": "Caption disabled",
            "actions.close": "Close",
            "actions.preview": "Preview",
            "actions.save": "Save",
            "actions.reset": "Restore Defaults",
            "stats.font_size": "Font size:",
            "stats.spacing": "Spacing:",
            "stats.reference": "Scale:",
            "sections.typography": "Typography",
            "sections.alignment": "Alignment",
            "sections.text_color": "Text Color",
            "sections.outline": "Outline",
            "sections.shadow": "Shadow",
            "sections.position": "Text Position",
            "sections.timing": "Timing",
            "sections.font": "Font",
            "sections.language": "Language",
            "labels.font_size": "Font size",
            "labels.line_spacing": "Line spacing",
            "labels.reference_height": "Reference height",
            "labels.alignment": "Alignment",
            "labels.red": "Red (R)",
            "labels.green": "Green (G)",
            "labels.blue": "Blue (B)",
            "labels.outline_enabled": "Enable outline",
            "labels.outline_thickness": "Outline thickness",
            "labels.shadow_enabled": "Enable shadow",
            "labels.shadow_offset_x": "Shadow offset X",
            "labels.position_x": "Horizontal position (X)",
            "labels.position_y": "Vertical position (Y)",
            "labels.fade_in": "Fade in",
            "labels.fade_out": "Fade out",
            "labels.extra_time": "Extra caption time",
            "labels.custom_font": "Custom font",
            "labels.fallback_font": "Fallback font",
            "labels.ui_language": "Interface language",
            "values.right": "Right",
            "values.center": "Center",
            "values.left": "Left",
            "languages.english": "English",
            "languages.arabic": "Arabic",
            "tabs.text": "Text",
            "tabs.color": "Color",
            "tabs.outline": "Outline",
            "tabs.shadow": "Shadow",
            "tabs.position": "Position",
            "tabs.timing": "Timing",
            "tabs.font": "Font",
            "tabs.general": "General"
        }
    };

    const PREVIEW_TEXTS = {
        ar: [
            "هذا نص تجريبي لمعاينة الترجمة — Preview 123",
            "سطر ثاني <clr:255,180,0>ملون<clr> و <B>عريض</B> و <I>مائل</I><cr>سطر ثالث للتجربة",
            "النصوص التجريبية تساعدك على معايرة الإعدادات بدقة"
        ],
        en: [
            "This is sample text for previewing captions — Preview 123",
            "A second line with <clr:255,180,0>color<clr>, <B>bold</B>, and <I>italics</I><cr>A third line for testing",
            "Preview text helps you fine-tune the settings"
        ]
    };

    let activeLanguage = "ar";

    function tr(key) {
        return (TRANSLATIONS[activeLanguage] && TRANSLATIONS[activeLanguage][key]) || key;
    }

    function getPreviewText() {
        return PREVIEW_TEXTS[activeLanguage].join("<cr>");
    }

    function applyLanguage(value) {
        const numeric = Number(value);
        activeLanguage = numeric === 1 ? "en" : "ar";
        const isEnglish = activeLanguage === "en";
        document.documentElement.lang = activeLanguage;
        document.documentElement.dir = isEnglish ? "ltr" : "rtl";
        document.body.classList.toggle("ui-ltr", isEnglish);
        document.body.classList.toggle("ui-rtl", !isEnglish);
        document.querySelectorAll("[data-i18n]").forEach((el) => {
            el.textContent = tr(el.getAttribute("data-i18n"));
        });
        const languageValue = Number(window.__state && window.__state.ui_language);
        const languageLabel = document.querySelector("#ui_language .fdrop-value");
        if (languageLabel) {
            languageLabel.textContent = tr(languageValue === 1
                ? "languages.english"
                : "languages.arabic");
        }
        window.__previewText = getPreviewText();
        if (window.__previewEnabled) window.__previewRequested = true;
        if (window.hlaBanner && window.__translationVisible !== undefined) {
)C11"
    R"C12(            window.hlaBanner.setTranslationVisible(window.__translationVisible);
        }
    }

    // ============== Formatters ==============
    // Each entry maps a control id to a function (value -> display string).
    // NOTE: title-block stats reuse the same formatter so they stay in sync.
    const FORMATTERS = {
        extra_display_time:       v => v.toFixed(2) + "s",
        mouse_sensitivity:        v => v.toFixed(2) + "x",
        mouse_accel_factor:       v => v.toFixed(2) + "x",
        fade_in_time:             v => v.toFixed(2) + "s",
        fade_out_time:            v => v.toFixed(2) + "s",
        pos_x:                    v => Math.round(v * 100) + "%",
        pos_y:                    v => Math.round(v * 100) + "%",
        font_size:                v => v.toFixed(1) + " px",
        line_spacing:             v => v.toFixed(2),
        max_line_width_percent:   v => Math.round(v * 100) + "%",
        text_r:                   v => Math.round(v),
        text_g:                   v => Math.round(v),
        text_b:                   v => Math.round(v),
        shadow_offset_x:          v => v.toFixed(1) + " px",
        outline_thickness:        v => v.toFixed(1) + " px",
        bg_padding_x:             v => Math.round(v),
        bg_padding_y:             v => Math.round(v),
        bg_border_radius:         v => v.toFixed(1) + " px",
        font_size_reference_height: v => Math.round(v) + " px",
    };

    function fmt(id, v) {
        const f = FORMATTERS[id];
        return f ? f(v) : String(v);
    }

    // Paints the hazard-orange fill on a range input's track (the track
    // pseudo is transparent, so the input's own gradient shows through).
    // Color sliders keep their static R/G/B gradients.
    function paintSlider(input) {
        if (!input ||
            input.classList.contains("color-r") ||
            input.classList.contains("color-g") ||
            input.classList.contains("color-b")) return;
        const min = parseFloat(input.min), max = parseFloat(input.max), v = parseFloat(input.value);
        if (isNaN(min) || isNaN(max)) return;
        const pct = Math.max(0, Math.min(100, ((v - min) / (max - min)) * 100));
        input.style.background =
            "linear-gradient(90deg, var(--hazard) 0%, var(--hazard-bright) " +
            pct + "%, #262d2e " + pct + "%, #262d2e 100%)";
    }

    // ============== Tab switching ==============
    document.querySelectorAll(".nav-item").forEach((btn) => {
        btn.addEventListener("click", () => {
            const target = btn.dataset.tab;
            document.querySelectorAll(".nav-item").forEach((b) =>
                b.classList.toggle("active", b === btn));
            document.querySelectorAll(".content section").forEach((s) =>
                s.classList.toggle("active", s.id === target));
        });
    });

    // ============== Title-block stats sync ==============
    // Reflects a few "headline" values at the top of the panel so the user
    // sees the current configuration even before scrolling.
    // The stats use a tighter "compact" formatter than the inline slider values
    // so the title-bar reads as "34px" rather than "34.0 px".
    const STAT_IDS = {
        font_size:                "stat-font-size",
        line_spacing:             "stat-line-spacing",
        font_size_reference_height: "stat-reference-height",
    };
    const STAT_FORMATTERS = {
        font_size:                v => Math.round(v) + "px",
        line_spacing:             v => v.toFixed(2),
        font_size_reference_height: v => Math.round(v) + "px",
    };

    function refreshTitleStats() {
        for (const key in STAT_IDS) {
            const el = document.getElementById(STAT_IDS[key]);
            if (!el || !(key in window.__state)) continue;
            const f = STAT_FORMATTERS[key];
            el.textContent = f ? f(window.__state[key]) : String(window.__state[key]);
        }
    }

    // ============== Slider wiring ==============
    function wireSlider(id) {
        const input = document.getElementById(id);
        const out = document.getElementById(id + "_out");
        if (!input) return;
        const update = () => {
            const v = parseFloat(input.value);
            if (out) out.textContent = fmt(id, v);
            window.__state[id] = v;
            window.__stateDirty = true;
            paintSlider(input);
            refreshTitleStats();
        };
        input.addEventListener("input", update);
        update();
    }

    // ============== Checkbox / Switch wiring ==============
    function wireCheckbox(id) {
        const input = document.getElementById(id);
        if (!input) return;
        const update = () => { window.__state[id] = input.checked; window.__stateDirty = true; };
        input.addEventListener("change", update);
        update();
    }

    // ============== Radio (segmented control) wiring ==============
    function wireRadio(id) {
        const buttons = document.querySelectorAll('#' + id + ' .seg-btn');
        if (!buttons.length) return;
        buttons.forEach((btn) => {
            btn.addEventListener("click", () => {
                const v = parseInt(btn.dataset.value, 10);
                buttons.forEach((b) =>
                    b.classList.toggle("selected", b === btn));
                window.__state[id] = v;
                window.__stateDirty = true;
            });
        });
    }

    // ============== Text input wiring ==============
    function wireText(id) {
        const input = document.getElementById(id);
        if (!input) return;
        input.addEventListener("input", () => {
            window.__state[id] = input.value;
            window.__stateDirty = true;
        });
    }

// ============== Custom dropdown wiring (.fdrop) ==============
// The font pickers + the language picker are plain HTML button+list widgets
// (see settings.html) — no native <select> popup, so the FIRST click always
// opens the list (the WebKit popup of a native select needs view focus that
// the embedded view loses in-game, making the first click dead).
function dropSetValue(id, v) {
    const root = document.getElementById(id);
    if (!root) return;
    const label = root.querySelector(".fdrop-value");
    let found = null;
    root.querySelectorAll(".fdrop-item").forEach((li) => {
        const match = li.dataset.value === String(v);
        li.classList.toggle("selected", match);
        if (match) found = li;
    });
    if (found && label) label.textContent = found.textContent;
    else if (label) label.textContent = String(v);
}

// Position the dropdown list; with `position:absolute` the list is
// placed directly below the button via CSS `top:calc(100% + 4px)`.
// This helper only handles the flip-above case when there is not enough
// space below the button (e.g. near the bottom of the viewport) and
// ensures the list can escape the panel's scroll clipping.
)C12"
    R"C13(function positionDrop(root) {
    const btn = root.querySelector(".fdrop-btn");
    const list = root.querySelector(".fdrop-list");
    if (!btn || !list) return;
    const r = btn.getBoundingClientRect();
    const vh = window.innerHeight || document.documentElement.clientHeight;
    const gap = 4;
    const listH = Math.min(220, list.scrollHeight + 8);
    // Reset to default (below)
    list.style.top = "calc(100% + 4px)";
    list.style.bottom = "auto";
    list.style.left = "0";
    list.style.right = "0";
    list.style.width = "";
    list.style.minWidth = "";
    list.style.maxHeight = "220px";
    // Flip above if not enough space below
    if (r.bottom + gap + listH > vh - 8) {
        list.style.top = "auto";
        list.style.bottom = "calc(100% + 4px)";
        list.style.maxHeight = Math.min(220, r.top - gap - 8) + "px";
    } else {
        list.style.maxHeight = Math.min(220, vh - r.bottom - gap - 8) + "px";
    }
}

function wireDrop(id, numeric) {
    const root = document.getElementById(id);
    if (!root) return;
    const btn = root.querySelector(".fdrop-btn");
    const list = root.querySelector(".fdrop-list");
    if (!btn || !list) return;
    const panelCol = document.querySelector(".panel-col");
    const appEl = document.getElementById("app");
    const close = () => {
        root.classList.remove("open");
        if (panelCol) panelCol.style.overflow = "";
        if (appEl) appEl.style.overflow = "";
        list.style.top = "";
        list.style.bottom = "";
        list.style.left = "";
        list.style.right = "";
        list.style.width = "";
        list.style.minWidth = "";
        list.style.maxHeight = "";
    };

    const openAndPosition = () => {
        document.querySelectorAll(".fdrop.open").forEach((el) => {
            if (el !== root) el.classList.remove("open");
        });
        root.classList.add("open");
        if (panelCol) panelCol.style.overflow = "visible";
        if (appEl) appEl.style.overflow = "visible";
        positionDrop(root);
    };

    btn.addEventListener("click", (e) => {
        e.stopPropagation();
        if (root.classList.contains("open")) {
            close();
        } else {
            openAndPosition();
        }
    });
    // Keyboard: Enter/Space toggles, Esc closes.
    btn.addEventListener("keydown", (e) => {
        if (e.key === "Enter" || e.key === " " || e.keyCode === 13 || e.keyCode === 32) {
            e.preventDefault();
            if (root.classList.contains("open")) close();
            else openAndPosition();
        } else if (e.key === "Escape" || e.keyCode === 27) {
            close();
        }
    });
    list.addEventListener("click", (e) => {
        const li = e.target.closest(".fdrop-item");
        if (!li) return;
        const v = li.dataset.value;
        dropSetValue(id, v);
        window.__state[id] = numeric ? parseInt(v, 10) : v;
        if (id === "ui_language") applyLanguage(window.__state[id]);
        window.__stateDirty = true;
        close();
    });
    // Close when clicking anywhere outside the widget (list is fixed but still child of root).
    document.addEventListener("mousedown", (e) => {
        if (root.classList.contains("open") && !root.contains(e.target)) close();
    });
    document.addEventListener("keydown", (e) => {
        if (e.key === "Escape" || e.keyCode === 27) close();
    });
    // Reposition on viewport changes while open
    window.addEventListener("resize", () => {
        if (root.classList.contains("open")) positionDrop(root);
    });
    if (panelCol) {
        panelCol.addEventListener("scroll", () => {
            if (root.classList.contains("open")) positionDrop(root);
        }, { passive: true });
    }
    // Also reposition on app scroll/resize (compact mode)
    window.addEventListener("scroll", () => {
        if (root.classList.contains("open")) positionDrop(root);
    }, { passive: true });

    // Seed custom dropdown state from its marked selected item. Native config
    // synchronization may replace this value immediately after DOMReady.
    if (!(id in window.__state)) {
        const selected = root.querySelector(".fdrop-item.selected") || root.querySelector(".fdrop-item");
        if (selected) window.__state[id] = numeric
            ? parseInt(selected.dataset.value, 10)
            : selected.dataset.value;
    }
}

    // ============== Initialize global state ==============
    window.__state = window.__state || {};

    // Bind every control
    [
        // Sliders
        "extra_display_time",
        "mouse_sensitivity", "mouse_accel_factor", "fade_in_time", "fade_out_time",
        "pos_x", "pos_y",
        "font_size", "line_spacing", "max_line_width_percent",
        "text_r", "text_g", "text_b",
        "shadow_offset_x", "outline_thickness",
        "bg_padding_x", "bg_padding_y", "bg_border_radius",
        "font_size_reference_height",
    ].forEach(wireSlider);

    [
        // Switches / checkboxes
        "ui_animations", "mouse_acceleration", "shadow_enabled",
        "outline_enabled", "background_enabled",
    ].forEach(wireCheckbox);

    ["text_alignment"].forEach(wireRadio);
    wireDrop("ui_language", true);                 // numeric (1=EN, 2=AR)
    applyLanguage(window.__state.ui_language || 2);
    wireDrop("custom_font_path", false);           // string (file name)
    wireDrop("fallback_font_path", false);         // string (file name)

    // ============== Color preview (live) ==============
    function updateColorPreview() {
        const r = parseInt(document.getElementById("text_r").value);
        const g = parseInt(document.getElementById("text_g").value);
        const b = parseInt(document.getElementById("text_b").value);
        const p = document.getElementById("text-preview");
        if (p) p.style.background = "rgb(" + r + "," + g + "," + b + ")";
    }
    ["text_r", "text_g", "text_b"].forEach((id) => {
        const el = document.getElementById(id);
        if (el) el.addEventListener("input", updateColorPreview);
    });

    // ============== Caption visibility toggle ==============
    // The native Renderer owns the state. A click only raises a one-shot
    // request; UltralightManager consumes it on the render thread and then
    // sends the authoritative state back through setTranslationVisible().
    const btnHlToggle = document.getElementById("btn-hl-toggle");
    if (btnHlToggle) {
        btnHlToggle.addEventListener("click", (e) => {
            e.preventDefault();
            window.__captionToggleRequested = true;
        });
    }

    // ============== Apply / Reset / Close ==============
    function serializeState() {
        return JSON.stringify(window.__state);
    }
    // Brief visual confirmation after a footer action.
    function flashBtn(el) {
        if (!el) return;
        el.classList.remove("flash");
        void el.offsetWidth;          // force reflow -> restart animation
)C13"
    R"C14(        el.classList.add("flash");
    }
    // The native side (UltralightManager::Render) polls these flags every
    // frame. Setting them triggers SaveConfig / ResetConfig on the render
    // thread; the native side then calls hlaConfig._clearFlags() to reset.
    document.getElementById("btn-apply").addEventListener("click", () => {
        window.__saveRequested = true;
        console.log("[HLA] APPLY " + serializeState());
        flashBtn(document.getElementById("btn-apply"));
    });
    document.getElementById("btn-reset").addEventListener("click", () => {
        window.__resetRequested = true;
        console.log("[HLA] RESET");
        flashBtn(document.getElementById("btn-reset"));
    });
    // ============== Preview button (loop + live update) ==============
    window.__previewEnabled = false;
    window.__previewText = getPreviewText();
    const btnPreview = document.getElementById("btn-preview");
    if (btnPreview) {
        btnPreview.addEventListener("click", () => {
            const isOn = !window.__previewEnabled;
            window.__previewEnabled = isOn;
            window.__previewText = getPreviewText();
            window.__previewRequested = isOn;
            if (!isOn) window.__previewClear = true;
            btnPreview.classList.toggle("on", isOn);
            console.log("[HLA] PREVIEW " + (isOn ? "ON" : "OFF"));
        });
    }
    document.getElementById("btn-close").addEventListener("click", () => {
        closePanel();
    });

    // ============== Panel slide-in / slide-out ==============
    // Replays the language-side slide-in animation. Called by the native
    // side (UltralightManager::OnPanelOpened) every time the panel opens.
    function panelOpen() {
        const app = document.getElementById("app");
        if (!app) return;
        window.__panelHidden = false;
        document.body.classList.add("panel-open");   // show the blueprint grid
        app.style.transform = "";   // clear any inline override from a previous no-anim open
        window.__noAnim = window.__state && window.__state.ui_animations === false;
        if (app.offsetWidth) {
            app.classList.toggle("compact", app.offsetWidth < 620);
        }
        if (window.__noAnim) {
            app.classList.remove("closing");
            app.classList.remove("open-anim");
            app.style.transform = "translateX(0)"; // show instantly (no animation)
            return;
        }
        app.classList.remove("closing");
        void app.offsetWidth;               // force reflow -> restart animation
        app.classList.remove("open-anim");
        void app.offsetWidth;
        app.classList.add("open-anim");
    }

    // Plays the language-side slide-out animation, then flags the native side
    // (window.__panelHidden) which is polled every frame by
    // UltralightManager::Render and finishes the close.
    function closePanel() {
        const app = document.getElementById("app");
        const finish = () => {
            window.__panelHidden = true;
            document.body.classList.remove("panel-open");   // hide the blueprint grid
            app.style.transform = "";   // fall back to the CSS default (off-screen)
            // Also clear looping preview when panel closes
            if (window.__previewEnabled) {
                window.__previewEnabled = false;
                window.__previewClear = true;
                const btn = document.getElementById("btn-preview");
                if (btn) btn.classList.remove("on");
            }
        };
        if (!app || window.__noAnim) { finish(); return; }
        app.classList.remove("open-anim");
        app.classList.add("closing");
        let done = false;
        const onEnd = (e) => {
            if (done || e.propertyName !== "transform") return;
            done = true;
            app.removeEventListener("transitionend", onEnd);
            app.classList.remove("closing");
            finish();
        };
        app.addEventListener("transitionend", onEnd);
        // Fallback in case the transition event never fires.
        setTimeout(() => {
            if (!done) {
                done = true;
                app.classList.remove("closing");
                finish();
            }
        }, 400);
    }

    // Esc closes the panel (slide-out, then native hides the overlay).
    document.addEventListener("keydown", (e) => {
        if (e.key === "Escape" || e.keyCode === 27) {
            e.preventDefault();
            closePanel();
        }
    });

    // Called from native on every open (see UltralightManager::OnPanelOpened).
    window.hlaPanelOpen = panelOpen;

    // ============== Font pickers ==============
    // The native side (RefreshAllFromConfig) calls window.hlaFonts.setList([...])
    // on page load and after a Reset, with every font file found in
    // {ModDir}/resources (all supported extensions). We fill both dropdowns and
    // re-select the stored values.
    window.hlaFonts = {
        setList: function (names) {
            const ids = ["custom_font_path", "fallback_font_path"];
            ids.forEach((id) => {
                const root = document.getElementById(id);
                if (!root) return;
                const list = root.querySelector(".fdrop-list");
                if (!list) return;
                const stored = (window.__state && window.__state[id])
                    ? String(window.__state[id]) : "";
                const candidates = (names || []).slice();
                // Keep an old INI value (e.g. an absolute path) selectable even
                // if it is not among the resources files.
                if (stored && candidates.indexOf(stored) < 0) candidates.push(stored);
                if (!candidates.length) candidates.push("Cairo-Regular.ttf");  // safe default
                list.innerHTML = "";
                candidates.forEach((name) => {
                    const li = document.createElement("li");
                    li.className = "fdrop-item" + (name === stored ? " selected" : "");
                    li.dataset.value = name;
                    li.textContent = name;
                    list.appendChild(li);
                });
                dropSetValue(id, stored || candidates[0]);
            });
        }
    };

    // ============== Startup banner ==============
    // Native side (UltralightManager) updates the F11 hint label to reflect
    // whether the caption overlay is currently visible. All calls happen
    // on the render thread via EvaluateScript (thread-safe by design).
    window.hlaBanner = {
        setTranslationVisible: function (v) {
            window.__translationVisible = !!v;
            const label = v ? tr("translation.enabled") : tr("translation.disabled");
            // Startup banner hint (top-center).
            const hint = document.getElementById("banner-translation");
            if (hint) hint.textContent = label;
)C14"
    R"C15(            // Topbar F11 button — shows the live caption state.
            const btn = document.getElementById("btn-hl-toggle");
            if (btn) {
                const label = btn.querySelector("span:first-child");
                if (label) label.textContent = v ? tr("translation.enabled") : tr("translation.disabled");
                btn.classList.toggle("on", !!v);
                btn.setAttribute("aria-pressed", v ? "true" : "false");
            }
        }
    };

    // When the banner's fade-in/hold/fade-out animation finishes, tell the
    // native side (window.__bannerDone, polled in UltralightManager::Render)
    // so it stops rendering the closed overlay entirely (zero cost again).
    const bannerEl = document.getElementById("hud-banner");
    if (bannerEl) {
        bannerEl.addEventListener("animationend", () => {
            window.__bannerDone = true;
        });
    }

    updateColorPreview();
    refreshTitleStats();

    // ============================================================
    // Native -> JS bridge
    // The native side (RefreshAllFromConfig) calls
    //   window.hlaConfig._setValue['<key>'](<value>)
    // after every page load to sync the DOM with the current config.
    // ============================================================

    const SLIDER_KEYS = [
        "extra_display_time",
        "mouse_sensitivity", "mouse_accel_factor", "fade_in_time", "fade_out_time",
        "pos_x", "pos_y",
        "font_size", "line_spacing", "max_line_width_percent",
        "text_r", "text_g", "text_b",
        "shadow_offset_x", "outline_thickness",
        "bg_padding_x", "bg_padding_y", "bg_border_radius",
        "font_size_reference_height",
    ];
    const CHECK_KEYS = [
        "ui_animations", "mouse_acceleration", "shadow_enabled",
        "outline_enabled", "background_enabled",
    ];
    const RADIO_KEYS = ["text_alignment"];
    const SELECT_KEYS = ["ui_language"];
    const STR_SELECT_KEYS = ["custom_font_path", "fallback_font_path"];
    const TEXT_KEYS = [];

    function setControl(id, v, kind) {
        const el = document.getElementById(id);
        if (!el) return;
        if (kind === "slider") {
            el.value = String(v);
            const out = document.getElementById(id + "_out");
            if (out) out.textContent = fmt(id, parseFloat(v));
            paintSlider(el);
            window.__state[id] = parseFloat(v);
            refreshTitleStats();
        } else if (kind === "check") {
            el.checked = !!v;
            window.__state[id] = !!v;
            if (id === "ui_animations") {
                window.__noAnim = !v;
            }
        } else if (kind === "select") {
            const root = document.getElementById(id);
            if (root && root.classList.contains("fdrop")) {
                dropSetValue(id, v);
                window.__state[id] = parseInt(v, 10);
            } else {
                el.value = String(v);
                window.__state[id] = parseInt(v, 10);
            }
            if (id === "ui_language") applyLanguage(window.__state[id]);
        } else if (kind === "strselect") {
            const root = document.getElementById(id);
            if (root && root.classList.contains("fdrop")) {
                dropSetValue(id, v);
                window.__state[id] = String(v);
            } else {
                el.value = String(v);
                window.__state[id] = String(v);
            }
        } else if (kind === "radio") {
            document.querySelectorAll("#" + id + " .seg-btn").forEach((b) =>
                b.classList.toggle("selected", parseInt(b.dataset.value, 10) === parseInt(v, 10)));
            window.__state[id] = parseInt(v, 10);
        } else {  // text
            el.value = String(v);
            window.__state[id] = String(v);
        }
    }

    // Native side hooks a getter via hlaConfig:
    window.hlaConfig = {
        // returns JSON-serialized state to be read by EvaluateScript
        snapshot: () => serializeState(),
        // Called by the native side after a Save/Reset request is consumed,
        // so the same request is not re-triggered on the next frame.
        _clearFlags: () => {
            window.__saveRequested = false;
            window.__resetRequested = false;
            window.__previewRequested = false;
            window.__previewClear = false;
        },
        _setValue: (() => {
            const s = {};
            SLIDER_KEYS.forEach((k) => { s[k] = (v) => setControl(k, v, "slider"); });
            CHECK_KEYS.forEach((k) => { s[k] = (v) => setControl(k, v, "check"); });
            RADIO_KEYS.forEach((k) => { s[k] = (v) => setControl(k, v, "radio"); });
            SELECT_KEYS.forEach((k) => { s[k] = (v) => setControl(k, v, "select"); });
            STR_SELECT_KEYS.forEach((k) => { s[k] = (v) => setControl(k, v, "strselect"); });
            TEXT_KEYS.forEach((k) => { s[k] = (v) => setControl(k, v, "text"); });
            s.shadow_offset_y = () => {};   // dispatched by native, no DOM control
            return s;
        })(),
    };

    console.log("[UL] bindings initialized");
})();

</script>
</body>
</html>
)C15"
;

static const int kEmbeddedHTMLLen = sizeof(kEmbeddedHTML) - 1;

}  // namespace hlyx_caption
