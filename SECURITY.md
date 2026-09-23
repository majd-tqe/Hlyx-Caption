# Security policy

Hlyx Caption uses DLL proxying and signature-based hooks, so security reports
should not be posted publicly until a fix is available.

## Reporting

When this repository is hosted on GitHub, use GitHub's private vulnerability
reporting feature. Until it is enabled, contact the maintainers through a
private GitHub channel and do not include credentials, private game files, or
unredacted logs in an issue.

Please include the affected commit or release, Windows/game version, a minimal
reproduction, and the security impact. Allow reasonable time for triage before
public disclosure.

## Scope

Reports about arbitrary file access, code execution, unsafe hook behavior,
credential exposure, or release artifacts containing private/proprietary files
are in scope. Compatibility problems and ordinary crashes should be filed as
normal issues after removing personal paths and module addresses from logs.
