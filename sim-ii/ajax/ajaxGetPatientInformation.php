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

	// ajaxGetPatientInformation.php: AJAX call to fetch the Patient Information block


	// init
	require_once("../init.php");
	$returnVal = array();

	// is user logged in
	if(adminClass::isUserLoggedIn() === FALSE) {
		if ( SERVER_ADDR != REMOTE_ADDR )
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
	
	$profileArray = $_POST['profile'];
	$directory = dbClass::valuesFromPost('dir');
	
	// resize image
	$imageSizeArray = getImageSize(SERVER_ACTIVE_SCENARIOS . $directory . DIRECTORY_SEPARATOR . "images" . DIRECTORY_SEPARATOR .  $profileArray['image']);
	if($imageSizeArray !== FALSE) {
		// check width, max = 300px
		if($imageSizeArray[0] >= $imageSizeArray[1]) {
			$cssContent = ' style="width: 138px" ';
		} else {
			$cssContent = ' style="height: 138px" ';		
		}
	}
	
	$content = '
		<div id="patient-info">
			<h1>Patient Information</h1>
			<img src="' . BROWSER_ACTIVE_SCENARIOS . $directory . DIRECTORY_SEPARATOR . "images" . DIRECTORY_SEPARATOR .  $profileArray['image'] . '" ' . $cssContent . '>
			<table>
	';
	if(isset($profileArray['species']) === TRUE) {
		$content .= '
				<tr>
					<td>Species:</td>
					<td>' . $profileArray['species'] . '</td>
				</tr>
		';
	}
	if(isset($profileArray['breed']) === TRUE) {
		$content .= '
				<tr>
					<td>Breed:</td>
					<td>' . $profileArray['breed'] . '</td>
				</tr>
		';
	}
	if(isset($profileArray['age']) === TRUE) {
		$content .= '
				<tr>
					<td>Age:</td>
					<td>' . $profileArray['age'] . '</td>
				</tr>
		';
	}
	if(isset($profileArray['sex']) === TRUE) {
		$content .= '
				<tr>
					<td>Sex:</td>
					<td>' . $profileArray['sex'] . '</td>
				</tr>
		';
	}
	if(isset($profileArray['weight']) === TRUE) {
		$content .= '
				<tr>
					<td>Weight:</td>
					<td>' . $profileArray['weight'] . '</td>
				</tr>
		';
	}
	if(isset($profileArray['complaint']) === TRUE) {
		$content .= '
				<tr>
					<td>Complaint:</td>
					<td>' . $profileArray['complaint'] . '</td>
				</tr>
		';
	}
	if(isset($profileArray['description']) === TRUE) {
		$content .= '
				<tr>
					<td>Description:</td>
					<td>' . $profileArray['description'] . '</td>
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