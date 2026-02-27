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

// ii-demo.php: Demo version of the Instructor interface_exists

	require_once('init.php');
	
	$status = adminClass::isUserLoggedIn();
	if($status === FALSE) {
		header('location: index.php?sts='.$status );
	}
	
	$userRow = adminClass::getUserRowFromSession();
	$userName = $userRow['UserFirstName'] . " " . $userRow['UserLastName'];
	$uid = $userRow['UserID'];
	$sessionID = session_id();
	
	$uploadErrorCode = FILE_NO_ERROR;

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
	
	// are deleting a scenario?
	$deleteScenarioDir = dbClass::valuesFromGet('ddir');
	if($deleteScenarioDir != '') {
		if(fileClass::deleteDir(SERVER_ACTIVE_SCENARIOS . $deleteScenarioDir)) {
			$uploadErrorCode = SHOW_SCENARIO_MANAGER;
		} 
	}
	
	// process POST and see if we added a new scenario
	$scenarioDir = '';
	if(isset($_POST['scenario-submit']) === TRUE) {
		// check for $_FILES error codes
		$fileUploadError = $_FILES['scenario-file-select']['error'];
		if($fileUploadError == UPLOAD_ERR_OK) {
			// delete temp directory
			fileClass::deleteDir(TMP_SCENARIO_DIR);
			
			// make temp directory and unpack archive
			@mkdir(TMP_SCENARIO_DIR, 0777, TRUE );

			// attempt to extract the zip
			$scenarioZip = new ZipArchive;
			$resource = $scenarioZip->open($_FILES['scenario-file-select']['tmp_name']);
			if ($resource === TRUE) {
				$scenarioZip->extractTo(TMP_SCENARIO_DIR);
				$scenarioZip->close();
				
				fileClass::setDirModeRecursive(TMP_SCENARIO_DIR, 0777);
				$uploadErrorCode = FILE_EXTRACT_ZIP_OK;
			} else {
				$uploadErrorCode = FILE_EXTRACT_ZIP_FAIL;
				fileClass::deleteDir(TMP_SCENARIO_DIR);
			}			
		}
		
		// validate archive if extract is OK
		if($uploadErrorCode == FILE_EXTRACT_ZIP_OK) {
			if(!fileClass::validateScenarioArchive()) {
				$uploadErrorCode = FILE_SCENARIO_INVALID;				
				fileClass::deleteDir(TMP_SCENARIO_DIR);
			} else {
				// check for duplicate scenario directories
				$parts = pathinfo($_FILES['scenario-file-select']['name']);
				if(is_dir(SERVER_ACTIVE_SCENARIOS . $parts['filename'])) {
					$uploadErrorCode = FILE_SCENARIO_DUP;
					$scenarioDir = $parts['filename'];
				} else {
					// move the temp directory to a new scenario
					fileClass::copyDir(TMP_SCENARIO_DIR, SERVER_ACTIVE_SCENARIOS . $parts['filename'] . '/', 0777);
					fileClass::deleteDir(TMP_SCENARIO_DIR);					
				}
			}
		}
	}
	
	// get scenario list
	$scenarioFolderList = scandir(SERVER_ACTIVE_SCENARIOS);
	$scenarioContent = '';  
	foreach($scenarioFolderList as $key => $scenarioFolder) {
		if(is_dir(SERVER_ACTIVE_SCENARIOS . $scenarioFolder) === TRUE) {
			if( $scenarioFolder == '.' || $scenarioFolder == '..' || $scenarioFolder == '.git' ) {
				continue;
			}
			
			if(file_exists(SERVER_ACTIVE_SCENARIOS . $scenarioFolder . DIRECTORY_SEPARATOR . 'main.xml') === TRUE) {
				$scenarioHeader = scenarioXML::getScenarioHeaderArray($scenarioFolder . DIRECTORY_SEPARATOR . 'main');
				$fileName = $scenarioFolder . DIRECTORY_SEPARATOR . 'main';
				$scenarioHeaderArray = scenarioXML::getScenarioHeaderArray($fileName);
				$scenarioNameArray[$scenarioHeaderArray['title']['name']] = $scenarioFolder;
			}
		}
	}

	// sort the scenario names
	ksort($scenarioNameArray);
			
	foreach($scenarioNameArray as $scenarioName => $scenarioFolder) {
		$scenarioContent .= '
			<option value="' . $scenarioFolder . '">';
		$scenarioContent .= $scenarioName;
		$scenarioContent .= '</option>
		';
	}
