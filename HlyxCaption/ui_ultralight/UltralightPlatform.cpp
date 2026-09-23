/*
 * Hlyx Caption — original project source
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Hlyx Caption Contributors
 */

// UltralightPlatform.cpp - Platform handlers implementation.
#include "UltralightPlatform.h"
#include "UL_Debug.h"
#include <Ultralight/Bitmap.h>
#include <Ultralight/String.h>
#include <Ultralight/Buffer.h>
#include <Ultralight/platform/FontLoader.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>
#include <fstream>
#include <vector>
#include <cstdio>
#include <cctype>

#pragma comment(lib, "Shlwapi.lib")

namespace ultralight {

// ====================== Logger ======================

void CustomLogger::LogMessage(LogLevel log_level, const String& message) {
    const char* level_str = "INFO";
    switch (log_level) {
        case LogLevel::Error:   level_str = "ERROR"; break;
        case LogLevel::Warning: level_str = "WARN";  break;
        case LogLevel::Info:    level_str = "INFO";  break;
    }
    char buf[2048];
    snprintf(buf, sizeof(buf), "[Ultralight/%s] %s\n", level_str, message.utf8().data());
    OutputDebugStringA(buf);
    printf("%s", buf);
    UL_LogToFile("%s", buf);
}

// ====================== File System ======================

CustomFileSystem::CustomFileSystem(const String& baseDir)
    : m_BaseDir(baseDir) {
    // Cache the DLL directory so we can locate assets and resources.
    char selfPath[MAX_PATH] = {};
    HMODULE hm = GetModuleHandleA("wininet.dll");
    if (hm) GetModuleFileNameA(hm, selfPath, MAX_PATH);
    char* slash = selfPath[0] ? strrchr(selfPath, '\\') : nullptr;
    if (slash) *(slash + 1) = 0;
    m_DllDir = selfPath;
}

static std::string NormalizeRelative(const std::string& path) {
    std::string p = path;
    // Strip "file:///" prefix
    const std::string filePrefix = "file:///";
    if (p.size() >= filePrefix.size() && p.substr(0, filePrefix.size()) == filePrefix)
        p = p.substr(filePrefix.size());
    // Convert '/' to '\', strip leading separators
    std::string cleaned;
    for (char c : p) {
        if (c == '/') cleaned.push_back('\\');
        else cleaned.push_back(c);
    }
    while (!cleaned.empty() && (cleaned.front() == '\\' || cleaned.front() == '/'))
        cleaned.erase(cleaned.begin());

    // The custom file system is rooted at the asset/DLL directories. Reject
    // absolute paths, drive prefixes, and parent traversal before joining.
    if (cleaned.empty() || cleaned.find(':') != std::string::npos)
        return {};
    size_t start = 0;
    while (start <= cleaned.size()) {
        size_t end = cleaned.find('\\', start);
        if (end == std::string::npos) end = cleaned.size();
        const std::string component = cleaned.substr(start, end - start);
        if (component == ".." || component == ".") return {};
        if (end == cleaned.size()) break;
        start = end + 1;
    }
    return cleaned;
}

static std::string JoinPath(const std::string& a, const std::string& b) {
    std::string out = a;
    if (!out.empty() && out.back() != '\\') out.push_back('\\');
    out += b;
    return out;
}

static std::string ResolveFromBase(const std::string& baseDir, const std::string& rel) {
    std::string out = baseDir;
    if (!out.empty() && out.back() != '\\') out.push_back('\\');
    out += rel;
    return out;
}

static bool FileExistsAny(const std::vector<std::string>& candidates) {
    for (const auto& p : candidates) {
        DWORD attrs = GetFileAttributesA(p.c_str());
        if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY))
            return true;
    }
    return false;
}

static std::vector<uint8_t> ReadFileBytes(const std::string& path) {
    std::vector<uint8_t> out;
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return out;
    const std::streampos end = f.tellg();
    if (end <= 0) return out;
    const auto maxBytes = (std::streamoff)(64ull * 1024ull * 1024ull);
    if (end > maxBytes) return out;
    auto sz = (size_t)end;
    if (sz == 0) return out;
    f.seekg(0);
    out.resize(sz);
    f.read((char*)out.data(), sz);
    return out;
}

bool CustomFileSystem::FileExists(const String& path) {
    std::string rel = NormalizeRelative(path.utf8().data());

    // Try several search paths:
    //   1) CustomFileSystem base dir (assets/)
    //   2) DLL directory (same as base in our case)
    //   3) <dll>/resources/...
    //   4) <dll>/<rel> directly
    std::vector<std::string> candidates;

    // Subfile inside assets/<rel>
    candidates.push_back(ResolveFromBase(m_BaseDir.utf8().data(), rel));

    // Direct file relative to the DLL dir (for resources/icudt67l.dat)
    if (!m_DllDir.empty()) candidates.push_back(ResolveFromBase(m_DllDir, rel));

    // If the path starts with "resources/" or matches a top-level SDK resource, allow it
    if (!m_DllDir.empty()) {
        std::string p = m_DllDir + rel;
        candidates.push_back(p);
    }

    return FileExistsAny(candidates);
}

