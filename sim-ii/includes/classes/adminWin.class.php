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

// Dummy admin class, used for Windows 10 Support
	class adminClass {
				
		function __construct() {

		}
		
		// salt for encoding passwords
		public static function generateSalt() {
			$salt = "";
			for ($i = 0; $i < 60; $i++) {
				$salt .= substr(".?ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", mt_rand(0, 63), 1);
			}
			return $salt;
		}
				
		// get admin record for login
		static public function isUserLoginValid($userEmail, $passWord) {
			return ( TRUE );
		}
		
		// get users rows
		static public function getAllUserRows() {
			$rows[0] = adminClass::getUserRow(1 );
			return $rows;
		}
		
		// get users row
		static public function getUserRow($userID) {
			if ( $userID == 1 )
			{
				$row['UserID'] = 1;
				$row['UserFirstName'] = "";
				$row['UserLastName'] = "";
				$row['UserEmail'] = "";
				$row['UserPassWord'] = "***";
				$row['UserSalt'] = "***";
				$row['UserLastLogin'] = date("r");
				return $row;
			}
			else
			{
				return FALSE;
			}
		}
		
		// is admin logged in
		static public function isUserLoggedIn() {
			if(isset($_SESSION['User']['isUserLoggedIn']) == TRUE && $_SESSION['User']['isUserLoggedIn'] == TRUE && self::getUserNameFromSession() !== FALSE) {
				return TRUE;
			} else {
				return FALSE;
			}
		}
		
		// admin session variables
		static public function addUserToSession($userRow) {
			$_SESSION['User']['UserFirstName'] = $userRow['UserFirstName'];
			$_SESSION['User']['UserLastName'] = $userRow['UserLastName'];
			$_SESSION['User']['UserID'] = $userRow['UserID'];
			$_SESSION['User']['isUserLoggedIn'] = TRUE;
			return;
		}
		
		static public function removeUserFromSession() {
			unset($_SESSION['User']);

			return;
		}
		
		static public function getUserNameFromSession() {
			if(isset($_SESSION['User']['UserID']) === TRUE && dbClass::isIndex($_SESSION['User']['UserID']) === TRUE) {
				return ( "" );
			} else {
				return FALSE;
			}
		}
		
		static public function getUserRowFromSession() {
			return ( adminClass::getUserRow(1 ) );
		}
	}
?>