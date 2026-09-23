<p align="center">
  <img src="docs/wiki/public/logo.svg" width="120" alt="Hlyx Caption logo">
</p>

<h1 align="center">Hlyx Caption</h1>

<p align="center">
  Arabic-aware, customizable captions for <em>Half-Life: Alyx</em> on Windows x64.
</p>

<p align="center">
  <img alt="Version 0.9.4" src="https://img.shields.io/badge/version-0.9.4-orange">
  <img alt="Platform Windows x64" src="https://img.shields.io/badge/platform-Windows%20x64-0078D4?logo=windows">
  <img alt="C++ 17" src="https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus">
  <a href="https://github.com/majd-tqe/Hlyx-Caption/actions/workflows/docs.yml"><img alt="Documentation build status" src="https://github.com/majd-tqe/Hlyx-Caption/actions/workflows/docs.yml/badge.svg?branch=main"></a>
  <a href="LICENSE"><img alt="License GPL-3.0-or-later" src="https://img.shields.io/badge/license-GPL--3.0--or--later-blue"></a>
</p>

Hlyx Caption is a `wininet.dll` proxy mod that intercepts the game's captions and
renders them through a D3D11 overlay. Its text pipeline uses FriBidi, HarfBuzz,
and FreeType to correctly display Arabic and mixed Arabic/Latin text. An
Ultralight-powered panel provides live caption customization in Arabic and
English.

> [!IMPORTANT]
> Hlyx Caption is an independent project and is not affiliated with Valve,
> Steam, Half-Life, or Ultralight. It relies on a signature in `client.dll`, so
> a game update can require a compatibility update to the mod.

## Demo

<p align="center">
  <a href="https://youtu.be/nT7m12L_vHw">
    <img src="https://img.youtube.com/vi/nT7m12L_vHw/maxresdefault.jpg" width="800" alt="Watch the Hlyx Caption demo on YouTube">
  </a>
</p>

<p align="center">
  <a href="https://youtu.be/nT7m12L_vHw">▶ Watch the Hlyx Caption demo on YouTube</a>
</p>

## Features

- Correct Arabic joining, ligatures, and bidirectional Arabic/Latin layout.
- Primary and fallback fonts, including user-installed fonts.
- Configurable size, spacing, wrapping, position, alignment, colors, outline,
  shadow, background, timing, and fades.
- Support for common Valve caption tags such as `<clr>`, `<playerclr>`, `<I>`,
  `<B>`, `<cr>`, `<delay>`, and `<sb>`.
- In-game HTML settings panel with live preview, save, and reset actions.
- Arabic and English settings UI.
- Persistent configuration in `resources/settings.ini`.

## Documentation

- [Live wiki](https://majd-tqe.github.io/Hlyx-Caption/)

## Requirements

### To run

- Windows 10 or 11, x64.
- Half-Life: Alyx and a D3D11-capable GPU.
- The Hlyx Caption DLL, deployment resources, and the Ultralight runtime.

### To build

- Visual Studio 2022 with **Desktop development with C++**.
- MSVC v143 and a Windows 10 SDK.
- [vcpkg](https://github.com/microsoft/vcpkg) with the
  `x64-windows-static-md` triplet.
- [Ultralight SDK](https://ultralig.ht/) supplied separately.

The native target uses C++17 and produces an x64 DLL only.

## Build from source

Run the following from a **Developer PowerShell for Visual Studio 2022**:

```powershell
$env:VCPKG_ROOT = 'C:\path\to\vcpkg'
$env:ULTRALIGHT_SDK_ROOT = 'C:\path\to\ultralight'

# The project reads libraries from VCPKG_ROOT\installed\x64-windows-static-md.
& "$env:VCPKG_ROOT\vcpkg.exe" install `
  --triplet x64-windows-static-md `
  --x-install-root="$env:VCPKG_ROOT\installed"

msbuild HlyxCaption.sln -p:Configuration=Release -p:Platform=x64
```

The output is:

```text
HlyxCaption/x64/Release/wininet.dll
```

You can pass `-p:VcpkgRoot=...` and `-p:UltralightSdkRoot=...` to MSBuild
instead of setting environment variables.

## Installation

1. Locate the x64 game directory containing `hlvr.exe`, normally:

   ```text
   Steam/steamapps/common/Half-Life Alyx/game/bin/win64/
   ```

2. Copy the built `wininet.dll` into that directory.
3. Copy the contents of `HlyxCaption/dist/` into the same directory.
4. If the runtime DLLs are not present in your source checkout, copy
   `Ultralight.dll`, `UltralightCore.dll`, `WebCore.dll`, and `AppCore.dll` from
   your licensed Ultralight SDK, together with its required runtime resources.

The resulting layout should include:

```text
game/bin/win64/
├── hlvr.exe
├── wininet.dll
├── Ultralight.dll
├── UltralightCore.dll
├── WebCore.dll
├── AppCore.dll
├── Cairo-Regular.ttf
└── resources/
    ├── settings.ini
    ├── Cairo-Regular.ttf
    ├── icudt67l.dat
    └── cacert.pem
```

> [!CAUTION]
> Ultralight is proprietary. Its SDK binaries are intentionally not part of the
> public source and must not be redistributed unless its license permits it.
> Review [third-party notices](THIRD_PARTY_NOTICES.md) before packaging a build.

## Usage

Launch the game normally after deployment.

| Key | Action |
| --- | --- |
| `F10` | Open or close the settings panel |
| `F11` | Show or hide the caption overlay |

- Saved settings are written to `resources/settings.ini`.
- User fonts can be placed in `resources/` as `.ttf`, `.otf`, `.ttc`, `.otc`,
  `.woff`, or `.woff2` files.
- Runtime and hook diagnostics are written to `wininet_hook.log` next to
  `hlvr.exe`.

For every setting and its default value, see the
[configuration reference](docs/wiki/configuration.md).

## Known limitations

- Caption interception depends on a byte signature and may stop working after a
  game update. Check `wininet_hook.log` if captions do not appear.
- Text shaping is optimized for Arabic and Latin; other right-to-left scripts
  are not guaranteed to render correctly.
- Line wrapping currently recognizes ASCII spaces only.
- A few compatibility settings remain INI-only and some visual fields are not
  exposed by the HTML panel.
- Native builds and in-game compatibility are not currently covered by
  automated CI tests.

## Project structure

```text
HlyxCaption.sln
HlyxCaption/
├── hooks.cpp                 Bootstrap, caption hook, and configuration bridge
├── proxy.cpp                 Forwarding to the system WinINet library
├── caption_parser.cpp        Valve caption-tag parser
├── caption_queue.cpp         Timing, fades, and stacking
├── caption_texture.cpp       Wrapping and texture composition
├── shaper.cpp                FreeType, HarfBuzz, and FriBidi pipeline
├── renderer.cpp              D3D11/ImGui rendering and input
├── ui_ultralight/            HTML settings UI and D3D11 blitter
└── dist/                     Deployment resources
docs/wiki/                    English VitePress documentation
```

See [CONTRIBUTING.md](CONTRIBUTING.md) before submitting changes and
[SECURITY.md](SECURITY.md) for private vulnerability reporting. Original
project source is GPL-3.0-or-later; bundled code, fonts, SDKs, and runtime data
retain their own terms as documented in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
