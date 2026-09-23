/*
 * Hlyx Caption — original project source
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Hlyx Caption Contributors
 */

// UltralightManager.h - Settings panel using Ultralight HTML/CSS/JS engine.
// Approach: CPU render -> BGRA Bitmap -> D3D11 Texture2D -> blit to backbuffer.
#pragma once

#include <d3d11.h>
#include <Ultralight/RefPtr.h>
#include <Ultralight/Listener.h>
#include <mutex>
#include <deque>
#include <atomic>

namespace ultralight {
class Logger;
class Renderer;
class View;
}

class UltralightManager : public ultralight::LoadListener {
public:
    static UltralightManager& Get();

    // Initialize with existing D3D11 device/context.
    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context,
                   int width, int height);

    // Render settings panel to backbuffer (must be already bound as render target).
    void Render();

    // Forward Win32 message to the view (mouse, keyboard, resize).
    bool ProcessWin32Message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Lifecycle helpers
    void Resize(int width, int height);
    void Shutdown();
    bool IsInitialized() const { return m_Initialized.load(std::memory_order_acquire); }
    void SetOpen(bool open) { m_Open.store(open, std::memory_order_release); }
    bool IsOpen() const { return m_Open.load(std::memory_order_acquire); }

    // Called from the render thread whenever the panel is toggled OPEN
    // (F10): replays the CSS slide-in-from-the-right animation in the page.
    void OnPanelOpened();

    // Queue an F11 caption-visibility change coming from the game's message
    // thread (WndProc). The View is only ever touched on the render thread,
    // so the value is stored here and applied inside Render().
    void RequestCaptionVisibleChange(bool visible);

    // LoadListener override
    virtual void OnDOMReady(ultralight::View* caller, uint64_t frame_id,
                            bool is_main_frame, const ultralight::String& url) override;

private:
    UltralightManager() = default;
    ~UltralightManager() = default;
    UltralightManager(const UltralightManager&) = delete;
    UltralightManager& operator=(const UltralightManager&) = delete;

    // A Win32 message (mouse/keyboard) queued from the game's message thread
    // and dispatched later on the render thread (inside Render), so the
    // Ultralight View is only ever touched from a single thread.
    struct PendingEvent {
        UINT   msg;
        WPARAM wParam;
        LPARAM lParam;
    };

    void DispatchInputMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    void DrainInputQueue();

    // Apply a queued F11 caption-visibility change to the page (render thread).
    void ConsumePendingCaptionVisible();

    // Consume the one-shot request raised by the settings button and hand the
    // actual state change to Renderer (the renderer remains the source of truth).
    void ConsumeCaptionToggleRequest();

    // JS->C++ field setters
    void ApplyConfigField(const char* key, float v);
    void ApplyConfigFieldBool(const char* key, bool v);
    void ApplyConfigFieldString(const char* key, const char* v);

    // Native actions triggered from JS
    void RequestSaveConfig();
    void RequestCloseUI();

    std::atomic<bool> m_Initialized{false};
    std::atomic<bool> m_Open{false};

    // Input queue (producer: game thread, consumer: render thread)
    std::mutex             m_EventMutex;
    std::deque<PendingEvent> m_EventQueue;

    // Number of warmup Update() calls done while the panel was closed.
    int m_WarmupCounter = 0;

    // True once the startup banner's fade-out animation has completed
    // (window.__bannerDone). After that the closed panel is not rendered
    // at all, restoring the original zero-cost closed state.
    bool m_BannerFinished = false;

    // True once BannerConfig (banner_config.h) has been injected into the
    // page. One-shot; retried each Render tick until the DOM is ready.
    bool m_BannerCfgApplied = false;

    // Pending F11 caption-visibility change (producer: game message thread,
    // consumer: render thread inside Render()).
    bool m_PendingOverlayVisible = false;
    bool m_PendingOverlayVisibleValue = true;

    ultralight::Logger*  m_Logger = nullptr;
    ultralight::RefPtr<ultralight::Renderer> m_Renderer;
    ultralight::RefPtr<ultralight::View>    m_View;

    // Off-screen texture that mirrors the View bitmap
    ID3D11Texture2D*          m_BitmapTexture = nullptr;
    ID3D11ShaderResourceView* m_BitmapSRV = nullptr;
    uint32_t                  m_BitmapTexW = 0;
    uint32_t                  m_BitmapTexH = 0;

    int m_Width = 0;
    int m_Height = 0;

    HWND m_Hwnd = nullptr;
};
