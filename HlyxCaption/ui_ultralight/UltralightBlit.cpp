/*
 * Hlyx Caption — original project source
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Hlyx Caption Contributors
 */

// UltralightBlit.cpp - D3D11 blit pipeline implementation.
// Maintains a persistent staging texture that we re-fill each frame from the
// Ultralight CPU bitmap, then draws as a fullscreen quad onto the backbuffer.
#include "UltralightBlit.h"
#include "UL_Debug.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <vector>
#include <cstdio>
#include <cstring>

#pragma comment(lib, "d3dcompiler.lib")

#define ULDBG(fmt, ...) do { \
    char _buf[512]; snprintf(_buf, sizeof(_buf), "[ULDBG] " fmt "\n", ##__VA_ARGS__); \
    OutputDebugStringA(_buf); fputs(_buf, stdout); \
    UL_LogToFile("[ULDBG] " fmt, ##__VA_ARGS__); \
} while(0)

namespace ultralight_blit {

struct SavedPipelineState {
    ID3D11RasterizerState* rasterizer = nullptr;
    ID3D11BlendState* blend = nullptr;
    FLOAT blendFactor[4] = {};
    UINT sampleMask = 0xffffffffu;
    ID3D11InputLayout* inputLayout = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ID3D11VertexShader* vertexShader = nullptr;
    ID3D11PixelShader* pixelShader = nullptr;
    ID3D11ShaderResourceView* pixelSrv = nullptr;
    ID3D11SamplerState* pixelSampler = nullptr;
    ID3D11Buffer* pixelConstantBuffer = nullptr;
    UINT viewportCount = 0;
    D3D11_VIEWPORT viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    bool active = false;
};

static SavedPipelineState g_SavedState;

static void ReleaseSavedState() {
    if (g_SavedState.rasterizer) g_SavedState.rasterizer->Release();
    if (g_SavedState.blend) g_SavedState.blend->Release();
    if (g_SavedState.inputLayout) g_SavedState.inputLayout->Release();
    if (g_SavedState.vertexShader) g_SavedState.vertexShader->Release();
    if (g_SavedState.pixelShader) g_SavedState.pixelShader->Release();
    if (g_SavedState.pixelSrv) g_SavedState.pixelSrv->Release();
    if (g_SavedState.pixelSampler) g_SavedState.pixelSampler->Release();
    if (g_SavedState.pixelConstantBuffer) g_SavedState.pixelConstantBuffer->Release();
    g_SavedState = SavedPipelineState{};
}

void BeginFrame(ID3D11DeviceContext* ctx) {
    if (!ctx) return;
    ReleaseSavedState();
    ctx->RSGetState(&g_SavedState.rasterizer);
    ctx->OMGetBlendState(&g_SavedState.blend, g_SavedState.blendFactor,
                         &g_SavedState.sampleMask);
    ctx->IAGetInputLayout(&g_SavedState.inputLayout);
    ctx->IAGetPrimitiveTopology(&g_SavedState.topology);
    ctx->VSGetShader(&g_SavedState.vertexShader, nullptr, nullptr);
    ctx->PSGetShader(&g_SavedState.pixelShader, nullptr, nullptr);
    ctx->PSGetShaderResources(0, 1, &g_SavedState.pixelSrv);
    ctx->PSGetSamplers(0, 1, &g_SavedState.pixelSampler);
    ctx->PSGetConstantBuffers(0, 1, &g_SavedState.pixelConstantBuffer);
    g_SavedState.viewportCount = ARRAYSIZE(g_SavedState.viewports);
    ctx->RSGetViewports(&g_SavedState.viewportCount, g_SavedState.viewports);
    g_SavedState.active = true;
}

// ----- Shaders -----

static const char* kVS_Code = R"HLSL(
struct VS_Out {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};
VS_Out main(uint vid : SV_VertexID) {
    float2 pos[3] = {
        float2(-1.0, -1.0),
        float2( 3.0, -1.0),
        float2(-1.0,  3.0),
    };
    float2 uv[3] = {
        float2(0.0, 0.0),
        float2(2.0, 0.0),
        float2(0.0, 2.0),
    };
    VS_Out o;
    o.pos = float4(pos[vid], 0.0, 1.0);
    o.uv  = uv[vid];
    return o;
}
)HLSL";

static const char* kPS_Code = R"HLSL(
Texture2D    tex      : register(t0);
SamplerState samp0    : register(s0);
float        alphaMod : register(b0);
struct PS_In {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};
float4 main(PS_In i) : SV_Target {
    // Ultralight bitmap: v=0 is top. D3D NDC: y=+1 is top.
    // Sample with flipped v so the image is right-side-up.
    float4 c = tex.Sample(samp0, float2(i.uv.x, 1.0 - i.uv.y));
    c.a *= alphaMod;
    return c;
}
)HLSL";

