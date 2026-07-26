'use strict';

/*
 * controller-survey.js
 *
 * Read-only survey of a BeagleBone simulation controller.
 *
 * WHY THIS EXISTS
 * ---------------
 * There are ~150 controllers deployed worldwide, most owned by other
 * institutions, running Debian images installed over several years. We have no
 * inventory of what is actually out there, and the controller software has
 * almost certainly drifted from unit to unit.
 *
 * Before we can offer a remote update we need to know which images can run a
 * given set of binaries. This module answers that by connecting over SSH and
 * running a handful of harmless read-only commands. It writes nothing to the
 * controller and changes nothing — it cannot break a unit.
 *
 * The results are displayed, can be copied to the clipboard or saved to a file,
 * and are appended to a local log so a fleet picture accumulates over time.
 *
 * NOT YET INCLUDED: the empirical compatibility test (upload a binary, run ldd
 * on it, execute it with -h). That needs ARM binaries packaged with the app,
 * which do not exist yet. The survey is deliberately shippable without them.
 */

const { BrowserWindow, dialog, clipboard, safeStorage, app, ipcMain } = require('electron');
const fs   = require('fs');
const path = require('path');
const http = require('http');
const net  = require('net');
const os   = require('os');

const SSH_PORT       = 22;
const CONNECT_TIMEOUT = 10000;   // ms — units may be off or unreachable
const COMMAND_TIMEOUT = 15000;   // ms per command

// ─── Survey commands ──────────────────────────────────────────────────────────
//
// All read-only. Ordered roughly from "identifies the image" to "identifies the
// installed software".
//
// /etc/dogtag is a BeagleBoard.org convention naming the exact image the unit
// was flashed from (e.g. "BeagleBoard.org Debian Buster IoT Image 2020-04-06").
// It is the single most useful line for fleet inventory.
//
// The soundSense -h usage string differs between the two source trees, so it
// distinguishes sim-ctl from sim-ctl-master without needing a version file:
//   sim-ctl        ->  [-d] [-m] <tty port>
//   sim-ctl-master ->  [-d] [-m][-t] [tty port 1] [tty port 2]
const SURVEY = [
  { key: 'dogtag',    label: 'Image',              cmd: 'cat /etc/dogtag 2>/dev/null' },
  { key: 'osrelease', label: 'OS',                 cmd: "grep -E '^(VERSION|VERSION_ID|VERSION_CODENAME|ID)=' /etc/os-release" },
  { key: 'kernel',    label: 'Kernel',             cmd: 'uname -r' },
  { key: 'arch',      label: 'Architecture',       cmd: 'dpkg --print-architecture' },
  { key: 'glibc',     label: 'glibc',              cmd: 'ldd --version | head -1' },
  { key: 'gpiod',     label: 'libgpiod / libxml',  cmd: "dpkg -l 2>/dev/null | grep -E 'libgpiod|libxml' | awk '{print $2, $3}'" },
  { key: 'gpiodso',   label: 'libgpiod soname',    cmd: 'ls /usr/lib/*/libgpiod.so* /usr/lib/libgpiod.so* 2>/dev/null' },
  { key: 'binaries',  label: 'Installed binaries', cmd: "ls -l --time-style=long-iso /usr/local/bin/ 2>/dev/null | awk 'NR>1 {print $6, $8}'" },
  // SIMCTL_VERSION from comm/version.h, reported by ctlstatus.cgi. This is the
  // authoritative controller software version. Try the CGI directly first, then
  // over local HTTP via nginx, since older images differ in what is available.
  { key: 'ctlversion', label: 'Controller version',
    cmd: "( /var/www/cgi-bin/ctlstatus.cgi 2>/dev/null " +
         "|| wget -qO- http://127.0.0.1/cgi-bin/ctlstatus.cgi 2>/dev/null ) " +
         "| tr ',' '\\n' | grep -i simCtlVersion | head -1 | tr -d ' \"' " +
         "|| echo '(not reported)'" },
  // Fallback identification: the usage string differs between the two source
  // trees, so it distinguishes them even when no version is reported.
  { key: 'soundver',  label: 'soundSense build',   cmd: '/usr/local/bin/soundSense -h 2>&1 | head -3' },
  { key: 'service',   label: 'simctl service',     cmd: 'systemctl is-active simctl 2>/dev/null || echo unknown' },
  { key: 'daemons',   label: 'Running daemons',    cmd: "ps -eo comm= | grep -E 'simController|soundSense|rfidScan|breathSense|cprScan|^pulse$' | sort | uniq -c | awk '{print $2}' | tr '\\n' ' '" },
  { key: 'tags',      label: 'RFID tag count',     cmd: "grep -c '<tagId>' /simulator/rfid.xml 2>/dev/null || echo 'no rfid.xml'" },

  // Is this a real per-manikin tag table, or the repository default?
  //
  // /simulator/rfid.xml maps the RFID tags physically embedded in one specific
  // manikin and cannot be reconstructed. A unit running the repo default has
  // effectively lost its table — the stethoscope will not recognise its own pads.
  // The count alone cannot distinguish the two, because the default also has
  // plausible-looking IDs.
  //
  // The pristine default (sim-ctl-master/initialization/rfid.xml) is:
  //     md5          72440faf98c725fc0ae9d41cdf7ca591
  //     tag count    52
  //     first descriptions   TestTag1, TestTag2
  //
  // A matching md5 means the file was never customised. "TestTag" in the first
  // descriptions is a strong signal even if the file was edited afterwards.
  { key: 'rfidfp',    label: 'RFID table identity',
    cmd: "( md5sum /simulator/rfid.xml 2>/dev/null | cut -d' ' -f1 | sed 's/^/md5 /' ; " +
         "grep -o '<description>[^<]*' /simulator/rfid.xml 2>/dev/null | head -2 " +
         "| sed 's|<description>|first: |' ) || echo '(no rfid.xml)'" },
  { key: 'simmgr',    label: 'simmgrName',         cmd: "grep -vE '^\\s*#|^\\s*$' /simulator/simmgrName 2>/dev/null | head -2" },
  { key: 'uptime',    label: 'Uptime',             cmd: 'uptime -p 2>/dev/null || uptime' },
  { key: 'disk',      label: 'Disk free',          cmd: "df -h / | awk 'NR==2 {print $4 \" free of \" $2}'" },
];