String CustomFileSystem::GetFileMimeType(const String& path) {
    std::string p = path.utf8().data();
    auto endsWith = [&](const char* s) {
        size_t n = strlen(s);
        return p.size() >= n && p.compare(p.size() - n, n, s) == 0;
    };

    if (endsWith(".html")) return "text/html";
    if (endsWith(".htm"))  return "text/html";
    if (endsWith(".css"))  return "text/css";
    if (endsWith(".js"))   return "application/javascript";
    if (endsWith(".png"))  return "image/png";
    if (endsWith(".jpg"))  return "image/jpeg";
    if (endsWith(".jpeg")) return "image/jpeg";
    if (endsWith(".gif"))  return "image/gif";
    if (endsWith(".svg"))  return "image/svg+xml";
    if (endsWith(".ttf"))  return "font/ttf";
    if (endsWith(".otf"))  return "font/otf";
    if (endsWith(".woff")) return "font/woff";
    if (endsWith(".woff2"))return "font/woff2";
    if (endsWith(".dat"))  return "application/octet-stream";
    if (endsWith(".pem"))  return "application/x-pem-file";
    if (endsWith(".json")) return "application/json";
    if (endsWith(".xml"))  return "text/xml";
    return "application/octet-stream";
}

String CustomFileSystem::GetFileCharset(const String& path) {
    return "utf-8";
}

RefPtr<Buffer> CustomFileSystem::OpenFile(const String& path) {
    std::string rel = NormalizeRelative(path.utf8().data());

    // Try the same order as FileExists
    std::vector<std::string> candidates;
    candidates.push_back(ResolveFromBase(m_BaseDir.utf8().data(), rel));
    if (!m_DllDir.empty()) candidates.push_back(ResolveFromBase(m_DllDir, rel));
    if (!m_DllDir.empty()) {
        std::string p = m_DllDir + rel;
        candidates.push_back(p);
    }

    for (const auto& f : candidates) {
        auto bytes = ReadFileBytes(f);
        if (!bytes.empty()) {
            return Buffer::CreateFromCopy(bytes.data(), bytes.size());
        }
    }
    return nullptr;
}

// ====================== Font Loader ======================

CustomFontLoader::CustomFontLoader(const String& baseDir)
    : m_BaseDir(baseDir) {}

String CustomFontLoader::fallback_font_for_characters(const String& characters, int weight, bool italic) const {
    return "Tahoma";
}

RefPtr<FontFile> CustomFontLoader::Load(const String& family, int weight, bool italic) {
    std::string f = family.utf8().data();
    std::string lower;
    for (char c : f) lower.push_back((char)tolower((unsigned char)c));

    auto pick = [&](const char* path) -> std::vector<uint8_t> {
        return ReadFileBytes(path);
    };

    auto pickFromAssets = [&](const char* path) -> std::vector<uint8_t> {
        // Resolve relative to the baseDir so the loader can serve the
        // custom woff2 files bundled in <assets>/fonts/.
        std::string rel = path;
        for (char& c : rel) if (c == '/') c = '\\';
        std::string full = m_BaseDir.utf8().data();
        if (!full.empty() && full.back() != '\\') full.push_back('\\');
        full += rel;
        return ReadFileBytes(full);
    };

    std::vector<uint8_t> bytes;

    // Custom UI fonts — bundled as woff2 variable fonts in assets/fonts/.
    // We map any of the three families to the same single file (one per
    // subset) since they are variable fonts and the file covers all weights.
    if (lower.find("cairo") != std::string::npos) {
        // The Arabic subset is what the panel actually renders; the Latin
        // file is only used for stray Latin glyphs (e.g. "RESISTANCE TERMINAL").
        bytes = pickFromAssets("fonts/Cairo-Arabic.woff2");
        if (bytes.empty()) bytes = pickFromAssets("fonts/Cairo-Latin.woff2");
    }
    else if (lower.find("jetbrains") != std::string::npos) {
        bytes = pickFromAssets("fonts/JetBrainsMono-Latin.woff2");
    }
    else if (lower.find("oxanium") != std::string::npos) {
        bytes = pickFromAssets("fonts/Oxanium-Latin.woff2");
    }

    // System fonts (Windows).
    if (bytes.empty()) {
        if (lower.find("tahoma") != std::string::npos)
            bytes = pick("C:\\Windows\\Fonts\\tahoma.ttf");
        else if (lower.find("arial") != std::string::npos)
            bytes = pick("C:\\Windows\\Fonts\\segoeui.ttf"); // legacy config compatibility
        else if (lower.find("segoe") != std::string::npos)
            bytes = pick("C:\\Windows\\Fonts\\segoeui.ttf");
        else if (lower.find("courier") != std::string::npos)
            bytes = pick("C:\\Windows\\Fonts\\cour.ttf");
        else if (lower.find("verdana") != std::string::npos)
            bytes = pick("C:\\Windows\\Fonts\\verdana.ttf");
        else if (lower.find("times") != std::string::npos)
            bytes = pick("C:\\Windows\\Fonts\\times.ttf");
        else {
            bytes = pick("C:\\Windows\\Fonts\\tahoma.ttf");
            if (bytes.empty()) bytes = pick("C:\\Windows\\Fonts\\segoeui.ttf");
        }
    }

    if (bytes.empty()) return nullptr;
    auto buf = Buffer::CreateFromCopy(bytes.data(), bytes.size());
    return FontFile::Create(buf);
}

// ====================== Init ======================

static CustomLogger* g_LoggerInstance = nullptr;
static CustomFileSystem* g_FileSystemInstance = nullptr;
static CustomFontLoader* g_FontLoaderInstance = nullptr;

void InitializeUltralightPlatform(const String& baseDir) {
    if (!g_LoggerInstance) {
        g_LoggerInstance = new CustomLogger();
        Platform::instance().set_logger(g_LoggerInstance);
    }
    if (!g_FileSystemInstance) {
        g_FileSystemInstance = new CustomFileSystem(baseDir);
        Platform::instance().set_file_system(g_FileSystemInstance);
    }
    if (!g_FontLoaderInstance) {
        g_FontLoaderInstance = new CustomFontLoader(baseDir);
        Platform::instance().set_font_loader(g_FontLoaderInstance);
    }
}

}  // namespace ultralight
