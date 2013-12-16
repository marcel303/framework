<?php

require_once('db_provider.php');
require_once('grs_types.php');

class StoreInfo
{
	public $TBL_GAMES = 'games';
	public $TBL_GAMEMODES = 'gamemodes';
	public $TBL_DATA = 'scores';
	public $TBL_CRASH_LOGS = 'crash_logs';
}

// --------------------
// store
// --------------------

class Store
{
	public function __construct($provider, $storeInfo)
	{
		$this->provider = $provider;
		$this->storeInfo = $storeInfo;
		
		$this->DB_create_statements();
	}
	
	private function DB_create_statements()
	{
		// game
		$this->statement_game_get = "SELECT * FROM " . $this->storeInfo->TBL_GAMES . " WHERE id=[0]";
		$this->statement_game_find_by_name = "SELECT * FROM " . $this->storeInfo->TBL_GAMES . " WHERE name=[0]";
		$this->statement_game_exists = "SELECT count(*) AS count FROM " . $this->storeInfo->TBL_GAMES . " WHERE id=[0]";
		
		// gamemode
		$this->statement_gamemode_exists = "SELECT count(*) AS count FROM " . $this->storeInfo->TBL_GAMEMODES . " WHERE game_id=[0] AND mode=[1]";

		// score
		$this->statement_score_add = "INSERT INTO " . $this->storeInfo->TBL_DATA . " (user_id, score, user_name, tag, country_code, hash, game_id, game_mode, game_version, date, is_hacked) VALUES([0], [1], [2], [3], [4], [5], [6], [7], [8], NOW(), [10])";
		$this->statement_score_get = "SELECT * FROM " . $this->storeInfo->TBL_DATA . " WHERE id=[0]";
		$this->statement_score_count = "SELECT count(*) AS count FROM " .$this->storeInfo->TBL_DATA . " WHERE game_id=[0] AND game_mode=[1]";
		$this->statement_score_count_hist = "SELECT count(*) AS count FROM " .$this->storeInfo->TBL_DATA . " WHERE game_id=[0] AND game_mode=[1] AND date >= DATE_SUB(NOW(), INTERVAL [2] DAY)";
		$this->statement_score_fetch_rank = "SELECT count(*) AS count FROM " . $this->storeInfo->TBL_DATA . " WHERE game_id=[0] AND game_mode=[1] AND (score>[2] OR (score=[2] AND date < [3]))";
		$this->statement_score_fetch_rank_hist = "SELECT count(*) AS count FROM " . $this->storeInfo->TBL_DATA . " WHERE game_id=[0] AND game_mode=[1] AND date >= DATE_SUB(NOW(), INTERVAL [2] DAY) AND (score>[3] OR (score=[3] AND date < [4]))";
		$this->statement_score_fetch_best = "SELECT * FROM " . $this->storeInfo->TBL_DATA . " WHERE game_id=[0] AND game_mode=[1] AND user_id=[2] ORDER BY score DESC LIMIT 1";
		$this->statement_score_fetch_best_hist = "SELECT * FROM " . $this->storeInfo->TBL_DATA . " WHERE game_id=[0] AND game_mode=[1] AND date >= DATE_SUB(NOW(), INTERVAL [2] DAY) AND user_id=[3] ORDER BY score DESC LIMIT 1";

		$this->statement_score_find = "SELECT * FROM " . $this->storeInfo->TBL_DATA . " WHERE 1=1";
		$this->statement_score_exists = "SELECT count(*) AS count FROM " . $this->storeInfo->TBL_DATA . " WHERE hash=[0]";
		
		// crash_log
		$this->statement_crash_log_add = "INSERT INTO " . $this->storeInfo->TBL_CRASH_LOGS . " (game_id, game_version, date, message, hash) VALUES([0], [1], [2], [3], [4])";
		$this->statement_crash_log_exists = "SELECT count(*) AS count FROM " . $this->storeInfo->TBL_CRASH_LOGS . " WHERE hash=[0]";

		// stats
		$this->statement_usercount_get = "SELECT count(distinct user_id) AS count FROM scores";
		$this->statement_usercount_legit_get = "SELECT count(distinct user_id) AS count FROM scores WHERE is_hacked=0";
		$this->statement_usercount_cracked_get = "SELECT count(distinct user_id) AS count FROM scores WHERE is_hacked=1";
	}
	
	// game
	
	public function QRY_game_get($gameId)
	{
		$row = $this->provider->DB_select($this->statement_game_get, $gameId);
		
		if ($row == null)
			return null;
		
		return new Game($row);
	}
	
