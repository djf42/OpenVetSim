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
	
	// get params
	$logFileName = dbClass::valuesFromPost('fn');

	if(file_exists(SERVER_SIM_LOGS . $logFileName) === FALSE) {
		$returnVal['status'] = AJAX_STATUS_FAIL;
		echo json_encode($returnVal);
		exit();
	}
	
	$logArray = file(SERVER_SIM_LOGS . $logFileName, FILE_IGNORE_NEW_LINES + FILE_SKIP_EMPTY_LINES);
//FB::log($logArray);

	if(count($logArray) == 0) {
		$returnVal['status'] = AJAX_STATUS_FAIL;
		echo json_encode($returnVal);
		exit();	
	}

	// add in checkboxes
	$content = '
		<div class="log-filter">
			<p id="settings-status" class="settings-class legend"></p><p class="legend-label">Hide II Settings</p>
			<p id="simulator-status" class="simulator-class legend"></p><p class="legend-label">Hide Simulator</p>
			<p id="et-status" class="et-class legend"></p><p class="legend-label">Hide Events & Triggers</p>
		</div>
		<div class="clearer"></div>
	';
	
	
	$content .= '<table id="player-event-list">
					<tr>
						<td class="event-header time align-center">Elapsed Time</td>
						<td class="event-header time align-center">Scenario Time</td>
						<td class="event-header time align-center">Scene Time</td>
						<td class="event-header align-center">Event Description</td>
					</tr>
	';
	
	$timeStampBase = strtotime("1/1/2000 0:0:0");
	
	foreach($logArray as $logRecord) {
		list($timeStamp, $scenarioTime, $sceneTime, $event) = explode(" ", $logRecord, 4);
		$eventTimeStamp = strtotime('1/1/2000 ' . $timeStamp) - $timeStampBase;

		$content .= '
			<tr class="event-content-row" data-ts="' . $eventTimeStamp . '">
				<td class="time-stamp event-content align-center">' . $timeStamp . '</td>
				<td class="time-stamp event-content align-center">' . $scenarioTime . '</td>
				<td class="time-stamp event-content align-center">' . $sceneTime . '</td>
				<td class="event event-content">' . $event . '</td>
			</tr>
		';
	}
	$content .= '</table>';

	$returnVal['status'] = AJAX_STATUS_OK;
	$returnVal['html'] = $content;
	echo json_encode($returnVal);
	exit();
?>