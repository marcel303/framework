<?php

include('Mail.php');

function SendMail()
{
	$target = 'granniesgames@gmail.com';

	$dateNow = date("d.m.Y H:i"); 
      
	$ip = $_SERVER['REMOTE_ADDR']; 
      
	$body = "===================================================\n"; 
	$body .= "Grannies-Games Contact Form\n"; 
	$body .= "===================================================\n\n"; 
    
	$body .= $_SERVER['PHP_SELF'] . "\n\n";

	$body .= "Name: " . Arg('email_name', '(not filled in)') . "\n"; 
	$body .= "E-mail adress: " . Arg('email_from', '(not filled in)') . "\n"; 
	$body .= "Message:\n"; 
	$body .= Arg('email_body', '(not filled in)') . "\n\n"; 
      
	$body .= "Sent @ " . $dateNow . ", from IP " . $ip . "\n\n"; 
      
	$body .= "===================================================\n\n"; 

	$body = wordwrap($body, 70);
    
	// -------------------- 
	// spambot protectie 
	// ------ 
	// van de tutorial: http://www.phphulp.nl/php/tutorials/10/340/ 
	// ------ 
	
	$subject = Arg('email_subject', '(not filled in)');
	$subject = str_replace("\n", "", $subject); // Remove \n 
	$subject = str_replace("\r", "", $subject); // Remove \r 
	$subject = str_replace("\"", "\\\"", str_replace("\\", "\\\\", $subject)); // Slashes from quotes 

	$from = Arg('email_from', 'anonymous@anonymous.org');

//	$result = mail($target, $subject, $body, $headers);
//	$result = mail($target, $subject, $body);
//	mail("granniesgames@gmail.com", "test", "hello world");

	$mail = Mail::factory('smtp', array('host' => '127.0.0.1', 'auth' => false));
	$mailResult = $mail->send($target, array('From' =>  $from, 'To' => $target, 'Subject' => $subject), $body);

//	TPL_Write($target);
//	TPL_Write($from);
//	TPL_Write($body);

	if (!PEAR::isError($mailResult))
	{
		LOG_inf('your mail has been sent'); 
	}
	else
	{
		LOG_err('an error occurred while sending your mail');
	}
}

function Render_Page()
{
	$submit = Arg('submit', '');

	if ($submit != '')
	{
		SendMail();
	}

	TPL_Write('<h1>Contact</h1>');
	TPL_Write('Please let us know what you think about our games. Don\'t hesitate to mail us!');

//	TPL_Write('<br />');
//	TPL_Write('<br />');
//	TPL_Write('<div style="background:white;color:black">Our contact form is temporarily out-of-order! Please contact us directly at <a href="mailto://granniesgames@gmail.com" style="color:blue">granniesgames@gmail.com</a>.</div>');

	TPL_Write('<br />');
	TPL_Write('<br />');

	TPL_Write('<form method="POST">');
	TPL_Write('<h2>Mail form</h2>');
	TPL_Write('<table>');
	TPL_Write('<tr><td>Your name:</td>');
	TPL_Write('<td><input type="text" name="email_name" /></td></tr>');
	TPL_Write('<tr><td>Your e-mail address:</td>');
	TPL_Write('<td><input type="text" name="email_from" /></td></tr>');
	TPL_Write('<tr><td>Subject:</td>');
	TPL_Write('<td><input type="text" name="email_subject" /></td></tr>');
	TPL_Write('<tr><td>&nbsp;</td></tr>');
	TPL_Write('<tr><td colspan="2"><textarea name="email_body" rows="6" cols="45">message</textarea></td></tr>');
	TPL_Write('<tr><td colspan="2"><input type="submit" name="submit" value="Send mail"></td></tr>');
	TPL_Write('</table>');
	TPL_Write('</form>');
}

Render_Page();

?>
