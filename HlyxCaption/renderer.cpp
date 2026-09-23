/*
 * Hlyx Caption — original project source
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Hlyx Caption Contributors
 */

#include "renderer.h"
#include "config.h"
#include "shaper.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"
#include "MinHook.h"
#include "ui_ultralight/UltralightManager.h"
#include "ui_ultralight/UL_Debug.h"
#include <cmath>
#ifdef _DEBUG
#include <iostream>
#endif

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

typedef HRESULT(__stdcall* tPresent)(IDXGISwapChain*, UINT, UINT);
typedef HRESULT(__stdcall* tResize)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

tPresent oPresent = nullptr;
tResize oResize = nullptr;

typedef BOOL(WINAPI* tSetCursorPos)(int, int);
typedef BOOL(WINAPI* tClipCursor)(const RECT*);
typedef HWND(WINAPI* tSetCapture)(HWND);

tSetCursorPos oSetCursorPos = nullptr;
tClipCursor    oClipCursor    = nullptr;
tSetCapture    oSetCapture    = nullptr;

// Software mouse position - we track this ourselves via Raw Input
// Never rely on OS cursor position (game bypasses SetCursorPos hook via ntdll syscall)
static std::atomic<float> g_SoftMouseX{0.0f};
static std::atomic<float> g_SoftMouseY{0.0f};

// Cached display size, refreshed every frame on the render thread.
// Used by the game-thread WndProc for software-mouse math WITHOUT touching
// ImGui (ImGui context belongs to the render thread).
static std::atomic<float> g_DisplayWidth{0.0f};
static std::atomic<float> g_DisplayHeight{0.0f};

// =====================================================================
// User32 hooks for mouse capture (prevent game from re-capturing mouse)
// =====================================================================
//
// When the settings panel is open, the game must not be allowed to
// re-capture the cursor. These hooks intercept the user32 calls that
// HLA uses every frame and silently fail them, keeping the cursor
// free for the HTML UI.

BOOL WINAPI hkSetCursorPos(int X, int Y) {
    if (Renderer::m_SettingsOpen) {
        // Block the game from recentering the cursor - the synthetic
        // WM_MOUSEMOVE that we inject from g_SoftMouseX/Y controls
        // position now.
        return TRUE;
    }
    return oSetCursorPos(X, Y);
}

BOOL WINAPI hkClipCursor(const RECT* lpRect) {
    if (Renderer::m_SettingsOpen) {
        // Block the game from clipping the cursor to its viewport rect.
        return TRUE;
    }
    return oClipCursor(lpRect);
}

HWND WINAPI hkSetCapture(HWND hWnd) {
    if (Renderer::m_SettingsOpen) {
        // Block the game from capturing mouse input - we want all
        // mouse events to flow to the WndProc handler for Ultralight.
        return nullptr;
    }
    return oSetCapture(hWnd);
}

std::atomic<bool> Renderer::m_Initialized = false;
std::atomic<bool> Renderer::m_ImguiInitialized = false;
std::atomic<bool> Renderer::m_SettingsOpen{false};
std::atomic<bool> Renderer::m_OverlayVisible{true};
ID3D11Device* Renderer::m_Device = nullptr;
ID3D11DeviceContext* Renderer::m_Context = nullptr;
ID3D11RenderTargetView* Renderer::m_BackBufferRTV = nullptr;
HWND Renderer::m_Window = nullptr;
WNDPROC Renderer::m_OriginalWndProc = nullptr;

CRITICAL_SECTION Renderer::m_CS;
std::atomic<bool> Renderer::m_CSInitialized{false};
std::atomic<bool> Renderer::m_ShuttingDown{false};
std::vector<CaptionEntry> Renderer::m_Queue;

TextShaper* Renderer::m_Shaper = nullptr;
double Renderer::m_AnimSpeed = 8.0;

static IDXGISwapChain* g_SwapChain = nullptr;
static int g_FrameCount = 0;
static LARGE_INTEGER g_QPCFreq = {};
static LARGE_INTEGER g_QPCStart = {};
static double g_LastFrameQPC = 0.0;

double GetTimeQPC() {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    if (g_QPCFreq.QuadPart <= 0) return 0.0;
    return (double)(now.QuadPart - g_QPCStart.QuadPart) / g_QPCFreq.QuadPart;
}

float GetScaledFontSize(float baseSize) {
    double screenH = 0.0;
    if (Renderer::m_ImguiInitialized) {
        ImGuiIO& io = ImGui::GetIO();
        screenH = io.DisplaySize.y;
    }
    if (screenH <= 0) screenH = GetSystemMetrics(SM_CYSCREEN);
    const float reference = (std::isfinite(g_Config.font_size_reference_height) &&
                             g_Config.font_size_reference_height > 1.0f)
        ? g_Config.font_size_reference_height : 1080.0f;
    return baseSize * (float)(screenH / reference);
}

void Renderer::EnsureSynchronization() {
    m_ShuttingDown.store(false, std::memory_order_release);
    bool expected = false;
    if (m_CSInitialized.compare_exchange_strong(expected, true,
                                                std::memory_order_acq_rel)) {
        InitializeCriticalSection(&m_CS);
    }
    if (g_QPCFreq.QuadPart <= 0) {
        QueryPerformanceFrequency(&g_QPCFreq);
        QueryPerformanceCounter(&g_QPCStart);
        g_LastFrameQPC = GetTimeQPC();
    }
}

void CaptionEntry::ReleaseTexture() {
    if (texID) {
        if (srv) { srv->Release(); srv = nullptr; }
        if (texture) { texture->Release(); texture = nullptr; }
        texID = nullptr;
        pixelWidth = 0;
        pixelHeight = 0;
    }
}

static std::string FindArabicFont() {
    const char* candidates[] = {
        "C:\\Windows\\Fonts\\tahoma.ttf",
        "C:\\Windows\\Fonts\\segoeui.ttf",
        "C:\\Windows\\Fonts\\seguihis.ttf",
    };
    for (auto& c : candidates) {
        if (GetFileAttributesA(c) != INVALID_FILE_ATTRIBUTES) {
#ifdef _DEBUG
            std::cout << "[Font] Candidate exists: " << c << "\n";
#endif
            return c;
        }
    }
    return "";
}

