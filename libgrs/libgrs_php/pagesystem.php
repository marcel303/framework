<?php

define('INT_MAX', 1000000000);

// timer

class Timer
{
	public function __construct()
	{
		$this->time1 = $this->Time_get();
	}

	private function Time_get()
	{
		$time = microtime(); 

		$time = explode(' ', $time); 
		$time = $time[1] + $time[0]; 

		return $time;
	}

	public function TimeDelta_get()
	{
		$this->time2 = $this->Time_get();

		$delta = $this->time2 - $this->time1;

		return $delta;
	}

	private $time1;
	private $time2;
}

$pageTimer = new Timer();

// template

$output = array();

$TPL_log = array();

function TPL_FromText($text)
{
	$result = "";
	
	$lines = explode("\n", $text);
	
	foreach ($lines as $line)
	{
		if (substr($line, 0, 3) == "[D]")
		{
			if (DEBUG)
				$line = substr($line, 3);
			else
				continue;
		}

		$result .= $line . "\n";
	}

	return $result;
}

function TPL_FromFile($fileName)
{
	$text = file_get_contents($fileName);

	return TPL_FromText($text);
}

function TPL_Write($text)
{
	global $output;
	
	$output[] = $text;
}

function TPL_Render()
{
	global $output;
	global $TPL_log;

	$text = implode($output);
	
	// show error messages

	$tpl_log_inf = TPL_FromFile('tpl_log_inf.txt');
	$tpl_log_wrn = TPL_FromFile('tpl_log_wrn.txt');	
	$tpl_log_err = TPL_FromFile('tpl_log_err.txt');

	$errText = "";
	
	foreach ($TPL_log as $kvps)
	{
		$type = $kvps['type'];
		$msg = $kvps['msg'];

		$tpl = '';

		if ($type == 'inf')
			$tpl = $tpl_log_inf;
		if ($type == 'wrn')
			$tpl = $tpl_log_wrn;
		if ($type == 'err')
			$tpl = $tpl_log_err;

		$errText .= TPL_Format($tpl, array('text' => $msg));
	}
	
	$text = TPL_Format($text, array('err' => $errText));
	
	// output
	
	echo($text);
}

function TPL_Format($text, $arguments)
{
	$result = $text;
	
	foreach ($arguments as $key => $value)
	{
		$find = "[" . $key . "]";
		
		$replace = "";
		
		if ($value !== null)
			$replace = $value;
		
		$result = str_replace($find, $replace, $result);
	}
	
	return $result;
}

function TPL_Output($text, $arguments)
{
	$text = TPL_Format($text, $arguments);
		
	TPL_Write($text);
}

function TPL_GetPageTime()
{
	global $pageTimer;

	return $pageTimer->TimeDelta_get();
}

// logging & error reporting

function LOG_inf($text)
{
	global $TPL_log;

	$TPL_log[] = array('type' => 'inf', 'msg' => 'notice: ' . $text);
}

function LOG_wrn($text)
{
	global $TPL_log;

	$TPL_log[] = array('type' => 'wrn', 'msg' => 'warning: ' . $text);
}

function LOG_err($text)
{
	global $TPL_log;

	$TPL_log[] = array('type' => 'err', 'msg' => 'error: ' . $text);
}

// page arguments

function GetToSession()
{
	foreach ($_GET as $key => $value)
	{
		if ($key == 'action')
			continue;

		$_SESSION[$key] = $value;
	}
}

function Arg($name, $default)
{
	if (isset($_REQUEST[$name]))
		return $_REQUEST[$name];

	if (isset($_SESSION[$name]))
		return $_SESSION[$name];

	return $default;
}

function Arg_Int($name, $default, $min, $max)
{
	$result = Arg($name, $default);
	
	if ($result < $min)
		$result = $min;
	if ($result > $max)
		$result = $max;

	return $result;
}

// privileges

$AUTH_IsPrivileged = false;

function Authenticate()
{
	global $AUTH_IsPrivileged;

	$user = Arg('auth_user', '');
	$pass = Arg('auth_pass', '');

	$AUTH_IsPrivileged = $user == 'admin' && $pass == 'pickwick01';	
}

function IsPrivileged()
{
	global $AUTH_IsPrivileged;

	return $AUTH_IsPrivileged;
}

// todo: move to page begin function

session_start();

GetToSession();

Authenticate();

?>