// ─── Credential storage ───────────────────────────────────────────────────────
//
// Passwords are encrypted with Electron safeStorage, which uses the macOS
// Keychain and Windows DPAPI. If encryption is unavailable we simply do not
// offer to remember — we never write a plaintext password to disk.

function credPath() {
  return path.join(app.getPath('userData'), 'controller-credentials.json');
}

function loadAllCreds() {
  try { return JSON.parse(fs.readFileSync(credPath(), 'utf8')); }
  catch { return {}; }
}

function loadCreds(host) {
  const entry = loadAllCreds()[host];
  if (!entry) return null;
  let password = '';
  if (entry.enc && safeStorage.isEncryptionAvailable()) {
    try { password = safeStorage.decryptString(Buffer.from(entry.enc, 'base64')); }
    catch { password = ''; }
  }
  return { username: entry.username || 'debian', password };
}

function saveCreds(host, username, password) {
  if (!safeStorage.isEncryptionAvailable()) return false;
  const all = loadAllCreds();
  all[host] = {
    username,
    enc: safeStorage.encryptString(password).toString('base64'),
    saved: new Date().toISOString(),
  };
  try {
    fs.writeFileSync(credPath(), JSON.stringify(all, null, 2), { mode: 0o600 });
    return true;
  } catch { return false; }
}

function forgetCreds(host) {
  const all = loadAllCreds();
  delete all[host];
  try { fs.writeFileSync(credPath(), JSON.stringify(all, null, 2), { mode: 0o600 }); } catch {}
}

