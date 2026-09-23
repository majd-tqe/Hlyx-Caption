/*
 * Hlyx Caption — original project source
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Hlyx Caption Contributors
 */

// UL_Debug.h - Shared file-based logging for the Ultralight UI subsystem.
// Writes diagnostic output to wininet_hook.log (next to hlvr.exe / the test
// harness) so we can trace the UI flow even when no console is visible.
#pragma once

#include <windows.h>
#include <cstdio>
#include <cstdarg>

inline void UL_LogToFile(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, args);
    va_end(args);

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
        fputs(buf, f);
        fputs("\n", f);
        fflush(f);
        fclose(f);
    }
}
