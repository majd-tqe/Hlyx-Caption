/*
 * Hlyx Caption — original project source
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Hlyx Caption Contributors
 */

// UltralightManager.cpp - Settings panel using Ultralight HTML/CSS/JS engine.
// Approach: CPU render -> BGRA Bitmap -> D3D11 Texture2D -> blit to backbuffer.
#include "UltralightManager.h"
#include "UltralightPlatform.h"
#include "UltralightBlit.h"
#include "embedded_ui.h"
#include "UL_Debug.h"

#include <Ultralight/Ultralight.h>
#include <Ultralight/Bitmap.h>
#include <Ultralight/View.h>
#include <Ultralight/Renderer.h>
#include <Ultralight/Session.h>

#include "../config.h"
#include "../config_parse.h"
#include "../renderer.h"
#include "../banner_config.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <mutex>
#include <cmath>

#define ULDBG(fmt, ...) do { \
    char _buf[512]; snprintf(_buf, sizeof(_buf), "[ULDBG] " fmt "\n", ##__VA_ARGS__); \
    OutputDebugStringA(_buf); fputs(_buf, stdout); \
    UL_LogToFile("[ULDBG] " fmt, ##__VA_ARGS__); \
} while(0)

// Type aliases so we don't clash with project's Renderer class.
namespace ul = ultralight;

ID3D11Device*        g_Device = nullptr;
ID3D11DeviceContext* g_Context = nullptr;
ul::View*            g_ActiveView = nullptr;

namespace {

void DispatchConfigToJS(ul::View* view, const char* key, double v) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "if (window.hlaConfig && window.hlaConfig._setValue) "
             "{ window.hlaConfig._setValue['%s'](%g); }",
             key, v);
    view->EvaluateScript(ul::String(buf));
}

void DispatchConfigToJSBool(ul::View* view, const char* key, bool v) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "if (window.hlaConfig && window.hlaConfig._setValue) "
             "{ window.hlaConfig._setValue['%s'](%s); }",
             key, v ? "true" : "false");
    view->EvaluateScript(ul::String(buf));
}

// Builds the JS that applies BannerConfig (banner_config.h) onto
// #hud-banner, overriding the settings.css defaults. All px values in
// banner_config.h are defined @1080p and multiplied at runtime by
//   scale = viewportHeight / kReferenceHeight
// so the banner keeps the exact same visual shape on every resolution
// (720p, 1080p, 1440p, 4K ...). Fully adaptive — no upper clamp as
// requested. The function also stores scale on window.__bannerScale and
// window.__bannerBase* for the Resize handler to re-apply without a full
// C++ round-trip. Redefining @keyframes with the same name in a later
// stylesheet wins, and because we keep the same animation name,
// bindings.js's animationend -> window.__bannerDone handshake keeps working.
std::string BuildBannerConfigJS() {
    const float total = BannerConfig::kFadeInSec + BannerConfig::kHoldSec +
                        BannerConfig::kFadeOutSec;
    const float pIn   = total > 0.0f
        ? (BannerConfig::kFadeInSec / total) * 100.0f : 8.0f;
    const float pHold = total > 0.0f
        ? ((BannerConfig::kFadeInSec + BannerConfig::kHoldSec) / total) * 100.0f : 75.0f;
    char buf[2048];
    snprintf(buf, sizeof(buf),
        "(function(){"
        "var el=document.getElementById('hud-banner');if(!el)return'false';"
        "var refH=%g;var vh=(window.innerHeight||document.documentElement.clientHeight||refH);"
        "var scale=vh/refH;" // fully adaptive, no clamp
        "window.__bannerScale=scale;"
        "window.__bannerBase={logo:%g,title:%g,hints:%g,kbd:%g,gap:%g,offset:%g};"
        "el.style.top='%g%%';el.style.left='%g%%';"
        "el.style.transform='translateX(-50%%)';"
        "el.style.gap=(%g*scale)+'px';"
        "var lg=document.getElementById('banner-logo');"
        "if(lg){lg.style.maxWidth=(%g*scale)+'px';lg.style.maxHeight=(%g*scale)+'px';}"
        "var tt=el.querySelector('.banner-title');if(tt)tt.style.fontSize=(%g*scale)+'px';"
        "var hh=el.querySelector('.banner-hints');if(hh)hh.style.fontSize=(%g*scale)+'px';"
        "var kk=el.querySelector('.banner-hints .kbd');if(kk)kk.style.fontSize=(%g*scale)+'px';"
        // also scale the hint pill padding/border so the shape stays identical
        "if(hh){hh.style.padding=((6*scale)+'px '+(18*scale)+'px');hh.style.gap=(14*scale)+'px';hh.style.borderRadius=(999*scale)+'px';}"
        "if(kk){kk.style.padding=((1*scale)+'px '+(8*scale)+'px');kk.style.borderRadius=(5*scale)+'px';}"
        "var br=el.querySelector('.banner-row');if(br)br.style.gap=(14*scale)+'px';"
        "var st=document.createElement('style');"
        "st.setAttribute('data-banner','1');"
        "st.textContent='@keyframes banner-lifecycle{"
        "0%%{opacity:0;transform:translate(-50%%,'+(%g*scale)+'px)}"
        "%g%%{opacity:1;transform:translate(-50%%,0)}"
        "%g%%{opacity:1}"
        "100%%{opacity:0}}';"
        "var old=document.querySelector('style[data-banner=\"1\"]');if(old)old.remove();"
        "document.head.appendChild(st);"
        "el.style.animation='none';"
        "el.offsetWidth;" // force reflow so it restarts cleanly
        "el.style.animation='banner-lifecycle %gs ease forwards';"
        // re-apply on resize without C++ — keeps the same shape live
        "if(!window.__bannerResizeHook){window.__bannerResizeHook=true;window.addEventListener('resize',function(){"
        "var e=document.getElementById('hud-banner');if(!e||!window.__bannerBase)return;"
        "var rh=%g;var vh2=(window.innerHeight||document.documentElement.clientHeight||rh);var sc=vh2/rh;window.__bannerScale=sc;"
        "var b=window.__bannerBase;e.style.gap=(b.gap*sc)+'px';"
        "var l=document.getElementById('banner-logo');if(l){l.style.maxWidth=(b.logo*sc)+'px';l.style.maxHeight=(b.logo*sc)+'px';}"
        "var t=e.querySelector('.banner-title');if(t)t.style.fontSize=(b.title*sc)+'px';"
        "var h=e.querySelector('.banner-hints');if(h){h.style.fontSize=(b.hints*sc)+'px';h.style.padding=((6*sc)+'px '+(18*sc)+'px');h.style.gap=(14*sc)+'px';}"
        "var k=e.querySelector('.banner-hints .kbd');if(k){k.style.fontSize=(b.kbd*sc)+'px';k.style.padding=((1*sc)+'px '+(8*sc)+'px');}"
        "var r=e.querySelector('.banner-row');if(r)r.style.gap=(14*sc)+'px';"
        "var ks=document.querySelector('style[data-banner=\"1\"]');if(ks)ks.textContent='@keyframes banner-lifecycle{0%%{opacity:0;transform:translate(-50%%,'+(b.offset*sc)+'px)}%g%%{opacity:1;transform:translate(-50%%,0)}%g%%{opacity:1}100%%{opacity:0}}';"
        "});}"
        "return'true';})()",
        BannerConfig::kReferenceHeight,
        BannerConfig::kLogoMaxPx, BannerConfig::kTitleFontPx, BannerConfig::kHintsFontPx, BannerConfig::kKbdFontPx, BannerConfig::kRowGapPx, BannerConfig::kStartOffsetYpx,
        BannerConfig::kTopPct,
        BannerConfig::kLeftPct,
        BannerConfig::kRowGapPx,
        BannerConfig::kLogoMaxPx,
        BannerConfig::kLogoMaxPx,
        BannerConfig::kTitleFontPx,
        BannerConfig::kHintsFontPx,
        BannerConfig::kKbdFontPx,
        BannerConfig::kStartOffsetYpx,
        pIn,
        pHold,
        total,
        BannerConfig::kReferenceHeight,
        pIn,
        pHold);
    return std::string(buf);
}

