<?php

function Render_Page()
{
	$tpl_screenshot = TPL_FromFile('tpl_screenshot.txt');

	TPL_Write(file_get_contents('game_usg.txt'));
	TPL_Write('<br />');

//	TPL_Write('<b>Gameplay trailer</b>');
//	TPL_Write('<br />');
//	TPL_Write('<object width="500" height="320"><param name="movie" value="http://www.youtube.com/v/wCIiPh0627E&hl=nl_NL&fs=1&hd=1"></param><param name="allowFullScreen" value="true"></param><param name="allowscriptaccess" value="always"></param><embed src="http://www.youtube.com/v/wCIiPh0627E&hl=nl_NL&fs=1&hd=1" type="application/x-shockwave-flash" allowscriptaccess="always" allowfullscreen="true" width="500" height="320"></embed></object>');
	TPL_Write(TextForVideo('/files/critwave_gameplay.mov', '/files/critwave_gameplay.jpg', 480, 320));
	TPL_Write('<br />');
	TPL_Write('<br />');
	TPL_Write('<br />');
	TPL_Write('<br />');

	TPL_Write('Screenshots');
	TPL_Write('<div>');
	TPL_Output($tpl_screenshot, array('url' => 'gallery/critwave/01.png'));
	TPL_Output($tpl_screenshot, array('url' => 'gallery/critwave/09.png'));
	TPL_Output($tpl_screenshot, array('url' => 'gallery/critwave/10.png'));
	TPL_Output($tpl_screenshot, array('url' => 'gallery/critwave/05.png'));
	TPL_Output($tpl_screenshot, array('url' => 'gallery/critwave/08.png'));
	TPL_Output($tpl_screenshot, array('url' => 'gallery/critwave/11.png'));
	TPL_Output($tpl_screenshot, array('url' => 'gallery/critwave/07.png'));
	TPL_Write('</div>');
}

Render_Page();

?>
