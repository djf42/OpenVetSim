/*
sim-ii: Copyright (C) 2019  VetSim, Cornell University College of Veterinary Medicine Ithaca, NY

See gpl.html
*/
var scenario = {
	currentScenarioFileName: '',
	scenarioActive: false,
	startVideoWithScenario: false,
	scenarioState: {
		STOPPED: 0,
		PAUSED: 1,
		RUNNING: 2,
		TERMINATED: 3
	},
	
	currentScenarioState: null,
	
	scenarioProfile: '',
	scenarioHeader: '',
	scenarioEvents: '',
	scenarioMedia: '',
	scenarioVocals: '',
	
	init: function() {
//		this.currentScenarioState = this.scenarioState.STOPPED;
//		simmgr.sendChange( {'set:scenario:state' : 'terminate'} );
//		scenario.stopScenario();
//console.log(this.currentScenarioState);
console.log("Scenario Init");		
console.log("Current Scenario State: " + this.currentScenarioState);		
		// bind change of scenario
//		$('#scenario-select select').change(function() {
//			$('#scenario-name-display').html($(this).children('option:selected').html());
//		});
		
		// bind change of start video check box
		$('#start-video').unbind().change(function() {
			if($(this).is(':checked') == false) {
				simmgr.sendChange( {'set:scenario:record' : 0} );
			} else {
				simmgr.sendChange( {'set:scenario:record' : 1} );
			}
		});
		
		// bind click of scenario button
		$('#scenario-button').unbind().click(function() {
			scenario.sendNextScenarioState();
		});
		
		// init terminate button
		$('#scenario-terminate-button').css({'background-color': buttons.disconnectColor, 
									border: '1px solid ' + buttons.disconnectColor
									}).html('Terminate Scenario');
		if(scenario.currentScenarioState == scenario.scenarioState.STOPPED) {
			$('#scenario-terminate-button').hide();
		}
									
		$('#scenario-terminate-button').unbind().click(function() {
			simmgr.sendChange( {'set:scenario:state' : 'Terminate'} );
			scenario.recordStartStop(false );
		});
		
		// bind change of scenario dropdown
		$('#scenario-select select').unbind().change(function() {
			simmgr.sendChange({'set:scenario:active': $(this).children('option:selected').val()});
		});
		
		$('#scenario-select select option').unbind().click(function(evt) {
			var newScenarioFileName = $(this).val();
			if(newScenarioFileName == scenario.currentScenarioFileName) {
				simmgr.sendChange({'set:scenario:active': 'blank'});
				setTimeout(function() {
					simmgr.sendChange({'set:scenario:active': newScenarioFileName});
				}, 1000);
			}
		});
	},
	
	// start scenario
	// send to simmgr set:scenario:active='scenario-name-no-quotes'
	// scenario will get put into 'RUNNING' state
	// set START button to PAUSE SCENARIO
	// expose TERMINATE SCENARIO BUTTON
	// NOTE: This function is never used
	startScenario: function() {
		$('#scenario-button').css({'background-color': buttons.connectColor, 
									border: '1px solid ' + buttons.connectColor
									}).html('Pause1 Scenario');
		$('#scenario-terminate-button').show();
		$('#scenario-select select').prop('disabled', true);
		$('#start-video').prop('disabled', true);
		$('.profile-display.scenario img').show();
		return;
	},

	stopScenario: function() {
		$('#scenario-button').css({'background-color': buttons.disconnectColor,
									border: '1px solid ' + buttons.disconnectColor}).html('Start Scenario');
		$('#scenario-terminate-button').hide();
		$('#scenario-select select').prop('disabled', false);
		$('#start-video').prop('disabled', false);
		$('.profile-display.scenario img').hide();
		// if in telesim, clear out images
		if( !profile.isVitalsMonitor && localStorage.telesim == 1 ) {
			simmgr.sendChange( { 
				'set:telesim:name' : "0:none",
				'set:telesim:command' : "0:" + TELESIM_CLEAR,
				'set:telesim:param' : "0:0",
				'set:telesim:next' : "0:" + (parseInt(telesim.imageNext[0]) + 1).toString()
			} );					
			simmgr.sendChange( { 
				'set:telesim:name' : "1:none",
				'set:telesim:command' : "1:" + TELESIM_CLEAR,
				'set:telesim:param' : "1:0",
				'set:telesim:next' : "1:" + (parseInt(telesim.imageNext[0]) + 1).toString()
			} );
		}

		
		return;
	},
	
	pauseScenario: function() {
		$('#scenario-button').css({'background-color': buttons.disconnectColor,
									border: '1px solid ' + buttons.disconnectColor}).html('Continue Scenario');
		$('#scenario-terminate-button').show();
		$('#scenario-select select').prop('disabled', true);
		$('#start-video').prop('disabled', true);
		$('.profile-display.scenario img').hide();
		return;	
	},

	continueScenario: function() {
		$('#scenario-button').css({'background-color': buttons.connectColor, 
									border: '1px solid ' + buttons.connectColor
									}).html('Pause Scenario');
		$('#scenario-terminate-button').show();
		$('#scenario-select select').prop('disabled', true);
		$('#start-video').prop('disabled', true);
		$('.profile-display.scenario img').show();
		return;
	},
	// Hide the video-wait overlay and clean up any pending timers/intervals.
	_hideVideoOverlay: function() {
		clearTimeout(scenario._videoOverlayTimer);
		clearInterval(scenario._videoCheckInterval);
		$('#video-wait-overlay').removeClass('visible');
	},

	recordStartStop: function(start) {
		var chk = $('#start-video').attr('checked');
		console.log("chk", chk );
		if ( chk == 'checked')
		{
			if ( start ) {
				// Show overlay and note the time so we only look for files
				// created from this moment onwards.
				$('#video-wait-overlay').addClass('visible');
				var startedAt = Math.floor(Date.now() / 1000);

				// Poll simlogs/video every 500 ms for a new .mp4 file.
				scenario._videoCheckInterval = setInterval(function() {
					$.ajax({
						url: BROWSER_AJAX + 'ajaxCheckVideoRecording.php',
						type: 'post',
						data: { since: startedAt },
						dataType: 'json',
						success: function(response) {
							if (response.found) {
								scenario._hideVideoOverlay();
							}
						}
					});
				}, 500);

				// Safety fallback: give up waiting after 20 s.
				scenario._videoOverlayTimer = setTimeout(function() {
					scenario._hideVideoOverlay();
				}, 20000);
			}

			setTimeout(function() {
				var cmd;
				if ( start )
					cmd = 'StartRecording';
				else
					cmd = 'StopRecording';
				var obs = new OBSWebSocket();
				obs.connect().then(
				function (){
					obs.send(cmd).then(
						function(){console.log("Recording Started");},
						function(result){console.log("Recording Start Failed");} );
					obs.disconnect();
				},
				function (result){
					console.log("obs.send failed", result);
				})
				.catch()
				{
					//console.log("obs.connect failed",  result);
				};
			}, 3000 );
		}
	},
	sendNextScenarioState: function() {
		switch(this.currentScenarioState) {
			// if stopped, send currently selected scenario
			case this.scenarioState.STOPPED:
				simmgr.sendChange( {'set:scenario:state' : 'Running'} );
				scenario.recordStartStop(true );
				break;
			
			// if paused, send running
			case this.scenarioState.PAUSED:
				simmgr.sendChange( {'set:scenario:state' : 'Running'} );
				break;

			// if running, send paused
			case this.scenarioState.RUNNING:
				simmgr.sendChange( {'set:scenario:state' : 'Paused'} );
				break;
		}
	},
	
	loadScenario: function() {
		$.ajax({
			url: BROWSER_AJAX + 'ajaxGetScenario.php',
			type: 'post',
			async: false,
			data: {fn: scenario.currentScenarioFileName},
			dataType: 'json',
			success: function(response) {
				scenario.scenarioProfile = response.profile;
				scenario.scenarioHeader = response.header;
				scenario.scenarioEvents = response.events;
				scenario.scenarioMedia = response.media;
				scenario.scenarioVocals = response.vocals;
				if ( typeof(response.soundtags) !== 'undefined' )
				{
					scenario.soundtags = response.soundtags;
					if ( typeof(simsound) !== 'undefined' )
					{
						simsound.parseTags();
					}
				}
				
				if( typeof(response.telesim) !== 'undefined' ) {
					scenario.telesim = response.telesim;
				}
				// update scenario dropdown if needed
				$('#scenario-select select').val(scenario.currentScenarioFileName);
			}
		});
	},

	deleteScenario: function(scenarioDir) {
/*
		if ( userID == 5 )
		{
			alert("Delete is not allowed from Demo" );
		}
		else
		{
*/
			if(confirm("Are you sure you want to delete this scenario?") == true) {
				window.location = "ii.php?ddir=" + scenarioDir;
			}
//		}
	},
	
	addScenario: function(scenarioDir) {
/*		if ( userID == 5 )
		{
			alert("Add is not allowed from Demo" );
		}
		else
		{
*/
			$.ajax({
				url: BROWSER_AJAX + 'ajaxAddScenario.php',
				type: 'post',
				async: false,
				data: {sd: scenarioDir},
				dataType: 'json',
				success: function(response) {
					window.location = "ii.php?ddir=x...";
				}
			});
//		}
	}
}