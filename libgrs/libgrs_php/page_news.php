<?php

function DaysBetween($start, $time)
{
	$timestamp = strtotime($start);
	$diff = $time - $timestamp;

	return round($diff / 86400);
}

function Render_Page()
{
	$providerInfo = new ProviderInfo();
	$provider = new Provider($providerInfo);
	$provider->DB_connect();
	$store = new NewsStore($provider);

	$tpl_news_item = TPL_FromFile('tpl_news_item.txt');

	$items = $store->QRY_newsitem_get_all();

	foreach ($items as $item)
	{
		TPL_Output($tpl_news_item,
			array(
				'date' => $item->date,
				'age' => DaysBetween($item->date, time()),
				'author' => $item->author,
				'title' => $item->title,
				'text' => $item->text));
	}
}

Render_Page();

?>
