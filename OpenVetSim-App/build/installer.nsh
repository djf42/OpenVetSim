; ============================================================================
; OpenVetSim -- custom NSIS installer fragments
; Included by electron-builder via nsis.include in package.json.
;
; What this does:
;   Install:   Creates %PUBLIC%\OpenVetSim\ and copies the web files
;              (sim-ii, sim-mgr, sim-ctl, sim-player, sim-remote) there so
;              they are readable and writable by any Windows user account.
;              Also pre-creates the simlogs\video directory tree.
;              Places a shortcut on the All Users desktop (%PUBLIC%\Desktop).
;
;              SCENARIOS ARE HANDLED SEPARATELY AND CAREFULLY -- see below.
;
;   Uninstall: Removes the installed web files from %PUBLIC%\OpenVetSim.
;              Prompts before deleting user data, defaulting to KEEPING it.
;
; ---------------------------------------------------------------------------
; SCENARIOS: WHY THIS IS NOT A PLAIN xcopy
; ---------------------------------------------------------------------------
; Scenarios are USER DATA. Instructors edit the bundled ones and create their
; own, and a scenario represents real teaching preparation that cannot be
; reconstructed from the installer.
;
; This previously ran:
;     xcopy /E /I /Y /Q "...\resources\scenarios" "%PUBLIC%\OpenVetSim\scenarios"
;
; /Y means "overwrite without prompting". Any scenario whose name collided with
; a bundled one -- Example_Scenario, blank, default, healthy_dog -- was silently
; replaced, along with every file inside it. An instructor who had customised
; "default" or "healthy_dog", or who happened to name a scenario the same as a
; bundled one, lost that work on install with no warning and no backup. This
; happened in the field.
;
; The install now:
;   1. Detects whether scenarios already exist.
;   2. If none exist, installs the bundled set (normal first install).
;   3. If any exist, ALWAYS takes a timestamped backup first, then asks what to
;      do -- with "keep my scenarios" as the DEFAULT button, so pressing Enter
;      or Space is the safe outcome.
;   4. In the keep case, uses robocopy with /XC /XN /XO so only scenarios that
;      are not already present get added. An existing file is never touched,
;      regardless of its date.
;
; Note: the NSIS bundled with electron-builder does not resolve shell folder
; constants ($COMMONAPPDATA, $COMMONDESKTOP, etc.), so we use ReadEnvStr to
; read Windows environment variables directly instead.
;   %PUBLIC%  = C:\Users\Public  (All Users profile on Windows Vista+)
; ============================================================================

!include "FileFunc.nsh"     ; for ${GetTime}, used to timestamp backups

