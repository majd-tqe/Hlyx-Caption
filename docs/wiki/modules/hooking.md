# Module: Hooking & Injection

> Files: `minhook-detours-src/MinHook.{h,c}`, `minhook-detours-src/SlimDetours/*` (10 files), `minhook-detours-src/phnt/*.h` (41 headers), plus call sites in `hooks.cpp` and `renderer.cpp`; build flag `ModuleDefinitionFile=wininet.def`

Note: `MinHook.def` ships in the tree but the build never uses it. `HlyxCaption.vcxproj` sets `wininet.def` as the module definition file in both configurations, and MinHook is compiled in statically through `MinHook.c`. So the only link-time export surface is `wininet.def`.

## Responsibilities

- Provide the hooking engine (MinHook API over SlimDetours trampoline) and the DLL-proxy injection mechanism.
- Install and manage hooks for the game's caption function, DXGI `Present`/`Resize`, and Win32 cursor APIs.

## Key Files

- `minhook-detours-src/MinHook.h` + `MinHook.c` — MinHook `MH_*` API reimplemented over SlimDetours: `MH_Initialize`, `MH_CreateHook(target, detour, &orig)`, `MH_CreateHookApi`, `MH_EnableHook`, `MH_DisableHook`, `MH_RemoveHook`, `MH_Uninitialize`, `MH_StatusToString`. Thin shim; real work is in SlimDetours. The list is not exhaustive. The header (and the unused `MinHook.def`) also declares `Ex` variants, `Queue`/`ApplyQueued`, `RemoveDisabledHooks`, and `SetThreadFreezeMethod`, which this mod never calls. The one the docs used to omit matters most: `MH_CreateHookApi` is what the mod actually uses for the three user32 hooks.
- `minhook-detours-src/SlimDetours/` — 10 files: `SlimDetours.h`, `SlimDetours.inl`, `SlimDetours.NDK.inl`, `Transaction.c` (begin/commit transaction), `Trampoline.c` (allocate executable trampoline near target), `Thread.c` (suspend/resume threads to patch safely), `Memory.c` (VirtualProtect/VirtualAlloc helpers), `Instruction.c`/`Disassembler.c` (length-disassemble to find instruction boundaries for trampoline), `InlineHook.c` (write `JMP rel32` / `JMP [RIP+disp]`).
- `minhook-detours-src/phnt/` — 41 headers: `phnt.h`, `phnt_ntdef.h`, `phnt_windows.h`, `smbios.h`, `subprocesstag.h`, `usermgr.h`, `winsta.h`, plus 34 `nt*.h` Native API headers (`ntexapi`, `ntmmapi`, `ntpsapi`, `ntpebteb`, …) used by SlimDetours for `NtProtectVirtualMemory`, `NtQueryInformationProcess`, TEB/PEB access without Win32 wrappers.
- Call sites:
  - `hooks.cpp:MainThread` — `MH_Initialize`, then the three user32 hooks via `MH_CreateHookApi(L"user32", ...)` (hooks.cpp:475-511), then the caption hook via `MH_CreateHook(foundAddr, hkProcess, &oProcess)` where the target comes from a `client.dll` signature scan with a 60x500 ms retry loop (hooks.cpp:513-522). The hook bodies (`hkSetCursorPos`/`hkClipCursor`/`hkSetCapture`) live in `renderer.cpp`; only the installation lives in `hooks.cpp`. One detail to get right: `MH_EnableHook(MH_ALL_HOOKS)` appears exactly once, right after the `SetCursorPos` hook succeeds (hooks.cpp:480). There is no enable call after the `ClipCursor`/`SetCapture` creations. So describing all three as enabled immediately overstates it.
  - `renderer.cpp:HookPresent` — temp window+`D3D11CreateDeviceAndSwapChain` → `swapchain->lpVtbl[8]` (Present) + `[13]` (ResizeBuffers) → `MH_CreateHook(vtable[i], hkPresent/hkResize, &oPresent/oResize)`.
  - `renderer.cpp:hkSetCursorPos/hkClipCursor/hkSetCapture` — while `m_SettingsOpen`, return success without forwarding (swallow); otherwise tail-call `o*` trampolines. The current bootstrap explicitly enables only `SetCursorPos`; `ClipCursor` and `SetCapture` are created but remain disabled unless the enable order changes. The render path still calls the `oClipCursor(nullptr)` trampoline directly.
  - Caption hook behavior: `hkProcess` reads `stream` and forwards text to the renderer. It never calls `oProcess`. The pointer is stored at creation (hooks.cpp:521) and never invoked anywhere. The original function is dropped entirely, not chained through a trampoline.

