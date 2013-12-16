<?php

//error_reporting(0);

require_once('libgrs.php');

define('HACKS', false);

// read HTTP body

/*

GRS server

- communicates over HTTP POST
- messages are base64 encoded
- messages are SHA2 encrypted
- requests are encoded using PHP's string format

request string:

	request = submit | query
	
	submit:
		score = <score complex type>
	query
		query = <query complex type>
	
response string:

	result = ok | error
	score = <score complex type>
	
score complex type:
	
	name = <string>
	score = <real>

query complex type:
	
*/

class ServerQuery_List
{
	public $gameId = null;
	public $gameMode = null;
	public $hist = null;
	public $userId = null;
	public $rowBegin = null;
	public $rowCount = null;
	public $countryCode = null;
	public $deviceId = null;
}

class ServerQuery_Rank
{
	public $gameId = null;
	public $gameMode = null;
	public $score = null;
}

class Server
{
	public function __construct()
	{
		try
		{
			$this->grs = new GRS();
		}
		catch (Exception $e)
		{
			$this->HandleError(null, $e->getMessage());
		}
	}

	public function __destruct()
	{
		try
		{
			$this->grs = null;
		}
		catch (Exception $e)
		{
			$this->HandleError(null, $e->getMessage());
		}
	}
	
	function str2hex($string)
	{
	    $result = '';
	    
	    for ($i = 0; $i < strlen($string); ++$i)
	    {
	        $result .= dechex(ord($string[$i]));
	    }
	    
	    return $result;
	}
	
	function GetMessage()
	{
		global $_POST;
		
		if (!isset($_POST['message']))
		{
			$this->HandleError(null, 'expected \'message\'');
		}
		
		return $_POST['message'];
	}
	
	function HandleError($request, $text)
	{
		if ($request === null)
			$request = 'unknown';
		
		$writer = new XMLWriter();
		$writer->openMemory();
		$writer->setIndent(true); 
		
		$writer->startElement('message');
		$writer->writeAttribute('request', $request);
		$writer->writeAttribute('response', 'error');
		$writer->endElement();

		$xml = $writer->outputMemory();
		
		//die($xml);
		die('error: request=' . $request . ', text=' . $text);
	}
	
