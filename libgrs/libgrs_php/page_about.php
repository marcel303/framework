<?php

function Render_Page()
{
	TPL_Write(file_get_contents('page_about.txt'));
}

Render_Page();

?>