// ─── Controller discovery ─────────────────────────────────────────────────────
//
// The C++ status endpoint reports connected controllers from shared memory.
// This is the same source the connection status indicator uses.

function detectControllerIP(statusPort) {
  return new Promise((resolve) => {
    const req = http.get(
      `http://127.0.0.1:${statusPort}/cgi-bin/simstatus.cgi?status=1`,
      { timeout: 3000 },
      (res) => {
        let body = '';
        res.on('data', (c) => (body += c));
        res.on('end', () => {
          try {
            const j = JSON.parse(body);
            const list = j.controllers || {};
            const ip = Object.values(list).find(
              (v) => typeof v === 'string' && /^\d+\.\d+\.\d+\.\d+$/.test(v)
            );
            resolve(ip || null);
          } catch { resolve(null); }
        });
      }
    );
    req.on('error',   () => resolve(null));
    req.on('timeout', () => { req.destroy(); resolve(null); });
  });
}

// ─── Subnet scan ──────────────────────────────────────────────────────────────
//
// Finds candidate controllers by probing SSH across the local /24.
//
// Why this is needed: detectControllerIP() only sees controllers that have
// successfully connected to the simulation manager. A controller that cannot
// connect — old firmware looking for the legacy port on a manager that does not
// offer it, a wrong simmgrName, a subnet mismatch — never appears there. Those
// are precisely the units we most need to inspect, so the survey must be able
// to find a controller that is not talking to us.
//
// Probing port 22 is a reasonable proxy: every BeagleBone image runs sshd. It
// will also match other Linux hosts on the subnet, so results are presented as
// candidates rather than confirmed controllers.

function probeSSH(host, timeout = 400) {
  return new Promise((resolve) => {
    const sock = new net.Socket();
    let done = false;
    const finish = (ok) => {
      if (done) return;
      done = true;
      sock.destroy();
      resolve(ok ? host : null);
    };
    sock.setTimeout(timeout);
    sock.once('connect', () => finish(true));
    sock.once('timeout', () => finish(false));
    sock.once('error',   () => finish(false));
    sock.connect(22, host);
  });
}

function localSubnets() {
  const out = [];
  const ifaces = os.networkInterfaces();
  for (const name of Object.keys(ifaces)) {
    for (const a of ifaces[name] || []) {
      if (a.family !== 'IPv4' || a.internal) continue;
      const parts = a.address.split('.');
      if (parts.length !== 4) continue;
      out.push({ prefix: parts.slice(0, 3).join('.'), self: parseInt(parts[3], 10) });
    }
  }
  return out;
}

async function scanForControllers(onProgress) {
  const found = [];
  for (const sub of localSubnets()) {
    // Probe in batches to keep socket usage reasonable while staying quick.
    const BATCH = 32;
    for (let start = 1; start < 255; start += BATCH) {
      const batch = [];
      for (let i = start; i < Math.min(start + BATCH, 255); i++) {
        if (i === sub.self) continue;            // skip ourselves
        batch.push(probeSSH(`${sub.prefix}.${i}`));
      }
      if (onProgress) {
        onProgress(Math.min(start + BATCH - 1, 254), 254, sub.prefix);
      }
      const results = await Promise.all(batch);
      for (const r of results) if (r) found.push(r);
    }
  }
  return found;
}

// ─── Credential prompt ────────────────────────────────────────────────────────

