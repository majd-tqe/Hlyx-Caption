/*
 * Hlyx Caption — original project source
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Hlyx Caption Contributors
 */

#pragma once
#include <d3d11.h>
#include <atomic>
#include <string>
#include <vector>
#include <Windows.h>
#include "config.h"
#include "imgui.h"

class TextShaper;

double GetTimeQPC();
float GetScaledFontSize(float baseSize);

struct CaptionRun {
    std::string text;
    uint8_t r = 255, g = 255, b = 255, a = 255;
    bool bold = false;
    bool italic = false;
};

struct CaptionLine {
    std::vector<CaptionRun> runs;
};

struct CaptionPhrase {
    std::vector<CaptionLine> lines;
    float delay = 0.0f;
    float duration = 0.0f; // 0 = stays visible once active
};

struct CaptionEntry {
    std::vector<CaptionPhrase> phrases;
    float duration = 0.0f;
    double startQPC = 0.0;
    float opacity = 1.0f;
    bool expired = false;
    bool fromPlayer = false;

    double visualY = 0.0;
    double targetY = 0.0;
    int pixelHeight = 0;
    int pixelWidth = 0;

    ID3D11Texture2D* texture = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    void* texID = nullptr;

    int lastActivePhraseCount = 0;
    int lastActivePhraseSig = 0;
    double lastQPC = 0.0;

    bool isPreview = false;

    float renderedFontSize = 0.0f;
    float renderedLineSpacing = 0.0f;
    uint8_t renderedTextR = 0, renderedTextG = 0, renderedTextB = 0, renderedTextA = 0;
    float renderedMaxLineWidthPercent = 0.0f;
    OverlayConfig::Alignment renderedAlignment = OverlayConfig::ALIGN_CENTER;
    std::string renderedCustomFontPath;
    std::string renderedFallbackFontPath;
    double lastRebuildQPC = 0.0;

    void ReleaseTexture();
};

class Renderer {
public:
    static bool Initialize();
    static void Shutdown();
    static void SetCaptionText(const std::string& text, float duration, bool fromPlayer = false);
    static bool InitImGui(IDXGISwapChain* pSwapChain);
    static bool InitStandalone(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);
    static void ReloadFont();
    static void ReloadFontPreserveQueue();
    static void DrawCaptions();
    static void ShowPreview(const std::string& text, float duration = 9999.0f);
    static void ClearPreview();
    static bool HasPreview();
    // Toggle the renderer-owned caption visibility state and synchronize the UI.
    // Callers may be the game message thread or the render thread.
    static void ToggleCaptionVisibility();
    static void DrawSettingsWindow();
    static void DrawBackgroundBox(ImDrawList* dl, ImVec2 imgPos, ImVec2 imgSize, float opacity);

    static std::atomic<bool> m_ImguiInitialized;
    // F10 toggles this from the message thread while rendering reads it.
    static std::atomic<bool> m_SettingsOpen;
    static std::atomic<bool> m_OverlayVisible;    // F11: overlay visibility toggle
    static ID3D11Device* m_Device;
    static ID3D11DeviceContext* m_Context;
    static ID3D11RenderTargetView* m_BackBufferRTV;
    static HWND m_Window;
    static WNDPROC m_OriginalWndProc;
    static CRITICAL_SECTION m_CS;
    // The caption hook can run before the first Present. Keep the lock alive
    // for the whole DLL lifetime instead of relying on Renderer::Initialize.
    static std::atomic<bool> m_CSInitialized;
    static std::atomic<bool> m_ShuttingDown;

    // Called before any game hook is enabled.
    static void EnsureSynchronization();

    static std::vector<CaptionEntry> m_Queue;
    static void UpdateQueue(float dt);

private:
    static bool HookPresent();
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static void RenderEntryTexture(CaptionEntry& entry, int maxPixelWidth = 0);
    static int CalcActivePhraseCount(CaptionEntry& entry);

    static std::atomic<bool> m_Initialized;
    static TextShaper* m_Shaper;
    static double m_AnimSpeed;
};
