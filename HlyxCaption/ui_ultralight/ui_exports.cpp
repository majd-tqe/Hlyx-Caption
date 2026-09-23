/*
 * Hlyx Caption — original project source
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Hlyx Caption Contributors
 */

// ui_exports.cpp - Plain C interface for UI subsystem.
#include "ui_exports.h"
#include "UltralightManager.h"
#include "../renderer.h"  // for Renderer::m_SettingsOpen

extern "C" {

__declspec(dllexport) bool UI_Initialize(ID3D11Device* dev, ID3D11DeviceContext* ctx, int w, int h) {
    return UltralightManager::Get().Initialize(dev, ctx, w, h);
}

__declspec(dllexport) void UI_Render() {
    UltralightManager::Get().Render();
}

__declspec(dllexport) void UI_SetOpen(bool open) {
    // CRITICAL: must set BOTH flags. The user32 mouse hooks check
    // Renderer::m_SettingsOpen; if we only flip UltralightManager::m_Open,
    // the game will re-capture the cursor and the HTML UI will lose
    // mouse control. Same logic as WndProc F10 handler.
    Renderer::m_SettingsOpen = open;
    UltralightManager::Get().SetOpen(open);
}

__declspec(dllexport) bool UI_IsOpen() {
    return UltralightManager::Get().IsOpen();
}

__declspec(dllexport) bool UI_ProcessWin32Message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return UltralightManager::Get().ProcessWin32Message(hWnd, msg, wParam, lParam);
}

__declspec(dllexport) void UI_Shutdown() {
    UltralightManager::Get().Shutdown();
}

__declspec(dllexport) bool UI_IsInitialized() {
    return UltralightManager::Get().IsInitialized();
}

}  // extern "C"
