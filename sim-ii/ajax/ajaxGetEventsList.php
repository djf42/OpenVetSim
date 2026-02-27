<?php
ini_set('display_errors', 'Off');
error_reporting(E_ALL & ~E_DEPRECATED);

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

	// ajaxGetEventsList.php: AJAX call to fetch the Events List for a scenario

	// init
	require_once("../init.php");
	$returnVal = array();

	// is user logged in
	if(adminClass::isUserLoggedIn() === FALSE) {
		$returnVal['status'] = AJAX_STATUS_LOGIN_FAIL;
		echo json_encode($returnVal);
		exit();
	}
	
	// get profile info
	$events = dbClass::valuesFromPost('events');
	if( ! $events ) {
		$returnVal['status'] = AJAX_STATUS_FAIL;
		$returnVal['reason'] = "missing arg";
		echo json_encode($returnVal);
		exit();		
	}
	$eventsArray = json_decode(str_replace("\\", "", $events), TRUE);
	if( ! $events ) {
		$returnVal['status'] = AJAX_STATUS_FAIL;
		$returnVal['reason'] = "no array returned";
		echo json_encode($returnVal);
		exit();		
	}	
	if(  ! is_array($eventsArray) ) 
	{
		$returnVal['status'] = AJAX_STATUS_FAIL;
		$returnVal['reason'] = "eventsArray is Type".gettype($eventsArray);
		echo json_encode($returnVal);
		exit();		
	}
	if(  count($eventsArray) < 1 ) 
	{
		$returnVal['status'] = AJAX_STATUS_FAIL;
		$returnVal['reason'] = "eventsArray count is ".count($eventsArray);
		echo json_encode($returnVal);
		exit();		
	}		
	$content = '';
	$priority = array();
	$hotkeys = array();
	
	// flag for hot link duplication (b) or (c)
	$returnVal['hotLinkDup'] = 'false';
		
	// Normalise: single <category> collapses to assoc array; wrap so foreach always sees indexed array
	if (!isset($eventsArray['category'][0]) || !is_array($eventsArray['category'][0])) {
		$eventsArray['category'] = [ $eventsArray['category'] ];
	}

	// get elements of event library
	foreach($eventsArray['category'] as $eventCategory) {
		$content .= '
				<div class="event-library-col">
					<h2>' . $eventCategory['title'] . '</h2>
					<ul class="event-primary">
		';
		
		// get next level
		if(count($eventCategory) == 0) {
			$returnVal['status'] = AJAX_STATUS_FAIL;
			echo json_encode($returnVal);
			exit();		
		}
		foreach($eventCategory as $eventKey => $eventArray) {
			if($eventKey != 'event') {
				continue;
			}

			// iterate over events
			if(count($eventArray) == 0) {
				$returnVal['status'] = AJAX_STATUS_FAIL;
				echo json_encode($returnVal);
				exit();					
			}

			// HACK...need elements to be an array
			if(isset($eventArray[0]) === FALSE || is_array($eventArray[0]) === FALSE) {
				$eventArray[0] = array(
					'title' => $eventArray['title'],
					'id' => $eventArray['id'],
					'hotkey' => (isset($eventArray['hotkey']) ? $eventArray['hotkey'] : ''),
					'priority' => $eventArray['priority']
				);
				unset($eventArray['title']);
				unset($eventArray['id']);
				unset($eventArray['priority']);
				unset($eventArray['hotkey']);
			}

			foreach($eventArray as $event) {
				// are we trying to redefine the reserved hot links
				 if (isset($event['hotkey']) && strlen($event['hotkey']) == 0 )
				{
					unset($event['hotkey'] );
				}
				
				if( isset($event['hotkey']) && !checkHotkey($event['hotkey']) ) {
						$returnVal['hotLinkDup'] = 'true';
				}
				
				// Make sure the hotkey is valid. 
				if ( ! isset($event['hotkey']) || ! checkHotkey($event['hotkey'] ) )
				{
					$event['hotkey'] = '';
				}
				$content .= '
						<li>
							<img class="event-check primary" src="' . BROWSER_IMAGES . 'check.png" alt="event checkmark">
							<a data-category="' . $eventCategory['title'] . '" data-category-id="' . $eventCategory['name'] . '" data-event-id="' . $event['id'] . '" href="javascript: void(2);" onclick="events.sendEventLibraryClick(this);">' . $event['title'] . '</a>';
				if ( strlen($event['hotkey'] ) == 1 )
				{
					$content .= " (".$event['hotkey'].")";
					$hotkeys[] = array ( 'title' => $event['title'], 'hotkey' => $event['hotkey'], 'id' => $event['id'] );
				}
				$content .= '
						</li>					
				';
				
				if(isset($event['priority']) === TRUE && $event['priority'] == 1) {
					$priority[] = array(
						'title' => $event['title'],
						'id' => $event['id'],
						'hotkey' => $event['hotkey'],
					);
				}
			}
		}
		
		$content .= '
					</ul>
				</div>		
		';
	}

	$returnVal['status'] = AJAX_STATUS_OK;
	$returnVal['html'] = $content;
	$returnVal['priority'] = $priority;
	$returnVal['hotkeys'] = $hotkeys;
	echo json_encode($returnVal);
	exit();
	

	// Hotkey must be a single character and not 'b' or 'c'. These are reserved.
	function checkHotkey($key )
	{	
		if ( strlen($key ) == 1 && preg_match('/[^bc]/', $key) )
		{
			return ( TRUE );
		}
		return ( FALSE );
	}
?>