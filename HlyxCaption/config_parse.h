/*
 * Hlyx Caption — original project source
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Hlyx Caption Contributors
 */

#pragma once
#include "config.h"
#include <string>

// -------------------------------------------------------------------
// Config file IO + parsing (testable, no hooks/DllMain dependencies).
//
// The settings panel writes settings.ini (resources/settings.ini) via
// WritePrivateProfileStringW, which creates new files as UTF-16LE (with
// BOM). LoadConfig() previously read the file as a plain byte stream, so a
// freshly-saved file silently lost every setting. These helpers normalize
// any encoding to UTF-8 before parsing.
// -------------------------------------------------------------------

// Read a file that may be UTF-8 (with or without BOM) or UTF-16LE/BE (with
// BOM) and decode it to a UTF-8 string. Returns false if the file cannot
// be opened or read. Files without a BOM are assumed to be UTF-8/ANSI text.
bool ReadFileToUtf8(const wchar_t* path, std::string& outUtf8);

// Parse key=value lines from UTF-8 config text into cfg.
//   - ';' and '#' start a comment (rest of the line is dropped)
//   - lines without '=' (sections, blanks) are skipped
//   - {ModDir} in string values is expanded using modDirUtf8 (DLL dir)
//   - later occurrences of a key win (INI semantics)
// Returns the number of recognized settings.
int ParseConfigText(const std::string& text, OverlayConfig& cfg,
                    const std::string& modDirUtf8);

// Apply the same finite-value, range, and path validation used by file loads
// to values received from the live settings UI.
void SanitizeOverlayConfig(OverlayConfig& cfg);
