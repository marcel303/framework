<?php

require_once('news_store.php');

function Render_Page()
{
	if (!IsPrivileged())
	{
		LOG_err('user not privileged to access page');
		return;
	}

	$action = Arg('action', '');

	switch ($action)
	{
		case '':
		{
			break;
		}
		case 'news_item_add':
		{
			$headline = Arg('news_item_headline', null);
			$author = Arg('news_item_author', null);
			$date = Arg('news_item_date', null);
			$text = Arg('news_item_text', null);

			if ($headline === null || $author === null || $date === null || $text === null)
			{
				LOG_err('one or more required arguments not set');
			}
			else
			{
				if ($headline == '' || $author == '' || $date == '' || $text == '')
				{
					LOG_wrn('one or more arguments is empty');
				}

				$providerInfo = new ProviderInfo();
				$provider = new Provider($providerInfo);
				$provider->DB_connect();
				$store = new NewsStore($provider);

				$newsItem = new NewsItem(null);
				$newsItem->date = $date;
				$newsItem->author = $author;
				$newsItem->headline = $headline;
				$newsItem->text = $text;

				$store->QRY_newsitem_add($newsItem);

				LOG_inf('news item added');
			}
			break;
		}
	}

	TPL_Output(file_get_contents('cms_forms.txt'), array('date' => date('Y-m-d G:i:s')));
}

Render_Page();

?>