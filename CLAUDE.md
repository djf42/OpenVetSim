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
Claude OVS/                        ← repo root (~/Documents/Claude OVS)
├── OpenVetSim/                    ← C++ simulation engine (CMake project)
│   └── build/bin/
│       ├── WinVetSim              ← compiled macOS binary (universal fat)
│       ├── WinVetSim.exe          ← compiled Windows binary
│       └── PHP8.0/php             ← bundled static PHP (universal fat on Mac)
├── OpenVetSim-App/                ← Electron wrapper (Node.js)
│   ├── main.js                    ← main process: spawns C++ binary, manages UI
│   ├── package.json               ← electron-builder config, version number
│   ├── build/
│   │   ├── installer.nsh          ← NSIS hooks for Windows installer
│   │   └── entitlements.mac.plist ← macOS hardened runtime entitlements
│   └── scripts/
│       ├── notarize.js            ← (legacy) custom notarization hook
│       ├── beforeSign.js          ← strips extended attributes before signing (macOS)
│       └── windowsSign.js         ← calls signtool with EV cert on YubiKey (Windows)
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
| Windows (packaged) | `C:\Users\Public\OpenVetSim\` |
| Dev (both) | repo root (parent of `OpenVetSim-App/`) |

On macOS, `initUserData()` in `main.js` runs on every launch to:
1. Copy `sim-ii`, `sim-mgr`, `sim-ctl`, `sim-player` from bundle → Application Support
2. Seed `scenarios/` on first run only (never overwrites user-added scenarios)
3. Create `simlogs/video/` if missing
4. Create a Desktop symlink → scenarios folder

---

## Building

All build commands run from `~/Documents/Claude OVS/`.

### macOS

```bash
# Set signing credentials (required — these are session-only, set each time)
export APPLE_ID="djfletch42@gmail.com"
export APPLE_APP_SPECIFIC_PASSWORD="xxxx-xxxx-xxxx-xxxx"
export APPLE_TEAM_ID="29Q67RY9V7"

# 1. Compile universal C++ binary (do this if any .cpp or .h files changed)
cd ~/Documents/Claude\ OVS/OpenVetSim/build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
make -j$(sysctl -n hw.logicalcpu)

# 2. Download universal PHP (only needed if PHP version changes or first build on new machine)
# Must run from non-university network — CDN blocked on campus
cd ~/Documents/Claude\ OVS
./scripts/download-php.sh

# 3. Package signed DMG (caffeinate prevents sleep interrupting notarization)
cd ~/Documents/Claude\ OVS/OpenVetSim-App
caffeinate -i npm run dist:mac
# Produces: dist/OpenVetSim-x.x.x-universal.dmg (signed but not yet notarized)

# 4. Notarize (submit to Apple — takes 5-30 min, save the submission ID shown)
xcrun notarytool submit dist/OpenVetSim-x.x.x-universal.dmg \
  --apple-id "$APPLE_ID" \
  --password "$APPLE_APP_SPECIFIC_PASSWORD" \
  --team-id "$APPLE_TEAM_ID" \
  --verbose \
  --wait

# 5. Check notarization status later if needed (use submission ID from step 4)
xcrun notarytool info <submission-id> \
  --apple-id "$APPLE_ID" \
  --password "$APPLE_APP_SPECIFIC_PASSWORD" \
  --team-id "$APPLE_TEAM_ID"

# 6. Staple notarization ticket to DMG
xcrun stapler staple dist/OpenVetSim-x.x.x-universal.dmg
```

### Windows

```powershell
# 0. Insert YubiKey 5C NFC FIPS before building — signing will prompt for PIN

# 1. Compile C++ binary (requires Visual Studio with C++ workload + CMake)
cd OpenVetSim
mkdir build; cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
# Produces: build/bin/WinVetSim.exe (or build/bin/Release/WinVetSim.exe)

# 2. Download PHP
.\scripts\download-php.ps1

# 3. Package installer (unsigned)
cd OpenVetSim-App
npm install
npm run dist:win
# Produces: dist/OpenVetSim Setup x.x.x.exe  (unsigned)

