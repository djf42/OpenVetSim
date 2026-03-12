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

// ajaxCheckVideoStopped.php
// Returns the current byte-size of the most recent .mp4 file in simlogs/video/
// that was created at or after the provided Unix timestamp.
//
// The JavaScript side calls this every 500 ms and compares consecutive sizes.
// When the size stops growing (two identical non-zero values in a row) the
// file has been closed by OBS and debriefing can begin.
//
// Response:
//   { "found": false }                  — no matching file exists yet
//   { "found": true, "size": <bytes> }  — file exists; size may still be growing

require_once("../init.php");

$returnVal = array('found' => false, 'size' => 0);

$since = isset($_POST['since']) ? intval($_POST['since']) : 0;
$videoDir = SERVER_SIM_LOGS . 'video' . DIR_SEP;

if (is_dir($videoDir)) {
    $files = glob($videoDir . '*.mp4');
    if ($files) {
        // Find the newest .mp4 created at or after $since
        $newest     = null;
        $newestTime = 0;
        foreach ($files as $file) {
            $mtime = filemtime($file);
            if ($mtime >= $since && $mtime > $newestTime) {
                $newestTime = $mtime;
                $newest     = $file;
            }
        }
        if ($newest) {
            clearstatcache(true, $newest);   // ensure we get the live size
            $returnVal['found'] = true;
            $returnVal['size']  = filesize($newest);
        }
    }
}

echo json_encode($returnVal);
