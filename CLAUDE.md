# OpenVetSim — Project Guide for Claude

This file helps Claude get up to speed quickly on this project in any session,
on any machine. Read this before making changes.

---

## What This Is

OpenVetSim is a veterinary simulation manager built at Cornell University College
of Veterinary Medicine. It lets instructors run simulation scenarios and control
recording via OBS (Open Broadcast Studio). It is distributed as:

- A macOS DMG (universal: Apple Silicon + Intel)
- A Windows NSIS installer (x64)

---

## Repository Layout

```
Claude OVS/                        ← repo root (also the Box/git working dir)
├── OpenVetSim/                    ← C++ simulation engine (CMake project)
│   └── build/bin/
│       ├── WinVetSim              ← compiled macOS binary (universal fat)
│       ├── WinVetSim.exe          ← compiled Windows binary
│       └── PHP8.0/php             ← bundled static PHP (universal fat on Mac)
├── OpenVetSim-App/                ← Electron wrapper (Node.js)
│   ├── main.js                    ← main process: spawns C++ binary, manages UI
│   ├── package.json               ← electron-builder config, version number
│   └── build/installer.nsh        ← NSIS hooks for Windows installer
├── sim-ii/                        ← PHP web app (simulation UI)
├── sim-mgr/                       ← PHP web app (scenario manager)
├── sim-ctl/                       ← PHP web app (control panel)
├── sim-player/                    ← PHP web app (video player)
├── scenarios/                     ← bundled default scenarios
└── scripts/
    ├── download-php.sh            ← downloads + lipo's universal PHP (macOS)
    └── download-php.ps1           ← downloads PHP for Windows
```

---

## Architecture

```
Electron (main.js)
  └── spawns → WinVetSim (C++ binary)
                  ├── serves status/CGI on port 40845
                  └── launches → PHP -S on port 8081
                                    └── serves sim-ii, sim-mgr, etc.
```

- Electron polls port 40845 until the binary is ready, then polls port 8081
  until PHP is up, then loads the PHP UI in a `WebContentsView`.
- The C++ binary is passed `OPENVETSIM_HTML_PATH` env var pointing to the
  web root (Application Support on macOS, ProgramData on Windows).
- The binary name is still `WinVetSim` on both platforms (rename deferred).

---

## Key File Paths (Runtime)

| Platform | Web root / scenarios / simlogs |
|----------|-------------------------------|
| macOS (packaged) | `~/Library/Application Support/OpenVetSim/` |
| Windows (packaged) | `C:\ProgramData\OpenVetSim\` |
| Dev (both) | repo root (parent of `OpenVetSim-App/`) |

On macOS, `initUserData()` in `main.js` runs on every launch to:
1. Copy `sim-ii`, `sim-mgr`, `sim-ctl`, `sim-player` from bundle → Application Support
2. Seed `scenarios/` on first run only (never overwrites user-added scenarios)
3. Create `simlogs/video/` if missing
4. Create a Desktop symlink → scenarios folder

---

## Building

### macOS

```bash
# 1. Compile universal C++ binary (do this if WebSrv.cpp or other C++ changed)
cd OpenVetSim/build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
make -j$(sysctl -n hw.logicalcpu)

# 2. Download universal PHP (run from non-university network — CDN blocked on campus)
./scripts/download-php.sh
# Downloads arm64 + x86_64 static PHP from dl.static-php.dev and lipo's them together.
# Must be re-run whenever PHP version changes or before first build on a new machine.

# 3. Package DMG
cd OpenVetSim-App
npm run dist:mac
# Produces: dist/OpenVetSim-1.x.x-universal.dmg
```

### Windows

```powershell
# 1. Compile C++ binary (requires Visual Studio with C++ workload + CMake)
cd OpenVetSim
mkdir build; cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
# Produces: build/bin/WinVetSim.exe (or build/bin/Release/WinVetSim.exe)

# 2. Download PHP
.\scripts\download-php.ps1

# 3. Package installer
cd OpenVetSim-App
npm install
npm run dist:win
# Produces: dist/OpenVetSim-1.x.x-Setup.exe
```

---

## Gotchas & Hard-Won Lessons

### PHP binary must be universal on macOS
`@electron/universal` (used by `electron-builder --universal`) rejects single-arch
binaries that appear identically in both arch builds. `download-php.sh` downloads
both arm64 and x86_64 PHP and uses `lipo -create` to merge them. The `package.json`
has `"x64ArchFiles": "**/bin/**"` to tell `@electron/universal` to pass through
the already-universal binaries in `bin/` without trying to re-merge them.

### PHP CDN is blocked on university networks
`dl.static-php.dev` redirects to DigitalOcean Spaces, which is blocked on many
university networks. Run `download-php.sh` from a home network, phone hotspot,
or VPN. End users are NOT affected — PHP is bundled in the DMG/installer.

### C++ binary must also be universal
Compile with `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`. A single-arch binary
causes `EBADARCH` (error -86) on the wrong architecture Mac.

### PHP path must be absolute in WebSrv.cpp
`findPhpPath()` uses `fs::absolute()` to convert `./PHP8.0` to an absolute path.
Without this, the path breaks after the binary `cd`s to the HTML root to launch PHP.

### OBS WebSocket is v5 (built into OBS 28+)
The codebase uses `obs-websocket-5.js`, port 4455, `obs.connect()` / `obs.call()`,
and command names `StartRecord` / `StopRecord`. The old v4 plugin (port 4444) is
no longer supported. Users must enable the WebSocket server in OBS under
Tools → WebSocket Server Settings.

### Windows PHP path
The Windows PHP binary goes in `OpenVetSim/build/bin/PHP8.0/` (same as macOS).
The `package.json` `win.extraResources` section bundles `WinVetSim.exe` separately
from the mac section — make sure `WinVetSim.exe` exists there before packaging.

---

## Current Version

**v1.2.0** — Universal macOS DMG + Windows installer

### Release history
- v1.0.0 — initial release (arm64 only)
- v1.1.0 — OBS v5, Application Support paths, desktop shortcut (skipped in releases)
- v1.2.0 — universal binary (arm64 + Intel), Copy Video Log Path menu item

---

## Pushing Releases to GitHub

```bash
# macOS DMG
gh release create v1.2.0 \
  "OpenVetSim-App/dist/OpenVetSim-1.2.0-universal.dmg" \
  --title "v1.2.0" \
  --notes "..."

# Re-upload after a rebuild (overwrites existing asset)
gh release upload v1.2.0 "dist/OpenVetSim-1.2.0-universal.dmg" --clobber
```

---

## Deferred / Future Work

- Rename `WinVetSim` binary to `OpenVetSim` on both platforms
- Migrate `simlogs/` to Application Support on Windows (currently in ProgramData)
- Code signing / notarization for macOS (requires Apple Developer account)
- iPad: not feasible with current architecture (no child process spawning on iOS);
  a future version could run the engine on a Mac/server and use iPad as a client
