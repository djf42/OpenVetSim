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

	// ajaxGetScenarioTableContent.php: AJAX call to fetch list of available scenarios

	// init
	require_once("../init.php");
	$returnVal = array();

	// is user logged in
	if(adminClass::isUserLoggedIn() === FALSE) {
		$returnVal['status'] = AJAX_STATUS_LOGIN_FAIL;
		echo json_encode($returnVal);
		exit();
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
	
	// get list of scenarios and descriptions
	$scenarioDirs = scandir(SERVER_ACTIVE_SCENARIOS);
	$scenarioList = array();
	foreach($scenarioDirs as $scenarioDir) {
		if(is_dir(SERVER_ACTIVE_SCENARIOS . $scenarioDir) === true && $scenarioDir != '.' && $scenarioDir != '..' && $scenarioDir != '.git') {
			$scenarioList[] = $scenarioDir;
		}
	}
	
	// generate content
	$content = '
		<h2 id="user-table-title">Manage Scenarios</h2>
		
		<h2 id="scenario-add" style="float: none; margin: 10px 0 10px;">Please add scenarios by unzipping *.zip scenario files into the scenario folder.<br />The scenario folder can be found using the icon on your desktop.</h2>
		<!-- <h2 id="scenario-add">Add / Update Scenario (.zip archive only)</h2>
		<form class="clearer" id="scenario-select" method="post" action="ii.php" enctype="multipart/form-data">
			<input name="scenario-file-select" id="scenario-file-select" type="file" value="" accept=".zip">
			<input name="scenario-submit" type="submit" id="scenario-submit" style="visibility: hidden;">
		</form> -->
		
		<table id="user-table" class="scenario">
			<tr>
				<td class="scenario user-header col-200">Scenario Name</td>
				<td class="scenario user-header col-200">Scenario Dir</td>
				<td class="user-header scenario col-300">Description</td>
				<td class="user-header  col-150">Date Created</td>
				<td class="user-header col-150">&nbsp;</td>
			</tr>
	';
	
	$scenarioNameArray = array();
	foreach($scenarioList as $scenarioDir) {
		$fileName = $scenarioDir . DIRECTORY_SEPARATOR . 'main';
		$scenarioHeaderArray = scenarioXML::getScenarioHeaderArray($fileName);
		$scenarioNameArray[$scenarioHeaderArray['title']['name']] = $scenarioDir;
	}
	ksort($scenarioNameArray);

	foreach($scenarioNameArray as $scenarioName => $scenarioDir) {
		$fileName = $scenarioDir . DIRECTORY_SEPARATOR . 'main';
		$scenarioHeaderArray = scenarioXML::getScenarioHeaderArray($fileName);
		
		$content .= '
			<tr>
				<td class="scenario col-200">' . $scenarioHeaderArray['title']['name'] . '</td>
				<td class="scenario col-200">' . $scenarioDir . '</td>
				<td class="col-300 scenario">' . $scenarioHeaderArray['description'] . '</td>
				<td class="col-150 ">' . $scenarioHeaderArray['date_of_creation'] . '</td>
				<td class="col-150"><a href="javascript: void(2);" onclick="scenario.deleteScenario(\'' . $scenarioDir . '\');">Delete Scenario</a></td>
			</tr>
		';
	}
	
	$content .= '
		</table>
	';

	$returnVal['status'] = AJAX_STATUS_OK;
	$returnVal['html'] = $content;
	$returnVal['freeSpace'] = disk_free_space ( SERVER_SCENARIOS );
	echo json_encode($returnVal);
	exit();
?>