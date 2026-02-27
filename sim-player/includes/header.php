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
?>
		<meta charset="UTF-8">
		<title>Open VetSim Debrief Viewer</title>
		<link rel="shortcut icon" href="favicon.ico" />		

<?php
		if(MOBILIZED) {
			echo '<link rel="stylesheet" href="' . BROWSER_CSS . 'jquery.mobile-1.4.5.min.css" />';
			echo '<link rel="stylesheet" href="' . BROWSER_CSS . 'mobilize.css" />';
		}
?>

		<link rel="stylesheet" href="<?= BROWSER_CSS; ?>common.css" type="text/css" />
		<link rel="stylesheet" href="scripts/jquery-ui/1.11.4/jquery-ui.smoothness.min.css">
		<link rel="stylesheet" href="<?= BROWSER_CSS; ?>modal.css" type="text/css" />
		
		<?php
			// php defines in JS
			require_once(SERVER_INCLUDES."phpDefinesToJs.php");
			$ts = date("U");
		?>

		<script type="text/javascript" src="scripts/jquery/2.2.1/jquery.min.js"></script>
		<script src="scripts/jquery-ui/1.11.4/jquery-ui.js"></script>
<?php
		if(MOBILIZED) {
			echo '<script src="scripts/jquery.mobile.custom.min.js"></script>';
		}
?>
		
		<script type="text/javascript" src="<?= BROWSER_SCRIPTS; ?>menu.js?v=<?= $ts ?>"></script>
		<script type="text/javascript" src="<?= BROWSER_SCRIPTS; ?>modal.js?v=<?= $ts ?>"></script>
		<script type="text/javascript" src="<?= BROWSER_SCRIPTS; ?>user.js?v=<?= $ts ?>"></script>
		<script type="text/javascript" src="<?= BROWSER_SCRIPTS; ?>eventLog.js?v=<?= $ts ?>"></script>
		<script type="text/javascript" src="<?= BROWSER_SCRIPTS; ?>vid.js?v=<?= $ts ?>"></script>		