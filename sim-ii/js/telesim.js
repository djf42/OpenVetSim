/*
sim-ii: Copyright (C) 2019  VetSim, Cornell University College of Veterinary Medicine Ithaca, NY

See gpl.html
*/
	
	// telesim functions
	
	// convert to bitmap functionality
	// TELESIM_CLEAR takes priority over all other bits
	// TELESIM_LOAD, TELESIM_START, TELESIM_LOOP_ENABLE, and TELESIM_LOOP_DISABLE may be combined
	// Priority order of execution:
	//	1. TELESIM_LOAD
	//	2. TELESIM_START
	//	3. TELESIM_LOOP_ENABLE
	//	4. TELESIM_LOOP_DISABLE
	
	const TELESIM_CLEAR = 1;
	const TELESIM_STOP = 2;
	const TELESIM_SEEK = 4;
	
	const TELESIM_START = 8;
	const TELESIM_LOAD = 16;
	const TELESIM_LOOP_ENABLE = 32;
	const TELESIM_LOOP_DISABLE = 64;
	const TELESIM_VITALS_EXPAND = 128;
	const TELESIM_VITALS_REDUCE = 256;

	const TELESIM_WINDOW_WIDTH = '300px';
	const TELESIM_WINDOW_1_TOP = '257px';
	const TELESIM_WINDOW_1_RIGHT = '-300px';
	const TELESIM_TRANSFORM_SCALE = 0.8;
	
