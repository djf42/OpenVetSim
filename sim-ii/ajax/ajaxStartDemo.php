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

	// ajaxStartDemo.php: AJAX call to start a demo session

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
	
	// Get the Session ID
	if ( !isset($_COOKIE['PHPSESSID'] ) )
	{
		$returnVal['status'] = AJAX_STATUS_FAIL;
		$returnVal['cause'] = "No Session ID in Cookies";
		echo json_encode($returnVal);
		exit();
	}
	$sesid = $_COOKIE['PHPSESSID'];
	// Start a simmgr demo session
	$returnVal['cause'] = shell_exec("simmgrDemo ".$sesid );
	if ( $returnVal['cause'] != '' )
	{
		$returnVal['status'] = AJAX_STATUS_FAIL;
	}
	else
	{
		$returnVal['status'] = AJAX_STATUS_OK;
	}
	echo json_encode($returnVal);
	exit();
?>
	