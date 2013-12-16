<?php

require_once('db_provider.php');
require_once('news_types.php');

class NewsStore
{
	private $provider = null;
	private $sql_newsitem_get_all = null;
	private $sql_newsitem_add = null;

	public function __construct($provider)
	{
		$this->provider = $provider;

		$this->DB_create_statements();
	}

	private function DB_create_statements()
	{
		$this->sql_newsitem_get_all = "SELECT * FROM news_items ORDER BY date DESC";
		$this->sql_newsitem_add = "INSERT INTO news_items (title, author, date, text) VALUES([0], [1], [2], [3])";
	}

	public function QRY_newsitem_get_all()
	{
		$rows = $this->provider->DB_select_list($this->sql_newsitem_get_all);

		$result = array();
		
		for ($i = 0; $i < count($rows); ++$i)
		{
			$result[$i] = new NewsItem($rows[$i]);
		}
		
		return $result;
	}

	public function QRY_newsitem_add($newsItem)
	{
		$this->provider->DB_insert($this->sql_newsitem_add, $newsItem->headline, $newsItem->author, $newsItem->date, $newsItem->text);
	}
}
