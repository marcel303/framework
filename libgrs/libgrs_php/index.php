 <?php

define('GGROOT', './');

require_once('grs_view.php');
require_once('news_store.php');
require_once('pagesystem.php');

function Main()
{
	$self = $_SERVER['PHP_SELF'];

	if (IsPrivileged())
		define('DEBUG', 1);
	else
		define('DEBUG', 0);
		
	$pageToFile =
		array(
			'news' => 'page_news.php',
			'games' => 'page_games.php',
			'scores' => 'page_scores.php',
			'about' => 'page_about.php',
			'cms' => 'page_cms.php',
			'login' => 'page_login.php',
			'debug' => 'page_debug.php',
			'contact' => 'page_contact.php'
		);
	
	$page = Arg('page', 'news');
	
	// render page header
	
	RenderHeader();
	
	// render page contents
	
	if (!array_key_exists($page, $pageToFile))
	{
		LOG_err('page not found');
	}
	else
	{
		$pageFile = $pageToFile[$page];
		
		if (!file_exists($pageFile))
		{
			LOG_err('page content not found: ' . $page);
		}
		else
		{
			include($pageFile);
		}
	}

	// render page footer
	
	RenderFooter();
	
	// output HTML
	
	TPL_Render();
}

function RenderHeader()
{
	// load templates

	$tpl_page_header = TPL_FromFile('tpl_page_header.txt');
	$tpl_mb_g = TPL_FromFile('tpl_mb_g.txt');
	$tpl_mb_y = TPL_FromFile('tpl_mb_y.txt');

	// render menu buttons
	
	$menuButton_News = array("text" => "NEWS", "page" => "news", "width" => 100);
	$menuButton_Games = array("text" => "GAMES", "page" => "games", "width" => 100);
	$menuButton_Highscores = array("text" => "HIGHSCORES", "page" => "scores", "width" => 100);
	$menuButton_Contact = array("text" => "CONTACT", "page" => "contact", "width" => 100);
	$menuButton_About = array("text" => "ABOUT", "page" => "about", "width" => 100);

	$menuButton_News = TPL_Format($tpl_mb_g, $menuButton_News);
	$menuButton_Games = TPL_Format($tpl_mb_g, $menuButton_Games);
	$menuButton_Highscores = TPL_Format($tpl_mb_g, $menuButton_Highscores);
	$menuButton_Contact = TPL_Format($tpl_mb_g, $menuButton_Contact);
	$menuButton_About = TPL_Format($tpl_mb_g, $menuButton_About);

	// render header

	$info_Header = array(
		"button_news" => $menuButton_News,
		"button_games" => $menuButton_Games,
		"button_highscores" => $menuButton_Highscores,
		"button_contact" => $menuButton_Contact,
		"button_about" => $menuButton_About);

	TPL_Output($tpl_page_header, $info_Header);

	// render privileged options

	if (IsPrivileged())
	{
		TPL_Write('<div style="position:absolute;left:0px;top:0px"><a href="?page=debug">DEBUG</a> | <a href="?page=cms">CMS</a></div>');
	}
}

function RenderFooter()
{
	// load templates

	$tpl_page_footer = TPL_FromFile('tpl_page_footer.txt');

	// render footer

	TPL_Output($tpl_page_footer, array("page_time" => sprintf("%.5f", TPL_GetPageTime())));
}

function TextForVideo($movieUri, $posterUri, $width, $height)
{
return
	'<!-- Begin VideoJS -->' .
	'<div class="video-js-box">' .
		'<!-- Using the Video for Everybody Embed Code http://camendesign.com/code/video_for_everybody -->' .
		'<video id="example_video_1" class="video-js" width="' . $width . '" height="' . $height . '" controls="controls" preload="auto" poster="' . $posterUri . '">' .
		'<source src="' . $movieUri . '" type=\'video/mp4; codecs="avc1.42E01E, mp4a.40.2"\' />' .
		'<!-- Flash Fallback. Use any flash video player here. Make sure to keep the vjs-flash-fallback class. -->' .
		'<object id="flash_fallback_1" class="vjs-flash-fallback" width="' . $width . '" height="' . $height . '" type="application/x-shockwave-flash"' .
			'data="http://releases.flowplayer.org/swf/flowplayer-3.2.1.swf">' .
			'<param name="movie" value="http://releases.flowplayer.org/swf/flowplayer-3.2.1.swf" />' .
			'<param name="allowfullscreen" value="true" />' .
			'<param name="flashvars" value=\'config={"playlist":["' . $posterUri . '", {"url": "' . $movieUri . '","autoPlay":false,"autoBuffering":true}]}\' />' .
			'<!-- Image Fallback. Typically the same as the poster image. -->' .
			'<img src="' . $posterUri . '" width="' . $width . '" height="' . $height . '" alt="Poster Image"' .
				'title="No video playback capabilities." />' .
		'</object>' .
		'</video>' .
		'<!-- Download links provided for devices that can\'t play video in the browser. -->' .
		'<p class="vjs-no-video"><strong>Download Video:</strong>' .
		'<a href="' . $movieUri . '">MOV</a>' .
		'<!-- Support VideoJS by keeping this link. -->' .
		'| <a href="http://videojs.com">HTML5 Video Player</a> by VideoJS' .
		'</p>' .
	'</div>' .
	'<!-- End VideoJS -->';
}

Main();