// Resolve a font path from the config: existing/absolute paths are used as-is
// (backward compatibility with old INI values), bare file names (e.g.
// "Cairo-Regular.ttf" — the value stored by the settings dropdowns) resolve to
// {ModDir}/resources. Returns an empty string if nothing matched; callers
// then fall back to FindArabicFont().
static std::string ResolveFontPath(const std::string& cfg) {
    if (cfg.empty()) return cfg;
    char selfPath[MAX_PATH] = {};
    HMODULE hm = GetModuleHandleA("wininet.dll");
    if (hm) GetModuleFileNameA(hm, selfPath, MAX_PATH);
    char* slash = selfPath[0] ? strrchr(selfPath, '\\') : nullptr;
    if (slash) *(slash + 1) = 0;

    auto isInResources = [&](const std::string& candidate) {
        char full[MAX_PATH] = {};
        if (!GetFullPathNameA(candidate.c_str(), MAX_PATH, full, nullptr)) return false;
        std::string root = std::string(selfPath) + "resources";
        char fullRoot[MAX_PATH] = {};
        if (!GetFullPathNameA(root.c_str(), MAX_PATH, fullRoot, nullptr)) return false;
        const size_t rootLen = strlen(fullRoot);
        return _strnicmp(full, fullRoot, rootLen) == 0 &&
               (full[rootLen] == '\\' || full[rootLen] == '/' || full[rootLen] == '\0');
    };

    // Only load fonts from the mod's resources directory. This prevents an
    // INI value from turning the font loader into an arbitrary file reader.
    if (GetFileAttributesA(cfg.c_str()) != INVALID_FILE_ATTRIBUTES &&
        isInResources(cfg)) return cfg;
    if (cfg.find_first_of("\\/:*") != std::string::npos || cfg.find("..") != std::string::npos)
        return "";
    std::string p = std::string(selfPath) + "resources\\" + cfg;
    if (GetFileAttributesA(p.c_str()) != INVALID_FILE_ATTRIBUTES) return p;
    return "";
}

// =====================================================================
// REAL user32.dll arrow rendered as a D3D11 texture.
//
// The game owns the real OS cursor (it keeps yanking it to the screen
// center via NtUserSetCursorPos, which bypasses our SetCursorPos hook),
// so any visible OS cursor trembles/fights with the game. Instead we:
//   1) hide the OS cursor (blank 1x1 cursor),
//   2) load the REAL arrow from user32.dll (LoadCursor(IDC_ARROW)) and
//      convert it (GetIconInfo -> GetBitmapBits) into a D3D11 texture,
//   3) draw it at the software-tracked position (g_SoftMouseX/Y) - the
//      exact same arrow shape the OS would show, but perfectly smooth
//      because nothing can move it except our Raw-Input tracking.
// =====================================================================
static HCURSOR g_BlankCursor = nullptr;          // transparent cursor to hide the OS one
static ID3D11Texture2D*          g_CursorTex = nullptr;
static ID3D11ShaderResourceView* g_CursorSRV = nullptr;
static int g_CursorW = 0, g_CursorH = 0;
static int g_CursorHotX = 0, g_CursorHotY = 0;

static void EnsureCursorTexture() {
    if (g_CursorSRV || !Renderer::m_Device) return;

    // The REAL arrow, loaded from user32.dll.
    HCURSOR hCur = LoadCursorW(nullptr, IDC_ARROW);
    if (!hCur) return;

    ICONINFO ii = {};
    if (!GetIconInfo(hCur, &ii)) return;

    BITMAP bm = {};
    if (ii.hbmColor && GetObjectW(ii.hbmColor, sizeof(bm), &bm) &&
        bm.bmWidth > 0 && bm.bmHeight != 0 && bm.bmBitsPixel == 32) {
        const int w = bm.bmWidth;
        const int h = (bm.bmHeight < 0) ? -bm.bmHeight : bm.bmHeight;

        // Force TOP-DOWN row order with GetDIBits (biHeight negative). This is
        // deterministic on every Windows version/DPI - unlike GetBitmapBits,
        // which returns the raw internal storage order (often bottom-up) and
        // made the arrow render upside down.
        std::vector<BYTE> pixels(size_t(w) * h * 4);

        BITMAPINFO bi = {};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = w;
        bi.bmiHeader.biHeight = -h;      // negative -> top-down (row 0 = top of image)
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;

        int gotLines = 0;
        HDC hdc = GetDC(nullptr);
        if (hdc) {
            gotLines = GetDIBits(hdc, ii.hbmColor, 0, h, pixels.data(), &bi, DIB_RGB_COLORS);
            ReleaseDC(nullptr, hdc);
        }

        if (gotLines == h) {
            // Some cursors carry 0 alpha and rely on the 1bpp AND mask instead.
            bool hasAlpha = false;
            for (size_t i = 3; i < pixels.size(); i += 4) {
                if (pixels[i] != 0) { hasAlpha = true; break; }
            }
            if (!hasAlpha && ii.hbmMask) {
                BITMAP mb = {};
                if (GetObjectW(ii.hbmMask, sizeof(mb), &mb) && mb.bmWidth >= w) {
                    const int mh = (mb.bmHeight < 0) ? -mb.bmHeight : mb.bmHeight;
                    const int mStride = ((mb.bmWidth + 31) / 32) * 4;  // 1bpp rows pad to 32 bits
                    std::vector<BYTE> mask(size_t(mStride) * mh, 0);

                    BITMAPINFO mbi = {};
                    mbi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                    mbi.bmiHeader.biWidth = mb.bmWidth;
                    mbi.bmiHeader.biHeight = -mh;   // top-down, same convention
                    mbi.bmiHeader.biPlanes = 1;
                    mbi.bmiHeader.biBitCount = 1;
                    mbi.bmiHeader.biCompression = BI_RGB;

                    int gotMask = 0;
                    HDC mhdc = GetDC(nullptr);
                    if (mhdc) {
                        gotMask = GetDIBits(mhdc, ii.hbmMask, 0, mh, mask.data(), &mbi, DIB_RGB_COLORS);
                        ReleaseDC(nullptr, mhdc);
                    }
                    if (gotMask == mh) {
                        for (int y = 0; y < h; y++) {
                            for (int x = 0; x < w; x++) {
                                const size_t byteIdx = size_t(y) * mStride + x / 8;
                                const BYTE bit = BYTE(0x80 >> (x % 8));
                                pixels[size_t(y) * w * 4 + size_t(x) * 4 + 3] =
                                    (mask[byteIdx] & bit) ? 0 : 255;
                            }
                        }
                    }
                }
            }

            D3D11_TEXTURE2D_DESC td = {};
            td.Width = (UINT)w;
            td.Height = (UINT)h;
            td.MipLevels = 1;
            td.ArraySize = 1;
            td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            td.SampleDesc.Count = 1;
            td.Usage = D3D11_USAGE_DEFAULT;
            td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            D3D11_SUBRESOURCE_DATA sr = {};
            sr.pSysMem = pixels.data();
            sr.SysMemPitch = (UINT)(w * 4);

            if (SUCCEEDED(Renderer::m_Device->CreateTexture2D(&td, &sr, &g_CursorTex))) {
                D3D11_SHADER_RESOURCE_VIEW_DESC svd = {};
                svd.Format = td.Format;
                svd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                svd.Texture2D.MipLevels = 1;
                if (SUCCEEDED(Renderer::m_Device->CreateShaderResourceView(g_CursorTex, &svd, &g_CursorSRV))) {
                    g_CursorW = w;
                    g_CursorH = h;
                    g_CursorHotX = ii.xHotspot;
                    g_CursorHotY = ii.yHotspot;
                    UL_LogToFile("[CURSOR] real user32.dll arrow texture: %dx%d hotspot=(%d,%d)",
                                 w, h, ii.xHotspot, ii.yHotspot);
                } else {
                    g_CursorTex->Release();
                    g_CursorTex = nullptr;
                }
            }
        }
    }

    if (ii.hbmColor) DeleteObject(ii.hbmColor);
    if (ii.hbmMask) DeleteObject(ii.hbmMask);
}

