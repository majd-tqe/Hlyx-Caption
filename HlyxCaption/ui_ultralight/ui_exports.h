/*
 * Hlyx Caption — original project source
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Hlyx Caption Contributors
 */

// ui_exports.h - Plain C interface for tests and external control.
// These symbols are exported via .def file or __declspec(dllexport).
#pragma once

#include <d3d11.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the UI subsystem with the given D3D11 device/context/size.
__declspec(dllexport) bool UI_Initialize(ID3D11Device* dev, ID3D11DeviceContext* ctx, int w, int h);
// Render the UI to the currently-bound render target. No-op if not initialized.
__declspec(dllexport) void UI_Render();
// Show / hide the settings overlay.
__declspec(dllexport) void UI_SetOpen(bool open);
__declspec(dllexport) bool UI_IsOpen();
// Forward a Win32 message to the UI (return true if consumed).
__declspec(dllexport) bool UI_ProcessWin32Message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
// Tear down.
__declspec(dllexport) void UI_Shutdown();
// For tests: force a render frame without going through the F10 toggle.
__declspec(dllexport) bool UI_IsInitialized();

#ifdef __cplusplus
}
#endif
