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

	// ajaxGetLungSoundContent.php: AJAX call to fetch the modal for Lung Sound control

	// init
	require_once("../init.php");
	$returnVal = array();

	// is user logged in
	if(adminClass::isUserLoggedIn() === FALSE) {
		$returnVal['status'] = AJAX_STATUS_LOGIN_FAIL;
		echo json_encode($returnVal);
		exit();
	}
	
	$side = dbClass::valuesFromPost('side');
	if($side == '' || ($side != 'left' && $side != 'right') ) {
		$side = 'left';
	}
	
	$fileName = dbClass::valuesFromPost('fileName');
	
	$lungSoundArray = array(
		"Normal" => "normal",
		"Coarse Crackles" => "coarse_crackles",
		"Fine Crackles" => "fine_crackles",
		"Wheezes" => "wheezes",
		"Stridor" => "stridor",
		"Stertor" => "stertor"
	);
	
	// lung sound options
	$lungSoundOption = '
		<select id="sound-select" class="modal-select">
	';
	foreach($lungSoundArray as $index => $value) {
		// is this an unsupported sound
		if( $index == "Stridor" || $index == "Stertor" ) {
			$disabled = 'disabled';
		} else {
			$disabled = '';
		}
		
		$select = ($fileName == $value) ? ' selected="selected" ' : '';
		$lungSoundOption .= '
			<option value="' . $value . '"' . $select . ' ' . $disabled . '>' . $index . '</option>
		';
	}
	
	// add in extra option for left lung
	if($side == 'left') {
		$select = ($fileName == "same_as_right") ? ' selected="selected" ' : '';
		$lungSoundOption .= '
			<option value="same_as_right"' . $select . '>Same as Right Lung</option>
		';	
	}
	$lungSoundOption .= '
		</select>
	';
	
	$content = '
		<h1 id="modal-title">' . ucfirst($side) . ' Lung Sound Control</h1>
		<hr / class="modal-divider">
		<h2 class="modal-section-title">' . ucfirst($side) . ' Lung Sound Control</h2>
		<div class="control-modal-div">
			' . $lungSoundOption . '
		</div>
		<hr / class="modal-divider">
		<div class="control-modal-div">
			<div id="volume-controls">
				<!-- <p id="mute-volume-title" class="clearer">Mute</p> -->
				<p id="volume-title">- Volume + (<span class="volume-setting"></span>)</p>
				<!-- <input type="checkbox" id="mute-volume" class="clearer"> -->
				
				<!-- Old slider
				<div id="volume-slider"></div> -->
				
				<!-- New slider -->
				<div value="0" id="volume-slider" class="strip-value new control-slider-1 float-left lung" data-highlight="true"></div>		
				
			</div>
			<div class="clearer"></div>
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