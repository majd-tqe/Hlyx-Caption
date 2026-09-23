/*
 * Hlyx Caption — original project source
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Hlyx Caption Contributors
 */

// proxy.cpp — wininet.dll proxy: forwards all exports to the real system wininet.dll
// This file is part of the HL:A Custom Caption Overlay mod.
// When placed alongside hlvr.exe, Windows loads this DLL instead of the system one,
// and it transparently forwards all WinINet API calls while running the mod code.

#include <windows.h>

// Forward declarations for WinINet types (avoid including wininet.h which conflicts)
DECLARE_HANDLE(HINTERNET);
typedef unsigned long DWORD;
typedef int BOOL;
typedef void* LPVOID;
typedef unsigned long* LPDWORD;
typedef const char* LPCSTR;
typedef const wchar_t* LPCWSTR;
typedef unsigned long long DWORD_PTR;

#define INTERNET_STATUS_CALLBACK void (WINAPI*)(HINTERNET, DWORD_PTR, DWORD, LPVOID, DWORD)
typedef void (WINAPI* INTERNET_STATUS_CALLBACK_PTR)(HINTERNET, DWORD_PTR, DWORD, LPVOID, DWORD);

// ---------------------------------------------------------------------------
// Load the real system wininet.dll from the system directory
// ---------------------------------------------------------------------------
static HMODULE GetRealWininetDll() {
    static INIT_ONCE once = INIT_ONCE_STATIC_INIT;
    static HMODULE hReal = nullptr;
    InitOnceExecuteOnce(&once, [](PINIT_ONCE, PVOID, PVOID*) -> BOOL {
        wchar_t path[MAX_PATH] = {};
        UINT len = GetSystemDirectoryW(path, MAX_PATH);
        if (len == 0 || len >= MAX_PATH) return TRUE;
        if (wcscat_s(path, L"\\wininet.dll") != 0) return TRUE;
        hReal = LoadLibraryW(path);
        return TRUE;
    }, nullptr, nullptr);
    return hReal;
}

// ---------------------------------------------------------------------------
// Macro: generates a forwarding function that loads the real DLL on first call
// and delegates to it. Uses InterlockedCompareExchangePointer for thread safety.
// ---------------------------------------------------------------------------
#define FORWARD_FUNC(ret, name, params, call_args)                             \
    extern "C" ret __stdcall name params {                                     \
        typedef ret (__stdcall* FuncType) params;                              \
        static volatile FuncType s_func = nullptr;                             \
        if (!InterlockedCompareExchangePointer(                              \
                (PVOID volatile*)&s_func, nullptr, nullptr)) {               \
            HMODULE hMod = GetRealWininetDll();                                \
            if (hMod) {                                                        \
                auto real = (FuncType)GetProcAddress(hMod, #name);             \
                InterlockedCompareExchangePointer(                             \
                    (PVOID volatile*)&s_func, (PVOID)real, nullptr);           \
            }                                                                  \
        }                                                                      \
        FuncType fn = (FuncType)InterlockedCompareExchangePointer(             \
            (PVOID volatile*)&s_func, nullptr, nullptr);                       \
        if (!fn) {                                                             \
            SetLastError(ERROR_PROC_NOT_FOUND);                                \
            return (ret)0;                                                      \
        }                                                                       \
        return fn call_args;                                                    \
    }

// ---- Exported functions (7) from system wininet.dll ----

FORWARD_FUNC(HINTERNET, InternetOpenA,
    (LPCSTR lpszAgent, DWORD dwAccessType, LPCSTR lpszProxy, LPCSTR lpszProxyBypass, DWORD dwFlags),
    (lpszAgent, dwAccessType, lpszProxy, lpszProxyBypass, dwFlags))

FORWARD_FUNC(HINTERNET, InternetOpenUrlA,
    (HINTERNET hInternet, LPCSTR lpszUrl, LPCSTR lpszHeaders, DWORD dwHeadersLength,
     DWORD dwFlags, DWORD_PTR dwContext),
    (hInternet, lpszUrl, lpszHeaders, dwHeadersLength, dwFlags, dwContext))

FORWARD_FUNC(BOOL, InternetCloseHandle,
    (HINTERNET hInternet),
    (hInternet))

FORWARD_FUNC(BOOL, InternetCrackUrlA,
    (LPCSTR lpszUrl, DWORD dwUrlLength, DWORD dwFlags, LPVOID lpUrlComponents),
    (lpszUrl, dwUrlLength, dwFlags, lpUrlComponents))

FORWARD_FUNC(BOOL, HttpQueryInfoA,
    (HINTERNET hRequest, DWORD dwInfoLevel, LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex),
    (hRequest, dwInfoLevel, lpBuffer, lpdwBufferLength, lpdwIndex))

FORWARD_FUNC(INTERNET_STATUS_CALLBACK_PTR, InternetSetStatusCallbackA,
    (HINTERNET hInternet, INTERNET_STATUS_CALLBACK_PTR lpfnInternetCallback),
    (hInternet, lpfnInternetCallback))

FORWARD_FUNC(BOOL, InternetReadFile,
    (HINTERNET hFile, LPVOID lpBuffer, DWORD dwNumberOfBytesToRead, LPDWORD lpdwNumberOfBytesRead),
    (hFile, lpBuffer, dwNumberOfBytesToRead, lpdwNumberOfBytesRead))
