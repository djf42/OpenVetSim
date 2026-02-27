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
	// index.php: sim-ii entry page, user login
	
	require_once('init.php');

	define("SERVER_ACTIVE_SCENARIOS", SERVER_SCENARIOS );
	define("BROWSER_ACTIVE_SCENARIOS", BROWSER_SCENARIOS );
	
	if ( key_exists('NO_DB', $_SESSION ) )
	{
		header('location: /sim-ii/ii.php');
		exit();
	}
	else if ( ( key_exists('OS', $_SERVER) && strcmp($_SERVER['OS'], "Windows" ) == 0 ) ||
	 ( key_exists('SERVER_SOFTWARE', $_SERVER) && strncmp($_SERVER['SERVER_SOFTWARE'], "PHP ", 4 ) == 0 ) )
{
	$_SESSION['User']['UserFirstName'] = "";
	$_SESSION['User']['UserLastName'] = "";
	$_SESSION['User']['UserID'] = 1;
	$_SESSION['User']['isUserLoggedIn'] = TRUE;
	
	$_SESSION['NO_DB'] = TRUE;
	header('location: /sim-ii/ii.php');
	exit();
}
else
{
	// delete session data
	adminClass::removeUserfromSession();
	
	// check admin login
	$loginErrorFlag = 0;
	if(isset($_POST['submit'])) {
//FB::log($_POST);
		
		if ( isset($_POST['userID']) && ( $_POST['userID'] == 5 ) )
		{
			// Demo User
			$userID = '5';
			if(($userRow = adminClass::getUserRow($userID)) !== FALSE) 
			{
				adminClass::addUserToSession($userRow);
				$sessionID = session_id();
				
				// Demo Scenario Directory Support
				$dest = SERVER_DEMO_SCENARIOS.$sessionID;
				
				// Clear and refresh, if indicated
				if ( isset($_POST['clearScenarios'] ) && $_POST['clearScenarios'] == 'clear' )
				{
					$stat = @stat($dest );
					if ( $stat )
					{
						$files = new RecursiveIteratorIterator(
							new RecursiveDirectoryIterator($dest, RecursiveDirectoryIterator::SKIP_DOTS),
								RecursiveIteratorIterator::CHILD_FIRST );
						foreach ( $files as $fileinfo )
						{
							$path = $fileinfo->getRealPath();
							if ( $fileinfo->isDir() )
							{
								rmdir($path);
							}
							else
							{
								unlink($path);
							}
						}
						rmdir($dest );
					}
				}
				
				// Create a scenarios directory, if needed
				$stat = @stat($dest );
				if ( !$stat )
				{
					$sts = mkdir ($dest, 0755 );
					if ( $sts == FALSE )
					{
						$err = error_get_last();
						print_r($err );
						die('mkdir fails' );
					}
					// Copy the base scenarios to the Demo directory
					$source = SERVER_SCENARIOS;
					$iterator = new RecursiveIteratorIterator(
						new RecursiveDirectoryIterator($source, RecursiveDirectoryIterator::SKIP_DOTS),
						RecursiveIteratorIterator::SELF_FIRST);
					foreach ( $iterator as $item )
					{
						if ( $item->isDir()) 
						{
							$path = $iterator->getSubPathName();
							$pos = strpos($path, ".git");
							if ( $pos === FALSE )
							{
								$sts = @mkdir($dest . DIRECTORY_SEPARATOR . $iterator->getSubPathName());
								if ( $sts == FALSE )
								{
									echo "<pre>";
									$err = error_get_last();
									
									printf("Pos: %s\nErr: %s\nItem: %s\nDest: %s\n</pre>\n",
											$pos, 
											print_r($err, TRUE ),
											print_r($item, TRUE ),
											print_r($dest, TRUE ) );
									die('mkdir fails' );
								}
							}
						} 
						else
						{
							$path = $iterator->getSubPathName();
							$pos = strpos($path, ".git");
							if ( $pos === FALSE )
							{
								$sts = @copy($item, $dest . DIRECTORY_SEPARATOR . $path );
								if ( $sts == FALSE )
								{
									echo "<pre>";
									$err = error_get_last();
									
									printf("Pos: %s\nErr: %s\nItem: %s\nDest: %s\n</pre>\n",
											$pos, 
											print_r($err, TRUE ),
											print_r($item, TRUE ),
											print_r($dest, TRUE ) );
									die('copy fails' );
								}
							}
						}
					}
				}
				header('location: ii-demo.php');
			}
			else
			{
				adminClass::removeUserFromSession();
				$loginErrorFlag = 1;
			}
		}
		else if( isset($_POST['UserEmail']) && isset($_POST['UserPassWord']) && ($userRow = adminClass::isUserLoginValid($_POST['UserEmail'], $_POST['UserPassWord'])) !== FALSE) {
			adminClass::addUserToSession($userRow);
			header('location: ii.php');
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
			document.cookie = "TestCookie=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=;";
			document.cookie = "simIIUserID=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=;";
			$(document).ready(function() {
				var loginErrorFlag = <?php echo $loginErrorFlag; ?>;
				if(loginErrorFlag == 1) {
					$('p.error_login').toggle();
				}
				
				// focus on username
				$('input[name=UserEmail]').focus();
				/* $('#demo_submit').click(function(event){
					event.preventDefault();
					window.location.href = "ii-demo.php";
				}); */
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
			<form method="post" action="#" autocomplete="off">
				<button id="demo_submit" name="submit" class="admin-demo-btn red-button">Start Demo</button>
				<br>Clear Demo Scenario Directory <input type='checkbox' name='clearScenarios' value='clear'>
				<input type="hidden" name="userID" value="5" />
			</form>
			<div class="clearer"></div>
			</div>	
	</body>
</html>