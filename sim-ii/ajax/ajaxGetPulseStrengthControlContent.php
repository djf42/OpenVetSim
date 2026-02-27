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

	// ajaxGetPulseStrengthControlContent.php: AJAX call to fetch the modal for Pulse Strength control

	// init
	require_once("../init.php");
	$returnVal = array();

	// is user logged in
	if(adminClass::isUserLoggedIn() === FALSE) {
		$returnVal['status'] = AJAX_STATUS_LOGIN_FAIL;
		echo json_encode($returnVal);
		exit();
	}
	
	$pulseStrength = dbClass::valuesFromPost('PulseStrength');
	$side = dbClass::valuesFromPost('s');
	$pulseType = dbClass::valuesFromPost('p');
	
	if($side != 'left' && $side != 'right') {
		$side = 'left';
	}
	if($pulseType != 'femoral' && $pulseType != 'dorsal') {
		$pulseType = 'femoral';
	}

	$content = '
		<h1 id="modal-title">' . ucfirst($side) . ' ' . ucfirst($pulseType) . ' Pulse Control</h1>
		<hr / class="modal-divider">
		<h2 class="modal-section-title">Pulse Strength</h2>
		<div class="control-modal-div pulse-strength">
			<p class="modal-section-title pulse-strength-label clearer">None</p>
			<p class="modal-section-title pulse-strength-label">Weak</p>
			<p class="modal-section-title pulse-strength-label">Medium</p>
			<p class="modal-section-title pulse-strength-label">Strong</p>
			<input type="radio" name="pulse-strength" class="modal-radio pulse-strength clearer" value="none">
			<input type="radio" name="pulse-strength" class="modal-radio pulse-strength" value="weak">
			<input type="radio" name="pulse-strength" class="modal-radio pulse-strength" value="medium">
			<input type="radio" name="pulse-strength" class="modal-radio pulse-strength" value="strong">
		</div>
		<hr style="display: none;" class=" clearer modal-divider" />
		<div style="display: none;" class="control-modal-div">
			<div id="volume-controls">
				<p id="volume-title" class="bold" style="margin-left: 30px;">Touch Sensitivity: <span class="volume-setting">5</span></p>
				<!-- New slider -->
				<div value="0" id="volume-slider" class="strip-value new control-slider-1 float-left heart" data-highlight="true"></div>
				<p id="sensitivity-comment">0=light touch, 100=heavy touch</p>
			</div>
		</div>
		<hr class=" clearer modal-divider" />
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