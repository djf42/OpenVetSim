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

	// ajaxGetUserTableContent.php: AJAX call to fetch the modal for User Management

	// init
	require_once("../init.php");
	$returnVal = array();

	// is user logged in
	if(adminClass::isUserLoggedIn() === FALSE) {
		$returnVal['status'] = AJAX_STATUS_LOGIN_FAIL;
		echo json_encode($returnVal);
		exit();
	}
	
	// get list of users
	$userRows = adminClass::getAllUserRows();
	if(count($userRows) == 0) {
		$returnVal['status'] = AJAX_STATUS_FAIL;
		echo json_encode($returnVal);
		exit();
	}
	
	// generate content
	$content = '
		<h2 id="user-table-title">User Table</h2>
		
		<table id="user-table">
			<tr>
				<td class="user-header col-150"><a class="add-new-user" href="javascript: void(2);" onclick="alert(\'Not implemented just yet.\'); return false;">Add New User</a></td>
				<td class="user-header col-200">First Name</td>
				<td class="user-header col-200">Last Name</td>
				<td class="user-header col-300">Email</td>
				<td class="user-header col-100">New Password</td>
				<td class="user-header col-150">&nbsp;</td>
			</tr>
	';
	
	foreach($userRows as $userRow) {
		$content .= '
			<tr>
				<td class="col-150"><a href="javascript: void(2);" onclick="user.updateUser(' . $userRow['UserID'] . ')">Update User</a></td>
				<td class="col-200"><input type="text" name="UserFirstName-' . $userRow['UserID'] . '" value="' . $userRow['UserFirstName'] . '"></td>
				<td class="col-200"><input type="text" name="UserLastName-' . $userRow['UserID'] . '" value="' . $userRow['UserLastName'] . '"></td>
				<td class="col-300"><input type="text" name="UserEmail-' . $userRow['UserID'] . '" value="' . $userRow['UserEmail'] . '"></td>
				<td class="col-100"><input type="password" name="UserPassWord-' . $userRow['UserID'] . '" value=""></td>
				<td class="col-150"><a href="javascript: void(2);">Delete User</a></td>
			</tr>
		';
	}
	
	
	
	
	$content .= '
		</table>
	';

	$returnVal['status'] = AJAX_STATUS_OK;
	$returnVal['html'] = $content;
	echo json_encode($returnVal);
	exit();
?>