<?php

class ProviderInfo
{
	public function __construct()
	{
	}
	
	public $CFG_CONNECTION_HOST = 'grannies-games.com';
	public $CFG_CONNECTION_USER = 'grs';
	public $CFG_CONNECTION_PASS = 'grspass01';
	public $CFG_CONNECTION_DB = 'grs';
}

// --------------------
// provider
// --------------------

class Provider
{
	public function __construct($providerInfo)
	{
		$this->providerInfo = $providerInfo;
	}
	
	public function DB_connect()
	{
		// note: open a persistent connection
		
//		echo("using "  . $this->providerInfo->CFG_CONNECTION_HOST . "\n");
		
		$this->connection = mysql_pconnect(
			$this->providerInfo->CFG_CONNECTION_HOST,
			$this->providerInfo->CFG_CONNECTION_USER,
			$this->providerInfo->CFG_CONNECTION_PASS);
		
		if (!$this->connection)
			throw new Exception("unable to connect to database");
		
		if (!mysql_select_db($this->providerInfo->CFG_CONNECTION_DB, $this->connection))
			throw new Exception("unable to select database");
	}
	
	// applies an array of arguments to a statement and returns the result
	// all arguments are properly escaped and placed between ' '.
	
	private function DB_prepare($statement, $arguments)
	{
		$sql = $statement;
		
		for ($i = 1; $i < count($arguments); ++$i)
		{
			$find = "[" . ($i - 1) . "]";
			
			if ($arguments[$i] !== null)
				$replace = "'" . mysql_real_escape_string($arguments[$i], $this->connection) . "'";
			else
				$replace = "null";
			
			$sql = str_replace($find, $replace, $sql);
		}
		
//		echo('prepared: ' . $sql);
		
		return $sql;
	}
	
	public function DB_insert($statement /* arguments */)
	{
		$args = func_get_args();
		
		$sql = $this->DB_prepare($statement, $args);
		
		$res = mysql_query($sql, $this->connection);
		
		if (!$res)
			throw new Exception("failed to execute query: " . mysql_error() . ": " . $statement);
		
		return mysql_insert_id();
	}
	
	// selects a single row from the database and returns the result in an associative array
	
	public function DB_select($statement /* arguments */)
	{
		$args = func_get_args();
		
		$sql = $this->DB_prepare($statement, $args);
		
		$res = mysql_query($sql, $this->connection);
		
		if (!$res)
			throw new Exception("failed to execute query: " . mysql_error() . ": " . $statement);
			
		$result = mysql_fetch_assoc($res);
		
		mysql_free_result($res);
		
		if (!$result)
			return null;
		
		return $result;
	}
	
	// selects multiple rows from the database and returns the result in an array of associative arrays
	
	public function DB_select_list($statement /* arguments */)
	{
		$args = func_get_args();
		
		$sql = $this->DB_prepare($statement, $args);
		
		$res = mysql_query($sql, $this->connection);
		
		if (!$res)
			throw new Exception("failed to execute query: " . mysql_error() . ": " . $statement);
		
		$result = array();
		
		$i = 0;
		
		while ($row = mysql_fetch_assoc($res))
		{
			$result[$i++] = $row;
		}
		
		mysql_free_result($res);
		
		return $result;
	}
	
	private $providerInfo = null;
	private $connection = null;
}

?>
