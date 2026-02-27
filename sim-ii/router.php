<?php
// router.php

//<html><body><pre><?php print_r($_SERVER ); </pre>

if ( false )
	return false;

if (preg_match('/\.(?:mp4|mp3|wav)$/', $_SERVER["SCRIPT_FILENAME"])) {
	$file = $_SERVER["SCRIPT_FILENAME"];
	$path = pathinfo($_SERVER["SCRIPT_FILENAME"]);
	$fm=@fopen($_SERVER["SCRIPT_FILENAME"],'rb');
	
	if(!$fm) {
	  header ("HTTP/1.0 404 Not Found");
	  return ( true );
	}
	
	$size   = filesize($file); // File size
	$length = $size;           // Content length
	$begin  = 0;               // Start byte
	$end    = $size;       // End byte
	
	if ( $begin > 0 || $end < ($size))
	  header('HTTP/1.1 206 Partial Content');
	else
	  header('HTTP/1.1 200 OK');
  
	header("Content-Type: video/mp4");
	header('Accept-Ranges: bytes');
	header('Content-Length:'.($end-$begin));
	header("Content-Disposition: inline;");
	header("Content-Range: bytes $begin-$end/$size");
	//header("Content-Transfer-Encoding: binary\n");
	$cur=$begin;
	fseek($fm,$begin,0);

	while ( !feof($fm) && $cur<$end && (connection_status()==0))
	{ 
		print fread($fm,min(1024*16,$end-$cur));
		$cur+=1024*16;
//		usleep(1000);
	}
	return ( true );
}
else
{ 

    return false;    // serve the requested resource as-is.
}
?>
