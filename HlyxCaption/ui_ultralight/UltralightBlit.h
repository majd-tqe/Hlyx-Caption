/*
 * Hlyx Caption — original project source
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Hlyx Caption Contributors
 */

// UltralightBlit.h - D3D11 blit pipeline for displaying Ultralight Bitmap pixels.
#pragma once

#include <d3d11.h>
#include <Ultralight/Bitmap.h>

namespace ultralight_blit {

// Create VS, PS, buffers, and input layout needed to draw a fullscreen textured quad.
bool Initialize(ID3D11Device* device);
void Shutdown();

// Resize/recreate the persistent Bitmap->Texture2D staging.
bool EnsureTexture(ID3D11Device* device, ID3D11DeviceContext* ctx,
                   uint32_t width, uint32_t height);

// Dimensions of the currently uploaded bitmap texture (0x0 if none yet).
void GetBitmapSize(uint32_t* width, uint32_t* height);

// Upload Ultralight bitmap pixels into the persistent staging texture.
bool UpdateBitmapFromUG(ID3D11DeviceContext* ctx,
                        const ultralight::RefPtr<ultralight::Bitmap>& bitmap);

// Draw the prepared SRV to the currently bound render target as a fullscreen quad.
void DrawFullscreenQuad(ID3D11DeviceContext* ctx, int dst_w, int dst_h, float alpha_mod);

// Save the small set of pipeline state touched by the blit pass.
void BeginFrame(ID3D11DeviceContext* ctx);

// Restore the state saved by BeginFrame. This keeps the game's D3D11 pipeline
// intact for anything rendered after the settings panel.
void EndFrame(ID3D11DeviceContext* ctx);

}  // namespace ultralight_blit
