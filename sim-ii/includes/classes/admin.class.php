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
			if ( key_exists('NO_DB', $_SESSION ) )
			{
				$row['UserFirstName'] = "";
				$row['UserLastName'] = "";
				$row['UserID'] = 1;
				$row['isUserLoggedIn'] = TRUE;
				return ( $row );
			}
			$cleanUserEmail = dbClass::cleanMySQLInput($userEmail);
			$result = dbClass::dbSelectQueryResult("
								SELECT * FROM Users
								WHERE UserEmail = '$cleanUserEmail'
								");
			if(count($result) != 1) {
				return FALSE;
			} else {
				$row = $result[0];
				$passWordHash = hash('sha256', $passWord . $row['UserSalt'] . PEPPER);
				if($passWordHash === $row['UserPassWord']) {
					dbClass::dbUpdateQueryResult("UPDATE Users SET UserLastLogin = NOW() WHERE UserEmail = '$cleanUserEmail'");
					return $row;
				} else {
					return FALSE;
				}
			}
		}
		
		// get users rows
		static public function getAllUserRows() {
			if ( key_exists('NO_DB', $_SESSION ) )
			{
				$row['UserFirstName'] = "";
				$row['UserLastName'] = "";
				$row['UserID'] = 1;
				$row['isUserLoggedIn'] = TRUE;
				$rows[0] = $row;
				return ( $rows );
			}
			$rows = dbClass::dbSelectQueryResult("
								SELECT * FROM Users
								ORDER BY UserLastName ASC
								");
			return $rows;
		}
		
		// get users row
		static public function getUserRow($userID) {
			if ( key_exists('NO_DB', $_SESSION ) )
			{
				$row['UserFirstName'] = "";
				$row['UserLastName'] = "";
				$row['UserID'] = 1;
				$row['isUserLoggedIn'] = TRUE;
				return ( $row );
			}
			if(dbClass::isIndex($userID) === FALSE) {
				return FALSE;
			}
			
			$rows = dbClass::dbSelectQueryResult("
								SELECT * FROM Users
								WHERE UserID = $userID
								");
			if(count($rows) != 1) {
				return FALSE;
			}
			return $rows[0];
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
				if ( key_exists('NO_DB', $_SESSION ) )
				{
					return "";
				}
				$cleanUserID = $_SESSION['User']['UserID'];
				
				$result = dbClass::dbSelectQueryResult("
									SELECT * FROM Users 
									WHERE UserID = '$cleanUserID'
									");
				if(count($result) != 1) {
					return FALSE;
				} else {
					return $_SESSION['User']['UserFirstName'] . " " . $_SESSION['User']['UserLastName'];
				}
				
			} else {
				return FALSE;
			}
		}
		
		static public function getUserRowFromSession() {
			if ( key_exists('NO_DB', $_SESSION ) )
			{
				$row['UserFirstName'] = "";
				$row['UserLastName'] = "";
				$row['UserID'] = 1;
				$row['isUserLoggedIn'] = TRUE;
				return ( $row );
			}
			if(isset($_SESSION['User']['UserID']) === TRUE && dbClass::isIndex($_SESSION['User']['UserID']) === TRUE) {
				$cleanUserID = $_SESSION['User']['UserID'];
				
				$row = dbClass::dbSelectQueryResult("
									SELECT * FROM Users 
									WHERE UserID = '$cleanUserID'
									");
				if(count($row) != 1) {
					return FALSE;
				} else {
					return $row[0];
				}
			} else {
				return FALSE;
			}
		}
			
	}
?>