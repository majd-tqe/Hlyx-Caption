/*
 * Hlyx Caption — original project source
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Hlyx Caption Contributors
 */

#pragma once
#include <string>
#include <vector>
#include <tuple>

struct CaptionPhrase;

struct CaptionFormatState {
    uint8_t r = 255, g = 255, b = 255, a = 255;
    bool bold = false;
    bool italic = false;
    std::vector<std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>> colorStack;
};

std::vector<CaptionPhrase> ParseCaptionText(const std::string& raw, bool fromPlayer = false);
std::vector<CaptionPhrase> ParseCaptionText(const std::string& raw, CaptionFormatState& state, bool fromPlayer = false);
