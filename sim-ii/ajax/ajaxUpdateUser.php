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

	// ajaxUpdateUser.php: AJAX call to change a User record

	// init
	require_once("../init.php");
	$returnVal = array();

	// is user logged in
	if(adminClass::isUserLoggedIn() === FALSE) {
		$returnVal['status'] = AJAX_STATUS_LOGIN_FAIL;
		echo json_encode($returnVal);
		exit();
	}
	
	// get variables
	$userID = dbClass::valuesFromPost('uID');
	$userFirstName = dbClass::valuesFromPost('fn');
	$userLastName = dbClass::valuesFromPost('ln');
	$userEmail = dbClass::valuesFromPost('em');
	$userPassWord = dbClass::valuesFromPost('pw');
	
	// get list of users
	$userRow = adminClass::getUserRow($userID);

	if($userRow == FALSE) {
		$returnVal['status'] = AJAX_STATUS_FAIL;
		echo json_encode($returnVal);
		exit();
	}
	
	$cleanArray['UserID'] = $userID;
	$cleanArray['UserFirstName'] = $userFirstName;
	$cleanArray['UserLastName'] = $userLastName;
	$cleanArray['UserEmail'] = $userEmail;
	
	if($userPassWord != '') {
		$cleanArray['UserPassWord'] = $userPassWord;
		$cleanArray['UserSalt'] = adminClass::generateSalt();
	}

	$result = dbClass::dbUpdateQueryResult(modelClass::createUpdateQuery('Users', $cleanArray));
//FB::log(modelClass::createUpdateQuery('Users', $cleanArray));
	
	$returnVal['status'] = AJAX_STATUS_OK;
	echo json_encode($returnVal);
	exit();
?>