// JSON/JS-escape a string so it can be embedded as a JS string literal.
static std::string JsEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"':  out += "\\\""; break;
            case '\'': out += "\\'";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if ((unsigned char)c < 0x20) {
                    char b[8];
                    snprintf(b, sizeof(b), "\\u%04x", (unsigned char)c);
                    out += b;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

void DispatchConfigToJSString(ul::View* view, const char* key, const char* v) {
    std::string js = "if (window.hlaConfig && window.hlaConfig._setValue) "
                     "{ window.hlaConfig._setValue['" + std::string(key) + "']('"
                     + JsEscape(v ? v : "") + "'); }";
    view->EvaluateScript(ul::String(js.c_str()));
}

// Directory of the loaded wininet.dll == {ModDir} (same technique as
// CustomFileSystem). Trailing backslash included.
static std::string GetDllDir() {
    char selfPath[MAX_PATH] = {};
    HMODULE hm = GetModuleHandleA("wininet.dll");
    if (hm) GetModuleFileNameA(hm, selfPath, MAX_PATH);
    char* slash = selfPath[0] ? strrchr(selfPath, '\\') : nullptr;
    if (slash) *(slash + 1) = 0;
    return std::string(selfPath);
}

// List every font file found in {ModDir}/resources. Covers every format the
// renderer's FreeType 2.13 supports: ttf/otf/ttc/otc/woff/woff2. Returns
// file names only (relative), sorted alphabetically.
static std::vector<std::string> EnumerateResourceFonts() {
    std::vector<std::string> out;
    const char* exts[] = { ".ttf", ".otf", ".ttc", ".otc", ".woff", ".woff2" };
    std::string pattern = GetDllDir() + "resources\\*";
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(pattern.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return out;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        std::string name = fd.cFileName;
        // Only simple basenames are exposed to JavaScript and later file APIs.
        // This also prevents quote/path injection through a crafted filename.
        if (name.find_first_of("'\\/\"\r\n") != std::string::npos)
            continue;
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        for (const char* ext : exts) {
            size_t n = strlen(ext);
            if (lower.size() >= n &&
                lower.compare(lower.size() - n, n, ext) == 0) {
                out.push_back(name);
                break;
            }
        }
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);
    std::sort(out.begin(), out.end());
    return out;
}

// Push the font list into the page: window.hlaFonts.setList([...]).
// The JS side fills both dropdowns and then re-selects the stored values
// (dispatched right after via DispatchConfigToJSString).
static void DispatchFontList(ul::View* view) {
    std::vector<std::string> fonts = EnumerateResourceFonts();
    std::string js = "if (window.hlaFonts && window.hlaFonts.setList) "
                     "{ window.hlaFonts.setList([";
    for (size_t i = 0; i < fonts.size(); ++i) {
        if (i) js += ",";
        js += "'" + JsEscape(fonts[i]) + "'";
    }
    js += "]); }";
    view->EvaluateScript(ul::String(js.c_str()));
}

void RefreshAllFromConfig(ul::View* view) {
    if (!view) return;
    DispatchConfigToJS(view, "ui_language", g_Config.ui_language);
    DispatchConfigToJS(view, "extra_display_time", g_Config.extra_display_time);
    DispatchConfigToJSBool(view, "ui_animations", g_Config.ui_animations);

    DispatchConfigToJS(view, "mouse_sensitivity", g_Config.mouse_sensitivity);
    DispatchConfigToJSBool(view, "mouse_acceleration", g_Config.mouse_acceleration);
    DispatchConfigToJS(view, "mouse_accel_factor", g_Config.mouse_accel_factor);
    DispatchConfigToJS(view, "fade_in_time", g_Config.fade_in_time);
    DispatchConfigToJS(view, "fade_out_time", g_Config.fade_out_time);

    DispatchConfigToJS(view, "pos_x", g_Config.pos_x);
    DispatchConfigToJS(view, "pos_y", g_Config.pos_y);

    DispatchConfigToJS(view, "font_size", g_Config.font_size);
    DispatchConfigToJS(view, "line_spacing", g_Config.line_spacing);
    DispatchConfigToJS(view, "max_line_width_percent", g_Config.max_line_width_percent);
    DispatchConfigToJS(view, "text_r", g_Config.text_r);
    DispatchConfigToJS(view, "text_g", g_Config.text_g);
    DispatchConfigToJS(view, "text_b", g_Config.text_b);
    DispatchConfigToJSBool(view, "shadow_enabled", g_Config.shadow_enabled);
    DispatchConfigToJS(view, "shadow_offset_x", g_Config.shadow_offset_x);
    DispatchConfigToJSBool(view, "outline_enabled", g_Config.outline_enabled);
    DispatchConfigToJS(view, "outline_thickness", g_Config.outline_thickness);

    DispatchConfigToJSBool(view, "background_enabled", g_Config.background_enabled);
    DispatchConfigToJS(view, "bg_padding_x", g_Config.bg_padding_x);
    DispatchConfigToJS(view, "bg_padding_y", g_Config.bg_padding_y);
    DispatchConfigToJS(view, "bg_border_radius", g_Config.bg_border_radius);

    DispatchConfigToJS(view, "font_size_reference_height", g_Config.font_size_reference_height);

    // Populate the font dropdowns from {ModDir}/resources (all supported
    // extensions), then re-select the stored values so the lists reflect the
    // saved configuration after a page load or a Reset.
    DispatchFontList(view);
    DispatchConfigToJSString(view, "custom_font_path", g_Config.custom_font_path.c_str());
    DispatchConfigToJSString(view, "fallback_font_path", g_Config.fallback_font_path.c_str());

    char buf[256];
    snprintf(buf, sizeof(buf),
             "if (document.getElementById('text-preview')) {"
             " var r=document.getElementById('text_r').value|0;"
             " var g=document.getElementById('text_g').value|0;"
             " var b=document.getElementById('text_b').value|0;"
             " var p=document.getElementById('text-preview');"
             " p.style.background='rgb('+r+','+g+','+b+')';"
             "}");
    // NOTE: we intentionally do NOT call window.hlaPanelOpen() here. The panel
    // is hidden by default (CSS translateX(100%)) and only slides in when the
    // native side triggers OnPanelOpened() on F10. Calling it from the initial
    // DOMReady sync would make the settings panel visible at game start while
    // the startup banner is showing.
    view->EvaluateScript(ul::String(buf));
}

void ApplyConfigField(const char* key, float v) {
    auto byteValue = [](float value, uint8_t fallback) -> uint8_t {
        if (!std::isfinite(value)) return fallback;
        if (value <= 0.0f) return 0;
        if (value >= 255.0f) return 255;
        return (uint8_t)value;
    };
    auto enumValue = [](float value, int fallback, int minValue, int maxValue) -> int {
        if (!std::isfinite(value)) return fallback;
        int result = (int)value;
        return result < minValue || result > maxValue ? fallback : result;
    };
    if      (!strcmp(key, "extra_display_time"))      g_Config.extra_display_time = v;
    else if (!strcmp(key, "mouse_sensitivity"))       g_Config.mouse_sensitivity = v;
    else if (!strcmp(key, "mouse_accel_factor"))      g_Config.mouse_accel_factor = v;
    else if (!strcmp(key, "fade_in_time"))            g_Config.fade_in_time = v;
    else if (!strcmp(key, "fade_out_time"))           g_Config.fade_out_time = v;
    else if (!strcmp(key, "pos_x"))                   g_Config.pos_x = v;
    else if (!strcmp(key, "pos_y"))                   g_Config.pos_y = v;
    else if (!strcmp(key, "font_size"))               g_Config.font_size = v;
    else if (!strcmp(key, "line_spacing"))            g_Config.line_spacing = v;
    else if (!strcmp(key, "max_line_width_percent"))  g_Config.max_line_width_percent = v;
    else if (!strcmp(key, "text_r"))                  g_Config.text_r = byteValue(v, g_Config.text_r);
    else if (!strcmp(key, "text_g"))                  g_Config.text_g = byteValue(v, g_Config.text_g);
    else if (!strcmp(key, "text_b"))                  g_Config.text_b = byteValue(v, g_Config.text_b);
    else if (!strcmp(key, "shadow_offset_x"))         g_Config.shadow_offset_x = v;
    else if (!strcmp(key, "shadow_offset_y"))         g_Config.shadow_offset_y = v;
    else if (!strcmp(key, "outline_thickness"))       g_Config.outline_thickness = v;
    else if (!strcmp(key, "bg_padding_x"))            g_Config.bg_padding_x = v;
    else if (!strcmp(key, "bg_padding_y"))            g_Config.bg_padding_y = v;
    else if (!strcmp(key, "bg_border_radius"))        g_Config.bg_border_radius = v;
    else if (!strcmp(key, "font_size_reference_height")) g_Config.font_size_reference_height = v;
    else if (!strcmp(key, "ui_language"))             g_Config.ui_language = enumValue(v, g_Config.ui_language, 1, 2);
    else if (!strcmp(key, "ui_direction_override"))  g_Config.ui_direction_override = enumValue(v, g_Config.ui_direction_override, 0, 2);
    else if (!strcmp(key, "text_alignment"))          g_Config.text_alignment = (OverlayConfig::Alignment)enumValue(v, g_Config.text_alignment, 0, 2);
}

void ApplyConfigFieldBool(const char* key, bool v) {
    if      (!strcmp(key, "ui_animations"))     g_Config.ui_animations = v;
    else if (!strcmp(key, "mouse_acceleration")) g_Config.mouse_acceleration = v;
    else if (!strcmp(key, "shadow_enabled"))    g_Config.shadow_enabled = v;
    else if (!strcmp(key, "outline_enabled"))   g_Config.outline_enabled = v;
    else if (!strcmp(key, "background_enabled")) g_Config.background_enabled = v;
}

void ApplyConfigFieldString(const char* key, const char* v) {
    if      (!strcmp(key, "custom_font_path"))   g_Config.custom_font_path = v;
    else if (!strcmp(key, "fallback_font_path")) g_Config.fallback_font_path = v;
}

void SaveConfigToFile() {
    // Persist to the SAME file LoadConfig() reads at startup
    // ({ModDir}\resources\settings.ini). Previously this was
    // {ModDir}\HLAMod.ini — migration is handled in hooks.cpp MainThread.
    SaveConfig(g_IniPath);
}

// Parse a JSON object of { key: value } pairs (the format produced by
// window.hlaConfig.snapshot()) and apply every field to g_Config.
// Shared by the periodic snapshot poll and the Save flow.
void ApplyConfigJson(const std::string& s) {
    size_t pos = 0;
    while ((pos = s.find('"', pos)) != std::string::npos) {
        size_t key_start = pos + 1;
        size_t key_end = s.find('"', key_start);
        if (key_end == std::string::npos) break;
        std::string key = s.substr(key_start, key_end - key_start);
        pos = key_end + 1;
        size_t colon = s.find(':', pos);
        if (colon == std::string::npos) break;
        size_t val_start = colon + 1;
        while (val_start < s.size() && (s[val_start] == ' ' || s[val_start] == '\t'))
            val_start++;
        size_t val_end = s.find_first_of(",}", val_start);
        std::string val = s.substr(val_start, val_end - val_start);
        val.erase(0, val.find_first_not_of(" \t"));
        bool is_bool = (val == "true" || val == "false");
        bool is_num = !val.empty() && (val[0] == '-' || val[0] == '.' ||
                                        (val[0] >= '0' && val[0] <= '9'));
        if (is_bool) {
            ApplyConfigFieldBool(key.c_str(), val == "true");
        } else if (is_num) {
            ApplyConfigField(key.c_str(), (float)atof(val.c_str()));
        } else if (val.size() >= 2 && val.front() == '"') {
            ApplyConfigFieldString(key.c_str(),
                                   val.substr(1, val.size() - 2).c_str());
        }
        pos = val_end;
    }
    SanitizeOverlayConfig(g_Config);
}

}  // namespace