function promptCredentials(parentWin, defaultHost, saved) {
  return new Promise((resolve) => {
    const channel = `survey-creds-${Date.now()}`;
    const canRemember = safeStorage.isEncryptionAvailable();

    const html = `<!DOCTYPE html><html><head><meta charset="UTF-8"><style>
      * { box-sizing: border-box; margin: 0; padding: 0; }
      body { background:#111827; color:#f1f5f9; font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;
             padding:22px; font-size:13px; -webkit-user-select:none; user-select:none; }
      h1 { font-size:15px; font-weight:600; margin-bottom:4px; }
      p.sub { color:#94a3b8; font-size:12px; margin-bottom:16px; line-height:1.45; }
      label { display:block; margin-bottom:4px; color:#cbd5e1; font-size:12px; }
      input[type=text], input[type=password] {
        width:100%; padding:7px 9px; margin-bottom:12px; border:1px solid #334155;
        border-radius:5px; background:#1e293b; color:#f1f5f9; font-size:13px;
        -webkit-user-select:text; user-select:text; }
      input:focus { outline:none; border-color:#3b82f6; }
      .row { display:flex; align-items:center; gap:7px; margin-bottom:16px; }
      .row input { margin:0; }
      .row label { margin:0; color:#94a3b8; font-size:12px; }
      .btns { display:flex; gap:8px; justify-content:flex-end; }
      button { padding:7px 15px; border:none; border-radius:5px; font-size:13px; cursor:pointer; }
      .go { background:#2563eb; color:#fff; }
      .go:hover { background:#1d4ed8; }
      .cx { background:#334155; color:#e2e8f0; }
      .cx:hover { background:#475569; }
      .note { color:#64748b; font-size:11px; margin-top:10px; line-height:1.4; }
      .findrow { font-size:11px; margin:-8px 0 12px; display:flex; justify-content:space-between; gap:8px; }
      .findrow a { color:#60a5fa; text-decoration:none; }
      .findrow a:hover { text-decoration:underline; }
      .ok   { color:#4ade80; }
      .warn { color:#fbbf24; }
      #scanstatus { margin:-8px 0 10px; }
    </style></head><body>
      <h1>Survey Sim Controller</h1>
      <p class="sub">Reads configuration details from the controller. Nothing is written or changed.</p>

      <label>Controller address</label>
      <input type="text" id="host" value="${(defaultHost || '').replace(/"/g, '&quot;')}" placeholder="192.168.1.50">
      <p class="findrow">
        ${defaultHost
          ? '<span class="ok">Detected from the connected controller.</span>'
          : '<span class="warn">No connected controller detected.</span>'}
        <a href="#" onclick="findThem(); return false;">Scan network…</a>
      </p>
      <p id="scanstatus" class="note"></p>

      <label>Username</label>
      <input type="text" id="user" value="${(saved && saved.username) || 'debian'}">

      <label>Password</label>
      <input type="password" id="pass" value="${(saved && saved.password) ? saved.password.replace(/"/g, '&quot;') : ''}">

      ${canRemember ? `<div class="row">
        <input type="checkbox" id="remember" ${saved ? 'checked' : ''}>
        <label for="remember">Remember for this controller</label>
      </div>` : `<p class="note">Secure storage is unavailable on this system, so the password cannot be remembered.</p>`}

      <div class="btns">
        <button class="cx" onclick="cancel()">Cancel</button>
        <button class="go" onclick="go()">Survey</button>
      </div>
      <p class="note">Default login for a stock BeagleBone image is debian / temppwd.</p>

      <script>
        const send = (d) => window.surveyBridge.submit(d);
        function go() {
          send({
            host: document.getElementById('host').value.trim(),
            username: document.getElementById('user').value.trim(),
            password: document.getElementById('pass').value,
            remember: !!(document.getElementById('remember') || {}).checked,
          });
        }
        function cancel() { send(null); }

        // Scan the local subnet for hosts with SSH open — used when no
        // controller is connected, so we cannot learn its address from
        // the simulation manager.
        async function findThem() {
          const s = document.getElementById('scanstatus');
          s.textContent = 'Scanning local network…';
          s.className = 'note';
          try {
            const hosts = await window.surveyBridge.scan();
            if (!hosts.length) {
              s.textContent = 'No hosts with SSH found. Check the controller is powered on and on this network.';
              s.className = 'note warn';
              return;
            }
            const host = document.getElementById('host');
            if (!host.value) host.value = hosts[0];
            s.innerHTML = 'Found: ' + hosts.map((h) =>
              '<a href="#" onclick="pick(\\'' + h + '\\'); return false;">' + h + '</a>'
            ).join(' &nbsp; ') + '<br>These are hosts running SSH — not all are controllers.';
            s.className = 'note';
          } catch (err) {
            s.textContent = 'Scan failed: ' + err;
            s.className = 'note warn';
          }
        }
        function pick(h) {
          document.getElementById('host').value = h;
          document.getElementById('pass').focus();
        }

        document.addEventListener('keydown', (e) => {
          if (e.key === 'Enter') go();
          if (e.key === 'Escape') cancel();
        });
        window.addEventListener('load', () => {
          const h = document.getElementById('host');
          (h.value ? document.getElementById('pass') : h).focus();
        });
      </script>
    </body></html>`;

    const win = new BrowserWindow({
      width: 380, height: canRemember ? 480 : 460,
      parent: parentWin || undefined,
      modal: !!parentWin,
      resizable: false, minimizable: false, maximizable: false,
      title: 'Survey Sim Controller',
      backgroundColor: '#111827',
      webPreferences: {
        preload: path.join(__dirname, 'survey-preload.js'),
        contextIsolation: true,
        nodeIntegration: false,
        additionalArguments: [`--survey-channel=${channel}`],
      },
    });

    let settled = false;
    const finish = (val) => {
      if (settled) return;
      settled = true;
      ipcMain.removeHandler(channel);
      if (!win.isDestroyed()) win.close();
      resolve(val);
    };

    ipcMain.handle(channel, (_e, data) => { finish(data); });
    win.on('closed', () => finish(null));
    win.setMenu(null);
    win.loadURL('data:text/html;charset=utf-8,' + encodeURIComponent(html));
  });
}

