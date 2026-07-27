# Test Update: build on .125, install on .189

Builds the current controller software on your reference unit **without
installing it there**, then installs and tests it on the spare unit.

Your working unit (`.125`) keeps running the build you already validated. If
anything goes wrong, it is untouched.

**Every command below is labelled with the machine it runs on.** That is the
main thing to keep straight.

| Machine | Address | Role |
|---|---|---|
| Mac | — | holds the source, moves files around |
| Reference controller | 192.168.0.125 | builds the binaries; **not** updated in this test |
| Spare controller | 192.168.0.189 | receives the update and gets tested |

Password for both controllers is the usual `debian` login.

---

## Phase 1 — Put the current source on .125

### 1.1 · ON THE MAC — sync the controller's clock

The BeagleBone has no battery-backed clock, so it resets every boot. Files
copied from the Mac then look like they are from the future, and `make` decides
there is nothing to do — a fast, clean-looking build that changed nothing.

```
ssh -t debian@192.168.0.125 "sudo date -s '$(date -u +'%Y-%m-%d %H:%M:%S')'"
```

The `-t` is required, otherwise `sudo` cannot prompt for the password.

### 1.2 · ON .125 — move the old source aside

Keeps a fallback and guarantees no stale build artifacts confuse `make`.

```
ssh debian@192.168.0.125
rm -rf ~/sim-ctl-master.old
mv ~/sim-ctl-master ~/sim-ctl-master.old
exit
```

### 1.3 · ON THE MAC — copy the current source over

```
cd ~/Documents/Claude\ OVS
scp -r sim-ctl-master debian@192.168.0.125:~/
```

---

## Phase 2 — Build on .125 (build only, do NOT install)

### 2.1 · ON .125 — normalise timestamps, then build

```
ssh debian@192.168.0.125
cd ~/sim-ctl-master
find . -exec touch {} +
make 2>&1 | tee /tmp/build.log
```

### 2.2 · ON .125 — confirm the build is clean

```
grep -i "error" /tmp/build.log
```

**No output means success.** If anything appears, stop and send it to me.

`make` on its own does not touch `/usr/local/bin`. Only `make install` or
`scupdate` installs, and we are not running either here — so `.125` is still
running its previous, known-good binaries.

### 2.3 · ON .125 — confirm all six daemons were built

```
ls -l comm/simController cardiac/rfidScan cpr/cprScan pulse/pulse \
      respiration/breathSense wav-trig/soundSense
```

All six must be present and dated today.

---

## Phase 3 — Make the update bundle on .125

```
cd ~/sim-ctl-master
make updateDir
ls -l SimController.tarz
```

`make updateDir` gathers the six daemons, `ctlstatus.cgi`, the web content and
`scupdate` into `update/`, then packs `SimController.tarz`.

---

## Phase 4 — Move the bundle to the Mac, then to .189

### 4.1 · ON THE MAC — fetch the bundle

Keeping a copy on the Mac gives you the artifact for future units.

```
cd ~/Documents/Claude\ OVS
mkdir -p controller-builds
scp debian@192.168.0.125:~/sim-ctl-master/SimController.tarz \
    controller-builds/SimController-$(date +%Y%m%d).tarz
ls -l controller-builds/
```

### 4.2 · ON THE MAC — sync .189's clock too

```
ssh -t debian@192.168.0.189 "sudo date -s '$(date -u +'%Y-%m-%d %H:%M:%S')'"
```

---

## Phase 5 — Back up .189's configuration FIRST

`/simulator/rfid.xml` maps the RFID tags physically embedded in that manikin.
It cannot be reconstructed from the repository. Do this before anything else.

### 5.1 · ON THE MAC — pull the config off .189

```
cd ~/Documents/Claude\ OVS
mkdir -p controller-config-backups/189-$(date +%Y%m%d)
scp debian@192.168.0.189:/simulator/* \
    controller-config-backups/189-$(date +%Y%m%d)/
ls -l controller-config-backups/189-$(date +%Y%m%d)/
```

You should see `rfid.xml`, `simmgrName` and `soundList.csv`.