UltralightManager& UltralightManager::Get() {
    static UltralightManager s_Instance;
    return s_Instance;
}

bool UltralightManager::Initialize(ID3D11Device* device, ID3D11DeviceContext* context,
                                   int width, int height) {
    if (m_Initialized) {
        ULDBG("Initialize: already initialized, skipping");
        return true;
    }
    if (!device || !context) return false;

    g_Device = device;
    g_Context = context;
    m_Width = width;
    m_Height = height;

    ULDBG("Initialize: device=%p context=%p size=%dx%d", device, context, width, height);

    if (!ultralight_blit::Initialize(device)) {
        ULDBG("Initialize: blit pipeline failed");
        return false;
    }

    char selfPath[MAX_PATH] = {};
    HMODULE hm = GetModuleHandleA("wininet.dll");
    if (hm) GetModuleFileNameA(hm, selfPath, MAX_PATH);
    char* slash = selfPath[0] ? strrchr(selfPath, '\\') : nullptr;
    if (slash) *(slash + 1) = 0;

    char assetDir[MAX_PATH];
    snprintf(assetDir, sizeof(assetDir), "%sui_ultralight\\assets\\", selfPath);
    ULDBG("Initialize: assetDir=%s", assetDir);

    ul::InitializeUltralightPlatform(ul::String(assetDir));

    m_Renderer = ul::Renderer::Create();
    if (!m_Renderer) {
        ULDBG("Initialize: Renderer::Create failed");
        return false;
    }
    ULDBG("Initialize: Renderer::Create OK");

    ul::ViewConfig cfg;
    cfg.is_accelerated = false;
    cfg.is_transparent = true;
    cfg.initial_device_scale = 1.0;
    // Default font family. The CSS uses more specific families (Cairo,
    // Oxanium, JetBrains Mono) which are loaded via @font-face from the
    // bundled woff2 files; Tahoma here is the ultimate fallback.
    cfg.font_family_standard = "Tahoma";
    cfg.font_family_serif = "Tahoma";
    cfg.font_family_sans_serif = "Tahoma";
    cfg.enable_javascript = true;
    cfg.enable_images = true;

    m_View = m_Renderer->CreateView((uint32_t)width, (uint32_t)height, cfg, nullptr);
    if (!m_View) {
        ULDBG("Initialize: CreateView failed");
        return false;
    }
    ULDBG("Initialize: CreateView OK (%ux%u)", width, height);

    m_View->set_load_listener(this);

    // Use the embedded HTML string built at compile time from settings.html,
    // settings.css, and bindings.js. No disk path required at runtime, so
    // file:// URL encoding issues cannot bite us.
    ULDBG("Initialize: embedded HTML bytes=%d", hlyx_caption::kEmbeddedHTMLLen);
    g_ActiveView = m_View.get();
    m_View->LoadHTML(ul::String(hlyx_caption::kEmbeddedHTML, hlyx_caption::kEmbeddedHTMLLen));
    // Force a repaint pass on next Update so we don't wait for any pending
    // CSS load / layout to finish.
    m_View->set_needs_paint(true);

    m_Initialized = true;
    ULDBG("Initialize: DONE");
    return true;
}


