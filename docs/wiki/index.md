---
layout: home
title: Hlyx Caption Wiki
titleTemplate: "An enhanced, fully customizable caption system for Half-Life: Alyx NoVR"

hero:
  name: "Hlyx Caption"
  text: "An enhanced, fully customizable caption system for Half-Life: Alyx NoVR"
  tagline: "Proxy-DLL · HarfBuzz shaping · Ultralight settings — a complete developer wiki"
  image:
    src: /logo.svg
    alt: Hlyx Caption
  actions:
    - theme: brand
      text: Get Started
      link: /getting-started
    - theme: alt
      text: Architecture
      link: /architecture

features:
  - icon: ◈
    title: Arabic-Correct Captions
    details: FriBidi bidi → HarfBuzz shaping → FreeType raster per-entry. Ligatures, joining, mixed RTL/LTR on one line.
  - icon: ⚙️
    title: HTML Settings Panel
    details: F10 Ultralight offscreen View → Bitmap → D3D11 quad. Live edits, Save/Reset/Preview, searchable config.
  - icon: 🧩
    title: Contributor Friendly
    details: C++17 · VS2022 · single wininet.dll · docs/wiki is the single source of truth, VitePress search.
---

## What Is Hlyx Caption?

**Hlyx Caption (HlyxCaption)** is a Windows x64 proxy-DLL mod for *Half-Life: Alyx* (Source 2 VR) that replaces the game's built-in subtitles with a custom desktop overlay focused on **Arabic and Arabic/Latin mixed captions** (FreeType + HarfBuzz + FriBidi) and a **full HTML/CSS/JS settings panel** via Ultralight. Other RTL scripts are not guaranteed because the native shaping path selects Arabic for RTL runs.

- **Solution:** `HlyxCaption.sln` → single VS2022 C++ project `HlyxCaption/HlyxCaption.vcxproj`
- **Output:** `wininet.dll` (`TargetName=wininet`, exports in `wininet.def`) — loaded by DLL search-order hijacking next to `hlvr.exe`
- **Version:** `0.9.5.0` (`version.rc`)
- **Language:** C++17, `/utf-8`, Unicode, v143, **x64 only**

### Key Concepts

- **Proxy DLL** — forwards 8 WinINet exports to the real `C:\Windows\System32\wininet.dll` via `proxy.cpp` + `wininet.def`
- **Signature-hooked delivery** — byte pattern scan in `client.dll` finds `Process(caption…)` and hooks it with MinHook
- **Bidi pipeline** — FriBidi → HarfBuzz → FreeType per-entry into `DXGI_FORMAT_R8G8B8A8_UNORM_SRGB` texture via Dear ImGui
- **Ultralight panel** — offscreen HTML/CSS/JS `View → Bitmap → D3D11 quad`; JS↔C++ via `window.*` polling
- **`Renderer::m_CS`** — single `CRITICAL_SECTION` for `m_Queue` + `TextShaper`; Ultralight has its own `mutex` + `deque<PendingEvent>`

## Quick Links

| Area | Start | For |
|---|---|---|
| **New to the project?** | [Getting Started](/getting-started) | Build, deploy, first run (F10/F11), troubleshooting |
| **Want the big picture?** | [Architecture](/architecture) | System diagram, data flow, design decisions |
| **Need a setting?** | [Configuration](/configuration) | Every `OverlayConfig` field, INI example, encoding gotchas |
| **Translate / add language?** | [Localization](/localization) | Supported languages, string editing, RTL notes |
| **Hack on shaping?** | [Language Support](/language-support) | HarfBuzz pipeline and RTL behavior |
| **Add a feature?** | [Contributing](/contributing) | New setting / tag / tab / language recipes |
| **Deep dive?** | Module Map below | 8 module docs + class/sequence diagrams |

## Module Map

| Module | Purpose |
|---|---|
| [Core / Bootstrap](/modules/core-bootstrap) | DLL proxy, `DllMain`/`MainThread`, crash handlers |
| [Caption Pipeline](/modules/caption-pipeline) | `renderer.h` types, parser/queue/texture, `DrawCaptions` |
| [Text Shaping](/modules/text-shaping) | `TextShaper` + `arabic_fallback` (731-entry table) |
| [Configuration](/configuration) | `OverlayConfig`, `ReadFileToUtf8`/`ParseConfigText` |
| [UI — Ultralight](/modules/ui-ultralight) | `UltralightManager`/`Blit`/`Platform`, `embedded_ui.h` |
| [Hooking & Injection](/modules/hooking) | MinHook over SlimDetours, user32 cursor hooks |

> Source: the current Git checkout · VitePress wiki at `docs/wiki/`