# 4. Sign the installer (insert YubiKey first — will prompt for PIN once)
& "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe" sign /sha1 e444aa88291629b6e931b518f42e0b2ce48ea7cb /fd sha256 /tr http://timestamp.sectigo.com /td sha256 "dist\OpenVetSim Setup x.x.x.exe"
```

---

## Gotchas & Hard-Won Lessons

### Beat timing: never schedule against GetTickCount64() on Windows

Anything that decides *when* a heartbeat or breath fires must use
`sim_monotonic_msec()` (platform.h), which is `QueryPerformanceCounter` on
Windows and `CLOCK_MONOTONIC` on POSIX.

`GetTickCount64()` resolves only to the system timer tick — documented as
10–16 ms, in practice **15.625 ms** — and on modern Windows it does *not*
follow `timeBeginPeriod()`. Scheduling against it puts every beat on a 15.6 ms
grid, which aliases against the beat interval:

| Rate | Interval | Interval / 15.625 | Result |
|------|----------|-------------------|--------|
| 120 BPM | 500 ms | 32.000 | exact — sounds fine |
| 240 BPM | 250 ms | 16.000 | exact — sounds fine |
| 200 BPM | 300 ms | 19.200 | slips 0.2 tick/beat → one beat 15.6 ms late every 5 beats |
| 150 BPM | 400 ms | 25.600 | worst case — a stumble every ~1.7 beats |

The mean rate stays correct, so the monitor reads the right number while the
rhythm audibly stumbles. This was the primary cause of the long-standing
"sounds like an arrhythmia in sinus rhythm" bug, and it is rate-dependent —
testing only at 120 BPM will show nothing.

### timeBeginPeriod(1) must be called at startup

`winmm` is linked in CMakeLists.txt specifically for this. Without the call,
`Sleep(1)` actually sleeps ~15.6 ms, so every polling loop in pulse.cpp runs
~64 times/second instead of 1000. `sim_timer_resolution_begin()` is called
from `simmgrInitialize()`; don't remove it.

### SetThreadPriority is real on POSIX now

It used to be a stub that returned `true` without doing anything, so the pulse
threads silently ran at normal priority on macOS/Linux despite asking for
`THREAD_PRIORITY_TIME_CRITICAL`. It now maps to `SCHED_FIFO`. If the process
lacks privilege the call fails and pulse.cpp prints a warning — that warning
means degraded timing, not a crash.

### Never build browser URLs from PHP's SERVER_NAME

PHP runs as `php -S 0.0.0.0:8081` so phones can reach sim-remote, and the
built-in server reports that **bind** address verbatim as `SERVER_NAME`.
`0.0.0.0` is valid to bind to but is not a reachable destination — Windows
rejects it outright, macOS/Linux quietly treat it as loopback. Symptoms are
"Could not start PHP Server" (the health check couldn't reach a PHP that had
started fine) and interface elements failing to load from `http://0.0.0.0:40845/`.

Use `VS_BROWSER_HOST` in sim-ii/init.php and sim-player/init.php — derived from
`HTTP_HOST`, i.e. whatever the client actually connected to. Correct for both
the Electron window (127.0.0.1) and a phone on the LAN.

### The sim manager must be on wired Ethernet or 5 GHz Wi-Fi

Beat events are individual TCP messages whose *arrival time is the beat time*,
so network delay becomes audio delay directly. Measured at 240 BPM:

| Link | Result |
|------|--------|
| Wired | flawless |
| 5 GHz Wi-Fi | flawless — worst ±18 ms, no dropouts |
| 2.4 GHz Wi-Fi | **unusable** — multi-second stalls |

2.4 GHz produced 1.75 s and 5 s stalls: the AP buffers the beats and delivers
them in a burst, the controller collapses the burst into one sound, and you get
several seconds of silence. No amount of buffering in software fixes that.

### Controller firmware must be updated together with the PC software

The fix is split across both ends and neither half is sufficient alone:

