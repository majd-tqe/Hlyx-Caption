/*
 * Hlyx Caption — original project source
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Hlyx Caption Contributors
 */

#include <windows.h>
#ifdef _DEBUG
#include <iostream>
#endif
#include <vector>
#include <algorithm>
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>
#include <cctype>
#include <cstdio>
#include <cstdarg>
#include <crtdbg.h>
#include <signal.h>
#include <Psapi.h>
#include "MinHook.h"
#include "renderer.h"
#include "config.h"
#include "config_parse.h"
#include "ui_ultralight/UL_Debug.h"

// =====================================================================
// Fatal-error diagnostics. A deterministic crash on F10 (0xc0000409 =
// __fastfail inside ucrtbase) means a CRT fatal error fired somewhere in
// the open-settings path. These hooks log WHERE before the CRT terminates
// the process, turning the next occurrence into actionable data.
// =====================================================================
static void __cdecl OnInvalidParameter(const wchar_t* expression, const wchar_t* function,
                                       const wchar_t* file, unsigned int line, uintptr_t reserved) {
    UL_LogToFile("[CRT] invalid parameter: expr=%ls func=%ls file=%ls line=%u",
                 expression ? expression : L"?",
                 function ? function : L"?",
                 file ? file : L"?", line);
    if (IsDebuggerPresent()) _CrtDbgBreak();
}

static void __cdecl OnPureCall() {
    UL_LogToFile("[CRT] pure virtual function call (purecall)");
}

static void __cdecl OnAbort(int signal) {
    UL_LogToFile("[CRT] abort()/SIGABRT received (signal=%d) - process will terminate", signal);
}