// Draw the real-arrow texture at the tracked position. Second ImGui pass so it
// sits ABOVE the Ultralight HTML panel (which is drawn after the first pass).
static void DrawRealCursor() {
    if (!Renderer::m_SettingsOpen) return;
    const float displayW = g_DisplayWidth.load(std::memory_order_acquire);
    const float displayH = g_DisplayHeight.load(std::memory_order_acquire);
    if (displayW <= 0.0f || displayH <= 0.0f) return;

    EnsureCursorTexture();
    if (!g_CursorSRV) return;  // texture failed: no cursor (rare)

    const float x = g_SoftMouseX.load(std::memory_order_acquire) - (float)g_CursorHotX;
    const float y = g_SoftMouseY.load(std::memory_order_acquire) - (float)g_CursorHotY;

    ImGui::NewFrame();
    ImDrawList* dl = ImGui::GetForegroundDrawList();

    // The real arrow, at native size. ONE image only (a shadow copy made the
    // cursor look duplicated on top of itself).
    dl->AddImage((ImTextureID)g_CursorSRV,
                 ImVec2(x, y),
                 ImVec2(x + g_CursorW, y + g_CursorH),
                 ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    static bool s_Logged = false;
    if (!s_Logged) {
        s_Logged = true;
        UL_LogToFile("[CURSOR] drawing real arrow at (%.0f, %.0f)", x, y);
    }
}

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    g_FrameCount++;

    if (!Renderer::m_ImguiInitialized) {
        Renderer::InitImGui(pSwapChain);
        g_SwapChain = pSwapChain;
    }

    if (Renderer::m_ImguiInitialized) {
        if (g_SwapChain != pSwapChain) {
            if (Renderer::m_BackBufferRTV) {
                Renderer::m_BackBufferRTV->Release();
                Renderer::m_BackBufferRTV = nullptr;
            }
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui_ImplDX11_InvalidateDeviceObjects();
            Renderer::m_ImguiInitialized = false;
            Renderer::InitImGui(pSwapChain);
            g_SwapChain = pSwapChain;
        }

        // --- Calculate dt and UpdateQueue under CS ---
        double now = GetTimeQPC();
        float dt = (float)(now - g_LastFrameQPC);
        g_LastFrameQPC = now;
        EnterCriticalSection(&Renderer::m_CS);
        Renderer::UpdateQueue(dt);
        // Cache display size (ImGui IO is render-thread only, but we are on render thread)
        float cachedW = 0, cachedH = 0;
        {
            // Ensure IO exists before reading
            ImGuiIO& io = ImGui::GetIO();
            cachedW = io.DisplaySize.x;
            cachedH = io.DisplaySize.y;
            if (cachedW <= 0 || cachedH <= 0) {
                DXGI_SWAP_CHAIN_DESC d{};
                if (SUCCEEDED(pSwapChain->GetDesc(&d))) {
                    cachedW = (float)d.BufferDesc.Width;
                    cachedH = (float)d.BufferDesc.Height;
                }
            }
        }
        LeaveCriticalSection(&Renderer::m_CS);

        // --- Single ImGui frame for the desktop backbuffer ---
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        EnterCriticalSection(&Renderer::m_CS);
        Renderer::DrawCaptions();

        // Cache display size for the game-thread WndProc (software-mouse math
        // must not touch ImGui from the message thread).
        {
            g_DisplayWidth.store(cachedW, std::memory_order_release);
            g_DisplayHeight.store(cachedH, std::memory_order_release);
        }

        // Track settings toggle for animation edge detection.
        // IMPORTANT: all ImGui IO (MousePos / MouseDrawCursor) is applied HERE
        // on the render thread - the game's WndProc (message thread) only flips
        // the state flags and never touches ImGui (not thread-safe).
        static bool s_PrevSettingsOpen = false;
        if (Renderer::m_SettingsOpen != s_PrevSettingsOpen) {
            bool opened = Renderer::m_SettingsOpen;
            s_PrevSettingsOpen = Renderer::m_SettingsOpen;

            ImGuiIO& io = ImGui::GetIO();
            if (opened) {
                // Initialize software cursor at screen center
                if (io.DisplaySize.x > 0.0f && io.DisplaySize.y > 0.0f) {
                    g_SoftMouseX.store(io.DisplaySize.x * 0.5f, std::memory_order_release);
                    g_SoftMouseY.store(io.DisplaySize.y * 0.5f, std::memory_order_release);
                }
                io.MousePos = ImVec2(g_SoftMouseX.load(std::memory_order_acquire),
                                     g_SoftMouseY.load(std::memory_order_acquire));
                // The REAL OS cursor (user32.dll arrow) is shown while the
                // panel is open - ImGui's built-in drawn cursor stays disabled.
                io.MouseDrawCursor = false;

                // Replay the CSS slide-in-from-the-right animation in the
                // HTML settings panel (render thread -> EvaluateScript safe).
                UltralightManager::Get().OnPanelOpened();
            } else {
                io.MouseDrawCursor = false;
            }
        }

        // While settings are open keep ImGui's mouse pos in sync with the
        // software cursor (updated on the game thread via Raw Input).
        if (Renderer::m_SettingsOpen) {
            ImGui::GetIO().MousePos = ImVec2(g_SoftMouseX.load(std::memory_order_acquire),
                                             g_SoftMouseY.load(std::memory_order_acquire));
        }

        Renderer::DrawSettingsWindow();

        // Cache display size for WndProc (must be inside CS copy, but set outside)
        g_DisplayWidth.store(cachedW, std::memory_order_release);
        g_DisplayHeight.store(cachedH, std::memory_order_release);

        LeaveCriticalSection(&Renderer::m_CS);

        // Continuously release mouse capture while settings window is open
        if (Renderer::m_SettingsOpen) {
            HWND foreground = GetForegroundWindow();
            if (foreground == Renderer::m_Window || foreground == nullptr) {
                ::ReleaseCapture();
                if (oClipCursor) oClipCursor(nullptr);
            }
        }

        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();

        // Render the caption draw data over the game's backbuffer.
        EnterCriticalSection(&Renderer::m_CS);
        if (drawData) {
            ID3D11RenderTargetView* rtvs2[1] = { Renderer::m_BackBufferRTV };
            Renderer::m_Context->OMSetRenderTargets(1, rtvs2, nullptr);
            ImGui_ImplDX11_RenderDrawData(drawData);
        }
        LeaveCriticalSection(&Renderer::m_CS);

        // Rebind the backbuffer RTV before drawing the HTML settings panel.
        if (Renderer::m_BackBufferRTV) {
            ID3D11RenderTargetView* rtvs[1] = { Renderer::m_BackBufferRTV };
            Renderer::m_Context->OMSetRenderTargets(1, rtvs, nullptr);
        }

        // Ultralight renders the HTML/CSS settings panel on top of ImGui.
        if (Renderer::m_BackBufferRTV) {
            UltralightManager::Get().Render();
        }

        // Draw the REAL user32.dll arrow (as a D3D texture) ABOVE the HTML
        // panel at the software-tracked position. The OS cursor itself is
        // hidden because the game owns it and keeps warping it to the center.
        DrawRealCursor();

    }

#ifdef _DEBUG
    if (g_FrameCount % 120 == 0) {
        std::cout << "[Present] Frame " << g_FrameCount
                  << " | Queue=" << Renderer::m_Queue.size() << "\n";
    }
#endif

    return oPresent(pSwapChain, SyncInterval, Flags);
}

