# Beat Timing Fix — Rollout Checklist

Applies to the July 2026 heart-beat timing fix (Change 11).

**This update is recommended, not required.** OpenVetSim v2.6.2 is fully backward
compatible with existing controller firmware — the wire protocol is unchanged, so
an un-upgraded controller connects and runs normally. You do not need to do this
before installing v2.6.2, and you do not need to coordinate the two.

What this update adds is the remaining half of the timing fix:

| | Contribution to beat jitter | Fixed by |
|---|---|---|
| PC software | up to ±45 ms | v2.6.2 alone |
| Controller firmware | ±20–40 ms | this procedure |

So a site running v2.6.2 against old controller firmware is **noticeably better
but not fixed** — the stumble is still occasionally audible at awkward rates such
as 150 BPM. Doing both brings it to roughly ±2 ms.

Worth prioritising for controllers used in rhythm-recognition teaching, where an
irregular sinus rhythm is actively misleading. Lower priority elsewhere.

Budget roughly 30–45 minutes per controller the first time.

---

## Before you start

- [ ] The simulation manager PC is on **wired Ethernet or 5 GHz Wi-Fi**.
      2.4 GHz produces multi-second dropouts that no software change can fix.
- [ ] You can reach the BeagleBone over the network (`ssh debian@beaglebone.local`).
- [ ] You know this manikin's identity — each has its **own** RFID tag table.

---

## Part 1 — Simulation manager PC

- [ ] Pull the updated code

      git fetch origin
      git checkout beat-timing-fixes
      git pull

- [ ] Confirm you actually have the fix (returns 2 or more)

      grep -c sim_monotonic_msec OpenVetSim/platform.h

- [ ] Rebuild the C++ binary

      **Windows**
      cd OpenVetSim\build
      cmake .. -DCMAKE_BUILD_TYPE=Release
      cmake --build . --config Release

      **macOS**
      cd OpenVetSim/build
      cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
      make -j$(sysctl -n hw.logicalcpu)