; == Install ==================================================================
!macro customInstall

  ReadEnvStr $R0 PUBLIC        ; e.g. C:\Users\Public

  ; Create the shared data root and log/video directories.
  CreateDirectory "$R0\OpenVetSim"
  CreateDirectory "$R0\OpenVetSim\simlogs"
  CreateDirectory "$R0\OpenVetSim\simlogs\video"

  ; --- Web application folders -------------------------------------------
  ; These are program files, not user data, so overwriting them is correct.
  ; xcopy /E = recurse subdirectories, /I = assume destination is a directory,
  ;       /Y = overwrite without prompt, /Q = quiet.
  ;
  ; sim-remote is included here deliberately: it is bundled into resources by
  ; package.json but was previously never copied to Public, so the phone and
  ; tablet interface did not exist in a packaged Windows install.
  ExecWait 'xcopy /E /I /Y /Q "$INSTDIR\resources\sim-ii"     "$R0\OpenVetSim\sim-ii"'
  ExecWait 'xcopy /E /I /Y /Q "$INSTDIR\resources\sim-mgr"    "$R0\OpenVetSim\sim-mgr"'
  ExecWait 'xcopy /E /I /Y /Q "$INSTDIR\resources\sim-ctl"    "$R0\OpenVetSim\sim-ctl"'
  ExecWait 'xcopy /E /I /Y /Q "$INSTDIR\resources\sim-player" "$R0\OpenVetSim\sim-player"'
  ExecWait 'xcopy /E /I /Y /Q "$INSTDIR\resources\sim-remote" "$R0\OpenVetSim\sim-remote"'

  ; --- Scenarios: user data, handled carefully ---------------------------
  ; Is there already a scenarios folder with anything in it?
  IfFileExists "$R0\OpenVetSim\scenarios\*.*" scenariosExist scenariosFresh

  scenariosFresh:
    ; Nothing there -- normal first install. Safe to copy the bundled set.
    ExecWait 'xcopy /E /I /Y /Q "$INSTDIR\resources\scenarios" "$R0\OpenVetSim\scenarios"'
    Goto scenariosDone

  scenariosExist:
    ; Timestamp for the backup folder name.
    ; $1=Day $2=Month $3=Year $4=DayOfWeek $5=Hour $6=Minute $7=Second
    ${GetTime} "" "L" $1 $2 $3 $4 $5 $6 $7
    StrCpy $R1 "$3-$2-$1_$5$6$7"

    ; ALWAYS back up first, whatever the user then chooses.
    ExecWait 'xcopy /E /I /Y /Q "$R0\OpenVetSim\scenarios" "$R0\OpenVetSim\scenarios-backup-$R1"'

    ; Ask what to do. MB_DEFBUTTON2 makes "No" the default button, so the safe
    ; choice is what happens if someone presses Enter without reading.
    MessageBox MB_YESNO|MB_ICONEXCLAMATION|MB_DEFBUTTON2 \
      "Existing simulation scenarios were found in:$\n$R0\OpenVetSim\scenarios$\n$\nReplace them with the example scenarios included in this installer?$\n$\nChoose No to KEEP your existing scenarios (recommended). Any example scenario you do not already have will still be added, and nothing of yours will be changed.$\n$\nA backup of your current scenarios has been saved to:$\nOpenVetSim\scenarios-backup-$R1" \
      IDYES scenariosReplace

    ; --- Keep the user's scenarios (the default) ---
    ; robocopy /XC /XN /XO excludes Changed, Newer and Older files, which
    ; together means every file that already exists is skipped. Only scenarios
    ; not present at all are added; existing files are never touched.
    ;   /E   = include subdirectories, including empty ones
    ;   /NFL /NDL /NJH /NJS /NP = quiet output
    ; robocopy returns 0-7 on success, so its exit code is not an error here.
    ExecWait 'robocopy "$INSTDIR\resources\scenarios" "$R0\OpenVetSim\scenarios" /E /XC /XN /XO /NFL /NDL /NJH /NJS /NP'
    Goto scenariosDone

  scenariosReplace:
    ; User explicitly chose to replace. The backup above already exists.
    ExecWait 'xcopy /E /I /Y /Q "$INSTDIR\resources\scenarios" "$R0\OpenVetSim\scenarios"'

  scenariosDone:

  ; Create a shortcut on the All Users desktop so every account can find
  ; the scenarios folder easily.
  CreateShortCut "$R0\Desktop\OpenVetSim Scenarios.lnk" "$R0\OpenVetSim\scenarios"

!macroend

; == Uninstall ================================================================
!macro customUnInstall

  ReadEnvStr $R0 PUBLIC

  ; Remove the installed web application files. These are program files.
  RMDir /r "$R0\OpenVetSim\sim-ii"
  RMDir /r "$R0\OpenVetSim\sim-mgr"
  RMDir /r "$R0\OpenVetSim\sim-ctl"
  RMDir /r "$R0\OpenVetSim\sim-player"
  RMDir /r "$R0\OpenVetSim\sim-remote"

  ; Remove the desktop shortcut.
  Delete "$R0\Desktop\OpenVetSim Scenarios.lnk"

  ; Ask before deleting user data.
  ;
  ; MB_DEFBUTTON2 makes "No" (keep) the default button. Without it NSIS defaults
  ; to "Yes", so anyone pressing Enter through the uninstaller silently deleted
  ; every scenario and recorded session. Scenarios represent teaching
  ; preparation that cannot be recovered, so the destructive option must never
  ; be the one that happens by accident.
  MessageBox MB_YESNO|MB_ICONEXCLAMATION|MB_DEFBUTTON2 \
    "Also delete your simulation scenarios and session recordings?$\n$\nThis permanently removes everything in:$\n$R0\OpenVetSim\scenarios$\n$R0\OpenVetSim\simlogs$\n$\nChoose No to KEEP your scenarios and recordings (recommended). They will still be there if you reinstall OpenVetSim later." \
    IDNO keepUserData

    RMDir /r "$R0\OpenVetSim\scenarios"
    RMDir /r "$R0\OpenVetSim\simlogs"
    RMDir    "$R0\OpenVetSim"   ; removes dir only if now empty

  keepUserData:

!macroend