HRESULT __stdcall hkResize(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
    if (Renderer::m_ImguiInitialized && g_SwapChain == pSwapChain) {
        EnterCriticalSection(&Renderer::m_CS);
        if (Renderer::m_BackBufferRTV) {
            Renderer::m_BackBufferRTV->Release();
            Renderer::m_BackBufferRTV = nullptr;
        }
        ImGui_ImplDX11_InvalidateDeviceObjects();
        HRESULT hr = oResize(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
        ImGui_ImplDX11_CreateDeviceObjects();
        ID3D11Texture2D* bb = nullptr;
        if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb))) {
            Renderer::m_Device->CreateRenderTargetView(bb, nullptr, &Renderer::m_BackBufferRTV);
            bb->Release();
            UltralightManager::Get().Resize((int)Width, (int)Height);
        }
        LeaveCriticalSection(&Renderer::m_CS);
        return hr;
    }
    return oResize(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

bool Renderer::HookPresent() {
    QueryPerformanceFrequency(&g_QPCFreq);
    QueryPerformanceCounter(&g_QPCStart);
    g_LastFrameQPC = GetTimeQPC();

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"HlyxCaptionTempWindow";
    RegisterClassExW(&wc);

    HWND tempHwnd = CreateWindowW(L"HlyxCaptionTempWindow", L"HlyxCaptionTemp", WS_OVERLAPPEDWINDOW,
        0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);
    if (!tempHwnd) return false;

    IDXGISwapChain* tempSC = nullptr;
    ID3D11Device* tempDev = nullptr;
    ID3D11DeviceContext* tempCtx = nullptr;

    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.Width = 100;
    scd.BufferDesc.Height = 100;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = tempHwnd;
    scd.SampleDesc.Count = 1;
    scd.SampleDesc.Quality = 0;
    scd.Windowed = TRUE;

    D3D_FEATURE_LEVEL fl = D3D_FEATURE_LEVEL_11_0;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        &fl, 1, D3D11_SDK_VERSION, &scd, &tempSC, &tempDev, nullptr, &tempCtx);

    if (FAILED(hr)) {
#ifdef _DEBUG
        std::cout << "[-] Temp D3D11 device failed: 0x" << std::hex << hr << std::dec << "\n";
#endif
        DestroyWindow(tempHwnd);
        return false;
    }

    uintptr_t* vt = *(uintptr_t**)tempSC;
    uintptr_t addrPresent = vt[8];
    uintptr_t addrResize = vt[13];

    if (MH_CreateHook((LPVOID)addrPresent, &hkPresent, (LPVOID*)&oPresent) != MH_OK ||
        MH_EnableHook((LPVOID)addrPresent) != MH_OK) {
#ifdef _DEBUG
        std::cout << "[-] Failed to hook Present.\n";
#endif
        tempSC->Release(); tempCtx->Release(); tempDev->Release(); DestroyWindow(tempHwnd);
        return false;
    }

    if (MH_CreateHook((LPVOID)addrResize, &hkResize, (LPVOID*)&oResize) == MH_OK)
        MH_EnableHook((LPVOID)addrResize);

    tempSC->Release(); tempCtx->Release(); tempDev->Release(); DestroyWindow(tempHwnd);
#ifdef _DEBUG
    std::cout << "[+] Present hooked (vtable[8] = 0x" << std::hex << addrPresent << std::dec << ")\n";
#endif
    return true;
}

