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

		// Human-readable reason for the most recent failure. Callers that get
		// FALSE back can read this to report a specific cause to the user.
		public static $lastError = '';

		function __construct() {
		}

		static public function getScenarioArray($fileName) {
			self::$lastError = '';
			$filePath = SERVER_ACTIVE_SCENARIOS . $fileName . ".xml";
			if(file_exists($filePath) === FALSE) {
				self::$lastError = "Scenario file not found: " . basename($filePath);
				return FALSE;
			}
			libxml_use_internal_errors(true);
			libxml_clear_errors();
			$simpleXMLObj = simplexml_load_file($filePath);
			if($simpleXMLObj === FALSE) {
				$errors = libxml_get_errors();
				libxml_clear_errors();
				if(!empty($errors)) {
					$e = $errors[0];
					self::$lastError = sprintf(
						"XML parse error near line %d, column %d: %s",
						$e->line, $e->column, trim($e->message)
					);
				} else {
					self::$lastError = "The scenario XML could not be parsed.";
				}
				return FALSE;
			}
			return json_decode(json_encode($simpleXMLObj), TRUE);
		}
		
		static public function getScenarioProfileArray($fileName) {
			$scenarioArray = self::getScenarioArray($fileName);
			if($scenarioArray === FALSE) {
				return FALSE;
			}
			if(!isset($scenarioArray['profile']) || count($scenarioArray['profile']) == 0) {
				self::$lastError = "The scenario is missing its <profile> section (patient information).";
				return FALSE;
			}
			return $scenarioArray['profile'];
		}

		static public function getScenarioHeaderArray($fileName) {
			$scenarioArray = self::getScenarioArray($fileName);
			if($scenarioArray === FALSE) {
				return FALSE;
			}
			if(!isset($scenarioArray['header']) || count($scenarioArray['header']) == 0) {
				self::$lastError = "The scenario is missing its <header> section.";
				return FALSE;
			}
			return $scenarioArray['header'];
		}

		static public function getScenarioEventsArray($fileName) {
			$scenarioArray = self::getScenarioArray($fileName);
			if($scenarioArray === FALSE) {
				return FALSE;
			}
			if(!isset($scenarioArray['events']) || count($scenarioArray['events']) == 0) {
				self::$lastError = "The scenario is missing its <events> section.";
				return FALSE;
			}
			return $scenarioArray['events'];
		}
		
		static public function getScenarioMediaArray($fileName) {
			$scenarioArray = self::getScenarioArray($fileName);
			if($scenarioArray === FALSE) {
				return FALSE;
			}
			// media is optional; an absent or empty section is valid
			if(!isset($scenarioArray['media']) || !is_array($scenarioArray['media']) || count($scenarioArray['media']) == 0) {
				return array('file' => array());
			}
			return self::normalizeFileList($scenarioArray['media']);
		}

		static public function getScenarioVocalsArray($fileName) {
			$scenarioArray = self::getScenarioArray($fileName);
			if($scenarioArray === FALSE) {
				return FALSE;
			}
			// vocals is optional; an absent or empty section is valid
			if(!isset($scenarioArray['vocals']) || !is_array($scenarioArray['vocals']) || count($scenarioArray['vocals']) == 0) {
				return array('file' => array());
			}
			return self::normalizeFileList($scenarioArray['vocals']);
		}

		// Ensure a media/vocals section's 'file' entry is always an indexed
		// array, so a single <file> and multiple <file> elements look the same
		// to callers. A section with no <file> becomes an empty list.
		static private function normalizeFileList($section) {
			if(!isset($section['file']) || !is_array($section['file'])) {
				$section['file'] = array();
			} else if(!array_key_exists('0', $section['file'])) {
				$tmp = $section['file'];
				unset($section['file']);
				$section['file'][0] = $tmp;
			}
			return $section;
		}
		
		static public function getScenarioSoundtagsArray($fileName) {
			$scenarioArray = self::getScenarioArray($fileName);
			if($scenarioArray === FALSE || array_key_exists('soundtags', $scenarioArray) === FALSE || count($scenarioArray['soundtags']) == 0) {
				return FALSE;
			} else {
				return $scenarioArray['soundtags'];
			}
		}

		static public function getScenarioTelesimArray($fileName) {
			$scenarioArray = self::getScenarioArray($fileName);
			if($scenarioArray === FALSE || array_key_exists('telesim', $scenarioArray) === FALSE || count($scenarioArray['telesim']) == 0) {
				return FALSE;
			} else {
				return $scenarioArray['telesim'];
			}
		}
	}
?>