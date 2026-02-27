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
	require_once('init.php');
	if ( key_exists('NO_DB', $_SESSION ) ) {
		header('location: /sim-player/player.php');
		exit();
	} else if ( ( key_exists('OS', $_SERVER) && strcmp($_SERVER['OS'], "Windows" ) == 0 ) ||
	 ( key_exists('SERVER_SOFTWARE', $_SERVER) && strncmp($_SERVER['SERVER_SOFTWARE'], "PHP ", 4 ) == 0 ) ) {
		$_SESSION['User']['UserFirstName'] = "";
		$_SESSION['User']['UserLastName'] = "";
		$_SESSION['User']['UserID'] = 1;
		$_SESSION['User']['isUserLoggedIn'] = TRUE;
		
		$_SESSION['NO_DB'] = TRUE;
		header('location: /sim-player/player.php');
		exit();
	} else {
		// delete session data
		adminClass::removeUserfromSession();

		// check admin login
		$loginErrorFlag = 0;

		if(isset($_POST['submit'])) {
	//FB::log($_POST);

			if(($userRow = adminClass::isUserLoginValid($_POST['UserEmail'], $_POST['UserPassWord'])) !== FALSE) {
				adminClass::addUserToSession($userRow);
				header('location: player.php');
			} else {
				adminClass::removeUserFromSession();
				$loginErrorFlag = 1;
			}
		}
	}
?>
<!DOCTYPE html>
<html>
	<head>
		<?php require_once(SERVER_INCLUDES . "header.php"); ?>
		
		<script type="text/javascript">
			$(document).ready(function() {
				var loginErrorFlag = <?php echo $loginErrorFlag; ?>;
				if(loginErrorFlag == 1) {
					$('p.error_login').toggle();
				}
				
				// focus on username
				$('input[name=UserEmail]').focus();
			});
		</script>
	</head>
	<body>
		<div id="sitewrapper">
			<div id="admin_header">
				<img src="<?php echo BROWSER_IMAGES; ?>logo-open-vetsim.gif" alt="logo" style="height: 90px;">
				<h1>Please login to Open VetSim.</h1>
			</div>
			<div class="clearer" id="admin_login">
				<form method="post" action="#" autocomplete="off">
					<fieldset>
						<label>Username:</label>
						<input type="text" name="UserEmail" />
						<label>Password:</label>
						<input type="password" name="UserPassWord" />
					</fieldset>
					<button id="login_submit" name="submit" class="admin-btn red-button">Submit</button>
					<p class="error_login">Incorrect username or password.  Please try again.</p>
				</form>
			</div>
			
			<div class="clearer"></div>
		</div>	
	</body>
</html>