// ─── SSH survey ───────────────────────────────────────────────────────────────

function runSurvey({ host, username, password, onProgress }) {
  return new Promise((resolve, reject) => {
    let Client;
    try { ({ Client } = require('ssh2')); }
    catch {
      reject(new Error('The ssh2 package is not installed.\nRun  npm install  in OpenVetSim-App/'));
      return;
    }

    const conn    = new Client();
    const results = {};
    let finished  = false;

    const fail = (err) => {
      if (finished) return;
      finished = true;
      try { conn.end(); } catch {}
      reject(err);
    };

    conn.on('ready', async () => {
      try {
        for (let i = 0; i < SURVEY.length; i++) {
          const step = SURVEY[i];
          if (onProgress) onProgress(i + 1, SURVEY.length, step.label);
          results[step.key] = await execOne(conn, step.cmd);
        }
        finished = true;
        conn.end();
        resolve(results);
      } catch (e) { fail(e); }
    });

    conn.on('error', (err) => {
      // Translate the common cases into something a non-technical user can act on
      let msg = err.message || String(err);
      if (/All configured authentication methods failed/i.test(msg)) {
        msg = 'Login failed. Check the username and password.\n' +
              '(A stock BeagleBone image uses debian / temppwd.)';
      } else if (/ECONNREFUSED/i.test(msg)) {
        msg = `Connection refused by ${host}.\nSSH may be disabled on this controller.`;
      } else if (/EHOSTUNREACH|ENETUNREACH|ETIMEDOUT|timed out/i.test(msg)) {
        msg = `Could not reach ${host}.\nCheck that the controller is powered on and on the same network.`;
      } else if (/ENOTFOUND/i.test(msg)) {
        msg = `Unknown address: ${host}`;
      }
      fail(new Error(msg));
    });

    conn.connect({
      host,
      port: SSH_PORT,
      username,
      password,
      readyTimeout: CONNECT_TIMEOUT,
      // Field units run older Debian; their SSH offers legacy algorithms that
      // modern defaults exclude. Widen the sets so we can still reach them.
      algorithms: {
        kex: [
          'curve25519-sha256', 'curve25519-sha256@libssh.org',
          'ecdh-sha2-nistp256', 'ecdh-sha2-nistp384', 'ecdh-sha2-nistp521',
          'diffie-hellman-group-exchange-sha256',
          'diffie-hellman-group14-sha256', 'diffie-hellman-group14-sha1',
          'diffie-hellman-group1-sha1',
        ],
        serverHostKey: [
          'ssh-ed25519', 'ecdsa-sha2-nistp256',
          'rsa-sha2-512', 'rsa-sha2-256', 'ssh-rsa',
        ],
      },
    });
  });
}