bool Renderer::InitImGui(IDXGISwapChain* pSwapChain) {
    DXGI_SWAP_CHAIN_DESC desc;
    if (FAILED(pSwapChain->GetDesc(&desc))) return false;
    m_Window = desc.OutputWindow;

    ID3D11Device* dev = nullptr;
    if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&dev))) return false;
    dev->GetImmediateContext(&m_Context);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    ImGui::StyleColorsDark();

    if (!ImGui_ImplWin32_Init(m_Window)) return false;
    if (!ImGui_ImplDX11_Init(dev, m_Context)) return false;

    ID3D11Texture2D* backBuffer = nullptr;
    if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer))) {
        dev->CreateRenderTargetView(backBuffer, nullptr, &m_BackBufferRTV);
        backBuffer->Release();
    }

    m_Device = dev;
    m_ImguiInitialized = true;
    m_OriginalWndProc = (WNDPROC)SetWindowLongPtrW(m_Window, GWLP_WNDPROC, (LONG_PTR)WndProc);

    // Create a fully transparent 1x1 cursor used to HIDE the OS cursor while
    // the settings panel is open. The game owns the OS cursor (warps it to
    // the center via NtUserSetCursorPos), so we hide it and draw the real
    // user32.dll arrow as a texture at the tracked position instead.
    {
        // AND mask = all ones (screen shows through), XOR mask = 0 -> invisible
        static const BYTE andMask[2] = { 0xFF, 0xFF };
        static const BYTE xorMask[2] = { 0x00, 0x00 };
        g_BlankCursor = CreateCursor(GetModuleHandleW(nullptr), 0, 0, 1, 1,
                                     andMask, xorMask);
#ifdef _DEBUG
        if (!g_BlankCursor)
            std::cout << "[-] Failed to create blank cursor, err=" << GetLastError() << "\n";
#endif
    }

    // Initialize Ultralight (settings panel HTML/CSS/JS engine)
    UltralightManager::Get().Initialize(dev, m_Context,
                                        (int)desc.BufferDesc.Width,
                                        (int)desc.BufferDesc.Height);

    // Register for Raw Input (mouse) so we can read mouse deltas
    // when settings window is open - HLA uses Raw Input for mouse
    RAWINPUTDEVICE rid = {};
    rid.usUsagePage = 0x01;  // Generic Desktop
    rid.usUsage = 0x02;      // Mouse
    rid.dwFlags = RIDEV_INPUTSINK;  // Receive input even when not in foreground
    rid.hwndTarget = m_Window;
    BOOL regResult = RegisterRawInputDevices(&rid, 1, sizeof(rid));
#ifdef _DEBUG
    std::cout << "[RawInput] RegisterRawInputDevices result=" << regResult
              << " hwnd=" << (void*)m_Window << " lastError=" << GetLastError() << "\n";
#endif

    // Initialize HarfBuzz shaper
    m_Shaper = new TextShaper();
    std::string fontPath;
    if (!g_Config.custom_font_path.empty()) {
        fontPath = ResolveFontPath(g_Config.custom_font_path);
#ifdef _DEBUG
        std::cout << "[Font] Using custom path: " << fontPath << "\n";
#endif
    } else {
        fontPath = FindArabicFont();
    }
    if (fontPath.empty()) {
#ifdef _DEBUG
        std::cout << "[-] No Arabic font found on disk.\n";
#endif
    } else if (!m_Shaper->Initialize(fontPath, g_Config.font_size)) {
#ifdef _DEBUG
        std::cout << "[-] Shaper init FAILED for: " << fontPath << "\n";
#endif
        delete m_Shaper;
        m_Shaper = nullptr;
    }

    // Load fallback font (used by FriBidi for RTL directional runs)
    if (m_Shaper && !g_Config.fallback_font_path.empty()) {
        m_Shaper->InitializeFallback(ResolveFontPath(g_Config.fallback_font_path), g_Config.font_size);
    }

#ifdef _DEBUG
    std::cout << "[+] ImGui initialized (hwnd=" << (void*)m_Window << ")\n";
#endif
    return true;
}

bool Renderer::InitStandalone(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context) {
    if (m_Initialized) return true;
    EnsureSynchronization();

    QueryPerformanceFrequency(&g_QPCFreq);
    QueryPerformanceCounter(&g_QPCStart);
    g_LastFrameQPC = GetTimeQPC();

    m_Device = device;
    m_Context = context;
    m_Window = hwnd;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    ImGui::StyleColorsDark();

    if (!ImGui_ImplWin32_Init(hwnd)) return false;
    if (!ImGui_ImplDX11_Init(device, context)) return false;

    m_OriginalWndProc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);

    m_Shaper = new TextShaper();
    if (!g_Config.custom_font_path.empty()) {
        if (!m_Shaper->Initialize(ResolveFontPath(g_Config.custom_font_path), g_Config.font_size)) {
            std::string fallback = FindArabicFont();
            if (!fallback.empty())
                m_Shaper->Initialize(fallback, g_Config.font_size);
        }
    } else {
        std::string fallback = FindArabicFont();
        if (!fallback.empty())
            m_Shaper->Initialize(fallback, g_Config.font_size);
    }
    if (m_Shaper->IsInitialized() && !g_Config.fallback_font_path.empty()) {
        m_Shaper->InitializeFallback(ResolveFontPath(g_Config.fallback_font_path), g_Config.font_size);
    }

    m_Initialized = true;
    m_ImguiInitialized = true;
    return true;
}

void Renderer::ReloadFont() {
    if (m_Shaper) {
        m_Shaper->Shutdown();
        delete m_Shaper;
        m_Shaper = nullptr;
    }

    m_Shaper = new TextShaper();
    if (!g_Config.custom_font_path.empty()) {
        if (!m_Shaper->Initialize(ResolveFontPath(g_Config.custom_font_path), g_Config.font_size)) {
            std::string fallback = FindArabicFont();
            if (!fallback.empty())
                m_Shaper->Initialize(fallback, g_Config.font_size);
        }
    } else {
        std::string fallback = FindArabicFont();
        if (!fallback.empty())
            m_Shaper->Initialize(fallback, g_Config.font_size);
    }
    if (m_Shaper->IsInitialized() && !g_Config.fallback_font_path.empty()) {
        m_Shaper->InitializeFallback(ResolveFontPath(g_Config.fallback_font_path), g_Config.font_size);
    }

    for (auto& e : m_Queue)
        e.ReleaseTexture();
    m_Queue.clear();
}

