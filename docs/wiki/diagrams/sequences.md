# Sequence Diagrams

## 1. Bootstrap and Hook Installation

```mermaid
sequenceDiagram
    participant OS as Windows Loader
    participant DLL as wininet.dll proxy
    participant Main as MainThread
    participant Game as HLA client.dll
    participant RT as Renderer

    OS->>DLL: DllMain(DLL_PROCESS_ATTACH)
    DLL->>Main: CreateThread(MainThread)
    Main->>Main: diagnostics, DPI, paths, LoadConfig
    Main->>Main: MH_Initialize + user32 hooks
    loop retry up to 60 times
        Main->>Game: FindSignature(client.dll)
    end
    Main->>Game: MH_CreateHook(hkProcess)
    Main->>RT: Renderer::Initialize()
    RT->>RT: HookPresent() → Present/Resize
```

## 2. Caption Delivery and Rendering

```mermaid
sequenceDiagram
    participant Game as client.dll
    participant Hook as hkProcess
    participant Q as Renderer::m_Queue
    participant RT as hkPresent
    participant S as TextShaper
    participant UL as UltralightManager
    participant BB as DX11 backbuffer

    Game->>Hook: Process(raw, duration, fromPlayer)
    Hook->>Q: SetCaptionText(raw, duration)
    Q->>Q: split <sb> + parse tags + queue entries

    loop each Present
        RT->>Q: UpdateQueue(dt)
        Q->>Q: expire, fade, stack, detect rebuilds
        Q->>S: RenderEntryTexture → ShapeLine
        S-->>Q: glyph bitmaps
        RT->>BB: ImGui DrawCaptions (entry textures)
        RT->>UL: Render() → HTML bitmap → D3D11 quad
        RT->>BB: DrawRealCursor()
    end
```

## 3. Settings Save (HTML Panel → INI)

```mermaid
sequenceDiagram
    participant User
    participant JS as bindings.js
    participant UL as UltralightManager
    participant Cfg as g_Config
    participant Disk as resources/settings.ini

    User->>JS: edit a control
    JS->>JS: set __stateDirty
    UL->>JS: snapshot() during Render()
    JS-->>UL: JSON values
    UL->>Cfg: ApplyConfigField* (live update)
    User->>JS: click Save
    JS->>JS: set __saveRequested
    UL->>JS: snapshot()
    UL->>Cfg: ApplyConfigField* for each key
    UL->>Disk: SaveConfig(g_IniPath)
    Disk-->>UL: UTF-16LE file
```

### Notes

- `Renderer::m_CS` guards caption queue and shaping state.
- `UltralightManager::ProcessWin32Message` only queues input; the `View` is touched on the render thread.
- `ReadFileToUtf8` accepts UTF-8 and UTF-16 BOMs, so saved settings load consistently.
