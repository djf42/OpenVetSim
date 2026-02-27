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

	// beats.php: This file is primarily for development use. It does a periodic status update 
	// and displays a few parameters.
	 
	require_once('init.php');
	
	$userRow = adminClass::getUserRowFromSession();
	if ( isset($userRow['UserID'] ) )
		$uid = $userRow['UserID'];
	else
		$uid = 0;
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
	<script type="text/javascript">
			$(document).ready(function() {				
				quickTimer = setTimeout(function() { getStatus(); }, 1000 );
			});
	function getStatus() {
		// get unique time stamp
		timeStamp = new Date().getTime();
		
		$.ajax({
			url: BROWSER_CGI + 'simstatus.cgi',
			type: 'get',
			dataType: 'json',
			data: { status : simmgr.timeStamp },
			success: function(response,  textStatus, jqXHR ) {
				$('#msecTime').text(response.debug.msec );
				$('#p1').text(response.cardiac.pulseCount );
				$('#p2').text(response.cardiac.pulseCountVpc );
				$('#p3').text(response.cardiac.rate );
				var rdiff = ((response.cardiac.avg_rate - response.cardiac.rate) / response.cardiac.rate ) * 100;
				$('#p4').text(response.cardiac.avg_rate+" ("+rdiff+"%)" );
				$('#shockLast').text(response.defibrillation.last );
				$('#shockState').text(response.defibrillation.shock );
				$('#cprCompression').text(response.cpr.compression );
				$('#cprRunning').text(response.cpr.running );
				/*
				$('#dbg1').text(response.debug.debug1 );
				$('#dbg2').text(response.debug.debug2 );
				$('#dbg3').text(response.debug.debug3 );
				*/
			},

			error: function( jqXHR,  textStatus,  errorThrown){
				console.log("error: "+textStatus+" : "+errorThrown );
			},
			complete: function(jqXHR,  textStatus ){
				quickTimer = setTimeout(function() { getStatus(); }, 1000 );
			}
		});
	}
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

	</style>
	
</head>

<body>
	<table>
		<tr><td width='100'>Pulse</td><td id='p1' ></td></tr>
		<tr><td>MSec Time</td><td id='msecTime' ></td></tr>
		<tr><td>VPC</td><td id='p2' ></td></tr>
		
		<tr><td>Rate</td><td id='p3' ></td></tr>
		<tr><td>Avg_Rate</td><td id='p4'></td></tr>
		<tr><td>Shock</td><td id='shockLast' ></td><td id='shockState'></td></tr>
		<tr><td>CPR</td><td id='cprCompression' ></td><td id='cprRunning'></td></tr>
		<!--
		<tr><td>DBG1</td><td id='dbg1' ></td></tr>
		<tr><td>DBG2</td><td id='dbg2' ></td></tr>
		<tr><td>DBG3</td><td id='dbg3' ></td></tr>
		-->
		
	</table>
</body>
</html>