	public function HandleMessage($message)
	{
		try
		{

		if (HACKS)
		{
			$message = $this->grs->EC_encrypt($message);
			$message = $this->grs->ENC_encode($message);
		}
		
		$message = trim($message);
		
		// decode message
		
		$message = $this->grs->ENC_decode($message);
		
		// todo: check message integrity
		
		// calculate hash, will be stored in database
		
		$hash = $this->grs->HA_hash($message);
		
		// decrypt message
		
		$message = $this->grs->EC_decrypt($message);
		
		//echo('received=' . $message);
		//echo('received_hex=' . $this->str2hex($message));
		
		if (strlen($message) > 1000)
			$this->HandleError(null, 'message length exceeds max');
		
		$message = trim($message);
		
		$reader = new XMLReader();
		
		if (!$reader->XML($message))
			$this->HandleError(null, 'unable to parse XML');
		
		// parse request
		
		if (!$reader->read())
			$this->HandleError(null, 'unable to read from XML');
		
		if ($reader->name != "message")
			$this->HandleError(null, 'expected \'message\' tag. got: ' . $reader->name);
		
		$request = $reader->getAttribute("request");
		
		switch ($request)
		{
			case "submit_score":
			{
				// read score
				
				$reader->read();
				
				$score = $this->ReadScore($reader);
				
				$score->hash = $hash;
				
				// store score in database
				
				$this->grs->DM_score_add($score);
				
				// fetch rank and best rank
				
				$curr_rank = $this->grs->store->QRY_score_fetch_rank_hist($score->game_id, $score->game_mode, $score->history, $score->id);
				$best_rank = $this->grs->store->QRY_score_fetch_best_rank_hist($score->game_id, $score->game_mode, $score->history, $score->user_id);
				
				// write response
				
				$writer = new XMLWriter();
				$writer->openMemory();
				$writer->setIndent(true); 
				
				$writer->startElement('message');
				$writer->writeAttribute('request', $request);
				$writer->writeAttribute('response', 'ok');
				
				$writer->startElement('submit');
				$writer->writeAttribute('curr_rank', $curr_rank);
				$writer->writeAttribute('best_rank', $best_rank);
				$writer->writeAttribute('game_mode', $score->game_mode);
				$writer->endElement();
				
				$writer->endElement();
				
				$xml = $writer->outputMemory();
				
				echo($xml);
				
				break;
			}
			
			case "query_list":
			{
				// read query
				
				$reader->read();
				
				$query = $this->ReadListQuery($reader);
				
				// retrieve results from database
							
				// todo: use DM function - check game exists (?)

				$scoreCount = $this->grs->store->QRY_score_count($query->gameId, $query->gameMode);
				$rows = $this->grs->store->QRY_score_fetch_list($query->gameId, $query->gameMode, $query->rowBegin, $query->rowCount, $query->countryCode, $query->deviceId);
				
				// fetch best rank
				
				$best_rank = $this->grs->store->QRY_score_fetch_best_rank($query->gameId, $query->gameMode, $query->userId);
				
				// write response
				
				$writer = new XMLWriter();
				$writer->openMemory();
				$writer->setIndent(true); 
		        
				$writer->startElement('message');
				$writer->writeAttribute('request', $request);
				$writer->writeAttribute('response', 'ok');
				
				$writer->startElement('query');
				$writer->writeAttribute('game_mode', $query->gameMode);
				$writer->writeAttribute('score_count', $scoreCount);
				$writer->writeAttribute('begin', $query->rowBegin);
				$writer->writeAttribute('best_rank', $best_rank);
				$writer->endElement();
				
				foreach ($rows as $row)
				{
					$writer->startElement('score');
					
					// todo: may want to return score id
					
					$writer->writeAttribute('score', (int)$row->score);
					$writer->writeAttribute('user_name', $row->user_name);
					$writer->writeAttribute('tag', $row->tag);
					
					$writer->endElement();
				}
				
				$writer->endElement();
				
				$xml = $writer->outputMemory();
				
				echo($xml);
				
				break;
			}

			case "query_list_hist":
			{
				// read query
				
				$reader->read();
				
				$query = $this->ReadListHistQuery($reader);
				
				// retrieve results from database
							
				// todo: use DM function - check game exists (?)

				$scoreCount = $this->grs->store->QRY_score_count_hist($query->gameId, $query->gameMode, $query->hist);
				$rows = $this->grs->store->QRY_score_fetch_list_hist($query->gameId, $query->gameMode, $query->hist, $query->rowBegin, $query->rowCount, $query->countryCode, $query->deviceId);
				
				// fetch best rank
				
				$best_rank = $this->grs->store->QRY_score_fetch_best_rank_hist($query->gameId, $query->gameMode, $query->hist, $query->userId);
				
				//

				$request = 'query_list';

				// write response
				
				$writer = new XMLWriter();
				$writer->openMemory();
				$writer->setIndent(true); 
		        
				$writer->startElement('message');
				$writer->writeAttribute('request', $request);
				$writer->writeAttribute('response', 'ok');
				
				$writer->startElement('query');
				$writer->writeAttribute('game_mode', $query->gameMode);
				$writer->writeAttribute('score_count', $scoreCount);
				$writer->writeAttribute('begin', $query->rowBegin);
				$writer->writeAttribute('best_rank', $best_rank);
				$writer->endElement();
				
				foreach ($rows as $row)
				{
					$writer->startElement('score');
					
					// todo: may want to return score id
					
					$writer->writeAttribute('score', (int)$row->score);
					$writer->writeAttribute('user_name', $row->user_name);
					$writer->writeAttribute('tag', $row->tag);
					
					$writer->endElement();
				}
				
				$writer->endElement();
				
				$xml = $writer->outputMemory();
				
				echo($xml);
				
				break;
			}
			
			case "query_rank":
			{
//if ((rand() % 3) != 0) die();

				// read query
				
				$reader->read();
				
				$query = $this->ReadRankQuery($reader);
				
				// rank score
				
				$position = $this->grs->store->QRY_score_fetch_rank_by_score($query->gameId, $query->gameMode, $query->score);
				
				// write response
				
				$writer = new XMLWriter();
				$writer->openMemory();
				$writer->setIndent(true); 
		        
				$writer->startElement('message');
				$writer->writeAttribute('request', $request);
				$writer->writeAttribute('response', 'ok');
				
				$writer->startElement('rank');
				$writer->writeAttribute('position', $position);
				$writer->endElement();
				
				$writer->endElement();
				
				$xml = $writer->outputMemory();
				
				echo($xml);

				break;
			}
			
			case "submit_crashlog":
			{
				// read crash log
				
				$reader->read();
				
				$crashLog = $this->ReadCrashLog($reader);

				$crashLog->hash = $hash;
				
				// store crash log in database
				
				$this->grs->DM_crash_log_add($crashLog);
				
				// write response
				
				$writer = new XMLWriter();
				$writer->openMemory();
				$writer->setIndent(true); 
		        
				$writer->startElement('message');
				$writer->writeAttribute('request', $request);
				$writer->writeAttribute('response', 'ok');
				$writer->endElement();
				
				$xml = $writer->outputMemory();
				
				echo($xml);
				
				break;
			}
			
			default:
			{
				$this->HandleError($request, 'unknown request type');
			}
		}
		
		$reader->close();

		}
		catch (Exception $e)
		{
			$this->HandleError(null, $e->getMessage());
		}
	}

