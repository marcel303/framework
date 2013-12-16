<?php

require_once('db_provider.php');
require_once('grs_store.php');

define ('ENCRYPTION_ALGO', MCRYPT_RIJNDAEL_128);
define ('ENCRYPTION_MODE', MCRYPT_MODE_CBC);

/*

GRS - Game Ranking System

Features:
	- DB backend:
		- fields: game ID, game version, date, device ID, catagory, score, name, tag (free field), country ISO, hash
		- driver: MySQL
	- Post:
		- Submission over HTTP
		- Encryption to obscure format
		- Hashing to prevent alterations
		- Hashes stored in DB, to prevent replay attacks
	- Request:
		- Request high scores
		- filters: game ID + any of: device ID, catagory, country ISO, count limit, page number
	- Serialization:
		- Basic key-value pairs
		- ASCII to hexadecimal
	- Viewing:
		- built on top of request feature
		- supports pagination
		
	TODO: support UNICODE?

	Security considerations:
	- Spoofing -> use private key to encrypt and hash
	- Alteration -> use private key to encrypt and hash
	- Replay attacks -> store hash in DB and detect duplicate submissions
	- DoS by requesting large queries -> validate count limit
*/

class Settings
{
	public function __construct()
	{
		$this->PROVIDER_INFO = new ProviderInfo();
		$this->STORE_INFO = new StoreInfo();
	}
	
	public $PROVIDER_INFO = null;
	public $STORE_INFO = null;

	public $EC_KEY = "1234567890123456";
}

class GRS
{
	public function __construct()
	{
		$this->settings = new Settings();
		
		$this->provider = new Provider($this->settings->PROVIDER_INFO);
		
		$this->provider->DB_connect();
		
		$this->store = new Store($this->provider, $this->settings->STORE_INFO);
	}
	
	// --------------------
	// encoding
	// --------------------
	public function ENC_encode($string)
	{
		$result = $string;
		
		// encode base64
		
		$result = base64_encode($result);
		
		if (!$result)
			throw new Exception('failed to encode base64');
		
		// add http transform
		
		$result = str_replace("+", "-", $result);
		$result = str_replace("/", "_", $result);
		
		return $result;
	}
	
	public function ENC_decode($string)
	{
		$result = $string;
		
		// add http transform
		
		$result = str_replace("-", "+", $result);
		$result = str_replace("_", "/", $result);
		
		// decode base64
		
		$result = base64_decode($result);
		
		if (!$result)
			throw new Exception('failed to decode base64');
		
		return $result;
	}
		
	// --------------------
	// hashing
	// --------------------
	
	public function HA_hash($string)
	{
		return md5($string);
	}
	
	// --------------------
	// encryption
	// --------------------
	
	private function EC_RandomIV()
	{
	    return "1234567890123456";
	}
	
	public function EC_encrypt($string)
	{
		return mcrypt_encrypt(ENCRYPTION_ALGO, $this->settings->EC_KEY, $string, ENCRYPTION_MODE, $this->EC_RandomIV());
	}
	
	public function EC_decrypt($string)
	{
		return rtrim(mcrypt_decrypt(ENCRYPTION_ALGO, $this->settings->EC_KEY, $string, ENCRYPTION_MODE, $this->EC_RandomIV()), "\0");
	}
	
	// --------------------
	// data management
	// --------------------
	
	public function DM_score_add($score)
	{
		// validate game exists
		
		if (!$this->store->QRY_gamemode_exists($score->game_id, $score->game_mode))
			throw new Exception('invalid game id and/or mode');
		
		// todo: validate date OK
		
		// validate duplicate score does not exist
		
		if ($this->store->QRY_score_exists($score->game_id, $score->game_mode, $score->hash))
			throw new Exception('duplicate entry');
		
		// insert score
		
		$score->id = $this->store->QRY_score_add($score);
	}
	
	public function DM_crash_log_add($crashLog)
	{
		// validate duplicate crash log does not exist
		
		if ($this->store->QRY_crash_log_exists($crashLog->hash))
			throw new Exception('duplicate entry');
		
		// todo: validate date OK
		
		// insert crash log
		
		$crashLog->id = $this->store->QRY_crash_log_add($crashLog);
	}
	
	// --------------------
	// member variables
	// --------------------
	
	private $settings = null;
	private $provider = null;
	public $store = null;
}

function GRS_SelfTest()
{
	$grs = new GRS();
	
	// test fetching score lists
	
	for ($i = 0; $i < 1; ++$i)
	{
		$game = $grs->store->QRY_game_find_by_name("Unnamed Space Game");
		
		if ($game == null)
			echo("game does not exist\n");
		else
		{
			echo("game id: " . $game->id . "\n");
			
			$scores = $grs->store->QRY_score_fetch_list($game->id, "usg.arcade", 0, 1000, null, null);
			
			for ($j = 0; $j < count($scores); ++$j)
			{
				$score = $scores[$j];
				
				echo("score id: " . $score->id . "\n");
			}
		}
	}
	
	// test encryption / decryption
	
	$text = "Hello World";
	
	$text = $grs->EC_encrypt($text);
	$text = $grs->EC_decrypt($text);
	
	echo($text . '\n');
	
	// test score statistics
	
	$gameId = 0;
	$gameMode = 0;
	$userId = '65E23152-EE37-5DED-9678-8671446B9611';
	
	$bestScore = $grs->store->QRY_score_fetch_best($gameId, $gameMode, $userId);
	$bestRank = $grs->store->QRY_score_fetch_rank($gameId, $gameMode, $bestScore->id);
	
	echo('best score: ' . $bestScore->score . '\n');
	echo('best rank: ' . $bestRank . '\n');
	
	// test score insertion
	
	echo('score: create\n');
	
	$score = new Score(null);
	
	$score->game_id = 0;
	$score->game_mode = 0;
	$score->game_version = 0;
	$score->date = 55555;
	$score->user_id = '65E23152-EE37-5DED-9678-8671446B9611';
	$score->score = 100.0;
	$score->user_name = 'testuser2';
	$score->tag = 'blaat2';
	$score->country_code = 'nl';
	$score->hash = rand();
	
	echo('score: DM_add\n');
	
	$grs->DM_score_add($score);
	
	echo('score: id=' . $score->id);
	
	echo('score: QRY_get\n');
	
	$score = $grs->store->QRY_score_get($score->id);
	
	echo('score: id=' . $score->id);
	
	echo('score: QRY_fetch_best\n');
	
	$score = $grs->store->QRY_score_fetch_best($gameId, $gameMode, $score->user_id);
	
	echo('score: id=' . $score->id);

	// test gamemode statements

	$gameModeExists = $grs->store->QRY_gamemode_exists($gameId, $gameMode);

	echo('game mode exists: ' . $gameModeExists);
}

?>
