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
	
	class dbClass {		
		private static $connection = 1;
		function __construct($dbSelect = 'vet') {
		}

		
		public static function dbConnect($dbSelect) {
			return dbClass::$connection;
		}
		
		public static function dbClose($connection) {
			
		}
		
		public static function dbSelectQueryResult($queryString, $passedConnection = '', $dbSelect = DB_DEFAULT) {
			return FALSE;
		}
		
		public static function cleanMySQLInput($instring) {
			return $instring;		
		}
		
		public static function cleanOutput($instring, $mode="normal") {
			switch ($mode)
			{
				case "html":
					return stripslashes($instring);
					break;

				case "java":
					// javascript chokes on certain characters:
					$out = htmlentities(stripslashes($instring));
					$out = str_replace("\\","\\\\", $out);
					$out = str_replace("'","\'", $out);
					$out = str_replace("\n","\\n", $out);
					$out = str_replace("\r","\\r", $out);

					return $out;
					break;

				default:
				{
					$out = htmlentities(stripslashes($instring));
					//$out = str_replace("'","&apos;",$out);
					return $out;
				}
			}
		}
		
			// sanitize functions
		public static function valuesFromPost($varName) {

			if(isset($_POST[$varName])){
				$tempString =  $_POST[$varName];
			} else {
				$tempString = "";
			}
			return $tempString;
		}
		
		public static function valuesFromGet($varName) {

			if(isset($_GET[$varName])){
				$tempString = $_GET[$varName];
			} else {
				$tempString = "";
			}
			return $tempString;
		}
		
		public static function valuesFromSession($varName) {
			if(isset($_SESSION[$varName])){
				$tempString = $_SESSION[$varName];
			} else {
				$tempString = "";
			}
			return $tempString;
		}
		
		public static function isIndex($input) {
			if(is_int((int)$input) === TRUE && $input > 0) {
				return TRUE;
			} else {
				return FALSE;
			}
		} 
		
		public static function isInt($input) {
			if(filter_var($input, FILTER_VALIDATE_INT, array('options' => array('min_range' => 0))) === FALSE) {
				return FALSE;
			} else {
				return TRUE;
			}
		} 
		
	}
?>
