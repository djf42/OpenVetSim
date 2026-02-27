<?php
/*
sim-player: 

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
ini_set('display_errors', 'Off');
error_reporting(E_ALL & ~E_DEPRECATED);

	// session
	session_start();
	
	if ( key_exists('NO_DB', $_SESSION ) && key_exists('User', $_SESSION ) && $_SESSION['User']['isUserLoggedIn'] == TRUE )
	{
		$noDB = $_SESSION['NO_DB'];
	}
	else
	{
		if ( key_exists('NO_DB', $_SESSION ) ||
			( key_exists('OS', $_SERVER) && strncmp($_SERVER['OS'], "Windows", 7 ) == 0  ) ||
			( key_exists('SERVER_SOFTWARE', $_SERVER) && strncmp($_SERVER['SERVER_SOFTWARE'], "PHP ", 4 ) == 0 ) ||
			( array_key_exists('REMOTE_ADDR', $_SERVER) && $_SERVER['REMOTE_ADDR'] === '127.0.0.1' ) )
		{
			$noDB = TRUE;
			$_SESSION['NO_DB'] = 1;
			
				$_SESSION['User']['UserFirstName'] = "";
				$_SESSION['User']['UserLastName'] = "";
				$_SESSION['User']['UserID'] = 1;
				$_SESSION['User']['isUserLoggedIn'] = TRUE;
			
		}
		else
		{
			$noDB = FALSE;
		}
	}
	
	// debug
	define("DEBUG", TRUE);
	if(DEBUG === TRUE) {
		define("DB_DEBUG", TRUE);
	} else {
		define("DB_DEBUG", FALSE);	
	}
	$parts = explode('/',$_SERVER['SCRIPT_NAME'], 3 );
	if ( count($parts) > 2 )
	{
		$top_dir = $parts[1];
	}
	else
	{
		$top_dir = "ii";
	}

	// server defines
	define ("DIR_SEP", "/" );
	define("SERVER_ROOT", $_SERVER['DOCUMENT_ROOT'] . DIR_SEP . $top_dir . DIR_SEP);

	// sever locations
	define("SERVER_INCLUDES", SERVER_ROOT . "includes" . DIR_SEP);
	define("SERVER_CLASSES", SERVER_INCLUDES . "classes" . DIR_SEP);
	define("SERVER_SIM_LOGS", $_SERVER['DOCUMENT_ROOT'] . DIR_SEP . "simlogs" . DIR_SEP);	
	define("SERVER_SIM_VIDEO", SERVER_SIM_LOGS . "video" . DIR_SEP);	
	define("SERVER_SCENARIOS", $_SERVER['DOCUMENT_ROOT'] . DIR_SEP . "scenarios" . DIR_SEP);	
	define("SERVER_SCENARIOS_PATIENTS", SERVER_SCENARIOS . "patients" . DIR_SEP);	
	
	// server location for ini files
//	define("SERVER_PROFILES",  SERVER_SCENARIOS . "profiles" . DIR_SEP);
	define("SERVER_PROFILES", $_SERVER['DOCUMENT_ROOT'] . DIR_SEP . "profiles" . DIR_SEP);	
	define("SERVER_VOCALS", SERVER_SCENARIOS . "vocals" . DIR_SEP);
	define("SERVER_MEDIA", SERVER_SCENARIOS . "media" . DIR_SEP);

	// browser defines
	if(isset($_SERVER['HTTPS']) == true) {
		define("HOST_PROTOCOL", "https://");
		define("PROTOCOL", "https");
	} else {
		define("HOST_PROTOCOL", "//");
		define("PROTOCOL", "http");
	}
	if ( $_SERVER['SERVER_PORT'] != 80 )
	{
		define("SERVER_FULL", $_SERVER["SERVER_NAME"] . ":" . $_SERVER['SERVER_PORT'] );
	}
	else
	{
		define("SERVER_FULL", $_SERVER["SERVER_NAME"] );
	
	}

//	define("BROWSER_HTML", SERVER_FULL . DIR_SEP .  $top_dir . DIR_SEP);
//	define("BROWSER_HTML", $_SERVER["HTTP_HOST"].DIR_SEP);
	define("BROWSER_HTML", "" );
//	define("BROWSER_ROOT", HOST_PROTOCOL . BROWSER_HTML);
	define("BROWSER_ROOT", "" );
	define("BROWSER_PROFILES_IMAGES", ".." . DIR_SEP . "scenarios" . DIR_SEP . "images" . DIR_SEP);
	define("BROWSER_CGI",  ".." . DIR_SEP . "cgi-bin" . DIR_SEP);
	define("BROWSER_SCENARIOS", ".." . DIR_SEP . "scenarios" . DIR_SEP);
	define("BROWSER_SCENARIOS_IMAGES", BROWSER_SCENARIOS . "images" . DIR_SEP);
	define("BROWSER_SCENARIOS_PATIENTS", BROWSER_SCENARIOS . "patients" . DIR_SEP);
	define("BROWSER_SCENARIOS_MEDIA", BROWSER_SCENARIOS . "media" . DIR_SEP);
	define("BROWSER_SCENARIOS_VOCALS", BROWSER_SCENARIOS . "vocals" . DIR_SEP);
	
	define("BROWSER_CSS", BROWSER_ROOT . "css" . DIR_SEP);
	define("BROWSER_IMAGES", BROWSER_ROOT . "images" . DIR_SEP);
	define("BROWSER_VOCALS",  BROWSER_ROOT . "vocals" . DIR_SEP);
	define("BROWSER_AJAX", BROWSER_ROOT . "ajax" . DIR_SEP);
	define("BROWSER_SCRIPTS", BROWSER_ROOT . "js" . DIR_SEP);
	
	define("BROWSER_VIDEO", 
				PROTOCOL . '://' . SERVER_FULL . 
				DIR_SEP . 'simlogs' . DIR_SEP . 'video' . 
				DIR_SEP
			);
	define("BROWSER_ROOT_FULL", PROTOCOL . '://' . SERVER_FULL . DIR_SEP);
	
//	define("SERVER_ADDR", $_SERVER['SERVER_ADDR'] );
	// $_SERVER['SERVER_ADDR'] not defined for PHP server, but not needed for sim-player
	if( !isset( $_SERVER['SERVER_ADDR'] ) ) {
		define("SERVER_ADDR", '' );
	} else {
		define("SERVER_ADDR", $_SERVER['SERVER_ADDR'] );
	}
	define("REMOTE_ADDR", $_SERVER['REMOTE_ADDR'] );
	
	// Default DB select
	define('DB_DEFAULT', 'vet');
	
	// pepper for password encryption
	define("PEPPER", "vetschool");
	
	// define for file transfers
	define('FILE_NO_ERROR', 0);
	define('FILE_INVALID_TYPE', 1);
	define('FILE_TRANSFER_FAIL', 2);
	define('FILE_MISSING_FILE', 3);
	define('FILE_SYSTEM_ERROR', 4);
	define('FILE_SCENARIO_ERROR', 5);
	define('FILE_EXTRACT_ZIP_OK', 6);
	define('FILE_EXTRACT_ZIP_FAIL', 7);
	define('FILE_SCENARIO_INVALID', 8);
	define('FILE_SCENARIO_DUP', 9);
	define('SHOW_SCENARIO_MANAGER', 10);
	
	// AJAX defines
	// AJAX Constants
	define('AJAX_STATUS_OK', 0);
	define('AJAX_STATUS_FAIL', 1);
	define('AJAX_STATUS_LOGIN_FAIL', 2);
	
	// version
	define('VERSION_MAJOR', 2);
	define('VERSION_MINOR', 0);
	
	// mobilized
//	define('MOBILIZED', FALSE);
	define('MOBILIZED', TRUE);
	
	// define temp directory for scenario
	define('TMP_SCENARIO_DIR', '/var/www/html/temp/');
	
	// video stuff
	define("MISSING_VIDEO", 1);
	define("VIDEO_FOUND", 0);
		
	/************************************/
	// requires for global classes
	// requires for global classes
	if ( $noDB )
	{
		require_once(SERVER_CLASSES . "adminWin.class.php");
		require_once(SERVER_CLASSES . "dbWin.class.php");
	}
	else
	{
		require_once(SERVER_CLASSES . "admin.class.php");
		require_once(SERVER_CLASSES . "db.class.php");
	}
	require_once(SERVER_CLASSES . "file.class.php");
	require_once(SERVER_CLASSES . "model.class.php");
	require_once(SERVER_CLASSES . "log.class.php");
	require_once(SERVER_CLASSES . "controls.class.php");
	require_once(SERVER_CLASSES . "scenarioXML.class.php");
	// Excel class -- optional
//	require_once(SERVER_CLASSES.'PHPExcel.php');
//	require_once(SERVER_CLASSES.'PHPExcel/IOFactory.php');

	// mail class -- optional
//	require_once(SERVER_CLASSES . "mail.class.php");

	
?>