	public function QRY_game_get_all()
	{
		$rows = $this->provider->DB_select_list("SELECT * FROM games");

		$result = array();
		
		for ($i = 0; $i < count($rows); ++$i)
		{
			$result[$i] = new Game($rows[$i]);
		}
		
		return $result;
	}

	public function QRY_game_find_by_name($name)
	{
		$row = $this->provider->DB_select($this->statement_game_find_by_name, $name);
		
		if ($row == null)
			return null;
			
		return new Game($row);
	}
	
	public function QRY_game_exists($id)
	{
		$row = $this->provider->DB_select($this->statement_game_exists, $id);
		
		return $row['count'] > 0;
	}
	
	// game mode

	public function QRY_gamemode_exists($gameId, $gameMode)
	{
		$row = $this->provider->DB_select($this->statement_gamemode_exists, $gameId, $gameMode);

		return $row['count'] > 0;
	}

	public function QRY_gamemode_find_by_game($gameId)
	{
		$rows = $this->provider->DB_select_list("SELECT * FROM gamemodes WHERE game_id=[0]", $gameId);

		$result = array();
		
		for ($i = 0; $i < count($rows); ++$i)
		{
			$result[$i] = new GameMode($rows[$i]);
		}
		
		return $result;
	}

	// score
	
	public function QRY_score_add($score)
	{
		return $this->provider->DB_insert($this->statement_score_add,
			$score->user_id,
			$score->score,
			$score->user_name,
			$score->tag,
			$score->country_code,
			$score->hash,
			$score->game_id,
			$score->game_mode,
			$score->game_version,
			$score->date,
			$score->is_hacked);
	}
	
	public function QRY_score_get($scoreId)
	{
		$row = $this->provider->DB_select($this->statement_score_get, $scoreId);
		
		if ($row == null)
			return null;
		
		return new Score($row);
	}

	public function QRY_score_count($gameId, $gameMode)
	{
		$row = $this->provider->DB_select($this->statement_score_count, $gameId, $gameMode);

		return $row['count'];
	}

	public function QRY_score_count_hist($gameId, $gameMode, $hist)
	{
		$row = $this->provider->DB_select($this->statement_score_count_hist, $gameId, $gameMode, $hist);

		return $row['count'];
	}
	
	public function QRY_score_fetch_list($gameId, $gameMode, $rowBegin, $rowCount, $countryCode = null, $deviceId = null)
	{
		$sql = "SELECT * FROM " . $this->storeInfo->TBL_DATA;
		
		$sql .= " WHERE game_id=[0]";
		$sql .= " AND game_mode=[1]";
		
		if ($countryCode != null)
			$sql .= " AND country_code=[2]";
		if ($deviceId != null)
			$sql .= " AND user_id=[3]";

		$sql .= " ORDER BY score DESC, date";
			
		$sql .= " LIMIT " . intval($rowCount) . " OFFSET " . intval($rowBegin);
		
		$rows = $this->provider->DB_select_list($sql, $gameId, $gameMode, $countryCode, $deviceId, $rowBegin, $rowCount);
		
		$result = array();
		
		for ($i = 0; $i < count($rows); ++$i)
		{
			$result[$i] = new Score($rows[$i]);
		}
		
		return $result;
	}

	public function QRY_score_fetch_list_hist($gameId, $gameMode, $hist, $rowBegin, $rowCount, $countryCode = null, $deviceId = null)
	{
		$sql = "SELECT * FROM " . $this->storeInfo->TBL_DATA;
		
		$sql .= " WHERE game_id=[0]";
		$sql .= " AND game_mode=[1]";
		$sql .= " AND date >= DATE_SUB(NOW(), INTERVAL [2] DAY)";
		
		if ($countryCode != null)
			$sql .= " AND country_code=[3]";
		if ($deviceId != null)
			$sql .= " AND user_id=[4]";

		$sql .= " ORDER BY score DESC, date";
			
		$sql .= " LIMIT " . intval($rowCount) . " OFFSET " . intval($rowBegin);
		
		$rows = $this->provider->DB_select_list($sql, $gameId, $gameMode, $hist, $countryCode, $deviceId, $rowBegin, $rowCount);
		
		$result = array();
		
		for ($i = 0; $i < count($rows); ++$i)
		{
			$result[$i] = new Score($rows[$i]);
		}
		
		return $result;
	}
	
