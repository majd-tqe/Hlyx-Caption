# Module: Core / Bootstrap

> Files: `proxy.cpp`, `hooks.cpp`, `wininet.def`, `version.rc`/`resource.h`

## Responsibilities

- Make the DLL load instead of the system `wininet.dll` and remain invisible to the game by forwarding all 8 export entries to the real library.
- Bootstrap the mod on a background thread: install crash diagnostics, acquire DPI awareness, resolve `g_ModDirA`/`g_IniPath`, load `resources/settings.ini`, initialize MinHook, hook user32 cursor APIs early, sig-scan `client.dll` for the caption delivery function and the `cc_subtitles` ConVar pointer, and finally hook DXGI `Present`/`Resize`.
- Provide the `hkProcess` caption hook, global state (`g_ModDirA`, `g_IniPath`), and crash-logging helpers.

## Key Files

- `proxy.cpp` — 85 lines. `FORWARD_FUNC` macro: each export resolves the real library lazily through `GetRealWininetDll`, which calls `GetSystemDirectoryW`, appends `\wininet.dll`, and loads it with `LoadLibraryW` (proxy.cpp:24-35). There is no `GetModuleHandleW` call and no self-path comparison in this file. `GetProcAddress` runs once per export and publishes the pointer with `InterlockedCompareExchangePointer` — no lock anywhere (proxy.cpp:49-50). The call then goes out as a plain `return s_func(...)` through the function pointer (proxy.cpp:53). 7 functions, 8 exports (`InternetSetStatusCallback` has an `A` alias).
- `hooks.cpp` — `DllMain` (log `wininet_hook.log`, `DisableThreadLibraryCalls`, `CreateThread(MainThread)`), `InstallCrashDiagnostics` (`SetUnhandledExceptionFilter` + `_set_invalid_parameter_handler` + `_set_purecall_handler` + `_set_abort_behavior(0, _WRITE_ABORT_MSG)` + `signal(SIGABRT)`), `MainThread` (DPI `PER_MONITOR_AWARE_V2`, `SetDllDirectoryW`, `LoadConfig`, `MH_Initialize`, early `SetCursorPos`/`ClipCursor`/`SetCapture` hook creation from `hooks.cpp` though the bodies live in `renderer.cpp`, caption-function signature retry loop `60x500 ms`, discovery of the `cc_subtitles` ConVar pointer via signature in `client.dll`'s `Process`, then `Renderer::Initialize`), `hkProcess` (SFX filter before `Renderer::SetCaptionText`), `SaveConfig` (via `WritePrivateProfileStringW`). `SetCursorPos` is explicitly enabled; `ClipCursor` and `SetCapture` are created but are not explicitly enabled in the current bootstrap order. One correction on the diagnostics: only `OnUnhandledException` records the faulting module and offset through `EnumProcessModules`/`GetModuleInformation`. The other three handlers (`OnInvalidParameter`, `OnPureCall`, `OnAbort`) log a text message only.
- `wininet.def` — `LIBRARY wininet` + 8 `EXPORTS` entries.
- `version.rc` — `VS_VERSION_INFO` `0.9.5.0`.
- PCH files were deleted; precompiled headers are disabled (`NotUsing` everywhere).

## Public API

```cpp
// hooks.cpp — DllMain is the OS entry; MainThread is CreateThread entry.
BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID);
DWORD WINAPI MainThread(LPVOID);

// renderer.h declares only what hooks.cpp takes from it:
double GetTimeQPC();              // QPC seconds
float GetScaledFontSize(float);   // DPI-scaled font size
// class Renderer (see renderer.h) — Initialize, SetCaptionText live there.

// LoadConfig / SaveConfig / ResetConfig, g_IniPath, g_Config / g_DefaultConfig
// come from config.h, config_parse.h, and hooks.cpp — not from renderer.h.
extern wchar_t g_IniPath[];          // {ModDir}\resources\settings.ini
extern OverlayConfig g_Config, g_DefaultConfig;
```

## Internal Structure

```
DllMain(ATTACH) → log → DisableThreadLibraryCalls → CreateThread(MainThread)

MainThread:
  InstallCrashDiagnostics
  DPI (thread then process)
  SetDllDirectoryW(ModDir) ; g_ModDirA ← UTF-8 ModDir
  g_IniPath ← ModDir + L"\\resources\\settings.ini"
  LoadConfig(g_IniPath)
  MH_Initialize
  create user32: SetCursorPos/ClipCursor/SetCapture (bodies in renderer.cpp hk*; only SetCursorPos is explicitly enabled here)
  loop 60x { FindSignature(client.dll, "F3 0F 11 5C 24 ? …") }  -- 500 ms sleeps
  discover cc_subtitles ConVar pointer via client.dll Process signature
  MH_CreateHook(found, hkProcess, &oProcess) + enable
  Renderer::Initialize → HookPresent (temp window+device vtable patch)

void __fastcall hkProcess(void* pThis, void* stream, float duration,
            const char* tokenstream, bool fromplayer, bool direct):
  if IsSfxHidden() && raw contains exact "<sfx>" → drop entire raw caption
  Renderer::SetCaptionText(raw, duration, fromplayer)   // enters m_CS
```

Note on the caption hook: the second parameter is `void* stream`, not a `const char*` string. The body reads up to 16384 bytes from it with `strnlen` and builds the text from there. And the original is never called back — `oProcess` is stored and left unused.

## Dependencies

- Uses: `MinHook.h` (`MH_Initialize`/`MH_CreateHook`), `renderer.h` (`Renderer::Initialize`, `SetCaptionText`, `GetTimeQPC`), `config.h`/`config_parse.h`, `ui_ultralight/UL_Debug.h` (`UL_LogToFile`), Win32 (`Psapi` for crash logging).
- Used by: OS loader (DllMain), game `client.dll` (hkProcess is called by the game), `renderer.cpp` (reads `g_IniPath`/`g_Config`).

## Notable Patterns / Gotchas

- `DllMain` does almost nothing — only logging + `CreateThread`. All heavy work is in `MainThread` to avoid loader-lock deadlocks.
- `WritePrivateProfileStringW` in `SaveConfig` makes fresh INI files UTF-16LE with BOM; `ReadFileToUtf8` must handle that on reload — the original bug was silent loss of every setting after first save.
- The caption-function signature (`F3 0F 11 5C 24 ? 48 89 54 24` in `client.dll`) and the separate `cc_subtitles` pointer discovery can both break after a game update. `IsSfxHidden()` reads the ConVar integer at `+0x58` dynamically; failed discovery or a null pointer defaults to showing SFX. This does not add a mod INI or UI setting.
- `version.rc` holds the Windows DLL version (`0.9.5.0`); keep the package versions in `package.json` and `vcpkg.json` (`0.9.5`) synchronized when releasing.
- PCH files were deleted (were disabled anyway).