?>
<!DOCTYPE html>
<html>
	<head>
		<?php require_once(SERVER_INCLUDES . "header.php"); ?>
		
		<script type="text/javascript" src="<?= BROWSER_SCRIPTS; ?>demo.js?v=<?= $ts ?>"></script>
		<script type="text/javascript">
			var uploadErrorCode = <?= $uploadErrorCode; ?>;
			var userID = <?= $uid ?>;
			document.cookie = "userID="+userID+"; path=/";
			var isVitalsMonitor = 0;	// Student Display Flag
			$(document).ready(function() {
				console.log(document.cookie );
				// hide debrief menu item
				//$('.logout.debrief').hide();
				// init menu
				menu.init();
				
				// Initialize the demo
				simDemo.init();
				hotkeys.init();
				
				// init profile data
				scenario.loadScenario();
				profile.init();
				
				chart.init();
				
				controls.heartRate.init();
				controls.awRR.init();
				controls.SpO2.init();
				controls.etCO2.init();
				controls.Tperi.init();
				controls.nbp.init();
				controls.pulse.init();
				buttons.init();
				events.init();
				scenario.init();
//				media.init();
//				log.init();

				// init patient info
				profile.initPatientInfo();
				controls.heartRhythm.calculateVPCFreq();
console.log(controls['awRR'].increment);
				// init displayed values
				$('#vs-heartRhythm p.display-rate').html(controls.heartRate.value);
				$('#vs-awRR p.display-rate').html(controls.awRR.value);
				
				simmgr.init();
				controls.cpr.init();
				controls.manualRespiration.init();
				
				if(uploadErrorCode == <?= FILE_EXTRACT_ZIP_OK; ?>) {
					$('#scenario-click').click();
				}
				
				// duplicate scenario?
				if(uploadErrorCode == <?= FILE_SCENARIO_DUP; ?>) {
					if(confirm('Duplicate scenario...overwrite?')) {
						scenario.addScenario('<?= $scenarioDir; ?>');
					}
				} else if(uploadErrorCode == <?= FILE_EXTRACT_ZIP_FAIL; ?> || uploadErrorCode == <?= FILE_SCENARIO_INVALID; ?>) {
					alert("Invalid scenario zip file.");
				}
			});
		</script>
	</head>
	<body>
		<div id="sitewrapper">
			<div id="admin-nav">
				<h1>Open VetSim Instructor Interface</h1>
				<h1 class="welcome-title">Welcome <?= $userName; ?></h1>
				<div class="profile-display scenario">
					Scenario Name: 
					<span id="scenario-name-display">Default Scenario</span>
					&nbsp;&nbsp;|&nbsp;&nbsp;Scene Name:&nbsp;<span id="scene-name">Test</span>
					&nbsp;&nbsp;|&nbsp;&nbsp;Scene ID:&nbsp;<span id="scene-id">1</span>
				</div>
				
					<div >
						<a href="vitals-demo.php" target="vitals" class="event-link">Vitals</a>						
					</div>
				<ul id="main-nav">
					<li >
						<a href="javascript:void(2);" onclick="modal.showUsers();">Users</a>
					</li>
					<li class="menu-events">
						<a href="javascript:void(2);" onclick="modal.showEvents(); return false;">Events</a>
					</li>

					<li class="logout">
						Version: <?= VERSION_MAJOR . '.' . VERSION_MINOR; ?>						
					</li>
					<li class="logout">
						<a href="index.php" class="event-link">Logout</a>						
					</li>
					<li class="logout debrief">
						<a href="/sim-player/player.php" class="event-link">Debrief</a>						
					</li>
					<li class="logout">
						<a href="javascript: void(2);" onclick="modal.manageScenarios();" class="event-link" id="scenario-click">Scenarios</a>						
					</li>
					<!-- <li class="logout">
						<a href="../../editor/editor.php"  class="event-link breath-link">Editor</a>						
					</li> -->
					<li class="logout">
						<a href="javascript: void(2);"  class="event-link cpr-link">Start Comps (c)</a>						
					</li>
					<li class="logout">
						<a href="javascript: void(2);"  class="event-link breath-link">Manual Breath (b)</a>						
					</li>
				</ul>
			</div>

			<div id="mannequin" class="clearer float-left ii-border">
				<h1>Non Vital Controls</h1>
				<h2 id="button-palpate-title" class="nvs-button"></h2>
				<img class="nvs-button" id="button-palpate" src="<?= BROWSER_IMAGES; ?>palpate.png" alt="Palpate Icon">
				<h2 id="button-cpr-title" class="nvs-button"></h2>
				<img class="nvs-button" id="button-cpr" src="<?= BROWSER_IMAGES; ?>empty.png" alt="CPR Icon">
				<h2 id="button-SpO2-title" class="nvs-button"></h2>
				<img class="nvs-button" id="button-SpO2" src="<?= BROWSER_IMAGES; ?>spo2.png" alt="SpO2 Icon">
				<h2 id="button-ekg-title" class="nvs-button"></h2>
				<img class="nvs-button" id="button-ekg" src="<?= BROWSER_IMAGES; ?>ekg.png" alt="EKG Icon">
				<h2 id="button-Tperi-title" class="nvs-button"></h2>
				<img class="nvs-button" id="button-Tperi" src="<?= BROWSER_IMAGES; ?>Tperi.png" alt="Temperature Icon">
				<h2 id="button-CO2-title" class="nvs-button"></h2>
				<img class="nvs-button" id="button-CO2" src="<?= BROWSER_IMAGES; ?>co2.png" alt="CO2 Icon">
				<h2 id="button-bpcuff-title" class="nvs-button"></h2>
				<img class="nvs-button" id="button-bpcuff" src="<?= BROWSER_IMAGES; ?>bpcuff.png" alt="bpcuff Icon">
				
				<!-- PEA and Cardiac Arrest Indicators -->
				<h2 id="indicator-pea">PEA</h2>
				<h2 id="indicator-arrest">Arrest</h2>

				<div id="vocals-dog-control" class="dog-control">
					<a id="vocals-dog-control-title" href="javascript: void(2)" onclick="modal.vocals(); return false;">Vocals</a>
					<a class="sound-mute" id="vocals-mute" href="javascript: void(2)" onclick="modal.vocals(); return false;"><img src="<?= BROWSER_IMAGES; ?>sound_mute.png"></a>
				</div>
				<div id="left-femoral-pulse-dog-control" class="dog-control">
					<a id="left-femoral-pulse-dog-control-title" href="javascript: void(2)" onclick="modal.pulseStrength('left', 'femoral'); return false;">Pulse Strength</a>
				</div>
				<div id="right-femoral-pulse-dog-control" class="dog-control">
					<a id="right-femoral-pulse-dog-control-title" href="javascript: void(2)" onclick="modal.pulseStrength('right', 'femoral'); return false;">Pulse Strength</a>
				</div>
				<div id="left-dorsal-pulse-dog-control" class="dog-control">
					<a id="left-dorsal-pulse-dog-control-title" href="javascript: void(2)" onclick="modal.pulseStrength('left', 'dorsal'); return false;">Pulse Strength</a>
				</div>
				<div id="right-dorsal-pulse-dog-control" class="dog-control">
					<a id="right-dorsal-pulse-dog-control-title" href="javascript: void(2)" onclick="modal.pulseStrength('right', 'dorsal'); return false;">Pulse Strength</a>
				</div>
				<div id="left-lung-dog-control" class="dog-control">
					<a id="left-lung-dog-control-title" href="javascript: void(2)" onclick="modal.leftLung(); return false;">Left Lung</a>
					<a class="sound-mute" id="left-lung-mute" href="javascript: void(2)" onclick="modal.leftLung(); return false;"><img src="<?= BROWSER_IMAGES; ?>sound_mute.png"></a>
				</div>
				<div id="right-lung-dog-control" class="dog-control">
					<a id="right-lung-dog-control-title" href="javascript: void(2)" onclick="modal.rightLung(); return false;">Right Lung</a>
					<a class="sound-mute" id="right-lung-mute" href="javascript: void(2)" onclick="modal.rightLung(); return false;"><img src="<?= BROWSER_IMAGES; ?>sound_mute.png"></a>
				</div>
				<div id="heart-sound-dog-control" class="dog-control">
					<a id="heart-sound-dog-control-title" href="javascript: void(2)" onclick="modal.heartSound(); return false;">Heart Sound</a>
					<a class="sound-mute" id="heart-sound-mute" href="javascript: void(2)" onclick="modal.heartSound(); return false;"><img src="<?= BROWSER_IMAGES; ?>sound_mute.png"></a>
				</div>
				<div id="chest-dog-control" class="dog-control">
					<a id="chest-dog-control-title" href="javascript: void(2)" onclick="modal.chestRise(); return false;">Chest Movement</a>
				</div>
			</div>
			
			<div id="vsm" class="float-left ii-border">
				<h1><span id="startStopButton">VS</span> Monitor</h1>
				<div id="vs-left-col">
					<canvas id="vs-trace-1" class="vs-trace" width="500" height="125" onclick="modal.heartRhythm(); return false;"></canvas>
					<canvas id="vs-trace-2" class="vs-trace" width="500" height="125" onclick="modal.respRhythm(); return false;"></canvas>
				</div>
				<div id="vs-right-col">
					<div class="vs-controls clearer">
						<a href="javascript: void(0)" onclick="modal.heartRhythm(); return false;" class="strip-label color-green">HR</a>
					</div>
					<div id="vs-heartRhythm" class="vs-controls">
						<a href="javascript: void(0)" onclick="modal.heartRate(); return false;" class="display-rate color-green">70</a>
					</div>
					<div class="vs-controls clearer">
						<a href="javascript: void(0)" onclick="modal.etCO2(); return false;" class="strip-label color-white">ETCO<sub>2</sub></a>
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
						<a class="alt-control-title color-yellow" href="javascript: void(0)" onclick="modal.SpO2(); return false;">SpO<sub>2</sub></a>
						<a id="display-SpO2" class="alt-control-rate color-yellow" href="javascript: void(0)" onclick="modal.SpO2(); return false;">123</a>
					</div>
					<div id="vs-nbp" class="alt-control">
						<a class="alt-control-title color-red" href="javascript: void(0)" onclick="modal.nbp(); return false;">NIBP</a>
						<a id="display-nbp" class="alt-control-rate nbip color-red" href="javascript: void(0)" onclick="modal.nbp(); return false;"><span id="displayed-systolic">140</span>/<span id="displayed-diastolic">75</span> (<span id="displayed-meanNBP">80</span>) <span class="nbip-label">mmHg</span></a>
						<a id="display-nbp-hr" class="alt-control-rate color-red" href="javascript: void(0)" onclick="modal.nbp(); return false;"><span style="font-size: 18px;">PR</span> <span id="displayed-reportedHR">75</span></a>
						<button id="button-nbp" class="scenario-button red-button">Read NIBP</button>
					</div>
				</div>
			</div>
			
			<div id="media-col">
				<div id="scenario-select" class="float-left clearer ii-border">
					<h2 class="float-left clearer">Scenario Select:</h2>
					<select class=" clearer float-left">
						<?= $scenarioContent; ?>
					</select>
					<button id="scenario-button" class="scenario-button float-left">Start Scenario</button>
					<h2 id="scenario-video-label">Start Video With Scenario</h2>
					<input type="checkbox" id="start-video" class="float-left">
					<p id="scenario-run-time" class="clearer float-left">Scenario Running time:</p><p class="float-left" id="scenario-running-time">01:01</p>
					<p id="scene-run-time" class="clearer float-left">Scene Running time:</p><p class="float-left" id="scene-running-time">01:00</p>
					<button id="scenario-terminate-button" class="scenario-button float-left">Terminate Scenario</button>
				</div>
				<div id="media-select" class="float-left clearer ii-border">
					<h2 class="float-left clearer">Media Select:</h2>
					<select class="clearer float-left">
					</select>
					<button id="media-button" class="scenario-button float-left">Show Media</button>
				</div>
			</div>
			<div id="event-monitor" class="float-right ii-border">
				<table>
				</table>
			</div>
			<div id="comment-box" class="float-left ii-border">
				<input type="text" id="comment-input" value="Please enter comment for log">
				<button id="comment-button" class="scenario-button float-left">Log Comment</button>				
			</div>
			<div id="event-library" class="float-left"></div>

			<div class="clearer"></div>
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
		<pre>
		<?php
		/*
			printf("Session ID is %s\n", $sessionID );
			echo "SID: ".SID."<br>session_id(): ".session_id()."<br>COOKIE: ".$_COOKIE["PHPSESSID"];
			printf("\n_COOKIE:\n%s\n", print_r($_COOKIE, TRUE ) );
		*/
		?>
		</pre>
	</body>
</html>