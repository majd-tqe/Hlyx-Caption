# Changelog

## 0.9.5

- Follow the game's live `cc_subtitles` setting to drop whole raw captions
  containing exact `<sfx>` when nonzero; otherwise show SFX immediately after
  its scheduled start, with normal fade-out.
- Prepared the project metadata and documentation for GPL-3.0-or-later.
- Removed non-redistributable bundled font assets from the working tree.
- Added third-party license inventory and external SDK guidance.
- Replaced hard-coded vcpkg and Ultralight paths with configurable roots.
