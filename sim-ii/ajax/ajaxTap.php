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

	// ajaxTap.php: Keep alive function to prevent PHP session timeouts

	// Called periodically to keep PHP Session from timing out
	// init
	require_once("../init.php");
	$returnVal = array();

	// is user logged in
	if(adminClass::isUserLoggedIn() === FALSE) {
		$returnVal['status'] = AJAX_STATUS_LOGIN_FAIL;
		echo json_encode($returnVal);
		exit();
	}
	
	$count = 0;
	if ( isset($_SESSION['tapCount'] ) )
		$count = $_SESSION['tapCount'];
	$count++;
	$_SESSION['tapCount'] = $count;
	
	$returnVal['count'] = $count;
	$returnVal['status'] = AJAX_STATUS_OK;
	echo json_encode($returnVal);
	exit();
?>