void UltralightManager::Resize(int width, int height) {
    if (!m_Initialized) return;
    m_Width = width;
    m_Height = height;
    if (m_View) m_View->Resize((uint32_t)width, (uint32_t)height);
    // Force banner scale re-apply on next Render tick — viewport height
    // changed so (vh / 1080) must be recomputed. The JS resize hook also
    // handles live resizes, but re-injecting from C++ covers cases where the
    // hook hasn't fired yet or the banner was recreated.
    if (!m_BannerFinished) m_BannerCfgApplied = false;
}

void UltralightManager::OnPanelOpened() {
    if (!m_View) return;
    // Give the View input focus the moment the panel opens. WebKit otherwise
    // consumes the FIRST click on a control (e.g. a <select>) just to focus the
    // page, which makes native dropdowns require a second click to open.
    m_View->Focus();
    ULDBG("OnPanelOpened: Focus() applied, HasFocus=%d", m_View->HasFocus() ? 1 : 0);
    // Replay the CSS slide-in-from-the-right animation. The page also resets
    // window.__panelHidden here so the next close request is detected.
    m_View->EvaluateScript(ul::String(
        "if (window.hlaPanelOpen) { window.hlaPanelOpen(); }"));
}

void UltralightManager::RequestCaptionVisibleChange(bool visible) {
    // Called from the game's message thread (WndProc). The View must only be
    // touched on the render thread, so we just record the request here and
    // apply it inside Render() (see ConsumePendingCaptionVisible).
    std::lock_guard<std::mutex> lock(m_EventMutex);
    m_PendingOverlayVisible = true;
    m_PendingOverlayVisibleValue = visible;
}

