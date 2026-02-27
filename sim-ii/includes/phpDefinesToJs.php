<?php
/*
sim-ii: Copyright (C) 2019  VetSim, Cornell University College of Veterinary Medicine Ithaca, NY

See gpl.html
*/

//	define("BROWSER_SCENARIOS", ".." . DIR_SEP . "scenarios" . DIR_SEP);

?>
			<script type="text/javascript">
				var BROWSER_AJAX = "<?= BROWSER_AJAX; ?>";
				var BROWSER_IMAGES = "<?= BROWSER_IMAGES; ?>";
				var BROWSER_VOCALS = "<?= BROWSER_VOCALS; ?>";
				var BROWSER_PROFILES_IMAGES = "<?= BROWSER_PROFILES_IMAGES; ?>";
				var BROWSER_CGI = "<?= BROWSER_CGI; ?>";
				var BROWSER_SCENARIOS = "<?= BROWSER_SCENARIOS; ?>";
				var BROWSER_SCENARIOS_IMAGES = BROWSER_SCENARIOS+"images/";
				var BROWSER_SCENARIOS_MEDIA = BROWSER_SCENARIOS+"media/";
				var BROWSER_SCENARIOS_VOCALS = BROWSER_SCENARIOS+"vocals/";
								
				var AJAX_STATUS_FAIL = <?= AJAX_STATUS_FAIL; ?>;
				var AJAX_STATUS_OK = <?= AJAX_STATUS_OK; ?>;
				var AJAX_STATUS_LOGIN_FAIL = <?= AJAX_STATUS_LOGIN_FAIL; ?>;
				var SERVER_ADDR = "<?= SERVER_ADDR; ?>";
				var REMOTE_ADDR = "<?= REMOTE_ADDR; ?>";
				
				var ETCO2_NEW_WAVEFORM_COMPLETED = 3;
				var ETCO2_NEW_VALUE_ENTERED = 2;
				var ETCO2_NEW_WAVEFORM_IN_PROGRESS = 1;
				var ETCO2_OK = 0;
			</script>