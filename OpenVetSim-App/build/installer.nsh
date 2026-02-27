; ============================================================================
; OpenVetSim — custom NSIS installer fragments
; Included by electron-builder via nsis.include in package.json.
;
; What this does:
;   Install:   Creates %PROGRAMDATA%\OpenVetSim\ and copies all web files
;              (sim-ii, sim-mgr, sim-ctl, sim-player, scenarios) there so
;              they are readable and writable by any Windows user account.
;              Also pre-creates the simlogs\video directory tree.
;
;   Uninstall: Removes the installed web files from ProgramData.
;              Prompts before deleting user-generated data (simlogs, scenarios
;              that may have been modified).
; ============================================================================

; ── Install ──────────────────────────────────────────────────────────────────
!macro customInstall

  ; Create the shared data root and log/video directories
  CreateDirectory "$PROGRAMDATA\OpenVetSim"
  CreateDirectory "$PROGRAMDATA\OpenVetSim\simlogs"
  CreateDirectory "$PROGRAMDATA\OpenVetSim\simlogs\video"

  ; Copy web application folders from the bundled resources into ProgramData.
  ; xcopy /E = recurse subdirectories, /I = assume destination is a directory,
  ;       /Y = overwrite without prompt, /Q = quiet.
  ExecWait 'xcopy /E /I /Y /Q "$INSTDIR\resources\sim-ii"     "$PROGRAMDATA\OpenVetSim\sim-ii"'
  ExecWait 'xcopy /E /I /Y /Q "$INSTDIR\resources\sim-mgr"    "$PROGRAMDATA\OpenVetSim\sim-mgr"'
  ExecWait 'xcopy /E /I /Y /Q "$INSTDIR\resources\sim-ctl"    "$PROGRAMDATA\OpenVetSim\sim-ctl"'
  ExecWait 'xcopy /E /I /Y /Q "$INSTDIR\resources\sim-player" "$PROGRAMDATA\OpenVetSim\sim-player"'
  ExecWait 'xcopy /E /I /Y /Q "$INSTDIR\resources\scenarios"  "$PROGRAMDATA\OpenVetSim\scenarios"'

!macroend

; ── Uninstall ─────────────────────────────────────────────────────────────────
!macro customUnInstall

  ; Remove the installed web application files.
  RMDir /r "$PROGRAMDATA\OpenVetSim\sim-ii"
  RMDir /r "$PROGRAMDATA\OpenVetSim\sim-mgr"
  RMDir /r "$PROGRAMDATA\OpenVetSim\sim-ctl"
  RMDir /r "$PROGRAMDATA\OpenVetSim\sim-player"

  ; Ask before deleting user-generated data (scenarios and session logs/videos).
  MessageBox MB_YESNO|MB_ICONQUESTION \
    "Remove simulation scenarios and session logs from $PROGRAMDATA\OpenVetSim?$\n$\nChoose No to keep your recorded sessions and custom scenarios." \
    IDNO keepUserData

    RMDir /r "$PROGRAMDATA\OpenVetSim\scenarios"
    RMDir /r "$PROGRAMDATA\OpenVetSim\simlogs"
    RMDir    "$PROGRAMDATA\OpenVetSim"   ; removes dir only if now empty

  keepUserData:

!macroend