void Renderer::ReloadFontPreserveQueue() {
    EnterCriticalSection(&m_CS);
    if (m_Shaper) {
        m_Shaper->Shutdown();
        delete m_Shaper;
        m_Shaper = nullptr;
    }
    m_Shaper = new TextShaper();
    if (!g_Config.custom_font_path.empty()) {
        if (!m_Shaper->Initialize(ResolveFontPath(g_Config.custom_font_path), g_Config.font_size)) {
            std::string fallback = FindArabicFont();
            if (!fallback.empty())
                m_Shaper->Initialize(fallback, g_Config.font_size);
        }
    } else {
        std::string fallback = FindArabicFont();
        if (!fallback.empty())
            m_Shaper->Initialize(fallback, g_Config.font_size);
    }
    if (m_Shaper->IsInitialized() && !g_Config.fallback_font_path.empty()) {
        m_Shaper->InitializeFallback(ResolveFontPath(g_Config.fallback_font_path), g_Config.font_size);
    }
    ImGuiIO& io = ImGui::GetIO();
    double screenW = io.DisplaySize.x;
    if (screenW <= 0) screenW = GetSystemMetrics(SM_CXSCREEN);
    int maxW = (int)(screenW * g_Config.max_line_width_percent);
    for (auto& e : m_Queue) {
        if (e.expired || e.phrases.empty()) {
            e.ReleaseTexture();
            continue;
        }
        e.ReleaseTexture();
        RenderEntryTexture(e, maxW);
        e.renderedFontSize = GetScaledFontSize(g_Config.font_size);
        e.renderedLineSpacing = g_Config.line_spacing;
        e.renderedTextR = g_Config.text_r;
        e.renderedTextG = g_Config.text_g;
        e.renderedTextB = g_Config.text_b;
        e.renderedTextA = g_Config.text_a;
        e.renderedMaxLineWidthPercent = g_Config.max_line_width_percent;
        e.renderedAlignment = g_Config.text_alignment;
        e.renderedCustomFontPath = g_Config.custom_font_path;
        e.renderedFallbackFontPath = g_Config.fallback_font_path;
        e.lastRebuildQPC = GetTimeQPC();
    }
    LeaveCriticalSection(&m_CS);
}

void Renderer::DrawBackgroundBox(ImDrawList* dl, ImVec2 imgPos, ImVec2 imgSize, float opacity) {
    if (!g_Config.background_enabled) return;

    ImVec2 padding(g_Config.bg_padding_x, g_Config.bg_padding_y);
    ImVec2 min = ImVec2(imgPos.x - padding.x, imgPos.y - padding.y);
    ImVec2 max = ImVec2(imgPos.x + imgSize.x + padding.x, imgPos.y + imgSize.y + padding.y);

    ImU32 bgColor = IM_COL32(
        g_Config.bg_r, g_Config.bg_g, g_Config.bg_b,
        (int)(g_Config.bg_a * opacity)
    );

    if (g_Config.bg_border_radius > 0.0f) {
        dl->AddRectFilled(min, max, bgColor, g_Config.bg_border_radius);
    } else {
        dl->AddRectFilled(min, max, bgColor);
    }
}


void Renderer::DrawCaptions() {
    if (!m_OverlayVisible.load(std::memory_order_acquire)) return;

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 ss = io.DisplaySize;

    for (auto& e : m_Queue) {
        if (!e.texID || e.opacity <= 0.0f) continue;

        ImVec2 imgPos;
        switch (g_Config.text_alignment) {
            case OverlayConfig::ALIGN_LEFT:
                // Left edge at left of centered word-wrap area
                imgPos = ImVec2(ss.x * (1.0f - g_Config.max_line_width_percent) * 0.5f, (float)e.visualY);
                break;
            case OverlayConfig::ALIGN_RIGHT:
                // Right edge at right of word-wrap (max_line_width_percent)
                imgPos = ImVec2(ss.x * g_Config.max_line_width_percent - e.pixelWidth, (float)e.visualY);
                break;
            case OverlayConfig::ALIGN_CENTER:
            default:
                imgPos = ImVec2(ss.x * g_Config.pos_x - e.pixelWidth * 0.5f, (float)e.visualY);
                break;
        }
        ImVec2 imgSize = ImVec2((float)e.pixelWidth, (float)e.pixelHeight);

        // Keep the preview inside the unobscured part of the game while the
        // settings panel is open. English opens from the left; Arabic opens
        // from the right. Real captions keep their configured position.
        if (e.isPreview && m_SettingsOpen.load(std::memory_order_acquire)) {
            const float panelWidth = std::fmin(ss.x, std::fmax(420.0f, std::fmin(ss.x * 0.36f, 720.0f)));
            const bool englishUi = g_Config.ui_language == 1;
            const float freeLeft = englishUi ? panelWidth : 0.0f;
            const float freeRight = englishUi ? ss.x : ss.x - panelWidth;
            imgPos.x += englishUi ? panelWidth * 0.5f : -panelWidth * 0.5f;
            const float maxX = std::fmax(freeLeft, freeRight - imgSize.x);
            imgPos.x = std::fmax(freeLeft, std::fmin(imgPos.x, maxX));
        }

        ImDrawList* dl = ImGui::GetForegroundDrawList();

        // Draw background box first (behind everything)
        DrawBackgroundBox(dl, imgPos, imgSize, e.opacity);

        if (g_Config.outline_enabled) {
            ImU32 outlineCol = IM_COL32(
                g_Config.outline_r, g_Config.outline_g, g_Config.outline_b,
                (int)(g_Config.outline_a * e.opacity)
            );
            int t = (int)g_Config.outline_thickness;
            for (int ox = -t; ox <= t; ox++) {
                for (int oy = -t; oy <= t; oy++) {
                    if (ox == 0 && oy == 0) continue;
                    ImVec2 oPos = ImVec2(imgPos.x + ox, imgPos.y + oy);
                    dl->AddImage(
                        e.texID, oPos,
                        ImVec2(oPos.x + imgSize.x, oPos.y + imgSize.y),
                        ImVec2(0, 0), ImVec2(1, 1), outlineCol
                    );
                }
            }
        }

        if (g_Config.shadow_enabled) {
            ImVec2 shPos = ImVec2(
                imgPos.x + g_Config.shadow_offset_x,
                imgPos.y + g_Config.shadow_offset_y
            );
            dl->AddImage(
                e.texID, shPos,
                ImVec2(shPos.x + imgSize.x, shPos.y + imgSize.y),
                ImVec2(0, 0), ImVec2(1, 1),
                IM_COL32(
                    g_Config.shadow_r, g_Config.shadow_g, g_Config.shadow_b,
                    (int)(g_Config.shadow_a * e.opacity)
                )
            );
        }

        dl->AddImage(
            e.texID, imgPos,
            ImVec2(imgPos.x + imgSize.x, imgPos.y + imgSize.y),
            ImVec2(0, 0), ImVec2(1, 1),
            IM_COL32(255, 255, 255, (int)(255 * e.opacity))
        );
    }
}