static LONG WINAPI OnUnhandledException(EXCEPTION_POINTERS* ep) {
    EXCEPTION_RECORD* er = ep ? ep->ExceptionRecord : nullptr;
    UL_LogToFile("[CRASH] Unhandled exception code=0x%08lX addr=%p",
                 er ? er->ExceptionCode : 0,
                 er ? (void*)er->ExceptionAddress : nullptr);

    // Best-effort: log which module the fault address lives in.
    if (er) {
        HMODULE modules[64];
        DWORD needed = 0;
        if (EnumProcessModules(GetCurrentProcess(), modules, sizeof(modules), &needed)) {
            DWORD count = needed / sizeof(HMODULE);
            for (DWORD i = 0; i < count && i < 64; ++i) {
                MODULEINFO mi = {};
                if (GetModuleInformation(GetCurrentProcess(), modules[i], &mi, sizeof(mi))) {
                    uintptr_t base = (uintptr_t)mi.lpBaseOfDll;
                    uintptr_t end = base + mi.SizeOfImage;
                    uintptr_t fault = (uintptr_t)er->ExceptionAddress;
                    if (fault >= base && fault < end) {
                        char name[MAX_PATH] = "?";
                        GetModuleBaseNameA(GetCurrentProcess(), modules[i], name, MAX_PATH);
                        UL_LogToFile("[CRASH] faulting module: %s (base=%p offset=0x%llX)",
                                     name, (void*)base, (unsigned long long)(fault - base));
                        break;
                    }
                }
            }
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

static void InstallCrashDiagnostics() {
    UL_LogToFile("[DIAG] Installing crash diagnostics (unhandled-exception filter + CRT handlers)");
    SetUnhandledExceptionFilter(OnUnhandledException);
    _set_invalid_parameter_handler(OnInvalidParameter);
    _set_purecall_handler(OnPureCall);
    _set_abort_behavior(0, _WRITE_ABORT_MSG);  // no blocking "abort() has been called" dialog
    signal(SIGABRT, OnAbort);
}

// User32 mouse capture hooks (implemented in renderer.cpp)
extern BOOL WINAPI hkSetCursorPos(int X, int Y);
extern BOOL WINAPI hkClipCursor(const RECT* lpRect);
extern HWND WINAPI hkSetCapture(HWND hWnd);

extern decltype(&SetCursorPos) oSetCursorPos;
extern decltype(&ClipCursor)    oClipCursor;
extern decltype(&SetCapture)    oSetCapture;

static std::string g_ModDirA; // DLL directory in UTF-8, for {ModDir} expansion
wchar_t g_IniPath[MAX_PATH] = L""; // INI path for SaveConfig (extern accessible)

static std::vector<int> PatternToBytes(const char* pattern) {
    std::vector<int> bytes;
    for (const char* p = pattern; *p; ) {
        if (*p == '?') { bytes.push_back(-1); p++; if (*p == '?') p++; }
        else { char buffer[3] = { p[0], p[1], 0 }; bytes.push_back(strtoul(buffer, 0, 16)); p += 2; }
        while (*p == ' ') p++;
    }
    return bytes;
}

// Read a hook argument only while it remains inside committed, readable
// virtual-memory regions. This does not make arbitrary game pointers safe, but
// it prevents a malformed pointer from crossing into an unmapped page during
// the bounded NUL scan.
static size_t SafeCStringLength(const char* text, size_t maxLength) {
    if (!text || maxLength == 0) return 0;
    size_t offset = 0;
    while (offset < maxLength) {
        MEMORY_BASIC_INFORMATION mbi = {};
        if (VirtualQuery(text + offset, &mbi, sizeof(mbi)) != sizeof(mbi) ||
            mbi.State != MEM_COMMIT ||
            (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0) {
            return offset;
        }

        const uintptr_t regionEnd = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
        const uintptr_t current = (uintptr_t)(text + offset);
        if (regionEnd <= current) return offset;
        const size_t available = (size_t)std::min<uintptr_t>(
            regionEnd - current, maxLength - offset);
        const void* nul = std::memchr(text + offset, '\0', available);
        if (nul) return (size_t)((const char*)nul - text);
        offset += available;
        if (available == 0) break;
    }
    return maxLength;
}

uintptr_t FindSignature(const char* moduleName, const char* pattern) {
    HMODULE hModule = GetModuleHandleA(moduleName);
    if (!hModule) return 0;
    MODULEINFO mi = { 0 };
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &mi, sizeof(mi)) ||
        mi.SizeOfImage == 0) return 0;
    uintptr_t startAddress = (uintptr_t)mi.lpBaseOfDll;

    auto patternBytes = PatternToBytes(pattern);
    if (patternBytes.empty() || patternBytes.size() > mi.SizeOfImage) return 0;
    uint8_t* scanBytes = reinterpret_cast<uint8_t*>(startAddress);

    for (uintptr_t i = 0; i + patternBytes.size() <= mi.SizeOfImage; i++) {
        bool found = true;
        for (size_t j = 0; j < patternBytes.size(); j++) {
            if (scanBytes[i + j] != patternBytes[j] && patternBytes[j] != -1) { found = false; break; }
        }
        if (found) return reinterpret_cast<uintptr_t>(&scanBytes[i]);
    }
    return 0;
}

// -------------------------------------------------------------------
// LoadConfig — read settings.ini for font_size and custom_font_path
// {ModDir} expands to the directory containing wininet.dll
//
// The file lives at {ModDir}\resources\settings.ini (migrated from the
// legacy {ModDir}\HLAMod.ini). Encoding-robust: the settings panel writes
// this file via WritePrivateProfileStringW, which creates new files as
// UTF-16LE. The actual decoding + parsing lives in config_parse.cpp.
// -------------------------------------------------------------------
void LoadConfig(const wchar_t* iniPath) {
    if (!iniPath || !iniPath[0]) {
#ifdef _DEBUG
        std::cout << "[Config] No config path — using built-in defaults\n";
#endif
        return;
    }

    std::string text;
    if (!ReadFileToUtf8(iniPath, text)) {
#ifdef _DEBUG
        std::wcout << L"[Config] No settings.ini found at " << iniPath << L" — using built-in defaults\n";
#endif
        return;
    }
#ifdef _DEBUG
    std::wcout << L"[Config] Loading " << iniPath << L"\n";
#endif

    int loaded = ParseConfigText(text, g_Config, g_ModDirA);
#ifdef _DEBUG
    std::cout << "[Config] Loaded " << loaded << " setting(s)\n";
#endif
}

// -------------------------------------------------------------------
// SaveConfig — write all settings to resources/settings.ini
// -------------------------------------------------------------------
static std::wstring Utf8ToWstring(const std::string& s) {
    if (s.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &result[0], size);
    return result;
}

void SaveConfig(const wchar_t* iniPath) {
    // Ensure the target directory exists (resources\ may not exist on first run)
    if (iniPath && iniPath[0]) {
        wchar_t dir[MAX_PATH] = L"";
        wcscpy_s(dir, MAX_PATH, iniPath);
        wchar_t* slash = wcsrchr(dir, L'\\');
        if (slash) { *slash = L'\0'; CreateDirectoryW(dir, nullptr); }
    }
    auto WriteStr = [&](const wchar_t* section, const wchar_t* key, const std::string& val) {
        WritePrivateProfileStringW(section, key, Utf8ToWstring(val).c_str(), iniPath);
    };
    auto WriteInt = [&](const wchar_t* section, const wchar_t* key, int val) {
        WritePrivateProfileStringW(section, key, std::to_wstring(val).c_str(), iniPath);
    };
    auto WriteFloat = [&](const wchar_t* section, const wchar_t* key, float val) {
        wchar_t buf[64];
        swprintf_s(buf, L"%.2f", val);
        WritePrivateProfileStringW(section, key, buf, iniPath);
    };
    auto WriteBool = [&](const wchar_t* section, const wchar_t* key, bool val) {
        WritePrivateProfileStringW(section, key, val ? L"1" : L"0", iniPath);
    };

    // Text Color
    WriteInt(L"Text", L"text_r", g_Config.text_r);
    WriteInt(L"Text", L"text_g", g_Config.text_g);
    WriteInt(L"Text", L"text_b", g_Config.text_b);
    WriteInt(L"Text", L"text_a", g_Config.text_a);

    // Shadow
    WriteBool(L"Shadow", L"shadow_enabled", g_Config.shadow_enabled);
    WriteInt(L"Shadow", L"shadow_r", g_Config.shadow_r);
    WriteInt(L"Shadow", L"shadow_g", g_Config.shadow_g);
    WriteInt(L"Shadow", L"shadow_b", g_Config.shadow_b);
    WriteInt(L"Shadow", L"shadow_a", g_Config.shadow_a);
    WriteFloat(L"Shadow", L"shadow_offset_x", g_Config.shadow_offset_x);
    WriteFloat(L"Shadow", L"shadow_offset_y", g_Config.shadow_offset_y);

    // Outline
    WriteBool(L"Outline", L"outline_enabled", g_Config.outline_enabled);
    WriteInt(L"Outline", L"outline_r", g_Config.outline_r);
    WriteInt(L"Outline", L"outline_g", g_Config.outline_g);
    WriteInt(L"Outline", L"outline_b", g_Config.outline_b);
    WriteInt(L"Outline", L"outline_a", g_Config.outline_a);
    WriteFloat(L"Outline", L"outline_thickness", g_Config.outline_thickness);

    // Position & Alignment
    WriteFloat(L"Position", L"pos_x", g_Config.pos_x);
    WriteFloat(L"Position", L"pos_y", g_Config.pos_y);
    const char* alignStr = "center";
    if (g_Config.text_alignment == OverlayConfig::ALIGN_LEFT) alignStr = "left";
    else if (g_Config.text_alignment == OverlayConfig::ALIGN_RIGHT) alignStr = "right";
    WriteStr(L"Position", L"text_alignment", alignStr);

    // Timing
    WriteFloat(L"Timing", L"fade_in_time", g_Config.fade_in_time);
    WriteFloat(L"Timing", L"fade_out_time", g_Config.fade_out_time);
    WriteFloat(L"Timing", L"extra_display_time", g_Config.extra_display_time);

    // Typography
    WriteFloat(L"Typography", L"font_size", g_Config.font_size);
    WriteFloat(L"Typography", L"font_size_reference_height", g_Config.font_size_reference_height);
    WriteFloat(L"Typography", L"line_spacing", g_Config.line_spacing);
    WriteStr(L"Typography", L"custom_font_path", g_Config.custom_font_path);
    WriteStr(L"Typography", L"fallback_font_path", g_Config.fallback_font_path);

    // Word Wrap
    WriteFloat(L"WordWrap", L"max_line_width_percent", g_Config.max_line_width_percent);

    // Mouse
    WriteFloat(L"Mouse", L"mouse_sensitivity", g_Config.mouse_sensitivity);
    WriteBool(L"Mouse", L"mouse_acceleration", g_Config.mouse_acceleration);
    WriteFloat(L"Mouse", L"mouse_accel_factor", g_Config.mouse_accel_factor);

    // Background Box
    WriteBool(L"Background", L"background_enabled", g_Config.background_enabled);
    WriteInt(L"Background", L"bg_r", g_Config.bg_r);
    WriteInt(L"Background", L"bg_g", g_Config.bg_g);
    WriteInt(L"Background", L"bg_b", g_Config.bg_b);
    WriteInt(L"Background", L"bg_a", g_Config.bg_a);
    WriteFloat(L"Background", L"bg_padding_x", g_Config.bg_padding_x);
    WriteFloat(L"Background", L"bg_padding_y", g_Config.bg_padding_y);
    WriteFloat(L"Background", L"bg_border_radius", g_Config.bg_border_radius);

    // UI
    WriteInt(L"UI", L"ui_language", g_Config.ui_language);
    WriteInt(L"UI", L"ui_direction_override", g_Config.ui_direction_override);
    WriteFloat(L"UI", L"ui_scale", g_Config.ui_scale);
    WriteBool(L"UI", L"ui_animations", g_Config.ui_animations);

#ifdef _DEBUG
    std::wcout << L"[Config] Saved to " << iniPath << L"\n";
#endif
}

void ResetConfig() {
    // The interface language is a user preference, not an overlay default.
    // Keep it when restoring the caption/display settings.
    const int currentUiLanguage = g_Config.ui_language;
    g_Config = g_DefaultConfig;
    g_Config.ui_language = (currentUiLanguage == 1 || currentUiLanguage == 2)
        ? currentUiLanguage
        : g_DefaultConfig.ui_language;
#ifdef _DEBUG
    std::cout << "[Config] Reset to defaults (UI language preserved)\n";
#endif
}

typedef void(__fastcall* tProcess)(void* pThis, void* stream, float duration, const char* tokenstream, bool fromplayer, bool direct);
tProcess oProcess = nullptr;

void __fastcall hkProcess(void* pThis, void* stream, float duration, const char* tokenstream, bool fromplayer, bool direct) {
    if (!stream) return;

#ifdef _DEBUG
    std::cout << "\n========================================\n";
    std::cout << "DURATION: " << duration << "s\n";

    std::cout << "[Hex] ";
    unsigned char* bytes = static_cast<unsigned char*>(stream);
    for (int i = 0; i < 32 && i < 256; i++) {
        printf("%02X ", bytes[i]);
        if (bytes[i] == 0 && i > 0) { std::cout << "| "; break; }
    }
    std::cout << "\n";
#endif

    size_t len = SafeCStringLength(static_cast<const char*>(stream), 16384);
    if (len > 0) {
        std::string raw(static_cast<const char*>(stream), len);
#ifdef _DEBUG
        std::cout << "[UTF-8] \"" << raw << "\"\n";
#endif
        Renderer::SetCaptionText(raw, duration, fromplayer);
    }
#ifdef _DEBUG
    else {
        std::cout << "[!] stream appears empty or binary\n";
    }

    std::cout << "========================================\n";
#endif
}

DWORD WINAPI MainThread(LPVOID lpParam) {
#ifdef _DEBUG
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    SetConsoleOutputCP(CP_UTF8);
#endif

    // === Install fatal-error diagnostics BEFORE anything else ===
    // Logs the exact location if a CRT/assert/exception error terminates us.
    InstallCrashDiagnostics();

    // === Enable DPI awareness ===
    // Without this, when Windows display scaling is set to e.g. 125%/150%,
    // the entire game window gets bitmap-stretched by the OS, making all
    // text and graphics look blurry. We try thread-level awareness first
    // (safer, doesn't affect the game) and fall back to process-level.
    {
        if (HINSTANCE user32_dll = ::LoadLibraryA("user32.dll")) {
            using PFN_SetThreadDpiAwarenessContext =
                DPI_AWARENESS_CONTEXT(WINAPI*)(DPI_AWARENESS_CONTEXT);
            auto fn = (PFN_SetThreadDpiAwarenessContext)::GetProcAddress(
                user32_dll, "SetThreadDpiAwarenessContext");
            if (fn) {
                // PER_MONITOR_AWARE_V2 = -4 (Windows 10 1703+)
                fn((DPI_AWARENESS_CONTEXT)-4);
            }
        }
        if (HINSTANCE shcore_dll = ::LoadLibraryA("Shcore.dll")) {
            using PFN_SetProcessDpiAwareness = HRESULT(WINAPI*)(int);
            auto fn = (PFN_SetProcessDpiAwareness)::GetProcAddress(
                shcore_dll, "SetProcessDpiAwareness");
            if (fn) fn(2); // PROCESS_PER_MONITOR_DPI_AWARE
        }
        ::SetProcessDPIAware();
    }

    // Add our DLL directory to the search path (for delay-loaded harfbuzz/freetype)
    wchar_t modPath[MAX_PATH];
    wchar_t iniPath[MAX_PATH] = L"";
    wchar_t oldIniPath[MAX_PATH] = L"";
    wchar_t resDir[MAX_PATH] = L"";
    if (GetModuleFileNameW((HMODULE)lpParam, modPath, MAX_PATH)) {
        wchar_t* lastSlash = wcsrchr(modPath, L'\\');
        if (lastSlash) {
            *(lastSlash + 1) = L'\0';
            SetDllDirectoryW(modPath);

            // Save as UTF-8 for {ModDir} expansion
            char dirA[MAX_PATH];
            WideCharToMultiByte(CP_UTF8, 0, modPath, -1, dirA, MAX_PATH, 0, 0);
            g_ModDirA = dirA;

            // Ensure {ModDir}\resources\ exists for settings.ini
            wcscpy_s(resDir, MAX_PATH, modPath);
            wcscat_s(resDir, MAX_PATH, L"resources");
            CreateDirectoryW(resDir, nullptr);

            // New location: {ModDir}\resources\settings.ini
            wcscpy_s(iniPath, MAX_PATH, modPath);
            wcscat_s(iniPath, MAX_PATH, L"resources\\settings.ini");
            wcscpy_s(g_IniPath, MAX_PATH, iniPath); // Save for SaveConfig

            // Legacy location for migration: {ModDir}\HLAMod.ini
            wcscpy_s(oldIniPath, MAX_PATH, modPath);
            wcscat_s(oldIniPath, MAX_PATH, L"HLAMod.ini");

            // One-time migration: if new file missing but old exists, copy it
            if (GetFileAttributesW(iniPath) == INVALID_FILE_ATTRIBUTES &&
                GetFileAttributesW(oldIniPath) != INVALID_FILE_ATTRIBUTES) {
                CopyFileW(oldIniPath, iniPath, FALSE);
#ifdef _DEBUG
                std::wcout << L"[Config] Migrated legacy HLAMod.ini -> " << iniPath << L"\n";
#endif
            }
        }
    }

    // Load external config before anything else
    LoadConfig(iniPath);

#ifdef _DEBUG
    std::cout << "====================================\n";
    std::cout << " HL:A Caption Overlay (ImGui)      \n";
    std::cout << "====================================\n";
#endif

    // The caption hook may fire as soon as it is enabled, before the first
    // Present creates the ImGui context. Initialize synchronization first.
    Renderer::EnsureSynchronization();

    if (MH_Initialize() != MH_OK) {
#ifdef _DEBUG
        std::cout << "[-] MinHook init failed.\n";
#endif
        return 0;
    }

    // === Install user32 mouse-capture hooks IMMEDIATELY ===
    // Critical: do this BEFORE FindSignature, which can take 30+
    // seconds (60 × 500ms) waiting for client.dll to load. The hooks
    // must be ready as soon as the user presses F10.
    {
        auto LogToFile = [](const char* msg) {
            char path[MAX_PATH];
            if (GetModuleFileNameA(NULL, path, MAX_PATH)) {
                char* slash = strrchr(path, '\\');
                if (slash) *(slash + 1) = 0;
                strcat_s(path, sizeof(path), "wininet_hook.log");
            } else {
                strcpy_s(path, sizeof(path), "C:\\wininet_hook.log");
            }
            FILE* f = nullptr;
            fopen_s(&f, path, "a");
            if (f) {
                fputs(msg, f);
                fflush(f);
                fclose(f);
            }
        };

        // SetCursorPos
        MH_STATUS s1 = MH_CreateHookApi(L"user32", "SetCursorPos",
                                        &hkSetCursorPos,
                                        (LPVOID*)&oSetCursorPos);
        if (s1 == MH_OK) {
            MH_EnableHook(MH_ALL_HOOKS);
            LogToFile("[hkSetCursorPos] INSTALLED via MH_CreateHookApi\n");
        } else {
            char buf[128];
            snprintf(buf, sizeof(buf), "[hkSetCursorPos] FAILED: %d\n", s1);
            LogToFile(buf);
        }

        // ClipCursor
        MH_STATUS s2 = MH_CreateHookApi(L"user32", "ClipCursor",
                                        &hkClipCursor,
                                        (LPVOID*)&oClipCursor);
        if (s2 == MH_OK) {
            LogToFile("[hkClipCursor] INSTALLED via MH_CreateHookApi\n");
        } else {
            char buf[128];
            snprintf(buf, sizeof(buf), "[hkClipCursor] FAILED: %d\n", s2);
            LogToFile(buf);
        }

        // SetCapture
        MH_STATUS s3 = MH_CreateHookApi(L"user32", "SetCapture",
                                        &hkSetCapture,
                                        (LPVOID*)&oSetCapture);
        if (s3 == MH_OK) {
            LogToFile("[hkSetCapture] INSTALLED via MH_CreateHookApi\n");
        } else {
            char buf[128];
            snprintf(buf, sizeof(buf), "[hkSetCapture] FAILED: %d\n", s3);
            LogToFile(buf);
        }
    }

    uintptr_t addr = 0;
    for (int i = 0; i < 60; i++) {
        addr = FindSignature("client.dll",
            "F3 0F 11 5C 24 ? 48 89 54 24");
        if (addr) break;
        Sleep(500);
    }
    if (addr) {
        if (MH_CreateHook((LPVOID)addr, &hkProcess, (LPVOID*)&oProcess) == MH_OK) {
            MH_EnableHook((LPVOID)addr);
#ifdef _DEBUG
            std::cout << "[+] Hooked Process at 0x" << std::hex << addr << std::dec << "\n";
#endif
        }
    }
#ifdef _DEBUG
    else {
        std::cout << "[-] Process signature not found.\n";
    }
#endif


    if (!Renderer::Initialize()) {
#ifdef _DEBUG
        std::cout << "[-] Renderer init failed.\n";
#endif
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        // Log immediately so we know DLL was loaded
        char path[MAX_PATH];
        if (GetModuleFileNameA(NULL, path, MAX_PATH)) {
            char* slash = strrchr(path, '\\');
            if (slash) *(slash + 1) = 0;
            strcat_s(path, sizeof(path), "wininet_hook.log");
        } else {
            strcpy_s(path, sizeof(path), "C:\\wininet_hook.log");
        }
        FILE* f = nullptr;
        fopen_s(&f, path, "a");
        if (f) {
            fputs("[DllMain] DLL_PROCESS_ATTACH reached\n", f);
            fflush(f);
            fclose(f);
        }

        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, nullptr);
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        // Do not tear down D3D11/Ultralight/MinHook under the loader lock.
        // Process termination releases these resources automatically. A
        // deliberate unload must call the explicit shutdown API first.
        if (lpReserved == nullptr) {
            UL_LogToFile("[DllMain] explicit unload requested; shutdown must be coordinated by the host");
        }
    }
    return TRUE;
}
