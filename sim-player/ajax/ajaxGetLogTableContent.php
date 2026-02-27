<?php
/*
sim-player: 

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
ini_set('display_errors', 'Off');
error_reporting(E_ALL & ~E_DEPRECATED);
	// init
	require_once("../init.php");
	$returnVal = array();

	// is user logged in
	if(adminClass::isUserLoggedIn() === FALSE) {
		$returnVal['status'] = AJAX_STATUS_LOGIN_FAIL;
		echo json_encode($returnVal);
		exit();
	}
	
	// get selected file name
	$currentLogFile = dbClass::valuesFromPost('fn');
	
	// get list of log files
	$fileNameArray = is_dir(SERVER_SIM_LOGS) ? scandir(SERVER_SIM_LOGS, SCANDIR_SORT_DESCENDING) : [];

	// parse file list
	$fileList = array();
	if(count($fileNameArray) > 0) {
		foreach($fileNameArray as $fileName) {
			if($fileName == '.' || $fileName == '..' || $fileName == 'archive' || $fileName == 'video') {
				continue;
			}

			$nameArray = explode('_', $fileName);
			if ( count($nameArray) < 2 ) {
				continue;
			}
			$dateString = $nameArray[0] . ' ' . $nameArray[1];
			
			// get number of elements in name array
			$nameString = '';
			for($index = 2; $index < count($nameArray); $index++) {
				$nameString .= $nameArray[$index] . '-';
			}
			
			$fileList[] = array('name' => substr($nameString, 0, -5),
								'date' => $dateString,
								'orig' => $fileName
								);
		}
	}
	
	// generate content
	$content = '
		<h2 id="user-table-title">Found Log Files</h2>
		
		<table id="log-table-title" class="log-table">
			<tr>
				<td class="user-header col-scenario">Scenario Name</td>
				<td class="user-header col-date">Date</td>
				<td class="user-header col-vid">Video File Found?</td>
			</tr>
		</table>
		
		<div id="log-table-content" class="log-table">
		<table class="log-table">
	';
	
	$currentFilePos = -1;
	$returnVal['currentFilePos'] = $currentFilePos;
	foreach($fileList as $fileInfo) {
		$currentFilePos++;
		$selectContent = '';
		if($fileInfo['orig'] == $currentLogFile) {
			$selectContent = ' class="selected" ';
			$returnVal['currentFilePos'] = $currentFilePos;
		}
		
		$videoFileName = substr($fileInfo['orig'], 0, -4) . '.mp4';

		// does video file exist?
		if( !file_exists(SERVER_SIM_VIDEO . $videoFileName) ) {
			$videoFileName = "Video file not found!";
		}
		
		$content .= '
			<tr data-filename="' . $fileInfo['orig'] . '" ' . $selectContent . '>
				<td class="col-scenario log-name">' . $fileInfo['name'] . '</td>
				<td class="col-date">' . $fileInfo['date'] . '</td>
				<td class="col-vid log-video">' . $videoFileName . '</td>
			</tr>
		';
	}
	
	$content .= '
		</table>
		</div>
	';

	$returnVal['status'] = AJAX_STATUS_OK;
	$returnVal['html'] = $content;
	echo json_encode($returnVal);
	exit();
?>