void Renderer::DrawSettingsWindow() {
    // Settings panel is rendered by Ultralight (UltralightManager::Render).
    // The legacy ImGui panel was deleted; kept as no-op (called every frame from hkPresent).
}

static bool g_CursorWasClipped = false;
static POINT g_LastMousePos = { 0, 0 };
static bool g_FirstMouseRead = true;

void Renderer::ToggleCaptionVisibility() {
    const bool visible = !m_OverlayVisible.load(std::memory_order_acquire);
    m_OverlayVisible.store(visible, std::memory_order_release);
    // The View is updated by UltralightManager on the render thread.
    UltralightManager::Get().RequestCaptionVisibleChange(visible);
}

// Toggle the settings panel from the window-key path in WndProc.
static void ToggleSettingsPanel(HWND hWnd) {
    bool wasOpen = Renderer::m_SettingsOpen;
    Renderer::m_SettingsOpen = !Renderer::m_SettingsOpen;

    UL_LogToFile("[UI] F10 -> settings %s",
                 Renderer::m_SettingsOpen ? "OPEN" : "CLOSED");

    // SetOpen just flips a boolean; the actual WebKit work happens
    // in the next hkPresent call.
    UltralightManager::Get().SetOpen(Renderer::m_SettingsOpen);

    if (Renderer::m_SettingsOpen) {
        // Initialize software cursor at screen center.
        // NOTE: we must NOT touch ImGui here - WndProc runs on the game's
        // message thread while the ImGui context belongs to the render
        // thread (ImGui is not thread-safe). We only track plain floats
        // and let hkPresent apply them to io.MousePos / MouseDrawCursor
        // on the next present.
        // Sync the topbar F11 button label with the current translation
        // state (applied on the render thread via ConsumePendingCaptionVisible).
        const float displayW = g_DisplayWidth.load(std::memory_order_acquire);
        const float displayH = g_DisplayHeight.load(std::memory_order_acquire);
        UltralightManager::Get().RequestCaptionVisibleChange(
            Renderer::m_OverlayVisible.load(std::memory_order_acquire));
        if (displayW > 0.0f && displayH > 0.0f) {
            g_SoftMouseX.store(displayW * 0.5f, std::memory_order_release);
            g_SoftMouseY.store(displayH * 0.5f, std::memory_order_release);
        }

        // Focus game window so it routes input to our WndProc
        SetForegroundWindow(hWnd);

        // Hide the OS cursor immediately (WM_SETCURSOR keeps it hidden);
        // the real user32.dll arrow is drawn by DrawRealCursor at the
        // tracked position, so the game's warping cannot make it tremble.
        if (g_BlankCursor) SetCursor(g_BlankCursor);
        ::ReleaseCapture();
        if (oClipCursor) oClipCursor(nullptr);  // trampoline: bypass our own hook

        // Warp the OS cursor to screen center. Use the trampoline so our
        // own hkSetCursorPos hook (which blocks while settings are open)
        // does not swallow the warp.
        if (!wasOpen && oSetCursorPos && displayW > 0.0f && displayH > 0.0f) {
            oSetCursorPos((int)(displayW * 0.5f), (int)(displayH * 0.5f));
        }
    } else {
        // Hide the OS cursor again and restore focus
        ShowCursor(FALSE);
        SetForegroundWindow(hWnd);
    }
}

