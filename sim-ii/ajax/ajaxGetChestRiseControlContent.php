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

	// ajaxGetChestRiseControlContent.php: AJAX call to fetch the modal for Chest Rise controls


	// init
	require_once("../init.php");
	$returnVal = array();

	// is user logged in
	if(adminClass::isUserLoggedIn() === FALSE) {
		$returnVal['status'] = AJAX_STATUS_LOGIN_FAIL;
		echo json_encode($returnVal);
		exit();
	}
	
	$title = dbClass::valuesFromPost('ModalTitle');
	$controlTitle = dbClass::valuesFromPost('ControlTitle');
	$chestRiseActive = dbClass::valuesFromPost('ChestRise');
	$chestRiseOffSelect = ($chestRiseActive == 'false') ? ' selected="selected" ' : '';
	$chestRiseOnSelect = ($chestRiseActive == 'true') ? ' selected="selected" ' : '';
	$content = '
		<h1 id="modal-title">' . $title . '</h1>
		<hr / class="modal-divider">
		<h2 class="modal-section-title">' . $controlTitle . '</h2>
		<div class="control-modal-div">
			<select class="chest-rise modal-select">
				<option value="false"' . $chestRiseOffSelect . '>Chest Rise Off</option>
				<option value="true"' . $chestRiseOnSelect . '>Chest Rise On</option>
			</select>
		</div>
		<hr / class="modal-divider">
		<div class="control-modal-div">
			<button class="red-button modal-button apply">Apply</button>
			<button class="red-button modal-button cancel">Cancel</button>
		</div>
	';


	$returnVal['status'] = AJAX_STATUS_OK;
	$returnVal['html'] = $content;
	echo json_encode($returnVal);
	exit();


?>