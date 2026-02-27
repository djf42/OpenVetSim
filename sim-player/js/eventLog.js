/*
sim-ii: Copyright (C) 2019  VetSim, Cornell University College of Veterinary Medicine Ithaca, NY

See gpl.html
*/
	var eventLog = {
	
		url: '',		// url of event log
	
		init: function() {
			$.ajax({
				url: BROWSER_AJAX + 'ajaxGetEventsList.php',
				type: 'post',
				async: false,
				data: {fn: eventLog.url},
				dataType: 'json',
				success: function(response) {
					if(response.status == AJAX_STATUS_OK) {
						$('#event-log').html(response.html);
						$('#scenario-name-display').html(eventLog.url);
					} else {
						$('#event-log').html('');
						$('#scenario-name-display').html('');
					}
				}
			});
		}
	}