// ----- Persistent state -----

struct Constants { float alphaMod; float pad[3]; };

static ID3D11Buffer*           g_Constants = nullptr;
static ID3D11SamplerState*     g_Sampler = nullptr;
static ID3D11VertexShader*     g_VS = nullptr;
static ID3D11PixelShader*      g_PS = nullptr;
static ID3D11RasterizerState*  g_Raster = nullptr;
static ID3D11BlendState*       g_Blend = nullptr;

// Persistent staging texture + SRV. Created on first upload and re-uploaded each frame.
static ID3D11Texture2D*          g_BitmapTex = nullptr;
static ID3D11ShaderResourceView* g_BitmapSRV = nullptr;
static uint32_t                  g_BitmapTexW = 0;
static uint32_t                  g_BitmapTexH = 0;

// D3D11 device for (re)creating the texture.
static ID3D11Device*             g_OwningDevice = nullptr;

// ----- Shader compile helpers -----

static bool CompileShader(ID3D11Device* device, const char* src, size_t len,
                          const char* entry, const char* target,
                          ID3DBlob** out_blob) {
    UINT flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_ENABLE_STRICTNESS;
    ID3DBlob* errors = nullptr;
    HRESULT hr = D3DCompile(src, len, nullptr, nullptr, nullptr,
                            entry, target, flags, 0, out_blob, &errors);
    if (FAILED(hr)) {
        if (errors) {
            OutputDebugStringA((const char*)errors->GetBufferPointer());
            ULDBG("shader %s/%s compile error: %s", entry, target,
                  (const char*)errors->GetBufferPointer());
            errors->Release();
        }
        return false;
    }
    if (errors) errors->Release();
    return true;
}

// ----- Init -----

bool Initialize(ID3D11Device* device) {
    if (g_VS && g_PS) return true;
    ULDBG("Initialize: device=%p", device);
    g_OwningDevice = device;

    ID3DBlob* vsBlob = nullptr;
    if (!CompileShader(device, kVS_Code, strlen(kVS_Code), "main", "vs_5_0", &vsBlob))
        return false;
    HRESULT hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                            nullptr, &g_VS);
    vsBlob->Release();
    if (FAILED(hr)) return false;

    ID3DBlob* psBlob = nullptr;
    if (!CompileShader(device, kPS_Code, strlen(kPS_Code), "main", "ps_5_0", &psBlob))
        return false;
    hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
                                    nullptr, &g_PS);
    psBlob->Release();
    if (FAILED(hr)) return false;

    D3D11_SAMPLER_DESC sd = {};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    sd.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    sd.BorderColor[0] = sd.BorderColor[1] = sd.BorderColor[2] = sd.BorderColor[3] = 0.0f;
    sd.MinLOD = 0; sd.MaxLOD = D3D11_FLOAT32_MAX;
    if (FAILED(device->CreateSamplerState(&sd, &g_Sampler))) return false;

    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.FrontCounterClockwise = FALSE;
    rd.DepthClipEnable = FALSE;
    rd.ScissorEnable = FALSE;
    if (FAILED(device->CreateRasterizerState(&rd, &g_Raster))) return false;

    // Premultiplied alpha blend: result = src + (1-src.a)*dst
    D3D11_BLEND_DESC bd = {};
    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    if (FAILED(device->CreateBlendState(&bd, &g_Blend))) return false;

    D3D11_BUFFER_DESC cbd = {};
    cbd.ByteWidth = sizeof(Constants);
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(device->CreateBuffer(&cbd, nullptr, &g_Constants))) return false;

    ULDBG("Initialize OK");
    return true;
}

