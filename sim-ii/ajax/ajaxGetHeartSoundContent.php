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

	// ajaxGetHeartSoundContent.php: AJAX call to fetch the modal for Heart Sound control

	// init
	require_once("../init.php");
	$returnVal = array();

	// is user logged in
	if(adminClass::isUserLoggedIn() === FALSE) {
		$returnVal['status'] = AJAX_STATUS_LOGIN_FAIL;
		echo json_encode($returnVal);
		exit();
	}
	
	$soundName = dbClass::valuesFromPost('soundName');
	if($soundName == '') {
		$soundName = "normal";
	}
	
	$heartSoundArray = array(
		"normal" => "Normal",
		"systolic_murmur" => "Systolic Murmur",
		"pansystolic_murmur" => "Pansystolic Murmur",
		"holosystolic_murmur" => "Holoystolic Murmur",
		"continuous_murmur" => "Continuous Murmur",
		"diastolic_murmur" => "Diastolic Murmur",
		"gallop" => "Gallop"
	);
	
	// lung sound options
	$heartSoundOption = '
		<select id="sound-select" class="modal-select">
	';
	foreach($heartSoundArray as $index => $value) {
		$select = ($soundName == $index) ? ' selected="selected" ' : '';
		
		// is this an unsupported sound
		if( $index == "normal" || $index == "systolic_murmur" ) {
			$disabled = '';
		} else {
			$disabled = 'disabled';
		}

		$heartSoundOption .= '
			<option value="' . $index . '"' . $select . ' ' .  $disabled . '>' . $value . '</option>
		';
	}
	
	$heartSoundOption .= '
		</select>
	';
	
	$content = '
		<h1 id="modal-title">Heart Sound Control</h1>
		<hr / class="modal-divider">
		<h2 class="modal-section-title">Heart Sound Control</h2>
		<div class="control-modal-div">
			' . $heartSoundOption . '
		</div>
		<hr / class="modal-divider">
		<div class="control-modal-div">
			<div id="volume-controls">
				<!-- <p id="mute-volume-title" class="clearer">Mute</p> -->
				<p id="volume-title">- Volume + (<span class="volume-setting">5</span>)</p>
				<!-- <input type="checkbox" id="mute-volume" class="clearer"> -->

				<!-- Old slider
				<div id="volume-slider"></div> -->
				
				<!-- New slider -->
				<div value="0" id="volume-slider" class="strip-value new control-slider-1 float-left heart" data-highlight="true"></div>	
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