<?php
require_once ('classes2/App.php');

$options= array('uri'=>'http://appdirectory.messenger.msn.com/AppDirectory/AppDirectory.php', 'location' => 'http://appdirectory.messenger.msn.com/AppDirectory/AppDirectory.php');


$server = new SoapServer(NULL, $options);
$server->setClass('AppDirectory');
$server->setObject(new AppDirectory());
$server->handle();


//print "<pre>";
//print_r($server->getFunctions());
//print "</pre>";
?>