<?php

function Render_Page()
{
	$self = $_SERVER['PHP_SELF'];

	// parse page arguments

	$gameId = Arg_Int('game', 0, 0, INT_MAX);
	$gameMode = Arg_Int('mode', 0, 0, INT_MAX);
	$pageIndex = Arg_Int('page_index', 0, 0, INT_MAX);
	$pageSize = Arg_Int('page_size', 20, 1, 1000);
	
	// load templates

	$tpl_score_table_begin = TPL_FromFile('tpl_score_table_begin.txt');
	$tpl_score_table_row = TPL_FromFile('tpl_score_table_row.txt');
	$tpl_score_table_end = TPL_FromFile('tpl_score_table_end.txt');

	// setup view controller

	$view = new View($gameId, $gameMode);
	$game = $view->Game_get();
	$hist = Arg('hist', 7);

	// render view specific elements
	
	switch ($view)
	{
		default:
		{
			$games = $view->Game_get_all();

			foreach ($games as $game)
			{
				TPL_Write("<h3>" . $game->name . "</h3>");
				TPL_Write("<ul>");
				if ($game->id == $view->GameId_get())
				{
					$modes = $view->GameMode_get_all($game->id);

					foreach ($modes as $mode)
					{
						if (!DEBUG && $mode->debug)
							continue;

						TPL_Output("<li><a href=\"[url]\">[name]</a></li>", array("url" => $self . "?game=" . $game->id . "&amp;mode=" . $mode->mode, "name" => $mode->name));
					}
				}
				TPL_Write("</ul>");
			}

			TPL_Write('<div>View: <a href="' . $self . '?hist=10000">All-time</a> | <a href="' . $self . '?hist=7">Past 7 days</a> | <a href="' . $self . '?hist=30">Past 30 days</a></div>');

			$info_Game = array("game_name" => $game->name, "hist" => $hist);
			
			if (!$view->GameMode_exists($gameId, $gameMode))
			{
				LOG_wrn('game mode does not exist');
			}

			$scores = $view->SelectHist($gameMode, $hist, $pageIndex, $pageSize);
			
			TPL_Output($tpl_score_table_begin, $info_Game);
			
			$rank = $pageIndex * $pageSize + 1;
			
			foreach ($scores as $score)
			{
				$flag = '';

				if ($score->country_code != 'nl' && $score->country_code != 'xx')
					$flag = '<img src="images/flags/' . strtolower($score->country_code) . '.png"/>';

				$rank2 = '';

				if (DEBUG)
				{
					$rank2 = $view->Store_get()->QRY_score_fetch_rank_hist($score->game_id, $score->game_mode, $hist, $score->id);
				}

				TPL_Output($tpl_score_table_row,
					array(
						"flag" => $flag,
						"rank" => $rank,
						"rank2" => $rank2 + 1,
						"user_id" => htmlentities($score->user_id),
						"user_name" => htmlentities($score->user_name),
						"score" => (int)$score->score,
						"is_hacked" => $score->is_hacked,
						"date" => $score->date,
					));
				
				$rank++;
			}
			
			$prev = $pageIndex - 1;
			$next = $pageIndex + 1;

			if ($prev < 0)
				$prev = 0;
			if (count($scores) < $pageSize)
				$next = $pageIndex;

			TPL_Output($tpl_score_table_end, array("prev" => $self . "?page_index=" . $prev, "next" => $self . "?page_index=" . $next));

			$stats = $view->Stats_get();

			if (DEBUG)
			{
				TPL_Output('<div>users: [user_count], users_legit: [user_legit_count], users_cracked: [user_cracked_count]</div>', $stats);
			}
			
			break;
		}
	}
}

Render_Page();

?>