void UltralightManager::ConsumePendingCaptionVisible() {
    if (!m_View) return;
    bool pending = false;
    bool value = true;
    {
        std::lock_guard<std::mutex> lock(m_EventMutex);
        pending = m_PendingOverlayVisible;
        value = m_PendingOverlayVisibleValue;
        m_PendingOverlayVisible = false;
    }
    if (!pending) return;
    // Update the startup banner's F11 hint label ("الترجمة مفعلة" / "الترجمة موقوفة").
    m_View->EvaluateScript(ul::String(
        value ? "if (window.hlaBanner) window.hlaBanner.setTranslationVisible(true);"
               : "if (window.hlaBanner) window.hlaBanner.setTranslationVisible(false);"));
}

void UltralightManager::ConsumeCaptionToggleRequest() {
    if (!m_View) return;

    // This is deliberately a one-shot JS flag. It is read, cleared, and acted
    // upon only on the render thread; JavaScript never supplies the new state.
    ul::String8 requested8 = m_View->EvaluateScript(ul::String(
        "window.__captionToggleRequested===true?'1':'0'")).utf8();
    const char* requested = requested8.data() ? requested8.data() : "";
    if (strcmp(requested, "1") != 0) return;

    m_View->EvaluateScript(ul::String("window.__captionToggleRequested=false;"));
    Renderer::ToggleCaptionVisibility();
}

void UltralightManager::Shutdown() {
    if (m_BitmapSRV) { m_BitmapSRV->Release(); m_BitmapSRV = nullptr; }
    if (m_BitmapTexture) { m_BitmapTexture->Release(); m_BitmapTexture = nullptr; }
    m_View = nullptr;
    m_Renderer = nullptr;
    g_ActiveView = nullptr;
    ultralight_blit::Shutdown();
    m_Initialized = false;
}

