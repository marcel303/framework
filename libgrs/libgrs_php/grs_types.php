<?php

class Game
{
	public function __construct($row)
	{
		if ($row != null)
		{
			$this->id = $row["id"];
			$this->name = $row["name"];
		}
	}
	
	public $id = null;
	public $name = null;
}

class GameMode
{
	public function __construct($row)
	{
		if ($row != null)
		{
			$this->id = $row["id"];
			$this->debug = $row["debug"];
			$this->mode = $row["mode"];
			$this->name = $row["name"];
			$this->game_id = $row["game_id"];
		}
	}

	public $id = null;
	public $debug = null;
	public $mode = null;
	public $name = null;
	public $game_id = null;
}

class Score
{
	public function __construct($row)
	{
		if ($row != null)
		{
			$this->id = $row["id"];
			$this->game_id = $row["game_id"];
			$this->game_mode = $row["game_mode"];
			$this->game_version = $row["game_version"];
			$this->date = $row["date"];
			$this->user_id = $row["user_id"];
			$this->score = $row["score"];
			$this->user_name = $row["user_name"];
			$this->tag = $row["tag"];
			$this->country_code = $row["country_code"];
			$this->hash = $row["hash"];
			$this->is_hacked = $row["is_hacked"];
		}
	}
	
	public $id = null;
	public $game_id = null;
	public $game_mode = null;
	public $game_version = null;
	public $date = null;
	public $user_id = null;
	public $score = null;
	public $user_name = null;
	public $tag = null;
	public $country_code = null;
	public $hash = null;
	public $is_hacked = null;
	public $history = null;
}

class CrashLog
{
	public function __construct($row)
	{
		if ($row != null)
		{
			$this->id = $row["id"];
			$this->game_id = $row["game_id"];
			$this->game_version = $row["game_version"];
			$this->date = $row["date"];
			$this->message = $row["message"];
			$this->hash = $row["hash"];
		}
	}
	
	public $id = null;
	public $game_id = null;
	public $game_version = null;
	public $date = null;
	public $message = null;
	public $hash = null;
}
		
?>