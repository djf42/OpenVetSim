'use strict';

/*
 * survey-preload.js
 *
 * Narrow bridge for the controller survey windows (credential prompt and
 * results). Kept separate from preload.js so the survey windows expose only
 * what they need and nothing else.
 *
 * The credential prompt is given its IPC channel name via additionalArguments
 * so each invocation gets a unique, single-use channel rather than a global one.
 */

const { contextBridge, ipcRenderer } = require('electron');

const channelArg = process.argv.find((a) => a.startsWith('--survey-channel='));
const channel    = channelArg ? channelArg.split('=')[1] : null;

contextBridge.exposeInMainWorld('surveyBridge', {
  // Credential prompt → main. Pass null to cancel.
  submit: (data) => { if (channel) ipcRenderer.invoke(channel, data); },

  // Probe the local subnet for hosts with SSH open. Used when no controller is
  // connected, so its address cannot be read from the simulation manager.
  // Resolves to an array of IP strings.
  scan: () => ipcRenderer.invoke('survey-scan'),

  // Results window actions
  copy: (text) => ipcRenderer.send('survey-copy', text),
  save: (text) => ipcRenderer.send('survey-save', text),
});
