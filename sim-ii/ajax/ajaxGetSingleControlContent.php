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

	// ajaxGetSingleControlContent.php: AJAX call to fetch the modal for value control
	// This is used for several parameters where a single numerical value is controlled.

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
	$modalUnitsLabel = dbClass::valuesFromPost('ModalUnits');
	
	$content = '
		<h1 id="modal-title">' . $title . '</h1>
		<hr / class="modal-divider">
		<h2 class="modal-section-title">' . $controlTitle . ' (' . $modalUnitsLabel . ')</h2>
		<div class="control-modal-div">
			<p class="modal-input-label">Current</p>
			<p class="modal-input-label new-value">New</p>
			<a class="control-incr-decr-rate decr-rate clearer float-left" href="javascript: void(2);"><<</a>
			
			<input value="0" class="strip-value current float-left" readonly="readonly" disabled="disabled">
			
			<!-- old slider -->
			<!-- <div class="control-slider-1"></div>
			<input value="0" class="strip-value new"> -->
			
			<!-- New slider -->
			<div value="0" class="control-slider-1 float-left" data-highlight="true"></div>
			<input value="0" class="strip-value new">
			
			<a class="control-incr-decr-rate incr-rate float-left" href="javascript: void(2);">>></a>
			<div class="clearer"></div>
		</div>
		<hr / class="modal-divider clearer">
		<div class="control-modal-div">
			<p class="modal-section-title">Transfer Time</p>
			<select class="transfer-time modal-select">
				' . controls::getTransferDropDown() . '
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