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

	class logClass {
		
		function _construct() {
		}
		
		static public function openLogFile($logPath) {
			$fHandle = fopen($logPath, "a");
			if($fHandle === FALSE) {
				return FALSE;
			} else {
				return $fHandle;
			}
		}
		
		static public function closeLogFile($fHandle) {
			fclose($fHandle);
		}
		
		static public function initLogRecord($logPath) {
			$fHandle = self::openLogFile($logPath);
			if($fHandle === FALSE) {
				FB::log("Error opening log file".$logPath);
				return FALSE;
			}
			$initString = "--------\n".date("Y-m-d H:i:s")."\n";
			$status = fwrite($fHandle, $initString);
			self::closeLogFile($fHandle);
			return $status;
		}
		
		static public function addLogRecord($logPath, $logString) {
			$fHandle = self::openLogFile($logPath);
			if($fHandle === FALSE) {
				FB::log("Error opening log file".$logPath);
				return FALSE;
			}
			$status = fwrite($fHandle, $logString."\n");
			self::closeLogFile($fHandle);
			return $status;
		}
		
		static public function closeLogRecord($logPath, $logString) {
			$fHandle = self::openLogFile($logPath);
			if($fHandle === FALSE) {
				FB::log("Error opening log file".$logPath);
				return FALSE;
			}
			$status = fwrite($fHandle, $logString."\n--------\n");
			self::closeLogFile($fHandle);
			return $status;
		}

	}
?>