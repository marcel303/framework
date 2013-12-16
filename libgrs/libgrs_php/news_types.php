<?php

class NewsItem
{
	public function __construct($row)
	{
		if ($row != null)
		{
			$this->id = $row["id"];
			$this->date = $row["date"];
			$this->author = $row["author"];
			$this->title = $row["title"];
			$this->text = $row["text"];
		}
	}
	
	public $id = null;
	public $date = null;
	public $author = null;
	public $title = null;
	public $text = null;
}

?>