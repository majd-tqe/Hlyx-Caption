# Hlyx Caption — UI asset generator
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Hlyx Caption Contributors
#
# generate_embedded_ui.ps1
# Regenerates ui_ultralight/embedded_ui.h from the source assets:
#   assets/settings.html  (with settings.css inlined into <style>)
#   assets/settings.css
#   assets/bindings.js    (inlined into <script>)
#
# Usage:  pwsh ui_ultralight/generate_embedded_ui.ps1

$ErrorActionPreference = "Stop"
$root   = $PSScriptRoot
$htmlPath = Join-Path $root "assets\settings.html"
$cssPath  = Join-Path $root "assets\settings.css"
$jsPath   = Join-Path $root "assets\bindings.js"
$outPath  = Join-Path $root "embedded_ui.h"

# --- Banner image -> data: URI -------------------------------------------------
# The startup banner logo is embedded directly into the HTML as a base64 data URI
# so the runtime needs no disk files (consistent with the rest of the embedded UI).
$bannerPath = Join-Path $root "assets\banner.png"
$bannerUri = ""
if (Test-Path -LiteralPath $bannerPath) {
    $bytes = [System.IO.File]::ReadAllBytes($bannerPath)
    $b64 = [System.Convert]::ToBase64String($bytes)
    $bannerUri = "data:image/png;base64,$b64"
    Write-Host "Banner: embedded $($bytes.Length) bytes as data URI"
} else {
    Write-Host "WARNING: assets\banner.png not found - banner image will be missing"
}

$html = [System.IO.File]::ReadAllText($htmlPath)
$html = $html.Replace('{{BANNER_URI}}', $bannerUri)
$css  = [System.IO.File]::ReadAllText($cssPath)
$js   = [System.IO.File]::ReadAllText($jsPath)

# Inline the CSS into a <style> block and the JS into a <script> block.
$styleBlock = "<style>`n" + $css + "`n</style>"
$scriptBlock = "<script>`n" + $js + "`n</script>"
$html = $html.Replace('<link rel="stylesheet" href="settings.css" />', $styleBlock)
$html = $html.Replace('<script src="bindings.js"></script>', $scriptBlock)

# Split into chunks below MSVC's raw-string-literal size limit.
# Empirically MSVC errors C2026 ("string too big") somewhere above ~8.5KB
# per raw literal (the original hand-generated file used ~8.2-8.4KB chunks),
# so keep every chunk <= 7000 UTF-8 bytes.
$chunkMaxBytes = 7000
$lines = $html -split "`r?`n"
$chunks = [System.Collections.Generic.List[string]]::new()
$cur = ""
$first = $true
foreach ($line in $lines) {
    $lineBytes = [System.Text.Encoding]::UTF8.GetByteCount($line)

    # A single line longer than the literal budget (e.g. the embedded base64
    # banner image). Flush any pending chunk, then split the line into pieces.
    # NO newline is inserted between pieces - adjacent C string literals
    # concatenate with no separator, so the reassembled HTML is identical.
    if ($lineBytes -gt $chunkMaxBytes) {
        if ($cur.Length -gt 0) { $chunks.Add($cur + "`n"); $cur = "" }
        $first = $false

        # Split the long line into <= budget pieces. All but the LAST piece go
        # straight into $chunks; the last piece joins the normal accumulator so
        # the following line keeps its separating newline (handled below).
        $pieces = [System.Collections.Generic.List[string]]::new()
        if ($lineBytes -eq $line.Length) {
            # Pure ASCII (base64): safe to split by character count.
            for ($i = 0; $i -lt $line.Length; $i += $chunkMaxBytes) {
                $len = [Math]::Min($chunkMaxBytes, $line.Length - $i)
                $pieces.Add($line.Substring($i, $len))
            }
        } else {
            # Non-ASCII long line: split respecting UTF-8 byte boundaries.
            $sb = [System.Text.StringBuilder]::new()
            $byteCount = 0
            foreach ($ch in $line.ToCharArray()) {
                $chBytes = if ([int]$ch -lt 0x80) { 1 }
                           elseif ([int]$ch -lt 0x800) { 2 }
                           elseif ([int]$ch -lt 0xD800 -or [int]$ch -gt 0xDFFF) { 3 }
                           else { 3 }  # surrogate half; pair handled across chunks
                if ($byteCount + $chBytes -gt $chunkMaxBytes) {
                    if ($sb.Length -gt 0) { $pieces.Add($sb.ToString()); [void]$sb.Clear(); $byteCount = 0 }
                    else { break }  # single char bigger than budget: drop to avoid infinite loop
                }
                [void]$sb.Append($ch)
                $byteCount += $chBytes
            }
            if ($sb.Length -gt 0) { $pieces.Add($sb.ToString()) }
        }
        for ($p = 0; $p -lt $pieces.Count - 1; $p++) { $chunks.Add($pieces[$p]) }
        if ($pieces.Count -gt 0) { $cur = $pieces[$pieces.Count - 1] }
        continue
    }

    # Separator model: lines are joined with "\n". The first line of the whole
    # document has no leading newline; every later line (including lines that
    # follow an empty line) gets the "\n" separator. When a chunk boundary
    # occurs, the separator lives at the END of the flushed chunk, so the next
    # chunk simply starts with its first line.
    $candidate = if ($first) { $line } else { $cur + "`n" + $line }
    $first = $false
    if ([System.Text.Encoding]::UTF8.GetByteCount($candidate) -gt $chunkMaxBytes -and $cur.Length -gt 0) {
        # Flush with the separator newline preserved so the reassembled HTML
        # is byte-identical to the source (modulo \r\n -> \n normalization).
        $chunks.Add($cur + "`n")
        $cur = $line
    } else {
        $cur = $candidate
    }
}
if ($cur.Length -gt 0) { $chunks.Add($cur) }

