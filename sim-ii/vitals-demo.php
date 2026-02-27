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

	// vitals-demo.php: Demo version of the Student Vitals Display
	
	require_once('init.php');
	
	$userRow = adminClass::getUserRowFromSession();
	$userName = $userRow['UserFirstName'] . " " . $userRow['UserLastName'];
	$uid = $userRow['UserID'];
	$sessionID = session_id();
	
	$userRow = adminClass::getUserRowFromSession();
	if ( $userRow )
	{
		$userName = $userRow['UserFirstName'] . " " . $userRow['UserLastName'];
		$uid = $userRow['UserID'];
	}
	else
	{
		$userName = "";
		$uid = 0;
	}
	$sessionID = session_id();
	
	// If Demo user, then we use a temporary directory for Scenarios
	// Otherwise, the standard directory
	if ( $uid == 5 )
	{
		define("SERVER_ACTIVE_SCENARIOS", SERVER_DEMO_SCENARIOS . $sessionID . DIRECTORY_SEPARATOR);
		define("BROWSER_ACTIVE_SCENARIOS",BROWSER_DEMO_SCENARIOS . $sessionID . DIRECTORY_SEPARATOR);

	}
	else
	{
		define("SERVER_ACTIVE_SCENARIOS", SERVER_SCENARIOS );
		define("BROWSER_ACTIVE_SCENARIOS", BROWSER_SCENARIOS );
	}
?>
<!DOCTYPE html >
<html>
  <head>
    <?php require_once(SERVER_INCLUDES . "header.php"); ?>
	<script type="text/javascript" src="<?= BROWSER_SCRIPTS; ?>demo.js?v=<?= $ts ?>"></script>
		<script type="text/javascript">
			var userID = <?= $uid ?>;
			document.cookie = "userID="+userID+"; path=/";
			var windowScaleFactor = 1;
			
			function doWindowScale() {			
				// Resize Chart based on Window Size
				// Chart is 650 x 400 with 11px on left and 1px left, right and bottom
				var Wwidth = $(window).width();
				var Wheight = $(window).height();
				console.log("Screen size ", Wwidth, Wheight );
				// Calculate max scale for width and height
				var zoomW = ( ( Wwidth - 40 ) / (650) ) * 100;
				var zoomH = ( ( Wheight - 40 ) / (400) ) * 100;
				var zoomSet;
				
				if ( zoomW > zoomH )
					zoomSet = zoomH;
				else
					zoomSet = zoomW;
				
				windowScaleFactor = zoomSet / 100;
				$('#vsm').css({ 
					'transform'                : 'scale('+windowScaleFactor+')',
					'transform-origin'         : '0 0',
					'-moz-transform-origin'    : '0 0',         // Firefox
					'-ms-transform-origin'     : '0 0',         // IE
					'-webkit-transform-origin' : '0 0',         // Opera/Safari
					'-moz-transform'           : 'scale('+windowScaleFactor+')', // Firefox
					'-ms-transform'            : 'scale('+windowScaleFactor+')', // IE
					'-webkit-transform'        : 'scale('+windowScaleFactor+')'  // Opera/Safari
				});
				console.log("Transform: "+windowScaleFactor );
			}
			$( window ).resize(function() {
				doWindowScale();
			});
			$(document).ready(function() {
			
				// Initialize the demo
				simDemo.init();
				
				// init profile data
				scenario.loadScenario();
				profile.isVitalsMonitor = true;	// Student Display Flag
				profile.init();

				chart.init();
				controls.heartRate.init();
				controls.awRR.init();
				controls.SpO2.init();
				controls.etCO2.init();
				controls.Tperi.init();
				controls.nbp.init();
				//buttons.init();
				//events.init();
				scenario.init();
				//media.init();
				//log.init();
				
				// init patient info
				profile.initPatientInfo();
				
				simmgr.init();
			});

		</script>
<style type="text/css" media="screen">
  * {
    margin: 0px 0px 0px 0px;
    padding: 0px 0px 0px 0px;
  }

  body, html {
    padding: 3px 3px 3px 3px;

    background-color: black;
	color: yellow;

    font-family: Verdana, sans-serif;
    font-size: 11pt;
    text-align: left;
  }
  .dbgNote {
	font-size: 9px;
  }
	</style>
	
</head>

<body>
			<div id="vsm">
				<h1><span id="startStopButton">VS</span> Monitor</h1>
				<div id="vs-left-col">
					<canvas id="vs-trace-1" class="vs-trace" width="500" height="125" onclick="modal.heartRhythm(); return false;"></canvas>
					<canvas id="vs-trace-2" class="vs-trace" width="500" height="125" onclick="modal.respRhythm(); return false;"></canvas>
				</div>
				<div id="vs-right-col">
					<div class="vs-controls clearer">
						<a href="javascript: void(0)" onclick="modal.heartRhythm(); return false;"class="strip-label color-green">HR</a>
					</div>
					<div id="vs-heartRhythm" class="vs-controls">
						<a href="javascript: void(0)" onclick="modal.heartRate(); return false;" class="display-rate color-green">70</a>
					</div>
					<div class="vs-controls clearer">
						<a href="javascript: void(0)" class="strip-label color-white">ETCO<sub>2</sub></a>
					</div>
					<div id="vs-etCO2" class="vs-controls">
						<a href="javascript: void(0)" onclick="modal.etCO2(); return false;" class="display-rate color-white">75</a>
					</div>
				</div>
				<div class="wide-col">
					<div class="alt-control control-Tperi">
						<a class="alt-control-title color-blue" href="javascript: void(0)" onclick="modal.Tperi(); return false;">Temp</a>
						<a class="alt-control-rate color-blue" href="javascript: void(0)" onclick="modal.Tperi(); return false;" id="display-Tperi">123</a>
					</div>
					<div class="alt-control awRR">
						<a class="alt-control-title color-white" href="javascript: void(0)" onclick="modal.awRR(); return false;">awRR</a>
						<a class="alt-control-rate color-white" href="javascript: void(0)" onclick="modal.awRR(); return false;">123</a>
					</div>
					<div class="alt-control with-sub SpO2">
						<a class="alt-control-title color-yellow" href="javascript: void(0)" onclick="modal.SpO2(); return false;">SpO<sub>2<sub></a>
						<a id="display-SpO2" class="alt-control-rate color-yellow" href="javascript: void(0)" onclick="modal.SpO2(); return false;">123</a>
					</div>
					<div id="vs-nbp" class="alt-control">
						<a class="alt-control-title color-red" href="javascript: void(0)" onclick="modal.nbp(); return false;">NIBP</a>
						<p id="nibp-read-in-progress">Read in progress</p>
						<a id="display-nbp" class="alt-control-rate nbip color-red" href="javascript: void(0)" onclick="modal.nbp(); return false;"><span id="displayed-systolic">140</span>/<span id="displayed-diastolic">75</span> (<span id="displayed-meanNBP">80</span>) <span class="nbip-label">mmHg</span></a>
						<a id="display-nbp-hr" class="alt-control-rate color-red" href="javascript: void(0)" onclick="modal.nbp(); return false;"><span style="font-size: 24px;">PR</span> <span id="displayed-reportedHR">75</span></a>
					</div>
				</div>
			</div>
</body>
</html>