function execOne(conn, cmd) {
  return new Promise((resolve) => {
    const timer = setTimeout(() => resolve('(timed out)'), COMMAND_TIMEOUT);
    conn.exec(cmd, (err, stream) => {
      if (err) { clearTimeout(timer); resolve(`(error: ${err.message})`); return; }
      let out = '';
      stream.on('data', (d) => (out += d.toString()));
      stream.stderr.on('data', (d) => (out += d.toString()));
      stream.on('close', () => {
        clearTimeout(timer);
        resolve(out.trim() || '(no output)');
      });
    });
  });
}

// ─── Report formatting ────────────────────────────────────────────────────────

function buildReport(host, results) {
  const lines = [
    'OpenVetSim — Sim Controller Survey',
    `Controller: ${host}`,
    `Surveyed:   ${new Date().toISOString()}`,
    '='.repeat(64),
    '',
  ];
  for (const step of SURVEY) {
    const val = (results[step.key] || '').split('\n');
    lines.push(`${step.label}:`);
    for (const l of val) lines.push(`    ${l}`);
    lines.push('');
  }
  return lines.join('\n');
}

// Append every survey to a local log so a fleet picture accumulates, including
// units that turn out to be unusual. This is the inventory we currently lack.
function appendToLog(host, report) {
  try {
    const p = path.join(app.getPath('userData'), 'controller-surveys.log');
    fs.appendFileSync(p, `\n${'#'.repeat(70)}\n${report}\n`);
    return p;
  } catch { return null; }
}

// ─── Results window ───────────────────────────────────────────────────────────

function showResults(parentWin, host, report, logPath) {
  const esc = (s) => s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');

  const html = `<!DOCTYPE html><html><head><meta charset="UTF-8"><style>
    * { box-sizing:border-box; margin:0; padding:0; }
    body { background:#111827; color:#f1f5f9; font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;
           display:flex; flex-direction:column; height:100vh; -webkit-user-select:none; user-select:none; }
    header { padding:14px 18px 10px; border-bottom:1px solid #1f2937; }
    h1 { font-size:14px; font-weight:600; }
    header p { color:#94a3b8; font-size:12px; margin-top:3px; }
    pre { flex:1; overflow:auto; margin:0; padding:14px 18px; font-size:11.5px; line-height:1.5;
          font-family:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace; color:#cbd5e1;
          -webkit-user-select:text; user-select:text; white-space:pre-wrap; }
    footer { padding:11px 18px; border-top:1px solid #1f2937; display:flex; gap:8px; align-items:center; }
    button { padding:6px 13px; border:none; border-radius:5px; font-size:12.5px; cursor:pointer;
             background:#334155; color:#e2e8f0; }
    button:hover { background:#475569; }
    button.primary { background:#2563eb; color:#fff; }
    button.primary:hover { background:#1d4ed8; }
    .spacer { flex:1; }
    .saved { color:#64748b; font-size:11px; }
    #done { color:#4ade80; font-size:12px; display:none; }
  </style></head><body>
    <header>
      <h1>Sim Controller Survey — ${esc(host)}</h1>
      <p>Read-only. Nothing on the controller was changed.</p>
    </header>
    <pre id="report">${esc(report)}</pre>
    <footer>
      <button class="primary" onclick="copyIt()">Copy to Clipboard</button>
      <button onclick="saveIt()">Save to File…</button>
      <span id="done">Copied</span>
      <span class="spacer"></span>
      ${logPath ? `<span class="saved">Logged locally</span>` : ''}
      <button onclick="window.close()">Close</button>
    </footer>
    <script>
      function copyIt() {
        window.surveyBridge.copy(document.getElementById('report').textContent);
        const d = document.getElementById('done');
        d.style.display = 'inline';
        setTimeout(() => { d.style.display = 'none'; }, 1600);
      }
      function saveIt() { window.surveyBridge.save(document.getElementById('report').textContent); }
    </script>
  </body></html>`;

  const win = new BrowserWindow({
    width: 660, height: 620,
    parent: parentWin || undefined,
    title: `Sim Controller Survey — ${host}`,
    backgroundColor: '#111827',
    webPreferences: {
      preload: path.join(__dirname, 'survey-preload.js'),
      contextIsolation: true,
      nodeIntegration: false,
    },
  });
  win.setMenu(null);
  win.loadURL('data:text/html;charset=utf-8,' + encodeURIComponent(html));
}

