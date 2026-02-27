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

	if ( ! $noDB )
	{
		$status = adminClass::isUserLoggedIn();
		if($status === FALSE) {
			header('location: index.php');
		}
	}
	$userRow = adminClass::getUserRowFromSession();	
	$userName = $userRow['UserFirstName'] . " " . $userRow['UserLastName'];	
?>
<!DOCTYPE html>
<html>
	<head>
		<?php require_once(SERVER_INCLUDES . "header.php"); ?>
		
		<script type="text/javascript">
			$(document).ready(function() {
				// init menu				
				menu.init();
				// show log files
				modal.showLogFiles();
				
				$('#admin-nav').css({
					'width': '800px',
					'margin-bottom': '20px'
				});
				$('.profile-display.scenario').css('width', '340px');
				
				$('#sitewrapper').css('margin', '0');
				var wWidth = $('body').width();
					$('#event-log').width(wWidth - 75);
				$( window ).resize(function() {
					var wWidth = $('body').width();
					$('#event-log').width(wWidth - 75);
				});
				
			});
		</script>
	</head>
	<body>
		<div id="sitewrapper">
			<div id="admin-nav">
				<h1>Open VetSim Debrief Viewer - V<?= VERSION_MAJOR . '.' . VERSION_MINOR; ?></h1>
				<h1 class="welcome-title">Welcome <?= $userName; ?></h1>
				<div class="profile-display scenario">
					<!-- Log File Name: -->
					<span id="scenario-name-display"></span>
				</div>
				<ul id="main-nav">
					<!-- <li class="with-sub-nav">
						<a href="javascript:void(2);">File</a>
						<ul class="sub-nav">
							<li><a href="javascript: void(2);">File Another Item</a></li>
							<li><a href="javascript: void(2);">File Another Item</a></li>
							<li><a href="javascript: void(2);">File Another Item</a></li>
							<li><a href="javascript: void(2);">File Another Item</a></li>
						</ul>
					</li>
					<li class="with-sub-nav">
						<a href="javascript:void(2);">Settings</a>
						<ul class="sub-nav">
							<li><a href="javascript: void(2);">Settings Another Item</a></li>
							<li><a href="javascript: void(2);">Settings Another Item</a></li>
							<li><a href="javascript: void(2);">Settings Another Item</a></li>
							<li><a href="javascript: void(2);">Settings Another Item</a></li>
						</ul>
					</li> -->

					<li>
						<a href="javascript:void(2);" onclick="modal.showLogFiles();">Log Files</a>
					</li>
					<li class="debrief">
						<a href="/sim-ii/ii.php" class="event-link">Instructor Interface</a>						
					</li>

					<li class="logout">
						<a href="index.php" class="event-link">Logout</a>						
					</li>
				</ul>
			</div>
			
			<div id="event-log"></div>
			<div id="current-time" style="clear: both; float: left;"></div>
	
			<div class="clearer"></div>
			<a id="play-test" href="javascript: void(2)"><img src="./images/play.png" width="40"></a>
		</div> <!-- sitewrapper -->
		
		<!-- Modal -->
		<div id="modal">
			<div class="container">
				<div id="modal-content">
				</div>
				<a class="close_modal" href="javascript: void(2);">
					<img src="<?= BROWSER_IMAGES; ?>x.png" alt="Close Modal">
				</a>
			</div>
		</div>
	</body>
</html>