void Shutdown() {
    ULDBG("Shutdown");
    if (g_BitmapSRV) { g_BitmapSRV->Release(); g_BitmapSRV = nullptr; }
    if (g_BitmapTex) { g_BitmapTex->Release(); g_BitmapTex = nullptr; }
    g_BitmapTexW = g_BitmapTexH = 0;
    if (g_Constants) { g_Constants->Release(); g_Constants = nullptr; }
    if (g_Sampler)   { g_Sampler->Release();   g_Sampler = nullptr; }
    if (g_VS)        { g_VS->Release();        g_VS = nullptr; }
    if (g_PS)        { g_PS->Release();        g_PS = nullptr; }
    if (g_Raster)    { g_Raster->Release();    g_Raster = nullptr; }
    if (g_Blend)     { g_Blend->Release();     g_Blend = nullptr; }
}

// ----- Texture management -----

// Recreate g_BitmapTex if (w,h) changed. Resize is rare; GPU memory waste
// when slightly mismatched is negligible.
bool EnsureTexture(ID3D11Device* device, ID3D11DeviceContext* ctx,
                   uint32_t width, uint32_t height) {
    if (g_BitmapTex && g_BitmapTexW == width && g_BitmapTexH == height)
        return true;

    if (g_BitmapSRV) { g_BitmapSRV->Release(); g_BitmapSRV = nullptr; }
    if (g_BitmapTex) { g_BitmapTex->Release(); g_BitmapTex = nullptr; }

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = width;
    td.Height = height;
    td.MipLevels = 1;
    td.ArraySize = 1;
    // The Ultralight bitmap holds sRGB-encoded values. When the destination
    // backbuffer is an sRGB RTV (as in caption_sim and most games), sampling
    // through a UNORM (non-sRGB) view double-encodes gamma and washes every
    // color out. UL_BLIT_SRGB_SAMPLE is defined ONLY by the sim build:
    // sample through an sRGB view so the RTV re-encodes exactly once.
#ifdef UL_BLIT_SRGB_SAMPLE
    td.Format = DXGI_FORMAT_B8G8R8A8_TYPELESS;
#else
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
#endif
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    td.CPUAccessFlags = 0;
    td.MiscFlags = 0;

    HRESULT hr = device->CreateTexture2D(&td, nullptr, &g_BitmapTex);
    if (FAILED(hr)) {
        ULDBG("EnsureTexture CreateTexture2D %ux%u failed: 0x%lx", width, height, hr);
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
#ifdef UL_BLIT_SRGB_SAMPLE
    sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
#else
    sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
#endif
    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    sd.Texture2D.MipLevels = 1;
    sd.Texture2D.MostDetailedMip = 0;
    hr = device->CreateShaderResourceView(g_BitmapTex, &sd, &g_BitmapSRV);
    if (FAILED(hr)) {
        ULDBG("EnsureTexture CreateSRV failed: 0x%lx", hr);
        g_BitmapTex->Release(); g_BitmapTex = nullptr;
        return false;
    }
    g_BitmapTexW = width;
    g_BitmapTexH = height;
    ULDBG("EnsureTexture allocated %ux%u", width, height);
    return true;
}

void GetBitmapSize(uint32_t* width, uint32_t* height) {
    if (width)  *width = g_BitmapTexW;
    if (height) *height = g_BitmapTexH;
}

bool UpdateBitmapFromUG(ID3D11DeviceContext* ctx,
                        const ultralight::RefPtr<ultralight::Bitmap>& bitmap) {
    if (!g_BitmapTex || !bitmap || bitmap->IsEmpty()) return false;
    if (bitmap->width() != g_BitmapTexW || bitmap->height() != g_BitmapTexH)
        return false;

    uint32_t w = bitmap->width();
    uint32_t h = bitmap->height();
    uint32_t row = bitmap->row_bytes();

    const uint8_t* pixels = (const uint8_t*)bitmap->LockPixels();
    if (!pixels) return false;

    // Sample a few corners for diagnostic
    static int sample_counter = 0;
    if ((sample_counter++ % 120) == 0) {
        auto sample = [&](int x, int y) {
            const uint8_t* p = pixels + y * row + x * 4;
            return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
        };
        ULDBG("Bitmap %ux%u BGRA corners: TL=0x%08X TR=0x%08X BL=0x%08X BR=0x%08X MID=0x%08X",
              w, h,
              sample(0, 0), sample((int)w-1, 0), sample(0, (int)h-1),
              sample((int)w-1, (int)h-1), sample((int)w/2, (int)h/2));
    }

    D3D11_BOX box = {0, 0, 0, w, h, 1};
    ctx->UpdateSubresource(g_BitmapTex, 0, &box, pixels, row, h * row);
    bitmap->UnlockPixels();
    return true;
}

// ----- Draw -----

void DrawFullscreenQuad(ID3D11DeviceContext* ctx, int dst_w, int dst_h, float alpha_mod) {
    if (!g_VS || !g_PS || !g_BitmapSRV) return;

    D3D11_VIEWPORT vp = {};
    vp.Width = (float)dst_w;
    vp.Height = (float)dst_h;
    vp.MaxDepth = 1.0f;
    vp.MinDepth = 0.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    ctx->RSSetViewports(1, &vp);

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (SUCCEEDED(ctx->Map(g_Constants, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        Constants c = {};
        c.alphaMod = alpha_mod;
        memcpy(mapped.pData, &c, sizeof(c));
        ctx->Unmap(g_Constants, 0);
    }

    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_VS, nullptr, 0);
    ctx->PSSetShader(g_PS, nullptr, 0);
    ctx->PSSetShaderResources(0, 1, &g_BitmapSRV);
    ctx->PSSetSamplers(0, 1, &g_Sampler);
    ID3D11Buffer* cb = g_Constants;
    ctx->PSSetConstantBuffers(0, 1, &cb);
    ctx->RSSetState(g_Raster);
    ctx->OMSetBlendState(g_Blend, nullptr, 0xFFFFFFFF);

    ctx->Draw(3, 0);
}

void EndFrame(ID3D11DeviceContext* ctx) {
    if (!ctx || !g_SavedState.active) return;

    ctx->IASetInputLayout(g_SavedState.inputLayout);
    ctx->IASetPrimitiveTopology(g_SavedState.topology);
    ctx->VSSetShader(g_SavedState.vertexShader, nullptr, 0);
    ctx->PSSetShader(g_SavedState.pixelShader, nullptr, 0);
    ctx->PSSetShaderResources(0, 1, &g_SavedState.pixelSrv);
    ctx->PSSetSamplers(0, 1, &g_SavedState.pixelSampler);
    ctx->PSSetConstantBuffers(0, 1, &g_SavedState.pixelConstantBuffer);
    ctx->RSSetState(g_SavedState.rasterizer);
    if (g_SavedState.viewportCount > 0)
        ctx->RSSetViewports(g_SavedState.viewportCount, g_SavedState.viewports);
    else
        ctx->RSSetViewports(0, nullptr);
    ctx->OMSetBlendState(g_SavedState.blend, g_SavedState.blendFactor,
                         g_SavedState.sampleMask);
    ReleaseSavedState();
}

}  // namespace ultralight_blit
