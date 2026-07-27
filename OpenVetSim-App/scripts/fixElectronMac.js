'use strict';

/*
 * fixElectronMac.js — run automatically after `npm install`
 *
 * PROBLEM
 * -------
 * npm downloads Electron as a zip, so the extracted Electron.app carries the
 * com.apple.quarantine extended attribute and has no signature we control. On
 * macOS Sequoia and later, Gatekeeper refuses to run it at all:
 *
 *     "Electron.app" has been blocked because it may reduce your privacy and
 *     lower the security of your Mac. You should move it to the Trash.
 *
 * There is often no "Open Anyway" button offered, because the app was launched
 * from a terminal (via `npm start`) rather than double-clicked — so the usual
 * Privacy & Security workaround does not appear.
 *
 * FIX
 * ---
 * Clear the quarantine attributes and apply an ad-hoc signature, which marks
 * the binary as locally trusted without needing a certificate.
 *
 * This affects ONLY the development Electron in node_modules, used by
 * `npm start`. The shipped application is signed with the Developer ID and
 * notarized by electron-builder, and is unaffected by any of this.
 *
 * Silent and harmless on Windows and Linux, and non-fatal if anything fails —
 * a failure here should never block an install.
 */

const { execFileSync } = require('child_process');
const fs   = require('fs');
const path = require('path');

if (process.platform !== 'darwin') {
  process.exit(0);
}

const appPath = path.join(__dirname, '..', 'node_modules', 'electron', 'dist', 'Electron.app');

if (!fs.existsSync(appPath)) {
  // Electron not installed (or a different layout) — nothing to do.
  process.exit(0);
}

function run(cmd, args) {
  try {
    execFileSync(cmd, args, { stdio: 'ignore' });
    return true;
  } catch {
    return false;
  }
}

// Strip quarantine (and anything else) recursively.
const cleared = run('xattr', ['-cr', appPath]);

// Ad-hoc sign so Gatekeeper will run it. --deep is deprecated but remains the
// practical way to cover the nested helper apps and frameworks inside the bundle.
const signed = run('codesign', ['--force', '--deep', '--sign', '-', appPath]);

if (cleared && signed) {
  console.log('fixElectronMac: cleared quarantine and ad-hoc signed the dev Electron');
} else {
  console.log(
    'fixElectronMac: could not fully prepare the dev Electron.\n' +
    '  If `npm start` is blocked by macOS, run these by hand:\n' +
    `    xattr -cr "${appPath}"\n` +
    `    codesign --force --deep --sign - "${appPath}"`
  );
}