# Pick raw-string delimiters ("C0", "C1", ...) that do not collide with content.
function New-Delimiter([string]$content, [int]$idx) {
    $n = $idx
    while ($true) {
        $d = "C$n"
        if (-not $content.Contains(")$d`"")) { return $d }
        $n++
    }
}

$sb = [System.Text.StringBuilder]::new()
[void]$sb.AppendLine("// Hlyx Caption original source — GPL-3.0-or-later.")
[void]$sb.AppendLine("// Copyright (C) 2026 Hlyx Caption Contributors")
[void]$sb.AppendLine("// Auto-generated header. Do not edit by hand.")
[void]$sb.AppendLine("// Generated from ui_ultralight/assets/settings.html (settings.css + bindings.js inlined) at compile time.")
[void]$sb.AppendLine("// Rebuild with: pwsh ui_ultralight\generate_embedded_ui.ps1")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("#pragma once")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("#include <cstddef>")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("namespace hlyx_caption {")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("// Large string stored as concatenation of raw string chunks")
[void]$sb.AppendLine("// (MSVC imposes a limit on single raw string literal size).")
[void]$sb.AppendLine("static const char kEmbeddedHTML[] =")
for ($i = 0; $i -lt $chunks.Count; $i++) {
    $delim = New-Delimiter $chunks[$i] $i
    [void]$sb.Append("    R`"$delim(")
    [void]$sb.Append($chunks[$i])
    [void]$sb.AppendLine(")$delim`"")
}
[void]$sb.AppendLine(";")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("static const int kEmbeddedHTMLLen = sizeof(kEmbeddedHTML) - 1;")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("}  // namespace hlyx_caption")

# UTF-8 without BOM (matches the current header; /utf-8 is set in the vcxproj).
$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
[System.IO.File]::WriteAllText($outPath, $sb.ToString(), $utf8NoBom)

Write-Host "Wrote $outPath"
Write-Host "  chunks: $($chunks.Count) | total chars: $($html.Length)"

# --- Round-trip verification -------------------------------------------------
# Concatenate the emitted raw-string chunks back together and make sure they
# reproduce the exact expected HTML (banner data URI included). This catches
# any chunk-splitting bug (e.g. MSVC C2026 "string too big") before the build.
$headerText = [System.IO.File]::ReadAllText($outPath)
# Backreference pattern: the closing raw-string delimiter must equal the
# opening one (R"Cn(...)Cn"). Static regex call to avoid PS overload confusion.
$pattern = 'R"(C\d+)\((.*?)\)\1"'
$reassembled = ""
foreach ($m in [regex]::Matches($headerText, $pattern, [System.Text.RegularExpressions.RegexOptions]::Singleline)) {
    $reassembled += $m.Groups[2].Value
}
# The generator joins source lines with \n (dropping \r); HTML is insensitive
# to that, so normalize the expected side before comparing.
$expected = $html.Replace("`r`n", "`n")
if ($reassembled -eq $expected) {
    Write-Host "  round-trip OK ($($reassembled.Length) chars)"
} else {
    Write-Host "  ERROR: round-trip mismatch! generated=$($reassembled.Length) expected=$($expected.Length)"
    exit 1
}
