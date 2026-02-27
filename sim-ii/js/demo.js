/*
sim-ii: Copyright (C) 2019  VetSim, Cornell University College of Veterinary Medicine Ithaca, NY

See gpl.html
*/
/*
	Support for demo mode.
	
	In this mode, we start a private simmgr session. This is distinguished by the presence of the
	userID in the cookies, set to the user ID of the Demo User (UserID 5).
	
	The subsequent sinstatus.cgi calls will use the cookie PHPSESSID to key to the specific session.
*/

var simDemo = {
	simDemoActive: false,
	
	init: function() {
		if ( userID == 5 )
		{
			$.ajax({
				url: BROWSER_AJAX + 'ajaxStartDemo.php',
				type: 'post',
				async: false,
				dataType: 'json',
				success: function(response) {
					if ( response.status == AJAX_STATUS_OK )
					{
						console.log("Demo Started" );
						simDemo.simDemoActive = true;
					}
					else
					{
						
						console.log("Demo Start Failed", response.cause );
					}
				},
				error: function(jqXHR, textStatus, errorThrown ) {
					concole.log("Failed: ", textStats );
				}
			});
		}
	}
}