void UltralightManager::Render() {
    if (!m_Initialized || !m_View || m_Width <= 0 || m_Height <= 0) return;

    // === One-shot: centralize the startup banner settings ===
    // Inject BannerConfig (banner_config.h) into the page as soon as its DOM
    // exists - BEFORE the banner's CSS animation becomes visible. Runs on
    // every tick until it succeeds, regardless of panel open/closed state,
    // so an early F10 press can never skip it.
    if (!m_BannerCfgApplied && !m_BannerFinished) {
        ul::String res = m_View->EvaluateScript(ul::String(BuildBannerConfigJS().c_str()));
        const char* r = res.utf8().data() ? res.utf8().data() : "";
        if (strstr(r, "true")) {
            m_BannerCfgApplied = true;
            ULDBG("banner config applied (banner_config.h)");
        }
    }

    // === WARMUP while closed ===
    // When the overlay is closed we used to return immediately, which meant the
    // HTML page only started loading on the FIRST F10 press - producing a
    // load + layout + JS (OnDOMReady -> ~30 EvaluateScript calls) storm exactly
    // at the moment the user starts moving the mouse. Keep the engine ticking
    // for the first few hundred frames after init so the page is fully loaded
    // BEFORE the user opens the settings.
    if (!m_Open) {
        // === STARTUP BANNER while closed ===
        // Keep the WebKit engine ticking so the page (banner + settings panel)
        // is fully loaded BEFORE the user presses F10 for the first time.
        if (m_WarmupCounter < 300) {
            ++m_WarmupCounter;
        }
        m_Renderer->Update();

        // Apply any queued F11 caption-visibility change (render thread).
        ConsumePendingCaptionVisible();

        // Once the banner's fade-out animation has completed, stop rendering
        // the closed overlay entirely (restores the original zero-cost path).
        if (m_BannerFinished) return;

        // Poll JS for the banner-done flag (set on animationend in bindings.js).
        {
            ul::String js = m_View->EvaluateScript(
                ul::String("window.__bannerDone === true"));
            ul::String8 s8 = js.utf8();
            const char* p = s8.data() ? s8.data() : "";
            if (strstr(p, "true")) {
                m_BannerFinished = true;
                return;
            }
        }

        // Render the page (banner) into its CPU bitmap.
        m_Renderer->RefreshDisplay(0);
        m_Renderer->Render();

        ul::BitmapSurface* bs = static_cast<ul::BitmapSurface*>(m_View->surface());
        if (!bs) return;
        ul::RefPtr<ul::Bitmap> bitmap = bs->bitmap();
        if (!bitmap || bitmap->IsEmpty()) return;

        if (!ultralight_blit::EnsureTexture(g_Device, g_Context,
                                           bitmap->width(), bitmap->height())) {
            return;
        }

        // Upload pixels only when something actually changed (fade animation,
        // F11 label update) - avoids a full-screen upload on every frame.
        if (!bs->dirty_bounds().IsEmpty()) {
            if (!ultralight_blit::UpdateBitmapFromUG(g_Context, bitmap)) return;
            bs->ClearDirtyBounds();
        }

        ultralight_blit::DrawFullscreenQuad(g_Context, m_Width, m_Height, 1.0f);
        ultralight_blit::EndFrame(g_Context);
        return;
    }

    // === Bulletproof input focus while open ===
    // WebKit's native <select> popup only opens when the View is focused at
    // the moment of the mousedown (proven in the test harness: without focus
    // the first click focuses the page and the popup needs a SECOND click).
    // Set the focus flag BEFORE the input drain: the flag is synced by
    // Update() below, so a mousedown dispatched this frame must not beat the
    // sync. HasFocus() always reports true (initial_focus default), so this
    // must be unconditional. Idempotent and cheap.
    if (m_View) m_View->Focus();

    // === Dispatch queued input (produced by ProcessWin32Message on the game
    // thread) on THIS thread so the View is only ever touched from one thread.
    DrainInputQueue();

    // Convert the UI click into a native toggle on this render thread. The
    // renderer-owned atomic remains the sole source of truth for visibility.
    ConsumeCaptionToggleRequest();

    // Apply any queued F11 caption-visibility change (render thread).
    ConsumePendingCaptionVisible();

    // === JS-driven close (Esc key / Close button) ===
    // The page plays the slide-out-to-the-right animation and then sets
    // window.__panelHidden = true. Poll it here (render thread) and finish
    // the close natively once the animation has completed.
    {
        ul::String js = m_View->EvaluateScript(
            ul::String("window.__panelHidden === true"));
        ul::String8 s8 = js.utf8();
        const char* p = s8.data() ? s8.data() : "";
        if (strstr(p, "true")) {
            RequestCloseUI();
            return;
        }
    }

    // === Save / Reset actions from the footer buttons ===
    // The page sets window.__saveRequested / window.__resetRequested when the
    // user clicks "حفظ" / "استعادة الافتراضي". Consume them here (render
    // thread) so the heavy work (config write, font reload, DOM refresh) runs
    // on the same thread that owns the shaper and the UI.
    {
        ul::String8 s8 = m_View->EvaluateScript(ul::String(
            "(window.__saveRequested ? 'save' : "
            "(window.__resetRequested ? 'reset' : ''))")).utf8();
        const char* p = s8.data() ? s8.data() : "";
        bool consumed = false;
        if (strstr(p, "save")) {
            // Pull a FRESH snapshot from the DOM (the JS click handler stores
            // the current control values in window.__state immediately), so the
            // config file matches exactly what the user sees even if the
            // periodic 30-frame snapshot poll has not run since the last slider
            // change.
            ul::String8 snap8 = m_View->EvaluateScript(ul::String(
                "(window.hlaConfig && window.hlaConfig.snapshot) ? "
                "window.hlaConfig.snapshot() : '{}'")).utf8();
            const char* snapPtr = snap8.data() ? snap8.data() : "";
            EnterCriticalSection(&Renderer::m_CS);
            ApplyConfigJson(snapPtr);
            SaveConfig(g_IniPath);
            Renderer::ReloadFont();   // apply custom_font_path / font_size now
            LeaveCriticalSection(&Renderer::m_CS);
            ULDBG("Save: config written to %ls + font reloaded", g_IniPath);
            consumed = true;
        } else if (strstr(p, "reset")) {
            // The dropdown updates JS immediately, while the periodic native
            // snapshot may still be a few frames away. Preserve the language
            // selected in the UI even when Reset is clicked right away.
            int languageToPreserve = g_Config.ui_language;
            ul::String8 language8 = m_View->EvaluateScript(ul::String(
                "String(window.__state && window.__state.ui_language || '')")).utf8();
            if (language8.data() && language8.data()[0]) {
                const int selectedLanguage = std::atoi(language8.data());
                if (selectedLanguage == 1 || selectedLanguage == 2) {
                    languageToPreserve = selectedLanguage;
                }
            }
            EnterCriticalSection(&Renderer::m_CS);
            ResetConfig();
            g_Config.ui_language = languageToPreserve;
            Renderer::ReloadFont();
            LeaveCriticalSection(&Renderer::m_CS);
            RefreshAllFromConfig(m_View.get());  // re-sync every DOM control
            ULDBG("Reset: defaults restored + font reloaded");
            consumed = true;
        }
        if (consumed) {
            m_View->EvaluateScript(ul::String(
                "window.hlaConfig && window.hlaConfig._clearFlags && "
                "window.hlaConfig._clearFlags()"));
        }
    }

    // === Preview handling (localized text, loop until toggled off) ===
    {
        ul::String8 enabled8 = m_View->EvaluateScript(ul::String("window.__previewEnabled===true?'1':'0'")).utf8();
        bool isEnabled = enabled8.data() && enabled8.data()[0]=='1';
        ul::String8 flag8 = m_View->EvaluateScript(ul::String("window.__previewRequested===true?'1':'0'")).utf8();
        bool isRequested = flag8.data() && flag8.data()[0]=='1';
        static bool s_LastEnabled = false;
        if (isEnabled && (!s_LastEnabled || isRequested)) {
            ul::String8 txt8 = m_View->EvaluateScript(ul::String("window.__previewText||''")).utf8();
            const char* fallback = g_Config.ui_language == 1
                ? "Sample caption preview"
                : "هذا نص تجريبي لمعاينة الترجمة";
            std::string curText = txt8.data() && txt8.data()[0] ? txt8.data() : fallback;
            Renderer::ShowPreview(curText, 9999.0f);
            ULDBG("Preview: ON text='%s'", curText.c_str());
        } else if (!isEnabled && s_LastEnabled) {
            Renderer::ClearPreview();
            ULDBG("Preview: OFF");
        } else if (isRequested) {
            ul::String8 txt8 = m_View->EvaluateScript(ul::String("window.__previewText||''")).utf8();
            std::string txt = txt8.data() ? txt8.data() : "";
            if (!txt.empty()) {
                Renderer::ShowPreview(txt, 5.0f);
            }
        }
        if (isRequested) {
            m_View->EvaluateScript(ul::String("window.__previewRequested=false;"));
        }
        s_LastEnabled = isEnabled;
        {
            ul::String8 clr8 = m_View->EvaluateScript(ul::String("window.__previewClear===true?'1':'0'")).utf8();
            if (clr8.data() && clr8.data()[0]=='1') {
                Renderer::ClearPreview();
                m_View->EvaluateScript(ul::String("window.__previewClear=false;"));
            }
        }
    }

    static int frame_counter = 0;
    if (frame_counter == 0) {
        ULDBG("Render: first draw (open)");
    }
    ++frame_counter;

    // Live JS polling: every 5 frames + immediate when dirty flag set
    {
        bool shouldPoll = (frame_counter % 5 == 0);
        if (!shouldPoll) {
            ul::String dirty = m_View->EvaluateScript(ul::String("window.__stateDirty===true"));
            ul::String8 d8 = dirty.utf8();
            const char* dp = d8.data() ? d8.data() : "";
            if (strstr(dp, "true")) shouldPoll = true;
        }
        if (shouldPoll) {
            ul::String json = m_View->EvaluateScript(
                ul::String("(window.hlaConfig && window.hlaConfig.snapshot) ? "
                           "window.hlaConfig.snapshot() : '{}'"));
            ul::String8 utf8 = json.utf8();
            const char* jsonPtr = utf8.data();
            if (!jsonPtr) jsonPtr = "";
            std::string s = jsonPtr;
            if (frame_counter == 5) ULDBG("first snapshot: %s", s.c_str());
            std::string oldCustom = g_Config.custom_font_path;
            std::string oldFallback = g_Config.fallback_font_path;
            EnterCriticalSection(&Renderer::m_CS);
            ApplyConfigJson(s);
            if (g_Config.custom_font_path != oldCustom || g_Config.fallback_font_path != oldFallback) {
                Renderer::ReloadFontPreserveQueue();
                ULDBG("Live font change: %s -> %s", oldCustom.c_str(), g_Config.custom_font_path.c_str());
            }
            LeaveCriticalSection(&Renderer::m_CS);
            m_View->EvaluateScript(ul::String("window.__stateDirty=false;"));
        }
    }

    // Drive timers / events / navigation. Required for WebKit to progress.
    m_Renderer->Update();

    // Skip if our D3D11 device was lost.
    if (!g_Device || !g_Context) {
        return;
    }

    // Step the renderer
    m_Renderer->RefreshDisplay(0);
    m_Renderer->Render();

    // Get CPU pixel buffer.
    ul::BitmapSurface* bs = static_cast<ul::BitmapSurface*>(m_View->surface());
    if (!bs) return;
    ul::RefPtr<ul::Bitmap> bitmap = bs->bitmap();
    if (!bitmap || bitmap->IsEmpty()) return;

    // Resize the persistent D3D11 texture if the bitmap dims changed.
    if (!ultralight_blit::EnsureTexture(g_Device, g_Context,
                                       bitmap->width(), bitmap->height())) {
        return;
    }

    // Fill the texture with the latest CPU pixels.
    if (!ultralight_blit::UpdateBitmapFromUG(g_Context, bitmap)) {
        return;
    }

    // Draw the textured fullscreen quad onto the currently-bound render target.
    ultralight_blit::BeginFrame(g_Context);
    ultralight_blit::DrawFullscreenQuad(g_Context, m_Width, m_Height, 1.0f);

    // Restore the game's pipeline state after the UI pass.
    ultralight_blit::EndFrame(g_Context);
}