- **PC** (pulse.cpp) — removes the 15.6 ms tick quantization
- **Controller** (sim-ctl-master/wav-trig/soundSense.cpp) — removes polling-loop
  jitter. Before the fix the sound loop ran at 20 ms and anchored `LUB_DELAY` to
  when the loop *noticed* the beat rather than when it *arrived* — two
  independent 0–20 ms windows, so consecutive beats could differ by ~40 ms.

Diagnosing this: run `soundSense -d` on the BeagleBone. It prints
`arrival=` (beat spacing off the socket) next to `play=` (resulting sound
spacing). If those two track each other, the controller is fine and the jitter
arrived with the beat — look upstream at the network or the PC.

### sim-ctl-master needs libgpiod, and controllers have no internet

`sim-ctl-master` links `-lgpiod`; the older `sim-ctl` tree does not. BeagleBone
images ship `libgpiod2` (the runtime) but not `libgpiod-dev` (the headers), and
the units are not on the internet. The matching package is bundled:

```bash
sudo dpkg -i ~/sim-ctl-master/deps/libgpiod-dev_1.4.1-2rcnee3~buster+20190906_armhf.deb
```

Use the bundled one — stock Debian buster ships 1.2-3, which dpkg will refuse
against the 1.4.1 rcn-ee runtime on the image.

### BeagleBone clock skew breaks make and cp -u

The BBB has no battery-backed RTC and no network time, so its clock resets every
boot. `scp` preserves the Mac's timestamps, so uploaded files land "in the
future"; `make` then decides everything is up to date and does nothing, and
`make install`'s `cp -u` silently skips the copy. You get a fast, clean-looking
build that changed nothing.

Sync the clock from the Mac after every BBB reboot, before building:

```bash
ssh -t debian@beaglebone.local "sudo date -s '$(date -u +'%Y-%m-%d %H:%M:%S')'"
```

`-t` is required — `sudo` needs a tty. If a build still looks suspiciously
quick, `rm -f` the target binary and rebuild; and prefer an explicit
`sudo cp -f` over `make install`.

**Do not use `make factory` on the BeagleBone.** Unlike `make install` it copies
`rfid.xml` unconditionally, overwriting the per-manikin RFID tag table with the
repo's defaults. Those tag IDs are specific to the tags embedded in that
manikin and are not recoverable from the repo.

### Keychain dialog during signing
macOS may show a dialog "codesign wants to access key 'The RECOVER Initiative'"
during the signing step. If the build hangs at "signing", check all windows and
Spaces for this dialog — it can hide behind other apps. Click **"Always Allow"**
(not just "Allow") so it doesn't repeat for every file.

### Notarization hangs silently on university networks
Apple's notarization endpoint (`notary-submissions.developer.apple.com`) may be
blocked on university networks. Use `xcrun notarytool submit --wait` separately
rather than electron-builder's built-in notarize, which hangs with no output.
The `package.json` has `"notarize": false` — notarization is handled manually.
Note: once submitted, notarization runs on Apple's servers; you can Ctrl+C the
polling and check status later with `xcrun notarytool info <submission-id>`.

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

### Windows EV signing with YubiKey
Signing requires a Sectigo EV certificate on a YubiKey 5C NFC FIPS hardware token.
Prerequisites on the Windows build machine:
- **YubiKey Smart Card Minidriver** installed (makes Windows recognize the YubiKey as a smart card)
- **YubiKey Manager CLI** (`ykman`) installed
- Certificate **and** private key both in PIV **slot 9A** (Authentication)

The private key and certificate MUST be in the same slot. The original setup had the
private key in 9A and the cert in 9C — signing failed with "unexpected internal error"
(0x80100014) until the cert was moved to 9A with `ykman piv certificates export/delete/import`.

Signing is done manually after the build with a single signtool command (see Windows
build steps above). Wiring signing into electron-builder caused a PIN prompt for every
bundled file — hundreds of prompts. Manual signing requires exactly one PIN entry.

