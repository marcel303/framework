<?php

require_once('libgrs.php');

class View
{
	public function __construct($gameId)
	{
		$this->grs = new GRS();
		$this->gameId = $gameId;
		
		$this->game = $this->grs->store->QRY_game_get($gameId);
		
		if ($this->game == null)
			LOG_err('game does not exist');
	}
	
	public function Select($gameMode, $page, $pageSize)
	{
		return $this->grs->store->QRY_score_fetch_list($this->gameId, $gameMode, $page * $pageSize, $pageSize, null, null);
	}

	public function SelectHist($gameMode, $hist, $page, $pageSize)
	{
		return $this->grs->store->QRY_score_fetch_list_hist($this->gameId, $gameMode, $hist, $page * $pageSize, $pageSize, null, null);
	}

	public function GameId_get()
	{
		return $this->gameId;
	}
	
	public function Game_get()
	{
		return $this->game;
	}

	public function Game_get_all()
	{
		return $this->grs->store->QRY_game_get_all();
	}

	public function GameMode_get_all($gameId)
	{
		return $this->grs->store->QRY_gamemode_find_by_game($gameId);
	}

	public function GameMode_exists($gameId, $gameMode)
	{
		return $this->grs->store->QRY_gamemode_exists($gameId, $gameMode);
	}

	public function Stats_get()
	{
		$user_count = $this->grs->store->QRY_stat_user_count();
		$user_legit_count = $this->grs->store->QRY_stat_user_legit_count();
		$user_cracked_count = $this->grs->store->QRY_stat_user_cracked_count();

		return array(
			"user_count" => $user_count,
			"user_legit_count" => $user_legit_count,
			"user_cracked_count" => $user_cracked_count);
	}

	public function Store_get()
	{
		return $this->grs->store;
	}

	private $grs;
	private $gameId;
	private $game;
}

?>