bool UltralightManager::ProcessWin32Message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (!m_Initialized || !m_View) return false;

    static int msg_count = 0;
    if (msg_count < 50) {
        ULDBG("Msg #%d: uMsg=0x%x wParam=0x%lx lParam=0x%lx", msg_count, msg, (long)wParam, (long)lParam);
        ++msg_count;
    }

    // Only forward mouse + keyboard events
    bool isMouseEvt = (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST);
    bool isKeyEvt   = (msg >= WM_KEYFIRST && msg <= WM_KEYLAST);
    if (!isMouseEvt && !isKeyEvt) return false;

    // Queue the event instead of calling into the View directly: this runs on
    // the game's message thread, while Update()/Render() run on the present
    // thread. WebKit is not thread-safe for concurrent View access, so all
    // View calls happen on the render thread (inside Render -> DrainInputQueue).
    {
        std::lock_guard<std::mutex> lock(m_EventMutex);
        m_EventQueue.push_back({ msg, wParam, lParam });
        if (m_EventQueue.size() > 256) m_EventQueue.pop_front();  // drop oldest on overflow
    }
    return true;
}

void UltralightManager::DrainInputQueue() {
    if (!m_View) return;

    std::deque<PendingEvent> events;
    {
        std::lock_guard<std::mutex> lock(m_EventMutex);
        events.swap(m_EventQueue);
    }
    for (const auto& e : events)
        DispatchInputMessage(e.msg, e.wParam, e.lParam);
}

