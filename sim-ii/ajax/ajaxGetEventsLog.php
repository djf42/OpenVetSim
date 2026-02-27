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

	// ajaxGetEventsLogo.php: AJAX call to fetch the Events Log

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
	$logFileName = dbClass::valuesFromPost('fileName');
	if(file_exists(SERVER_SIM_LOGS . $logFileName) === FALSE) {
		$returnVal['status'] = AJAX_STATUS_FAIL;
		$returnVal['fileName'] = $logFileName;
		echo json_encode($returnVal);
		exit();
	}
	
	$logRecordCount = dbClass::valuesFromPost('recordCount');
	if(ctype_digit($logRecordCount) === FALSE) {
		$returnVal['status'] = AJAX_STATUS_FAIL;
		$returnVal['logRecordCount'] = $logRecordCount;
		echo json_encode($returnVal);
		exit();
	}
	
	$logArray = array_reverse(file(SERVER_SIM_LOGS . $logFileName, FILE_IGNORE_NEW_LINES + FILE_SKIP_EMPTY_LINES));
//FB::log($logArray);

	if(count($logArray) == 0) {
		$returnVal['status'] = AJAX_STATUS_FAIL;
		$returnVal['arrayCount'] = count($logArray);
		echo json_encode($returnVal);
		exit();	
	}

	$content = '';
	foreach($logArray as $logRecord) {
		list($timeSystemTime, $timeScenario, $timeScene, $event) = explode(" ", $logRecord, 4);
		$content .= '
			<tr>
				<td class="time-stamp">' . $timeScenario . '</td>
				<td class="event">' . $event . '</td>
			<tr>
		';
	}

	$returnVal['status'] = AJAX_STATUS_OK;
	$returnVal['html'] = $content;
	echo json_encode($returnVal);
	exit();
?>