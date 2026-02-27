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

	// ajaxGetScenario.php: AJAX call to fetch data for a scenario

	// init
	require_once("../init.php");
	$returnVal = array();
	
	// is user logged in
	if(adminClass::isUserLoggedIn() === FALSE) {
		if ( SERVER_ADDR != REMOTE_ADDR ) // This allows access without login for Student Vitals display
		{
			$returnVal['status'] = AJAX_STATUS_LOGIN_FAIL;
			$returnVal['cause'] = "No login";
			echo json_encode($returnVal);
			exit();
		}
	}
	
	$userRow = adminClass::getUserRowFromSession();
	$uid = $userRow['UserID'];
	$sessionID = session_id();
	// If Demo user, then we use a temporary directory for Scenarios
	// Otherwise, the standard directory
	if ( $uid == 5 )
	{
		define("SERVER_ACTIVE_SCENARIOS", SERVER_DEMO_SCENARIOS . $sessionID . DIRECTORY_SEPARATOR);
		define("BROWSER_ACTIVE_SCENARIOS",BROWSER_DEMO_SCENARIOS . $sessionID . DIRECTORY_SEPARATOR);

	}
	else
	{
		define("SERVER_ACTIVE_SCENARIOS", SERVER_SCENARIOS );
		define("BROWSER_ACTIVE_SCENARIOS", BROWSER_SCENARIOS );
	}
	
	// get file name
	$fileName = dbClass::valuesFromPost('fn');
	if($fileName == '') {
		$returnVal['status'] = AJAX_STATUS_FAIL;
		$returnVal['cause'] = "fn not set";
		echo json_encode($returnVal);
		exit();			
	}
	
	// update path to scenario
	$fileName = $fileName . DIRECTORY_SEPARATOR . 'main';
	
	$scenarioProfileArray = scenarioXML::getScenarioProfileArray($fileName);
	if($scenarioProfileArray === FALSE) {
		$returnVal['status'] = AJAX_STATUS_FAIL;
		$returnVal['cause'] = "scenarioProfileArray failed";
		echo json_encode($returnVal);
		exit();		
	}

	$scenarioHeaderArray = scenarioXML::getScenarioHeaderArray($fileName);
	if($scenarioHeaderArray === FALSE) {
		$returnVal['status'] = AJAX_STATUS_FAIL;
		$returnVal['cause'] = "scenarioHeaderArray failed";
		echo json_encode($returnVal);
		exit();		
	}

	$scenarioEventsArray = scenarioXML::getScenarioEventsArray($fileName);
	if($scenarioEventsArray === FALSE) {
		$returnVal['status'] = AJAX_STATUS_FAIL;
		$returnVal['cause'] = "getScenarioEventsArray failed";
		echo json_encode($returnVal);
		exit();		
	}
	
	$scenarioMediaArray = scenarioXML::getScenarioMediaArray($fileName);
	if($scenarioMediaArray === FALSE) {
		$returnVal['status'] = AJAX_STATUS_FAIL;
		$returnVal['cause'] = "getScenarioMediaArray failed";
		echo json_encode($returnVal);
		exit();
	}

	$scenarioVocalsArray = scenarioXML::getScenarioVocalsArray($fileName);
	if($scenarioVocalsArray === FALSE) {
		$returnVal['status'] = AJAX_STATUS_FAIL;
		$returnVal['cause'] = "getScenarioVocalsArray failed";
		echo json_encode($returnVal);
		exit();
	}

	$returnVal['status'] = AJAX_STATUS_OK;
	$returnVal['profile'] = $scenarioProfileArray;
	$returnVal['header'] = $scenarioHeaderArray;
	$returnVal['events'] = $scenarioEventsArray;
	$returnVal['media'] = $scenarioMediaArray;
	$returnVal['vocals'] = $scenarioVocalsArray;
	
	// The soundtags array is optional and only used for TeleSim mode
	$scenarioSoundtagsArray = scenarioXML::getScenarioSoundtagsArray($fileName);
	if($scenarioSoundtagsArray !== FALSE) {
		$returnVal['soundtags'] = $scenarioSoundtagsArray;
	}
	
	// same for the telesim information
	$scenarioTelesimArray = scenarioXML::getScenarioTelesimArray($fileName);
	if($scenarioTelesimArray !== FALSE) {
		$returnVal['telesim'] = $scenarioTelesimArray;
	}

	echo json_encode($returnVal);
	exit();
?>