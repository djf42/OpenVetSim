<?php
ini_set('display_errors', 'On');
error_reporting(E_ALL | E_STRICT);


	
	require_once('init.php');
	
	
?>
<pre>
<?php 	print_r($_SERVER );
printf("BROWSER_CGI: %s\n", BROWSER_CGI );
?>
</pre>