/*	const TELESIM_STOP = 0;
	const TELESIM_START = 1;
	const TELESIM_CLEAR = 2;
	const TELESIM_SEEK = 3;
	const TELESIM_LOAD = 4;
	const TELESIM_LOOP_DISABLE = 5;
	const TELESIM_LOOP_ENABLE = 6;
*/
	
	var telesim = {
		imageList: new Array,
		imageNext: new Array,
		videoObj: new Array,
		lastSeek: 0,
		coord: "",
		lastCommandWasLoadAndPlay: false,
		window_1_size: TELESIM_VITALS_REDUCE,
		loopState: {
			0: TELESIM_LOOP_ENABLE,
			1: TELESIM_LOOP_ENABLE
		},
		
		init: function() {
			// check if localStorage exists
			if( typeof(localStorage.telesim) === "undefined" ) {
				localStorage.telesim = 0;
				simmgr.sendChange( { 'set:telesim:enable' : 0 } );
			} else {
				simmgr.sendChange( { 'set:telesim:enable' : localStorage.telesim } );				
			}
			this.setTeleSim( localStorage.telesim );
			
			telesim.imageList[0] = {};
			telesim.imageList[1] = {};
			telesim.imageNext[0] = 0;
			telesim.imageNext[1] = 0;
			telesim.videoObj[0] = {};
			telesim.videoObj[1] = {};
			
			// clear out telesim settings for image
			if( !profile.isVitalsMonitor ) {
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

			// init telesim selects for window 0 and 1
			if( typeof scenario.telesim != "undefined" ) {
				// build up telesim window arrays and dropdown
				// clear out selects
				$('select.telesim-select').html('');
				var telesimSelectContent0 = '<option value="none">Please select</option>';
				var telesimSelectContent1 = '<option value="none">Please select</option>';
				if ( typeof(scenario.telesim.images ) !== 'undefined' && typeof(scenario.telesim.images.image) !== 'undefined' ) {
					$.each( scenario.telesim.images.image, function( key, value ){
						if(value.window == "0") {
							telesim.imageList[0][value.name] = value;
							telesimSelectContent0 += '<option value="' + value.name + '">' + value.name + '</option>';
						} else {
							telesim.imageList[1][value.name] = value;
							telesimSelectContent1 += '<option value="' + value.name + '">' + value.name + '</option>';
						}
					});
				}
				$('#telesim-select-0').html( telesimSelectContent0 );	
				$('#telesim-select-1').html( telesimSelectContent1 );	
				
				// add event handler for dropdown
				$('select.telesim-select').unbind().change(function() {
					var telesimWindow = ( $(this).attr("id") == "telesim-select-0" ) ? 0 : 1;
					var command = TELESIM_LOAD;
					
					if( $(this).val() == "none" ) {
						command = TELESIM_CLEAR;
					}
		
					simmgr.sendChange( { 
						'set:telesim:name' : telesimWindow + ':' + $(this).val(),
						'set:telesim:command' : telesimWindow + ":" + command,
						'set:telesim:param' : telesimWindow + ":0",
						'set:telesim:next' : telesimWindow + ":" + (parseInt(telesim.imageNext[telesimWindow]) + 1).toString()
					} );					
				});
				
			} else {
				// code goes here to disable telesim option
				
			}		
		},
		
		clearAuscultation: function() {
			$('.ausc-hotspot').removeClass('active');							
		},
		
		setAuscultation: function(coordinate) {
			telesim.clearAuscultation();
			
			// coordinate is in the format of "side-row-col"
			$( '.ausc-hotspot[data-coord=' + coordinate + ']' ).addClass( 'active' );
		},
		
		toggleTeleSim: function() {
			if( localStorage.telesim == 1) {
				simmgr.sendChange( { 'set:telesim:enable' : 0 } );
				this.setTeleSim( "0" );
			} else {
				simmgr.sendChange( { 'set:telesim:enable' : 1 } );
				this.setTeleSim( "1" );
			}
		},
		
		setTeleSim: function( action ) {
			// clear out any auscultation
			simmgr.sendChange( { 'set:auscultation:side' : 0 } );

			if( typeof action !== "undefined" ) {
				if( action == 1 ) {
					localStorage.telesim = 1;
					$('.tele-sim > a').html("Disable TeleSim");
					$('.tele-sim').addClass("enabled").removeClass("disabled");
					
					// manikin and buttons
					$('#mannequin, .nvs-button').addClass("tele-sim");

					if( typeof scenario.scenarioProfile !== "undefined") {
						// avatar
						$('#mannequin').css({
							'background-size': scenario.scenarioProfile.avatar['width_pct_telesim'] + '% ' +  scenario.scenarioProfile.avatar['height_pct_telesim'] + '%'
						});
				
						// controls
						$.each(scenario.scenarioProfile.controls.control, function() {
							$('#' + this.id).css('left', this.left_telesim + 'px');
							$('#' + this.id).css('top', this.top_telesim + 'px');
							$('#' + this.id + '-title').html(this.title).css({
																	'left': this.left_telesim + 'px',
																	'top': (parseInt(this.top_telesim) - 20).toString() + 'px',
																	});
						});
						
						// dog controls
						$("div.dog-control a.control-tele-sim").show();
						$("div.dog-control a.control-title").hide();
					}
					
					// display telesim right dev
					$('.telesim-right').show();
					
					// if vitals, rescale
					if( profile.isVitalsMonitor ) {
						doWindowScale( 0.8 );
					}
					
				} else if( action == 0 ) {
					localStorage.telesim = 0;
					$('.tele-sim > a').html("Enable TeleSim");
					$('.tele-sim').removeClass("enabled").addClass("disabled");
					
					// manikin
					$('#mannequin, .nvs-button').removeClass("tele-sim");
					
					if( typeof scenario.scenarioProfile !== "undefined") {
						// avatar
						$('#mannequin').css({
							'background-size': scenario.scenarioProfile.avatar['width_pct'] + '% ' +  scenario.scenarioProfile.avatar['height_pct'] + '%'
						});

						// controls
						$.each(scenario.scenarioProfile.controls.control, function() {
							$('#' + this.id).css('left', this.left + 'px');
							$('#' + this.id).css('top', this.top + 'px');
							$('#' + this.id + '-title').html(this.title).css({
																	'left': this.left + 'px',
																	'top': (parseInt(this.top) - 20).toString() + 'px',
																	});
						});
						
						// dog controls
						$("div.dog-control a.control-tele-sim").hide();
						$("div.dog-control a.control-title").show();
					}
					
					// hide telesim right dev
					telesim.clearTelesimImage( 0 );
					telesim.clearTelesimImage( 1 );

					$('.telesim-right').hide();

					// if vitals, rescale
					if( profile.isVitalsMonitor ) {
						doWindowScale( 1.0 );
					}
					else if ( simmgr.isLocalDisplay() )
					{
						simmgr.resetQuickInterval();
					}
				}			
			}			
		},

		// handler for adding an image
		addTelesimImage: function( window, imageObj ) {		
			if( localStorage.telesim == 0 ) {
				return;
			}
			
			// get type of image: static, movie, or auscultation and create file URL
			var fileExt = imageObj.filename.split('.')[imageObj.filename.split('.').length - 1].toLowerCase();
			var imageURL = BROWSER_SCENARIOS + scenario.currentScenarioFileName + '/media/' + imageObj.filename;

			// check if we are doing ausculation from this window
			if( $('#telesim-' + window + ' > div').hasClass('ausc-hotspot') ) {
				simmgr.sendChange( { 'set:auscultation:side' : 0 } );
			}
			
			// remove image to window
			telesim.clearTelesimImage( window );
			
			switch ( fileExt ) {
				case 'mp4': case 'webm': case 'ogg':
					var options;
					if(  profile.isVitalsMonitor ) {
						options = " muted ";
					} else {
						options = " controls ";
					}
					
					// add in loop control for non vitals monitor
					if( !profile.isVitalsMonitor ) {
//						$('#telesim-' + window).append('<a class="telesim-image" href="javascript: void(2);" onclick="telesim.testPlay(' + window + ')">Play</a>' + 
//														'<a class="telesim-image" href="javascript: void(2);" onclick="telesim.testPause(' + window + ')"> Pause</a>' + 
//														'<a class="telesim-image" href="javascript: void(2);" onclick="telesim.testSeek(' + window + ')"> Seek</a>' + 
//														'<img src="' + BROWSER_IMAGES + 'repeat.png" class="telesim-repeat telesim-image" title="Enable looping">');
						$('#telesim-' + window).append('<div class="repeat-wrapper telesim-image telesim-repeat "></div>');
					}
					$('#telesim-' + window).append('<video id="telesim-video-' + window + '"  width="100%" class="telesim-image" src="'+ imageURL +
														'" ' + options + '>' + 
														'<source src="'+ imageURL +'" type="video/' + fileExt + '">' + 
													'</video>');
								
					// bind loop button
					$('#telesim-' + window + ' .repeat-wrapper').click(function() {
						// is looping already active
						if( $(this).hasClass('disabled') ) {
							// *** stub to send status for enabling looping on this window video
							// prototype code
							// add active status
							simmgr.sendChange( { 
								'set:telesim:command' : window + ":" + TELESIM_LOOP_ENABLE,
								'set:telesim:next' : window + ":" + (parseInt(telesim.imageNext[window]) + 1).toString()
							} );
							telesim.loopState[ window ] = TELESIM_LOOP_ENABLE;
						} else {
							// *** stub to send status for disabling on looping this window video
							// prototype code
							// remove active status
							simmgr.sendChange( { 
								'set:telesim:command' : window + ":" + TELESIM_LOOP_DISABLE,
								'set:telesim:next' : window + ":" + (parseInt(telesim.imageNext[window]) + 1).toString()
							} );					
							telesim.loopState[ window ] = TELESIM_LOOP_DISABLE;
						}
					});
					
					// bind to video play, pause and seek
					telesim.videoObj[ window ] = document.getElementById('telesim-video-' + window);
					
					// if not vitals...
					if( !profile.isVitalsMonitor ) {
						telesim.videoObj[ window ].onplay = function() {
							telesim.lastSeek = 0;
							if ( lastCommandWasLoadAndPlay == true )
							{
								lastCommandWasLoadAndPlay = false;
							}
							else
							{
								simmgr.sendChange({ 
									'set:telesim:command' : window + ":" + TELESIM_START,
									'set:telesim:param' : window + ":" + parseFloat(telesim.videoObj[ window ].currentTime).toString(),
									'set:telesim:next' : window + ":" + (parseInt(telesim.imageNext[window]) + 1).toString()
								});
							}
						}
						telesim.videoObj[ window ].onpause = function() {
							simmgr.sendChange({ 
								'set:telesim:command' : window + ":" + TELESIM_STOP,
								'set:telesim:param' : window + ":-1",
								'set:telesim:next' : window + ":" + (parseInt(telesim.imageNext[window]) + 1).toString()
							});
						}
						telesim.videoObj[ window ].onseeking = function() {
							if ( telesim.lastSeek == 0 ) {
								telesim.lastSeek = telesim.videoObj[ window ].currentTime;
								simmgr.sendChange({ 
									'set:telesim:command' : window + ":" + TELESIM_SEEK,
									'set:telesim:param' : window + ":" + telesim.lastSeek,
									'set:telesim:next' : window + ":" + (parseInt(telesim.imageNext[window]) + 1).toString()
								});
							}
							telesim.lastSeek = 0;
						}
					}
					
													
					break;
				case 'jpg': case 'jpeg': case 'jif': case 'jfif':
				case 'png':	case 'gif':	case 'tif': case 'tiff':
				case 'jp2': case 'jpx': case 'j2k': case 'j2c':
				case 'fpx':	case 'pcd':	case 'pdf':
					$('#telesim-' + window).append('<img width="100%" class="telesim-image" src="'+ imageURL +'">');
					
					// add in auscultation
					if( typeof imageObj.auscultation_points !== "undefined" ) {
						// add back backgrouind color fir auscultation
						if( profile.isVitalsMonitor ) {
							$('#telesim-' + window).css( 'background-color', 'white' );	
						}
						
						$('#telesim-' + window + ' img').attr('width', imageObj.scale_width_pct + '%');
						$('#telesim-' + window + ' img').css({
							top: imageObj.offset_top_px + 'px',
							left: imageObj.offset_left_px + 'px',
							position: 'absolute'
						});
						
						var offsetLeftVitals = '0';
						var offsetTopVitals = '0';
						if(typeof imageObj.offset_top_vitals != 'undefined' && profile.isVitalsMonitor ) {
							offsetLeftVitals = imageObj.offset_left_vitals;
							offsetTopVitals = imageObj.offset_top_vitals;
						}
						
						$.each( imageObj.auscultation_points.auscultation_point, function( key, auscObj ){
							var offsetTop = ( parseInt( offsetTopVitals ) + parseInt( auscObj.offset_top_px ) );
							var offsetLeft = ( parseInt( offsetLeftVitals ) + parseInt( auscObj.offset_left_px ) );
							
							if( profile.isVitalsMonitor ) {
								offsetTop = offsetTop / TELESIM_TRANSFORM_SCALE;
								offsetLeft = offsetLeft / TELESIM_TRANSFORM_SCALE;								
							}
							
							var offsetTopPx = offsetTop.toString();
							var offsetLeftPx = offsetLeft.toString();
							
							if ( typeof(auscObj.xPosition) != 'undefined' )
							{
								$('#telesim-' + window).append('<div class="ausc-hotspot telesim-image" data-coord="' + auscObj.side + '-' + auscObj.yPosition +'-' + auscObj.xPosition + '" style="top: ' + offsetTopPx + 'px; left: ' + offsetLeftPx + 'px;">' + auscObj.name + '</div>');
							}
							else
							{
								$('#telesim-' + window).append('<div class="ausc-hotspot telesim-image" data-coord="' + auscObj.side + '-' + auscObj.row +'-' + auscObj.col + '" style="top: ' + offsetTopPx + 'px; left: ' + offsetLeftPx + 'px;">' + auscObj.name + '</div>');
							}
						});
						
						// rescale hot spots sizing
						if( profile.isVitalsMonitor ) {
							$('.ausc-hotspot').addClass('vitals');
						}

						// ausculation points in top right telesim port
						$('.ausc-hotspot').click( function( evt ) {
							evt.stopImmediatePropagation();
							
							// is tag already active?
							if( $(this).hasClass('active') ) {
								simmgr.sendChange( { 'set:auscultation:side' : 0 } );
							} else {					
								// get coord of tag
								var temp = $(this).attr('data-coord').split('-');
								simmgr.sendChange( { 
									'set:auscultation:side' : temp[0],
									'set:auscultation:row' : temp[1],
									'set:auscultation:col' : temp[2]
								} );

							}
						});
			
						$('#telesim-0').click(function( evt ) {
							simmgr.sendChange( { 'set:auscultation:side' : 0 } );
							evt.stopImmediatePropagation();				
						});
					} else if( profile.isVitalsMonitor ) {
						$('#telesim-' + window).css( 'background', 'none' );	
					}

					break;
			}
			
			if( window == '1' && !profile.isVitalsMonitor ) {
				// bind event
				$('#telesim-size').show().unbind().click(function() {
					if( $(this).hasClass('expand') ) {
						$(this).removeClass('expand').addClass('reduce');
						$(this).html('-');
						
						// send change to vitals
						simmgr.sendChange( { 
							'set:telesim:command' : window + ":" + TELESIM_VITALS_EXPAND,
							'set:telesim:next' : window + ":" + (parseInt(telesim.imageNext[window]) + 1).toString()
						} );
					} else {
						$(this).removeClass('reduce').addClass('expand');
						$(this).html('+');								
						
						// send change to vitals
						simmgr.sendChange( { 
							'set:telesim:command' : window + ":" + TELESIM_VITALS_REDUCE,
							'set:telesim:next' : window + ":" + (parseInt(telesim.imageNext[window]) + 1).toString()
						} );
					}
					
				});
			}
		},
		
		clearTelesimImage: function( window ) {
			// remove image to window
			$('#telesim-' + window +' > .telesim-image').remove();
			$('#telesim-' + window).css( 'background', 'none' );
			telesim.videoObj[ window ] = {};
		},
		
		processTelesimCommand: function( responseTelesimObj, window ) {
//console.log('-----');
//console.log('Window: ' + window);
//console.dir(responseTelesimObj[window]);
			// clear takes priority over everyhting.  if clear, no more checks.
			if( parseInt(responseTelesimObj[ window ].command) & TELESIM_CLEAR) {
				telesim.clearTelesimImage( window );
				if( window == 1 ) {
					$('#telesim-size').hide();
					$('#telesim-size').removeClass('reduce').addClass('expand');
					$('#telesim-size').html('+');								

				}

				// set dropdown
				$('#telesim-select-' + window).children('option[value="none"]').prop('selected', true);				
			} else if(typeof telesim.imageList[ window ][responseTelesimObj[ window ].name] !== "undefined" &&
				parseInt(responseTelesimObj[ window ].command) & TELESIM_LOAD ) {
				
				// check for load first, then process remaining btmapped commands only if video
				// perform load
				telesim.addTelesimImage( window, telesim.imageList[ window ][responseTelesimObj[ window ].name]);
				
				// set dropdown
				$('#telesim-select-' + window).children('option[value="' + responseTelesimObj[ window ].name + '"]').prop('selected', true);				
			}
			
			// check if we need to expand the window
			if( profile.isVitalsMonitor && ( parseInt(responseTelesimObj[ window ].command) & TELESIM_VITALS_EXPAND ) ) {
				telesim.window_1_size = TELESIM_VITALS_EXPAND;
				telesim.window1Expand();
			} 
			if( profile.isVitalsMonitor && ( parseInt(responseTelesimObj[ window ].command) & TELESIM_VITALS_REDUCE ) ) {
				telesim.window_1_size = TELESIM_VITALS_REDUCE;
				telesim.window1Reduce();
			}

			// check if loaded object is a video
			if ( typeof($("#telesim-video-"+window).prop ) === 'function' &&
				 $("#telesim-video-"+window).prop("nodeName") == "VIDEO" ) {

				// check for start, stop, seek, loop enable and loop disable
				if( parseInt(responseTelesimObj[ window ].command) & TELESIM_START  ) {
					if( profile.isVitalsMonitor ) {
						if ( responseTelesimObj[ window ].param > 0 )
						{
							telesim.videoObj[ window ].currentTime = parseFloat(responseTelesimObj[ window ].param );
						}
						$("#telesim-video-"+window).prop('muted', true);
						telesim.videoObj[ window ].play();
					} else {
						if ( ( parseInt(responseTelesimObj[ window ].command) & ( TELESIM_START | TELESIM_LOAD ) ) == ( TELESIM_START | TELESIM_LOAD ) )
						{
							lastCommandWasLoadAndPlay = true;
						}
						else
						{
							lastCommandWasLoadAndPlay = false;
						}
						telesim.videoObj[ window ].play();							
					}
				}
				else
				{
					lastCommandWasLoadAndPlay = false;
				}
				
				// check for stop
				if( parseInt(responseTelesimObj[ window ].command) & TELESIM_STOP ) {
					telesim.videoObj[ window ].pause();
				}
				
				// check for seek
				if( parseInt(responseTelesimObj[ window ].command) & TELESIM_SEEK ) {
					if ( telesim.lastSeek != parseFloat(responseTelesimObj[ window ].param)  ) {
						telesim.videoObj[ window ].currentTime = parseFloat(responseTelesimObj[ window ].param );
					}
					telesim.lastSeek = parseFloat(responseTelesimObj[ window ].param);
				}

				// check for loop enable and loop disable, default to current state of looping
				var currentLoopState = telesim.loopState[ window ];
				if( parseInt( responseTelesimObj[ window ].command ) & TELESIM_LOOP_ENABLE ) {
					currentLoopState = TELESIM_LOOP_ENABLE;
				} else if ( parseInt( responseTelesimObj[ window ].command ) & TELESIM_LOOP_DISABLE ) {
					currentLoopState = TELESIM_LOOP_DISABLE;					
				}
				if( currentLoopState == TELESIM_LOOP_ENABLE ) {
					$('#telesim-' + window + ' > .repeat-wrapper').removeClass('disabled');
					$('#telesim-' + window + ' > .repeat-wrapper').attr('title', 'Video Looping Enabled - Click to Disable');
					$('#telesim-video-' + window).attr('loop', true);						
				}
				if( currentLoopState == TELESIM_LOOP_DISABLE ) {
					$('#telesim-' + window + ' > .repeat-wrapper').addClass('disabled');
					$('#telesim-' + window + ' > .repeat-wrapper').attr('title', 'Video Looping Disabled - Click to Enable');
					$('#telesim-video-' + window).attr('loop', false);
				}
			}
		},
		
		window1Reduce: function() {
			$('#telesim-1').css({
				width: TELESIM_WINDOW_WIDTH,
				top: TELESIM_WINDOW_1_TOP,
				right: TELESIM_WINDOW_1_RIGHT,
				left: 'auto'
			});			
		},
		
		window1Expand: function() {
			$('#telesim-1').css({
				width: '110%',
				top: '0px',
				right: 'auto',
				left: '0px'
			});
		},
		
		testPlay: function(window) {
telesim.videoObj[ window ] = document.getElementById('telesim-video-' + window);
simmgr.sendChange({ 
	'set:telesim:command' : window + ":" + (TELESIM_START | TELESIM_SEEK | LOOP_ENABLE),
//	'set:telesim:param' : window + ":" + parseFloat(telesim.videoObj[ window ].currentTime).toString(),
	'set:telesim:param' : window + ":" + parseFloat(180).toString(),
	'set:telesim:next' : window + ":" + (parseInt(telesim.imageNext[window]) + 1).toString()
});
		},
		testPause: function(window) {
telesim.videoObj[ window ] = document.getElementById('telesim-video-' + window);
simmgr.sendChange({ 
	'set:telesim:command' : window + ":" + TELESIM_STOP,
	'set:telesim:param' : window + ":-1",
	'set:telesim:next' : window + ":" + (parseInt(telesim.imageNext[window]) + 1).toString()
});
		},
		testSeek: function(window) {
telesim.videoObj[ window ] = document.getElementById('telesim-video-' + window);
simmgr.sendChange({ 
	'set:telesim:command' : window + ":" + TELESIM_SEEK,
	'set:telesim:param' : window + ":" + 180,
	'set:telesim:next' : window + ":" + (parseInt(telesim.imageNext[window]) + 1).toString()
});
telesim.lastSeek = 180;
		},
	}
