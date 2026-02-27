/*
sim-ii: Copyright (C) 2019  VetSim, Cornell University College of Veterinary Medicine Ithaca, NY

See gpl.html
*/
	var profile = {
	
		isVitalsMonitor: false,
		init: function() {
			// do we have a valid progile loaded
			if(typeof scenario.scenarioProfile == 'undefined') {
				return
			}
			
			// set up background image for animal silhouette from profile
			if(profile.isVitalsMonitor == false ) {
				if ( typeof scenario.scenarioProfile.avatar != 'undefined') {
					$('#mannequin').css({
						'background-image': 'url(' + BROWSER_SCENARIOS + scenario.currentScenarioFileName + '/images/' + scenario.scenarioProfile.avatar['filename'] + ')',
						'background-repeat': 'no-repeat',
						'background-position': 'center center',
						'background-size': scenario.scenarioProfile.avatar['width_pct'] + '% ' +  scenario.scenarioProfile.avatar['height_pct'] + '%'
					});
					$('div.dog-control a').css('color', "'" + scenario.scenarioProfile.controls['color'] +"'");
				}
			
				// set positioning of controls
				// hide all controls
				$('.dog-control').hide();
				$.each(scenario.scenarioProfile.controls.control, function() {
					var height = $('#' + this.id).height();
					$('#' + this.id).css('left', this.left + 'px');
					$('#' + this.id).css('top', this.top + 'px');
					$('#' + this.id + '-title').html(this.title).css({
																	'left': this.left + 'px',
																	'top': (parseInt(this.top) - 20).toString() + 'px',
																	});
					
					// setup src for telesim icon
					$('#' + this.id + '-icon img').attr('src', BROWSER_SCENARIOS + scenario.currentScenarioFileName + '/images/' + this.icon_telesim);
					$('#' + this.id + '-icon img').attr('width', this.width_telesim);
					$('#' + this.id + '-icon img').attr('alt', this.attr_telesim);
					$('#' + this.id + '-icon img').attr('title', this.title);					
					$('#' + this.id).show();
				});

				// place title
				$('#mannequin h1').html("Scenario: " + scenario.scenarioHeader.title.name);
				$('#mannequin h1').css({
					top: scenario.scenarioHeader.title.top + 'px',
					left: scenario.scenarioHeader.title.left + 'px'
				});
				
				// update header
				$('#scenario-name-display').html(scenario.scenarioHeader.title.name);
			}
		},
		
		initPatientInfo: function() {
//			if(true && scenario.currentScenarioState == scenario.scenarioState.STOPPED) {
			if(profile.isVitalsMonitor == true && typeof scenario.scenarioProfile != 'undefined') {
				profile.removePatientInfo();
				if ( ( scenario.currentScenarioState != scenario.scenarioState.RUNNING ) &&
				     ( scenario.currentScenarioState != scenario.scenarioState.PAUSED ) ) {
				$.ajax({
						url: BROWSER_AJAX + 'ajaxGetPatientInformation.php',
						type: 'post',
						async: false,
						data: {profile: scenario.scenarioProfile.summary, dir: scenario.currentScenarioFileName},
						dataType: 'json',
						success: function(response) {
							profile.removePatientInfo();
							$('body').append(response.html);
							$('#patient-info').draggable().css({
								'left': '10px',
	//							'margin-left': (Math.floor($('#patient-info').width() / 2) * -1) + 'px',
								'transform'                : 'scale('+windowScaleFactor+')',
								'transform-origin'         : '0 0',
								'-moz-transform-origin'    : '0 0',         // Firefox
								'-ms-transform-origin'     : '0 0',         // IE
								'-webkit-transform-origin' : '0 0',         // Opera/Safari
								'-moz-transform'           : 'scale('+windowScaleFactor+')', // Firefox
								'-ms-transform'            : 'scale('+windowScaleFactor+')', // IE
								'-webkit-transform'        : 'scale('+windowScaleFactor+')'  // Opera/Safari
							});

						}
					});
				}
			} else {
				profile.removePatientInfo();			
			}
		},
		
		removePatientInfo: function() {
			$('#patient-info').remove();
		}
	}