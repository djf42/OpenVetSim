<?php
/*
sim-ii: 

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
	class scenarioXML {
	
		function __construct() {
		}
				
		static public function getScenarioArray($fileName) {
			$filePath = SERVER_ACTIVE_SCENARIOS . $fileName . ".xml";
			if(file_exists($filePath) === FALSE) {
				return FALSE;
			} else {
				libxml_use_internal_errors(true);
				$simpleXMLObj = simplexml_load_file($filePath);
				return json_decode(json_encode($simpleXMLObj), TRUE);
			}
		}
		
		static public function getScenarioProfileArray($fileName) {
			$scenarioArray = self::getScenarioArray($fileName);
			if($scenarioArray === FALSE || count($scenarioArray['profile']) == 0) {
				return FALSE;
			} else {
				return $scenarioArray['profile'];
			}
		}
		
		static public function getScenarioHeaderArray($fileName) {
			$scenarioArray = self::getScenarioArray($fileName);
			if($scenarioArray === FALSE || count($scenarioArray['header']) == 0) {
				return FALSE;
			} else {
				return $scenarioArray['header'];
				}
		}

		static public function getScenarioEventsArray($fileName) {
			$scenarioArray = self::getScenarioArray($fileName);
			if($scenarioArray === FALSE || count($scenarioArray['events']) == 0) {
				return FALSE;
			} else {
				return $scenarioArray['events'];
			}
		}
		
		static public function getScenarioMediaArray($fileName) {
			$scenarioArray = self::getScenarioArray($fileName);
			if($scenarioArray === FALSE || count($scenarioArray['media']) == 0) {
				return FALSE;
			} else if(!array_key_exists('0', $scenarioArray['media']['file']) === TRUE) {
				$tmp = $scenarioArray['media']['file'];
				unset($scenarioArray['media']['file']);
				$scenarioArray['media']['file'][0] = $tmp;
			}
			return $scenarioArray['media'];
		}
		
		static public function getScenarioVocalsArray($fileName) {
			$scenarioArray = self::getScenarioArray($fileName);
			if($scenarioArray === FALSE || count($scenarioArray['vocals']) == 0) {
				return FALSE;
			} else if(!array_key_exists('0', $scenarioArray['vocals']['file']) === TRUE) {
				$tmp = $scenarioArray['vocals']['file'];
				unset($scenarioArray['vocals']['file']);
				$scenarioArray['vocals']['file'][0] = $tmp;
			}
			return $scenarioArray['vocals'];
		}
		
		static public function getScenarioSoundtagsArray($fileName) {
			$scenarioArray = self::getScenarioArray($fileName);
			if(array_key_exists('soundtags', $scenarioArray) === FALSE || $scenarioArray === FALSE || count($scenarioArray['soundtags']) == 0) {
				return FALSE;
			} else {
				return $scenarioArray['soundtags'];
			}
		}
		
		static public function getScenarioTelesimArray($fileName) {
			$scenarioArray = self::getScenarioArray($fileName);
			if(array_key_exists('telesim', $scenarioArray) === FALSE || $scenarioArray === FALSE || count($scenarioArray['telesim']) == 0) {
				return FALSE;
			} else {
				return $scenarioArray['telesim'];
			}
		}
	}
?>