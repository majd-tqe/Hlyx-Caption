/*
 * Hlyx Caption — original project source
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Hlyx Caption Contributors
 */

// UltralightPlatform.h - Owns all the platform handlers that Ultralight needs.
#pragma once

#include <Ultralight/Ultralight.h>
#include <Ultralight/platform/Platform.h>
#include <string>

namespace ultralight {

// Custom logger - prints to debug output
class CustomLogger : public Logger {
public:
    CustomLogger() = default;
    void LogMessage(LogLevel log_level, const String& message) override;
};

// File system that searches multiple locations for assets.
class CustomFileSystem : public FileSystem {
public:
    CustomFileSystem(const String& baseDir);
    virtual bool FileExists(const String& path) override;
    virtual String GetFileMimeType(const String& path) override;
    virtual String GetFileCharset(const String& path) override;
    virtual RefPtr<Buffer> OpenFile(const String& path) override;

private:
    String m_BaseDir;
    std::string m_DllDir;
};

// Font loader that uses Windows to load system fonts, plus bundled
// woff2 files in <assets>/fonts/ for the custom UI fonts (Cairo, Oxanium,
// JetBrains Mono). The CSS @font-face declarations in settings.css hit the
// file system directly; this loader is only consulted when the WebKit engine
// falls back to a system-resolvable family name.
class CustomFontLoader : public FontLoader {
public:
    CustomFontLoader(const String& baseDir);
    virtual String fallback_font() const override { return "Arial"; }
    virtual String fallback_font_for_characters(const String& characters, int weight, bool italic) const override;
    virtual RefPtr<FontFile> Load(const String& family, int weight, bool italic) override;

private:
    String m_BaseDir;
};

void InitializeUltralightPlatform(const String& baseDir);

}  // namespace ultralight
