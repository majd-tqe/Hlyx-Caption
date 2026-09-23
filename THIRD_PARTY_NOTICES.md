# Third-party notices

The original Hlyx Caption source code is licensed under the GNU General Public
License, version 3 or any later version. This file records components that are
distributed with, or required to build, Hlyx Caption. Those components keep
their own copyright and license terms; the GPL does not relicense them.

## Bundled or vendored source

| Component | Location | License / notice |
| --- | --- | --- |
| Dear ImGui | `HlyxCaption/imgui/` | MIT. The upstream license is available at <https://github.com/ocornut/imgui/blob/master/LICENSE.txt>. The embedded stb headers retain their own MIT/Public Domain notices. |
| MinHook | `HlyxCaption/minhook-detours-src/MinHook.*` | BSD-2-Clause. Source: <https://github.com/TsudaKageyu/minhook>. |
| SlimDetours | `HlyxCaption/minhook-detours-src/SlimDetours/` | MIT. Source: <https://github.com/KNSoft/KNSoft.SlimDetours>. |
| PHNT | `HlyxCaption/minhook-detours-src/phnt/` | MIT. Source: <https://github.com/winsiderss/phnt>. |
| Cairo fonts | `HlyxCaption/dist/**/Cairo-*` and UI assets | SIL Open Font License 1.1. Source: <https://github.com/googlefonts/cairo>. Include the applicable OFL text with releases. |
| JetBrains Mono | `HlyxCaption/**/JetBrainsMono-*` | SIL Open Font License 1.1. Source: <https://github.com/JetBrains/JetBrainsMono>. |
| Oxanium | `HlyxCaption/**/Oxanium-*` | SIL Open Font License 1.1. Source: <https://fonts.google.com/specimen/Oxanium> (verify the exact upstream asset and version before release). |

## External SDKs and runtime data

| Component | Location / use | Distribution status |
| --- | --- | --- |
| Ultralight SDK | `HlyxCaption/vendor/Ultralight/` and the Ultralight UI | Proprietary SDK. It is not relicensed under GPL. Obtain it from <https://ultralig.ht/> and follow its applicable license, EULA, attribution, and redistribution requirements. Do not publish SDK binaries or headers until those rights are confirmed. |
| ICU data | `HlyxCaption/dist/resources/icudt67l.dat` | Verify the exact ICU version and include its Unicode/ICU notices before distributing. |
| CA certificate bundle | `HlyxCaption/dist/resources/cacert.pem` | Verify provenance, freshness, and the upstream Mozilla/cURL notice for each release. |

## Deliberately excluded assets

The Windows Arial font and `HalfLifeETR.ttf` must not be treated as GPL
materials. They are not covered by this project's license and must be removed
from public source and release artifacts unless a written redistribution right
is available.

When preparing a release, regenerate this inventory from the exact files in the
release archive and attach all required license texts and notices.

Standard license texts used by the open components are kept in `licenses/`.
They are reference copies; the copyright and notice requirements of each
upstream project still apply.