// Subnet scan, invoked from the credential prompt
ipcMain.handle('survey-scan', async () => {
  try { return await scanForControllers(); }
  catch { return []; }
});

// Clipboard / save handlers used by the results window
ipcMain.on('survey-copy', (_e, text) => clipboard.writeText(text));

ipcMain.on('survey-save', async (e, text) => {
  const win = BrowserWindow.fromWebContents(e.sender);
  const stamp = new Date().toISOString().slice(0, 19).replace(/[:T]/g, '-');
  const { canceled, filePath } = await dialog.showSaveDialog(win, {
    title: 'Save Controller Survey',
    defaultPath: `controller-survey-${stamp}.txt`,
    filters: [{ name: 'Text', extensions: ['txt'] }],
  });
  if (!canceled && filePath) {
    try { fs.writeFileSync(filePath, text); }
    catch (err) { dialog.showErrorBox('Could Not Save', err.message); }
  }
});

// ─── Entry point ──────────────────────────────────────────────────────────────

async function surveyController(parentWin, statusPort) {
  const detected = await detectControllerIP(statusPort);
  const saved    = detected ? loadCreds(detected) : null;
  const creds    = await promptCredentials(parentWin, detected, saved);
  if (!creds) return;                       // cancelled

  if (!creds.host) {
    dialog.showErrorBox('No Address', 'Enter the controller\'s IP address.');
    return;
  }

  const progress = new BrowserWindow({
    width: 320, height: 120,
    parent: parentWin || undefined, modal: !!parentWin,
    resizable: false, minimizable: false, maximizable: false,
    title: 'Surveying…', backgroundColor: '#111827',
    webPreferences: { contextIsolation: true, nodeIntegration: false },
  });
  progress.setMenu(null);
  const setProgress = (txt) => {
    if (progress.isDestroyed()) return;
    progress.loadURL('data:text/html;charset=utf-8,' + encodeURIComponent(
      `<body style="background:#111827;color:#f1f5f9;font-family:-apple-system,sans-serif;
         display:flex;align-items:center;justify-content:center;height:100vh;margin:0;
         font-size:13px;text-align:center;padding:16px">${txt}</body>`));
  };
  setProgress(`Connecting to ${creds.host}…`);

  try {
    const results = await runSurvey({
      host: creds.host,
      username: creds.username,
      password: creds.password,
      onProgress: (i, n, label) => setProgress(`Reading ${label}…<br><span style="color:#64748b">${i} of ${n}</span>`),
    });

    if (creds.remember) saveCreds(creds.host, creds.username, creds.password);
    else                forgetCreds(creds.host);

    const report  = buildReport(creds.host, results);
    const logPath = appendToLog(creds.host, report);

    if (!progress.isDestroyed()) progress.close();
    showResults(parentWin, creds.host, report, logPath);
  } catch (err) {
    if (!progress.isDestroyed()) progress.close();
    dialog.showErrorBox('Survey Failed', err.message || String(err));
  }
}

module.exports = { surveyController };