### Windows PHP path
The Windows PHP binary goes in `OpenVetSim/build/bin/PHP8.0/` (same as macOS).
The `package.json` `win.extraResources` section bundles `WinVetSim.exe` separately
from the mac section — make sure `WinVetSim.exe` exists there before packaging.

---

## Current Version

**v2.6.2** — heart beat timing fixes; requires matching controller firmware

> **v2.6.2 needs the controller updated too.** The fix is split across both ends
> and neither half is sufficient alone — see "Controller firmware must be updated
> together with the PC software" under Gotchas, and
> `CONTROLLER_UPDATE_CHECKLIST.md`.

### Release history
- v1.0.0 — initial release (arm64 only)
- v1.1.0 — OBS v5, Application Support paths, desktop shortcut (skipped in releases)
- v1.2.0 — universal binary (arm64 + Intel), Copy Video Log Path menu item, macOS code signing + notarization
- v2.5.0 — collaborator C++ updates (VetSim, pulse, scenario, simstatus, and others)
- v2.6.0 — ECG improvements (afib rate fix, dynamic resampling, peak preservation, pre-computed VFib waveforms) + ETCO₂ waveform types (normal, rebreathing, obstructive/shark-fin, curare cleft) with instructor UI dialog
- v2.6.1 — asystole waveform fix (flat line now displays correctly when asystole is selected)
- v2.6.2 — **heart beat timing.** Beats are now scheduled against
  `QueryPerformanceCounter` rather than `GetTickCount64`, whose 15.625 ms
  resolution was quantizing them onto a grid that aliased against the beat
  interval (exact at 120 and 240 BPM, worst at 150). Also: `timeBeginPeriod(1)`
  is finally called at startup; `SetThreadPriority` is real on POSIX instead of
  a no-op stub; controller sockets are drained so the controller's liveness
  bytes cannot fill our receive buffer and force a reconnect; the legacy pulse
  port 50200 is bound alongside 40844 for pre-2020 controller firmware, with
  duplicate connections from the same controller refused; `TCP_NODELAY` set on
  controller sockets; browser URLs no longer built from the `0.0.0.0` bind
  address (broke `npm start` and the PHP health check on Windows); read-only
  Sim Controller survey added to the Simulator menu.

  Controller side (`sim-ctl-master`): beat arrival is timestamped so the lub
  fires relative to arrival rather than to when the polling loop noticed it,
  sound loop reduced 20 ms → 2 ms, forced gain refreshes throttled, sound path
  runs `SCHED_FIFO`, and `popen("curl ...")` replaced with a direct HTTP GET so
  offline controllers need no external tools.

  **Also requires wired Ethernet or 5 GHz Wi-Fi** for the simulation manager —
  2.4 GHz produces multi-second dropouts. On macOS, `awdl0` (AirDrop/Handoff)
  periodically takes the radio off-channel and must be down for Wi-Fi to be
  usable: `sudo ifconfig awdl0 down`.

---

## Pushing Releases to GitHub

```bash
# Commit and push source changes
cd ~/Documents/Claude\ OVS
git add -p   # review and stage changes interactively
git commit -m "Your message here"
git push

# Create a GitHub release with the notarized DMG
gh release create v2.5.0 \
  "OpenVetSim-App/dist/OpenVetSim-2.5.0-universal.dmg" \
  --title "v2.5.0" \
  --notes "..."

# Re-upload after a rebuild (overwrites existing asset)
gh release upload v2.5.0 "dist/OpenVetSim-2.5.0-universal.dmg" --clobber
```

---

## Apple Developer Credentials

- **Apple ID**: djfletch42@gmail.com
- **Team ID**: 29Q67RY9V7 (The RECOVER Initiative)
- **Certificate**: Developer ID Application — installed in login Keychain
- **App-Specific Password**: stored separately (generate at appleid.apple.com if expired)

---

## Deferred / Future Work

- Rename `WinVetSim` binary to `OpenVetSim` on both platforms
- iPad: not feasible with current architecture (no child process spawning on iOS);
  a future version could run the engine on a Mac/server and use iPad as a client
