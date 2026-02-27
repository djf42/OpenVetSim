'use strict';

const { contextBridge, ipcRenderer } = require('electron');

// Expose a safe, narrow API to the toolbar renderer
contextBridge.exposeInMainWorld('simBridge', {
  // Commands
  start:   () => ipcRenderer.send('sim-start'),
  stop:    () => ipcRenderer.send('sim-stop'),
  restart: () => ipcRenderer.send('sim-restart'),

  // Query
  getStatus: () => ipcRenderer.invoke('sim-status'),

  // Events from main → renderer
  onState: (cb) => ipcRenderer.on('sim-state',  (_e, data) => cb(data)),
  onLog:   (cb) => ipcRenderer.on('sim-log',    (_e, line) => cb(line)),
  onExit:  (cb) => ipcRenderer.on('sim-exited', (_e, data) => cb(data)),
});
