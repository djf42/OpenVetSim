/*
sim-ii: Copyright (C) 2019  VetSim, Cornell University College of Veterinary Medicine Ithaca, NY

See gpl.html
*/
	var events = {
		currentLogRecord: 1,
		currentLogFileName: '',
		defaultComment: 'Please enter comment for log',
		
		init: function() {
			// set up list of events for this scenario into modal
			$.ajax({
				url: BROWSER_AJAX + 'ajaxGetEventsList.php',
				type: 'post',
				async: false,
				dataType: 'json',
				data: {events: JSON.stringify(scenario.scenarioEvents)},
				success: function(response) {
					if(response.status == AJAX_STATUS_OK) {
						$('#event-library').html(response.html).hide();

						// bind expansion of event library
						$('a.expand').click(function() {
							if($(this).hasClass('expanded') == true) {
								$(this).html('+').removeClass('expanded');
								$(this).next('ul').hide();
							} else {
								$(this).html("-").addClass('expanded');
								$(this).next('ul').show();					
							}
						});
					}
					
					// remove previous events
					$('.event-priority').remove();
					
					// Clear prior hotkeys
					hotkeys.clearAll();
					
					// get priority events if any
					if(typeof response.priority != 'undefined' && response.priority.length > 0) {
						var content = '<li class="event-priority event-divider">|</li><li class="event-priority">Quick Event Links:</li>';
						var hotkey = '';
						response.priority.forEach(function(element, index, event) {
							hotkey = '';
							if ( typeof element.hotkey !== 'undefined' && element.hotkey.length != 0 )
							{
								hotkey = ' ('+element.hotkey+')';
							}
							content += '<li class="event-priority"><a class="event-link" href="javascript: void(2);" onclick="events.sendPriorityEvent(\'' + element.id + '\');">' + element.title + hotkey + '</a></li>';
						});
						$('ul#main-nav li.menu-events').after(content);
					}
					
					// alert if a hotlink has been defined with a (b) or (c)
					if( response.hotLinkDup == 'true' ) {
						alert( "Predefined hotkeys (b) or (c) cannot be redefined.  Hotkey definition has been ignored" );
					}
					
					// Set defined hotkeys, if any
					if(typeof response.hotkeys != 'undefined' && response.hotkeys.length > 0) {
						response.hotkeys.forEach(function(key ) {
							console.log("hotkey:", key.hotkey, key.id );
							hotkeys.addKey(key.hotkey, function(){events.sendPriorityEvent(key.id )} );
						});
					}
				}
			});
						
			// force event monitor to scroll to bottom and hide div for modal
			$("#event-monitor").scrollTop(1000);
			
			// bind comments
			$('#comment-input').focus(function() {
				if($(this).val() == events.defaultComment) {
					$(this).val('');
				}
			});

			$('#comment-button').unbind().click(function() {
				if($('#comment-input').val() == '' || $('#comment-input').val() == events.defaultComment) {
					modal.showText('Please enter a comment');
				} else if($('#comment-input').val() != events.defaultComment) {
					simmgr.sendChange({'set:event:comment': $('#comment-input').val()});
					$('#comment-input').blur().val(events.defaultComment);
				}
			});


			$('#comment-input').keypress(function(evt) {
				if(evt.which === null) {
					return;
				} else if(evt.which == 13) {
					$('#comment-button').trigger('click');
				}
				evt.which = null;
			});
		},
		
		sendEventLibraryClick: function(eventObj) {
			$(eventObj).prev('img').show();
//			$('#event-monitor table').append('<tr><td class="time-stamp">00:02:00</td><td class="event">Event: ' + $(eventObj).attr('data-category') + ':' + $(eventObj).html() + '</td></tr>');
//			$("#event-monitor").scrollTop($('#event-monitor table').height());
			
			// code stub to send event to sim mgr
			simmgr.sendChange({'set:event:event_id': $(eventObj).attr('data-event-id')});
			modal.closeModal();
		},
		
		sendPriorityEvent: function(eventID) {
			// code stub to send event to sim mgr
			simmgr.sendChange({'set:event:event_id': eventID});
		},
		
		addEventsFromLog: function() {
			// set up list of events for this scenario into modal
			$.ajax({
				url: BROWSER_AJAX + 'ajaxGetEventsLog.php',
				type: 'post',
				async: false,
				dataType: 'json',
				data: {fileName: events.currentLogFileName, recordCount: events.currentLogRecord},
				success: function(response) {
					if(response.status == AJAX_STATUS_OK) {
						$('#event-monitor table').html(response.html);
					}
				}
			});
		}
		
	}