<?php
/*
sim-ii: Copyright (C) 2019  VetSim, Cornell University College of Veterinary Medicine Ithaca, NY

See gpl.html
*/
?>
		<meta charset="UTF-8">
		<meta name="google" content="notranslate">
		<meta http-equiv="Content-Language" content="en">

		<title>
<?php
		$page = basename($_SERVER["SCRIPT_FILENAME"], '.php');
		if ( strcmp($page, 'vitals' ) == 0 )
		{
			echo "Patient Vitals Monitor"; 
		}
		else
		{
			echo "Open VetSim Instructor Interface";
		}
?>
		</title>
		<link rel="shortcut icon" href="favicon.ico" />		

<?php
		define( 'LEGACY', FALSE );
		if( MOBILIZED ) {
//			echo '<link rel="stylesheet" href="' . BROWSER_CSS . 'jquery.mobile-1.4.5.min.css" />';
//			echo '<link rel="stylesheet" href="' . BROWSER_CSS . 'mobilize.css" />';
		}
?>
		<link rel="stylesheet" href="<?= BROWSER_CSS; ?>common.css" type="text/css" />
<?php
		if( LEGACY ) {
			echo '
				<link rel="stylesheet" href="scripts/jquery-ui/1.11.4/jquery-ui.smoothness.min.css">
			';
		} else {
			echo '
				<link rel="stylesheet" href="scripts/jquery-ui/1.13.2/jquery-ui.min.css">
			';
		}

?>
		<link rel="stylesheet" href="<?= BROWSER_CSS; ?>controls.css" type="text/css" />
		<link rel="stylesheet" href="<?= BROWSER_CSS; ?>modal.css" type="text/css" />
		
<?php
		// php defines in JS
		require_once(SERVER_INCLUDES."phpDefinesToJs.php");
		$ts = date("U");
		
		if( LEGACY ) {
			echo '
				<script type="text/javascript" src="scripts/jquery/2.2.1/jquery.min.js"></script>
				<script src="scripts/jquery-ui/1.11.4/jquery-ui.js"></script>
			';
		} else {
			echo '
				<script type="text/javascript" src="scripts/jquery/3.6.4/jquery.min.js"></script>
				<script src="scripts/jquery-ui/1.13.2/jquery-ui.js"></script>
			';
		}
?>
		<script src="scripts/hotkeys.js"></script>
		<script type="text/javascript" src="scripts/obs-websocket.js"></script>
<?php
		if(MOBILIZED ) {
//			echo '<script src="scripts/jquery.mobile.custom.min.js"></script>';
		}
?>
		
		<script type="text/javascript" src="<?= BROWSER_SCRIPTS; ?>menu.js?v=<?= $ts ?>"></script>
		<script type="text/javascript" src="<?= BROWSER_SCRIPTS; ?>chart.js?v=<?= $ts ?>"></script>
		<script type="text/javascript" src="<?= BROWSER_SCRIPTS; ?>controls.js?v=<?= $ts ?>"></script>
		<script type="text/javascript" src="<?= BROWSER_SCRIPTS; ?>modal.js?v=<?= $ts ?>"></script>
		<script type="text/javascript" src="<?= BROWSER_SCRIPTS; ?>buttons.js?v=<?= $ts ?>"></script>
		<script type="text/javascript" src="<?= BROWSER_SCRIPTS; ?>events.js?v=<?= $ts ?>"></script>
		<script type="text/javascript" src="<?= BROWSER_SCRIPTS; ?>profile.js?v=<?= $ts ?>"></script>
		<script type="text/javascript" src="<?= BROWSER_SCRIPTS; ?>scenario.js?v=<?= $ts ?>"></script>
		<script type="text/javascript" src="<?= BROWSER_SCRIPTS; ?>media.js?v=<?= $ts ?>"></script>
		<script type="text/javascript" src="<?= BROWSER_SCRIPTS; ?>log.js?v=<?= $ts ?>"></script>
		<script type="text/javascript" src="<?= BROWSER_SCRIPTS; ?>user.js?v=<?= $ts ?>"></script>		
		<script type="text/javascript" src="<?= BROWSER_SCRIPTS; ?>simmgr.js?v=<?= $ts ?>"></script>
		<script type="text/javascript" src="<?= BROWSER_SCRIPTS; ?>telesim.js?v=<?= $ts ?>"></script>
		