## Public API

```c
// MinHook.h — the surface the mod actually uses
typedef enum MH_STATUS { MH_OK=0, MH_ERROR_ALREADY_INITIALIZED, ... } MH_STATUS;
MH_STATUS MH_Initialize(void);
MH_STATUS MH_Uninitialize(void);
MH_STATUS MH_CreateHook(void* target, void* detour, void** original);
MH_STATUS MH_CreateHookApi(const wchar_t* library, const char* function, void* detour, void** original);
MH_STATUS MH_EnableHook(void* target);   // target==MH_ALL_HOOKS for all
MH_STATUS MH_DisableHook(void* target);
MH_STATUS MH_RemoveHook(void* target);
const char* MH_StatusToString(MH_STATUS);

// wininet.def exports (proxy injection surface)
EXPORTS
  InternetOpenA
  InternetOpenUrlA
  InternetCloseHandle
  InternetCrackUrlA
  HttpQueryInfoA
  InternetSetStatusCallback = InternetSetStatusCallbackA
  InternetSetStatusCallbackA
  InternetReadFile
```

## Internal Structure

```
Deploy: wininet.dll beside hlvr.exe
Load:   Windows loader resolves game imports → finds .\wininet.dll before System32 → loads mod
Proxy:  every WinINet export → proxy.cpp FORWARD_FUNC → real System32\wininet.dll (lazy LoadLibraryW+GetProcAddress)

Hook engine at runtime:
  MH_Initialize
    SlimDetours::TransactionBegin → prepare code pages (NtProtectVirtualMemory)

  MH_CreateHook(target, detour, &orig):
    length-disassemble target (Instruction/Disassembler) to find >=5 bytes of whole instructions (x64)
    alloc trampoline near target (Memory.c, ±2 GB on x64)
    copy head bytes → trampoline + JMP back past head
    *orig = trampoline
    enqueue patch: target head → JMP detour (InlineHook.c)

  MH_EnableHook / TransactionCommit:
    suspend threads (Thread.c, TEB walk via PHNT)
    VirtualProtect → patch
    flush ICache, resume threads
    VirtualProtect restore

Cursor hooks (input isolation):
  hkSetCursorPos(x,y): if m_SettingsOpen → return TRUE (swallow); else oSetCursorPos(x,y)
  hkClipCursor(rect):  if m_SettingsOpen → return TRUE (swallow); else oClipCursor(rect)
    render thread per-frame while open: oClipCursor(nullptr)  -- must use trampoline!
  hkSetCapture(hwnd):  if m_SettingsOpen → return nullptr (swallow); else oSetCapture(hwnd)
```

Trampoline size note: on this mod's only architecture (x64) the jump occupies 5 bytes (`SIZE_OF_JMP = 5` in `SlimDetours.inl:164-168`; 12 on ARM64). There is no 14-byte variant in this engine. That number belongs to classic MinHook's absolute jump and does not apply here.

## Dependencies

- Uses: Win32 (`VirtualProtect`, `VirtualAlloc`, `CreateThread`, `LoadLibraryW`), NT Native API (via PHNT), D3D11/DXGI (for `HookPresent`'s temp device trick).
- Used by: `hooks.cpp` (caption hook), `renderer.cpp` (Present/Resize + cursor hooks). `proxy.cpp` is independent of MinHook — it uses plain `GetProcAddress` forwarding.

## Notable Patterns / Gotchas

- SlimDetours is the real engine — `MinHook.c` is only a compatibility shim; debugging hook failures means reading `SlimDetours/*.c`, not `MinHook.c`.
- Thread suspension is required to patch safely — `Thread.c` walks the TEB via PHNT; anything that blocks `NtSuspendThread` will break hooking.
- Never call the hooked export when you mean the trampoline — `oClipCursor(nullptr)` vs `ClipCursor(nullptr)` is the difference between freeing the cursor and re-swallowing the call while the panel is open. The second form leaves the cursor clipped with no way out.
- `oSetCursorPos` is deliberately NOT called per-frame — the game yanks via `NtUserSetCursorPos` (undocumented syscall) that bypasses the hook; fighting it causes trembling, so the mod lets it go and uses a software cursor instead (Raw Input + `DrawRealCursor`).
- Injection is fragile to working directory — `SetDllDirectoryW(ModDir)` + `g_ModDirA` pin all later resolves (fonts, INI) to the DLL's directory, not the game's `cwd`; removing that call breaks font loading.
- Proxy must not link `wininet.lib` statically — it `LoadLibraryW` the real DLL at runtime to avoid circular import; `wininet.def` is the only link-time surface.