LRESULT CALLBACK Renderer::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // F10: Toggle settings window (F10 is a system key, so check both WM_KEYDOWN and WM_SYSKEYDOWN)
    if ((msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) && wParam == VK_F10) {
        ToggleSettingsPanel(hWnd);
        return 0;
    }
    // F11: Toggle caption visibility.
    if (msg == WM_KEYDOWN && wParam == VK_F11) {
        ToggleCaptionVisibility();
        return 0;
    }

    // When settings window is open - HIDDEN OS cursor + drawn real arrow
    if (m_SettingsOpen) {
        // Prevent game from hiding/showing a cursor with WM_SETCURSOR.
        // Keep the OS cursor invisible (blank); the real user32.dll arrow is
        // drawn as a texture by DrawRealCursor at the tracked position.
        if (msg == WM_SETCURSOR) {
            SetCursor(g_BlankCursor ? g_BlankCursor : LoadCursor(nullptr, IDC_ARROW));
            return TRUE;
        }

        // Process Raw Input → update the tracked mouse position
        // (g_SoftMouseX/Y drives the UI and the drawn real-arrow cursor).
        if (msg == WM_INPUT) {
            UINT dataSize = 0;
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &dataSize, sizeof(RAWINPUTHEADER));
            if (dataSize > 0) {
                std::vector<BYTE> data(dataSize);
                if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, data.data(), &dataSize, sizeof(RAWINPUTHEADER)) == dataSize) {
                    RAWINPUT* raw = (RAWINPUT*)data.data();
                    if (raw->header.dwType == RIM_TYPEMOUSE) {
                        int dx = raw->data.mouse.lLastX;
                        int dy = raw->data.mouse.lLastY;

                        if (dx != 0 || dy != 0) {
                            // Snapshot live settings under the same lock used by
                            // caption parsing/rendering. The UI can update the
                            // config on the Present thread while WM_INPUT runs
                            // on the window thread.
                            float sens = 1.0f;
                            bool mouseAcceleration = false;
                            float accelFactor = 2.0f;
                            EnterCriticalSection(&m_CS);
                            sens = g_Config.mouse_sensitivity;
                            mouseAcceleration = g_Config.mouse_acceleration;
                            accelFactor = g_Config.mouse_accel_factor;
                            LeaveCriticalSection(&m_CS);
                            float moveX = (float)dx * sens;
                            float moveY = (float)dy * sens;

                            // Mouse acceleration: faster movement = larger jumps
                            if (mouseAcceleration) {
                                float speed = sqrtf(moveX * moveX + moveY * moveY);
                                float accel = 1.0f + (speed / 50.0f) * accelFactor;
                                moveX *= accel;
                                moveY *= accel;
                            }

                            // Update software position (plain floats - no ImGui
                            // access from this thread).
                            float softX = g_SoftMouseX.load(std::memory_order_acquire) + moveX;
                            float softY = g_SoftMouseY.load(std::memory_order_acquire) + moveY;

                            // Clamp to screen using the cached display size
                            const float displayW = g_DisplayWidth.load(std::memory_order_acquire);
                            const float displayH = g_DisplayHeight.load(std::memory_order_acquire);
                            if (displayW > 0.0f) {
                                if (softX < 0) softX = 0;
                                if (softX >= displayW) softX = displayW - 1.0f;
                            }
                            if (displayH > 0.0f) {
                                if (softY < 0) softY = 0;
                                if (softY >= displayH) softY = displayH - 1.0f;
                            }
                            g_SoftMouseX.store(softX, std::memory_order_release);
                            g_SoftMouseY.store(softY, std::memory_order_release);

                            // NOTE: we do NOT call oSetCursorPos here. The game
                            // warps the OS cursor to the center via NtUserSetCursorPos
                            // (bypassing our hook) - fighting it makes the cursor
                            // tremble between two positions. The OS cursor stays
                            // hidden; DrawRealCursor renders the real user32.dll
                            // arrow at g_SoftMouseX/Y instead (smooth, no fight).

                            // Synthesize a WM_MOUSEMOVE so the HTML5 UI can react to cursor
                            // movement. Use MAKELPARAM to make a correct coordinate lParam.
                            // (Queued internally - fired on the render thread.)
                            LPARAM newMouseMove = MAKELPARAM((int)softX, (int)softY);
                            UltralightManager::Get().ProcessWin32Message(
                                hWnd, WM_MOUSEMOVE, 0, newMouseMove);
                        }
                    }
                }
            }
            return 0;  // Consumed - game doesn't get it
        }

        // Mouse logic: keep the mouse free on every move and forward
        // button/wheel events to Ultralight. ImGui is render-thread-only;
        // WM_MOUSEMOVE is NOT forwarded to ImGui (it would overwrite our tracked software position
        // with the OS position - the exact bug fixed by aaeeb76 "Fix cursor
        // jump: block WM_MOUSEMOVE from overwriting software cursor").
        if (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) {
            if (msg == WM_MOUSEMOVE) {
                // Continuously release the mouse from the game (8345bcf logic:
                // keep freeing it on every move).
                ::ReleaseCapture();
                if (oClipCursor) oClipCursor(nullptr);
                return 0;
            }
            // Buttons + wheel: queue events for Ultralight, which fires them
            // on the render thread.
            UltralightManager::Get().ProcessWin32Message(hWnd, msg, wParam, lParam);
            return 0;
        }
        // Block keyboard (except Alt+F4, Alt+Tab)
        if (msg >= WM_KEYFIRST && msg <= WM_KEYLAST) {
            bool isAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
            if (isAlt && (wParam == VK_F4 || wParam == VK_TAB)) {
                return CallWindowProcW(m_OriginalWndProc, hWnd, msg, wParam, lParam);
            }
            // Queue keyboard events for Ultralight (fired on the render thread).
            UltralightManager::Get().ProcessWin32Message(hWnd, msg, wParam, lParam);
            return 0;
        }
    }

    // ImGui is intentionally never touched from this message thread;
    // hkPresent owns the ImGui context exclusively.
    return CallWindowProcW(m_OriginalWndProc, hWnd, msg, wParam, lParam);
}

bool Renderer::Initialize() {
    if (m_Initialized) return true;
    EnsureSynchronization();

    if (!HookPresent()) {
#ifdef _DEBUG
        std::cout << "[-] Renderer init failed.\n";
#endif
        return false;
    }

    m_Initialized = true;
#ifdef _DEBUG
    std::cout << "[+] Renderer ready.\n";
#endif
    return true;
}

void Renderer::Shutdown() {
    if (!m_CSInitialized.load(std::memory_order_acquire) &&
        !m_ImguiInitialized && !m_Shaper && m_Queue.empty()) {
        return;
    }

    m_ShuttingDown.store(true, std::memory_order_release);
    if (m_CSInitialized.load(std::memory_order_acquire))
        EnterCriticalSection(&m_CS);

    for (auto& e : m_Queue)
        e.ReleaseTexture();
    m_Queue.clear();

    if (m_Shaper) {
        m_Shaper->Shutdown();
        delete m_Shaper;
        m_Shaper = nullptr;
    }

    if (m_OriginalWndProc && m_Window)
        SetWindowLongPtrW(m_Window, GWLP_WNDPROC, (LONG_PTR)m_OriginalWndProc);
    UltralightManager::Get().Shutdown();

    if (m_ImguiInitialized) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        m_ImguiInitialized = false;
    }

    if (m_BackBufferRTV) { m_BackBufferRTV->Release(); m_BackBufferRTV = nullptr; }
    if (m_Context) { m_Context->Release(); m_Context = nullptr; }
    if (m_Device) { m_Device->Release(); m_Device = nullptr; }

    if (m_CSInitialized.load(std::memory_order_acquire))
        LeaveCriticalSection(&m_CS);
    bool wasInitialized = m_CSInitialized.exchange(false, std::memory_order_acq_rel);
    if (wasInitialized)
        DeleteCriticalSection(&m_CS);
    m_Initialized = false;
#ifdef _DEBUG
    std::cout << "[+] Renderer shut down.\n";
#endif
}


