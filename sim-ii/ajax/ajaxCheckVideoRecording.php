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

// ajaxCheckVideoRecording.php
// Returns {"found": true} if a .mp4 file newer than the provided Unix
// timestamp exists in simlogs/video/, indicating OBS has started recording.

require_once("../init.php");

$returnVal = array('found' => false);

$since = isset($_POST['since']) ? intval($_POST['since']) : 0;
$videoDir = SERVER_SIM_LOGS . 'video' . DIR_SEP;

if (is_dir($videoDir)) {
    $files = glob($videoDir . '*.mp4');
    if ($files) {
        foreach ($files as $file) {
            if (filemtime($file) >= $since) {
                $returnVal['found'] = true;
                break;
            }
        }
    }
}

echo json_encode($returnVal);