void UltralightManager::DispatchInputMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    // Only called from the render thread (via DrainInputQueue), so the static
    // mouse-position cache and the View itself are safe.
    static int mx = 0, my = 0;
    switch (msg) {
        case WM_MOUSEMOVE: {
            mx = LOWORD(lParam);
            my = HIWORD(lParam);
            ul::MouseEvent e;
            e.type = ul::MouseEvent::kType_MouseMoved;
            e.x = mx;
            e.y = my;
            e.button = ul::MouseEvent::kButton_None;
            m_View->FireMouseEvent(e);
            return;
        }
        case WM_LBUTTONDOWN: {
            // TEMP DIAGNOSTIC: log focus state at each mousedown (first 20).
            { static int d = 0; if (d < 20) { ++d; ULDBG("mousedown: HasFocus-before=%d at (%d,%d)", m_View->HasFocus() ? 1 : 0, mx, my); } }
            ul::MouseEvent e;
            e.type = ul::MouseEvent::kType_MouseDown;
            e.x = mx; e.y = my;
            e.button = ul::MouseEvent::kButton_Left;
            m_View->FireMouseEvent(e);
            return;
        }
        case WM_LBUTTONUP: {
            ul::MouseEvent e;
            e.type = ul::MouseEvent::kType_MouseUp;
            e.x = mx; e.y = my;
            e.button = ul::MouseEvent::kButton_Left;
            m_View->FireMouseEvent(e);
            return;
        }
        case WM_RBUTTONDOWN: {
            ul::MouseEvent e;
            e.type = ul::MouseEvent::kType_MouseDown;
            e.x = mx; e.y = my;
            e.button = ul::MouseEvent::kButton_Right;
            m_View->FireMouseEvent(e);
            return;
        }
        case WM_RBUTTONUP: {
            ul::MouseEvent e;
            e.type = ul::MouseEvent::kType_MouseUp;
            e.x = mx; e.y = my;
            e.button = ul::MouseEvent::kButton_Right;
            m_View->FireMouseEvent(e);
            return;
        }
        case WM_MOUSEWHEEL: {
            ul::ScrollEvent e;
            e.type = ul::ScrollEvent::kType_ScrollByPixel;
            e.delta_x = 0;
            e.delta_y = -((short)HIWORD(wParam)) / 2;
            m_View->FireScrollEvent(e);
            return;
        }
        case WM_KEYDOWN: {
            ul::KeyEvent ke;
            ke.type = ul::KeyEvent::kType_RawKeyDown;
            ke.virtual_key_code = (uint32_t)wParam;
            ke.native_key_code = 0;
            ke.modifiers = 0;
            m_View->FireKeyEvent(ke);
            return;
        }
        case WM_KEYUP: {
            ul::KeyEvent ke;
            ke.type = ul::KeyEvent::kType_KeyUp;
            ke.virtual_key_code = (uint32_t)wParam;
            ke.native_key_code = 0;
            ke.modifiers = 0;
            m_View->FireKeyEvent(ke);
            return;
        }
        case WM_CHAR: {
            ul::KeyEvent ke;
            ke.type = ul::KeyEvent::kType_Char;
            ke.text = ul::String((char*)&wParam, 1);
            m_View->FireKeyEvent(ke);
            return;
        }
    }
}

void UltralightManager::OnDOMReady(ul::View* caller, uint64_t frame_id,
                                    bool is_main_frame, const ul::String& url) {
    if (!caller) return;
    ULDBG("OnDOMReady: frame_id=%llu is_main=%d url=%s", (unsigned long long)frame_id,
          (int)is_main_frame, url.utf8().data());

    // Diagnostics: prove the DOM actually loaded with our content.
    ul::String title   = caller->EvaluateScript(ul::String("document.title"));
    ul::String bodyLen = caller->EvaluateScript(ul::String(
        "(document.body && document.body.innerHTML.length) || -1"));
    ul::String childs  = caller->EvaluateScript(ul::String(
        "(document.documentElement && document.documentElement.children.length) || -1"));
    ul::String sect    = caller->EvaluateScript(ul::String(
        "(document.querySelectorAll) ? document.querySelectorAll('section').length : -1"));
    ul::String bg      = caller->EvaluateScript(ul::String(
        "window.getComputedStyle(document.body).backgroundColor"));
    ULDBG("DOM diag: title='%s' bodyLen=%s rootChildren=%s sections=%s bodyBg=%s",
          title.utf8().data(),
          bodyLen.utf8().data(),
          childs.utf8().data(),
          sect.utf8().data(),
          bg.utf8().data());

    RefreshAllFromConfig(caller);
}

void UltralightManager::ApplyConfigField(const char* key, float v) {
    ::ApplyConfigField(key, v);
}
void UltralightManager::ApplyConfigFieldBool(const char* key, bool v) {
    ::ApplyConfigFieldBool(key, v);
}
void UltralightManager::ApplyConfigFieldString(const char* key, const char* v) {
    ::ApplyConfigFieldString(key, v);
}
void UltralightManager::RequestSaveConfig() { SaveConfigToFile(); }
void UltralightManager::RequestCloseUI() {
    m_Open = false;
    if (m_View) m_View->Unfocus();
    if (Renderer::m_SettingsOpen) {
        Renderer::m_SettingsOpen = false;
        ShowCursor(TRUE);
    }
}