- [ ] **Windows only:** confirm the binary is at `OpenVetSim\build\bin\WinVetSim.exe`.
      MSVC may write it to `build\bin\Release\` instead, where the app won't find it.

      copy OpenVetSim\build\bin\Release\WinVetSim.exe OpenVetSim\build\bin\

- [ ] Launch and confirm it starts cleanly (`npm start` from `OpenVetSim-App/`,
      or the installed application).

---

## Part 2 — Controller (BeagleBone)

### 2a. Protect the RFID tag table — do this first

`/simulator/rfid.xml` maps the RFID tags physically embedded in **this** manikin.
It is not recoverable from the repository. Back it up before touching anything:

- [ ] Back up on the device

      cp /simulator/rfid.xml ~/rfid.xml.backup

- [ ] Copy it somewhere off the device too

      scp debian@beaglebone.local:/simulator/rfid.xml ./rfid-<manikin-name>-backup.xml

> **Never run `make factory`.** Unlike `make install`, it copies `rfid.xml`
> unconditionally and will overwrite this table with the repository defaults.

### 2b. Fix the clock

The BeagleBone has no battery-backed RTC and no internet, so its clock resets
every boot. Files copied from your machine then look "in the future", and `make`
concludes there is nothing to do — producing a fast, clean-looking build that
changed nothing.

- [ ] Sync the clock (from a Mac/Linux machine; `-t` is required for sudo)

      ssh -t debian@beaglebone.local "sudo date -s '$(date -u +'%Y-%m-%d %H:%M:%S')'"

      Or directly on the BeagleBone, in UTC:
      sudo date -s "2026-07-25 18:30:00"

- [ ] Verify: `date` on the BeagleBone now shows the correct time

### 2c. Copy the source across

- [ ] Transfer the tree (the bundled `deps/` .deb rides along)

      scp -r ~/Documents/Claude\ OVS/sim-ctl-master debian@beaglebone.local:~/

- [ ] Reset timestamps to the BeagleBone's own clock

      cd ~/sim-ctl-master
      find . -exec touch {} +

- [ ] Remove the old tree so nobody builds it by mistake

      rm -rf ~/sim-ctl

> `sim-ctl-master` and the older `sim-ctl` are different codebases with
> incompatible shared-memory layouts. Mixing binaries from the two corrupts
> shared memory — the symptom is RFID tags reading correctly while no sound plays.

### 2d. Install the libgpiod headers

`sim-ctl-master` links `-lgpiod`. BeagleBone images ship the runtime
(`libgpiod2`) but not the headers, and the controllers have no internet.

- [ ] Install the bundled package

      sudo dpkg -i ~/sim-ctl-master/deps/libgpiod-dev_1.4.1-2rcnee3~buster+20190906_armhf.deb

- [ ] Verify

      ls -l /usr/include/gpiod.h

> Do not substitute stock Debian's `libgpiod-dev` (1.2-3) — it will not match the
> 1.4.1 rcn-ee runtime already on the image.

### 2e. Build

- [ ] Build everything and check for errors

      cd ~/sim-ctl-master
      make 2>&1 | tee /tmp/build.log
      grep -i "error" /tmp/build.log

      No output from the grep means a clean build. Do not continue otherwise.

### 2f. Install

All six daemons must come from the same tree — see the shared-memory warning above.

- [ ] Stop the service

      sudo service simctl stop

- [ ] Install (`install`, **not** `factory`)

      cd ~/sim-ctl-master
      make install

- [ ] Reboot rather than restarting the service, so the shared memory segment is
      recreated at the new size

      sudo reboot

---

## Part 3 — Verify

- [ ] The manikin **barks** on startup
- [ ] All six daemons are running

      sudo service simctl status
      # simController, pulse, rfidScan, soundSense, breathSense, cprScan

- [ ] The tag table survived

      ls -l /simulator/rfid.xml
      grep -c "<tagId>" /simulator/rfid.xml

      Restore from backup if it looks wrong:
      sudo cp ~/rfid.xml.backup /simulator/rfid.xml && sudo reboot

- [ ] The stethoscope produces heart and lung sounds on the tag pads

- [ ] **Timing check.** Stop the service copy and run in the foreground:

      sudo killall soundSense
      sudo /usr/local/bin/soundSense -d

      Set a rate and hold the stethoscope on a tag. Expect:

          arrival=250ms  play=250ms  expected=250ms  worstLoop=2ms

      - `arrival` and `play` should track each other within ~1 ms
      - `worstLoop` should be 2–3 ms, not 20 ms
      - No `PLAY-JITTER` lines during steady rhythm

      Ctrl-C, then `sudo service simctl restart`.

- [ ] **Listen at 150 BPM.** This was the worst case for the original bug — the
      old code stumbled roughly every 1.7 beats at that rate, while 120 and 240
      sounded clean. If 150 is steady, the fix is working.

---

## Troubleshooting

| Symptom | Cause |
|---|---|
| Build finishes suspiciously fast, nothing changes | Clock skew — redo step 2b, then `rm -f` the target binary and rebuild |
| `gpiod.h: No such file or directory` | Step 2d not done |
| No bark; `soundSense` prints usage with `<tty port>` | Old `sim-ctl` binary still installed — rebuild from `sim-ctl-master` |
| RFID tags read fine but no sound | Mixed binaries from both trees; shared-memory layout mismatch. Reinstall all six |
| Tags no longer recognised | `rfid.xml` overwritten (`make factory`). Restore from backup |
| `cp: Text file busy` | Service still running — `sudo service simctl stop` first |
| Sound stumbles despite the update | Check the network: 2.4 GHz Wi-Fi is unusable. Confirm `arrival=` in `soundSense -d`; if arrival itself is uneven, the delay is upstream, not in the controller |

---

## Per-controller record

| Manikin | Date | rfid.xml backed up | PC build | Controller build | 150 BPM verified |
|---------|------|--------------------|----------|------------------|------------------|
|         |      |                    |          |                  |                  |
|         |      |                    |          |                  |                  |
|         |      |                    |          |                  |                  |
