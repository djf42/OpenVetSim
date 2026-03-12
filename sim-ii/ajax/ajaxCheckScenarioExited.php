<?php
/*
sim-ii

Copyright (C) 2019  VetSim, Cornell University College of Veterinary Medicine Ithaca, NY

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>
*/

// ajaxCheckScenarioExited.php
// Returns {"exited": true} when Electron has written simlogs/scenario_exited.flag
// after the provided Unix timestamp, indicating that the C++ binary logged
// "Scenario process is exiting" — the moment the scenario is fully wound down
// and the video file has been closed.
//
// Electron writes this flag file from the binary's stdout handler in main.js.
// The JavaScript overlay polls here every 500 ms after StopRecord is issued.

require_once("../init.php");

$returnVal = array('exited' => false);

$since    = isset($_POST['since']) ? intval($_POST['since']) : 0;
$flagFile = SERVER_SIM_LOGS . 'scenario_exited.flag';

if (file_exists($flagFile)) {
    clearstatcache(true, $flagFile);
    // Flag was written after the stop was requested → scenario has now exited
    if (filemtime($flagFile) >= $since) {
        $returnVal['exited'] = true;
    }
}

echo json_encode($returnVal);