	private function CheckNonEmpty($value)
	{
		if ($value == '')
			$this->HandleError(null, 'value is empty, which is not allowed');
	}
		
	private function ReadScore($reader)
	{
		$score = new Score(null);
		
		$score->user_id = $reader->getAttribute('user_id');
		$score->score = $reader->getAttribute('score');
		$score->user_name = $reader->getAttribute('user_name');
		$score->tag = $reader->getAttribute('tag');
		$score->country_code = $reader->getAttribute('country_code');
		$score->game_id = $reader->getAttribute('game_id');
		$score->game_mode = $reader->getAttribute('game_mode');
		$score->game_version = $reader->getAttribute('game_version');
		$score->date = $reader->getAttribute('date');
		$score->is_hacked = $reader->getAttribute('is_hacked');
		$score->history = $reader->getAttribute("history");
		
		$this->CheckNonEmpty($score->user_id);
		$this->CheckNonEmpty($score->score);
		$this->CheckNonEmpty($score->user_name);
		$this->CheckNonEmpty($score->country_code);
		$this->CheckNonEmpty($score->game_id);
		$this->CheckNonEmpty($score->game_mode);
		$this->CheckNonEmpty($score->game_version);
		$this->CheckNonEmpty($score->date);
		if ($score->is_hacked == '')
			$score->is_hacked = false;
		if ($score->history == '')
			$score->history = 1000;
		
		if ($score->is_hacked)
		{
			$score->score = -1337;
		}

		return $score;
	}
	
	private function ReadListQuery($reader)
	{
		$query = new ServerQuery_List();
		
		$query->gameId = $reader->getAttribute('game_id');
		$query->gameMode = $reader->getAttribute('game_mode');
		$query->userId = $reader->getAttribute('user_id');
		$query->rowBegin = $reader->getAttribute('row_begin');
		$query->rowCount = $reader->getAttribute('row_count');
		$query->countryCode = $reader->getAttribute('country_code');
		$query->deviceId = $reader->getAttribute('device_id');
		
		$this->CheckNonEmpty($query->gameId);
		$this->CheckNonEmpty($query->gameMode);
		$this->CheckNonEmpty($query->rowBegin);
		$this->CheckNonEmpty($query->rowCount);
		
		if ($query->countryCode == '')
			$query->countryCode = null;
		if ($query->deviceId == '')
			$query->deviceId = null;
		
		return $query;
	}

	private function ReadListHistQuery($reader)
	{
		$query = $this->ReadListQuery($reader);

		$query->hist = $reader->getAttribute('hist');
		
		$this->CheckNonEmpty($query->hist);
		
		return $query;
	}
	
	private function ReadRankQuery($reader)
	{
		$query = new ServerQuery_Rank();
		
		$query->gameId = $reader->getAttribute('game_id');
		$query->gameMode = $reader->getAttribute('game_mode');
		$query->score = $reader->getAttribute('score');
		
		$this->CheckNonEmpty($query->gameId);
		$this->CheckNonEmpty($query->gameMode);
		$this->CheckNonEmpty($query->score);
		
		return $query;
	}
	
	private function ReadCrashLog($reader)
	{
		$crashLog = new CrashLog(null);
		
		$crashLog->game_id = $reader->getAttribute('game_id');
		$crashLog->game_version = $reader->getAttribute('game_version');
		$crashLog->date = $reader->getAttribute('date');
		$crashLog->message = $reader->getAttribute('message');
		
		$this->CheckNonEmpty($crashLog->game_id);
		$this->CheckNonEmpty($crashLog->game_version);
		$this->CheckNonEmpty($crashLog->date);
		$this->CheckNonEmpty($crashLog->message);
		
		return $crashLog;
	}
	
	private $grs = null;
}

function HandlePOST()
{
	$server = new Server();
	
	$message = $server->GetMessage();

	syslog(LOG_DEBUG, "received: $message");
	
	$server->HandleMessage($message);

	$server = null;
}

//print_r($_POST);

//print_r(mcrypt_list_modes());
//print_r(mcrypt_list_algorithms());

if (HACKS)
{
	//$_POST['message'] = '<message request="submit"><score user_id="xx-yy-zz" catagory="usg.arcade" score="10.0f" user_name="testuser" tag="tag" country_code="NL" game_id="0" /></message>';
	//$_POST['message'] = '<message request="query"><filter game_id="0" catagory="usg.arcade" row_begin="0" row_count="10" /></message>';
	$_POST['message'] = '<message request="query"><filter game_id="0" game_mode="0" row_begin="0" row_count="100" /></message>';
}

HandlePOST();

?>