### 5.2 · ON THE MAC — note the tag count for comparison later

```
grep -c "<tagId>" controller-config-backups/189-$(date +%Y%m%d)/rfid.xml
```

Expect **52** from the earlier survey. Write it down.

`scupdate` does not touch `/simulator`, so this is a safety net rather than an
expected need — but it is the one file that cannot be recovered.

---

## Phase 6 — Install on .189

### 6.1 · ON THE MAC — send the bundle over

```
scp controller-builds/SimController-$(date +%Y%m%d).tarz \
    debian@192.168.0.189:~/SimController.tarz
```

### 6.2 · ON .189 — unpack and install

```
ssh debian@192.168.0.189
mkdir -p ~/scupdate-run
cd ~/scupdate-run
tar xzf ~/SimController.tarz
ls
```

You should see the six daemons, `ctlstatus.cgi`, `html/` and `scupdate`.

```
sudo ./scupdate
```

### 6.3 · What you should see

```
Sim Controller Update — scupdate 1.2.0
Installing: simController pulse rfidScan soundSense breathSense cprScan ...
Previous binaries backed up to /var/backups/simctl-<timestamp>
Stopping simctl service
Starting simctl service
All daemons running: simController pulse rfidScan soundSense breathSense cprScan
RFID tag table intact (52 tags) — /simulator was not modified
Controller now reports: simCtlVersion:1.1.13
Update complete
```

**If it reports a failure it will have rolled itself back automatically** and
told you so. The controller will be running its previous version. Send me the
output.

---

## Phase 7 — Verify

### 7.1 · ON .189 — service and daemons

```
sudo service simctl status
```

All six daemons should be listed.

### 7.2 · Physical checks

- [ ] Manikin **barked** when the service restarted
- [ ] Stethoscope on the tag pads produces heart and lung sounds
- [ ] The simulation manager shows the controller connected (green dot)

### 7.3 · ON THE MAC — survey it again

In OpenVetSim: **Simulator → Survey Sim Controller…**, address `192.168.0.189`.

Confirm:

- [ ] `Controller version` now reads **1.1.13** (was 1.1.6)
- [ ] `RFID tag count` still **52**
- [ ] All six daemons running
- [ ] `Runtime prerequisites` — `curl` no longer matters, it is not used

### 7.4 · The timing test — the point of all this

```
ssh debian@192.168.0.189
sudo killall soundSense
sudo /usr/local/bin/soundSense -d
```

Set the heart rate to **150 BPM** — the worst case for the original bug, which
stumbled roughly every 1.7 beats at that rate while sounding fine at 120.

Expect:

```
arrival=400ms  play=400ms  expected=400ms  worstLoop=2ms
```

- `arrival` and `play` within ~1 ms of each other
- `worstLoop` 2–3 ms, not 20 ms
- No `PLAY-JITTER` lines during a steady rhythm

`Ctrl-C` when done, then:

```
sudo service simctl restart
```

---

## If something goes wrong

| Symptom | Action |
|---|---|
| `scupdate` says the update is incomplete | A daemon is missing from the bundle. Re-run Phase 3 and check `ls` in 6.2 |
| `scupdate` rolled back | Controller is on its previous version and still working. Send me the output |
| No bark, daemons missing | `sudo service simctl status`, then `sudo /usr/local/bin/soundSense -d` and send the output |
| Tag count changed | Restore: `sudo cp ~/rfid-backup.xml /simulator/rfid.xml && sudo reboot` (copy your Mac backup over first) |
| Build fast with no output, nothing changed | Clock skew. Redo 1.1, then `make clean && make` |

Previous binaries stay in `/var/backups/simctl-<timestamp>` on `.189`, so a
manual rollback is always possible:

```
sudo service simctl stop
sudo cp -f /var/backups/simctl-<timestamp>/* /usr/local/bin/
sudo service simctl start
```

---

## Afterwards

Once `.189` is verified working, you can install the same bundle on `.125` with
the identical Phase 5–6 steps (back up its config first — it has **80** tags,
not 52). Until then, leave `.125` alone.