	public function QRY_score_fetch_rank($gameId, $gameMode, $scoreId)
	{
		$score = $this->QRY_score_get($scoreId);
		
		if ($score == null)
			throw new Exception('failed to get score');
		
		$row = $this->provider->DB_select($this->statement_score_fetch_rank, $gameId, $gameMode, $score->score, $score->date);
		
		return $row['count'];
	}

	public function QRY_score_fetch_rank_hist($gameId, $gameMode, $hist, $scoreId)
	{
		$score = $this->QRY_score_get($scoreId);
		
		if ($score == null)
			throw new Exception('failed to get score');
		
		$row = $this->provider->DB_select($this->statement_score_fetch_rank_hist, $gameId, $gameMode, $hist, $score->score, $score->date);
		
		return $row['count'];
	}
	
	public function QRY_score_fetch_rank_by_score($gameId, $gameMode, $score)
	{
		$row = $this->provider->DB_select($this->statement_score_fetch_rank, $gameId, $gameMode, $score, date('y-m-d'));
		
		return $row['count'];
	}
	
	public function QRY_score_fetch_best($gameId, $gameMode, $userId)
	{
		$row = $this->provider->DB_select($this->statement_score_fetch_best, $gameId, $gameMode, $userId);
		
		if ($row == null)
			throw new Exception('failed to find best score');
			
		return new Score($row);
	}

	public function QRY_score_fetch_best_hist($gameId, $gameMode, $hist, $userId)
	{
		$row = $this->provider->DB_select($this->statement_score_fetch_best_hist, $gameId, $gameMode, $hist, $userId);
		
		if ($row == null)
			throw new Exception('failed to find best score');
			
		return new Score($row);
	}
	
	public function QRY_score_fetch_best_rank($gameId, $gameMode, $userId)
	{
		try
		{
			$score = $this->QRY_score_fetch_best($gameId, $gameMode, $userId);
		
			$result = $this->QRY_score_fetch_rank($gameId, $gameMode, $score->id, $score->date);
			
			return $result;
		}
		catch (Exception $e)
		{
			return -1;
		}
	}

	public function QRY_score_fetch_best_rank_hist($gameId, $gameMode, $hist, $userId)
	{
		try
		{
			$score = $this->QRY_score_fetch_best_hist($gameId, $gameMode, $hist, $userId);
		
			$result = $this->QRY_score_fetch_rank_hist($gameId, $gameMode, $hist, $score->id, $score->date);
			
			return $result;
		}
		catch (Exception $e)
		{
			return -1;
		}
	}
	
	public function QRY_score_exists($gameId, $gameMode, $hash)
	{
		$row = $this->provider->DB_select($this->statement_score_exists, $hash);
		
		return $row['count'] == 1;
	}
	
	// crash_logs
	
	public function QRY_crash_log_add($crashLog)
	{
		return $this->provider->DB_insert($this->statement_crash_log_add,
			$crashLog->game_id,
			$crashLog->game_version,
			$crashLog->date,
			$crashLog->message,
			$crashLog->hash);
	}
	
	public function QRY_crash_log_exists($hash)
	{
		$row = $this->provider->DB_select($this->statement_crash_log_exists, $hash);
		
		return $row['count'] > 0;
	}

	public function QRY_stat_user_count()
	{
		$row = $this->provider->DB_select($this->statement_usercount_get);

		return $row['count'];
	}

	public function QRY_stat_user_legit_count()
	{
		$row = $this->provider->DB_select($this->statement_usercount_legit_get);

		return $row['count'];
	}

	public function QRY_stat_user_cracked_count()
	{
		$row = $this->provider->DB_select($this->statement_usercount_cracked_get);

		return $row['count'];
	}
	
	//
	
	private $provider = null;
	private $storeInfo = null;
	
	// store::game
	private $statement_game_get = null;
	private $statement_game_find_by_name = null;
	private $statement_game_exists = null;
	
	// store::gamemodes
	private $statement_gamemode_exists = null;
	
	// store::score
	private $statement_score_add = null;
	private $statement_score_get = null;
	private $statement_score_fetch_rank = null;
	private $statement_score_fetch_rank_hist = null;
	private $statement_score_fetch_best = null;
	private $statement_score_fetch_best_hist = null;
	private $statement_score_count = null;
	private $statement_score_count_hist = null;
	private $statement_score_exists = null;
	
	// store::crash_logs
	private $statement_crash_log_add = null;
	private $statement_crash_log_exists = null;

	// store::stats
	private $statement_usercount_get = null;
	private $statement_usercount_legit_get = null;
	private $statement_usercount_cracked_get = null;
}

?>
