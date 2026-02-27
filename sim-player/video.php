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
	
	// get filename
	$videoSrc = dbClass::valuesFromGet('url');
	
	// does file exist
	if($videoSrc == "" || !file_exists(SERVER_SIM_VIDEO . $videoSrc) ) {
		$status = MISSING_VIDEO;
	} else {
		$status = VIDEO_FOUND;
	}
	
	$videoURL = BROWSER_VIDEO . $videoSrc;
?>
<!DOCTYPE html>
<html>
	<head>
		<?php require_once(SERVER_INCLUDES . "header.php"); ?>
		<script type="text/javascript">
			$(document).ready(function() {

			});
		</script>
	</head>
	<body>
		<div id="video-div">
			<video id="video-player" width=100% height="auto" controls>
				<source src="<?= $videoURL; ?>" type="video/mp4">
			</video>
		</div>
	</body>
</html>