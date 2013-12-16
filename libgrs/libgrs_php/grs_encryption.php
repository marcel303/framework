<?php 

echo("<h3> Symmetric Encryption </h3>"); 

$key_value = "KEYVALUE"; 
$plain_text = "PLAINTEXT"; 
//$iv = microtime();

$encrypted_text = mcrypt_ecb(MCRYPT_DES, $key_value, $plain_text, MCRYPT_ENCRYPT);

echo ("<p><b> Text after encryption : </b>"); 
echo ( base64_encode($encrypted_text) );

$decrypted_text = mcrypt_ecb(MCRYPT_DES, $key_value, $encrypted_text, MCRYPT_DECRYPT); 

echo ("<p><b> Text after decryption : </b>"); 
echo ( $decrypted_text );

//

$phrase = "Hello World";
$sha1a =  base64_encode(sha1($phrase));
$sha1b =  hash('sha1',$phrase);
$sha256= hash('sha256',$phrase);
$sha384= hash('sha384',$phrase);
$sha512= hash('sha512',$phrase);

echo ("<pre>");

echo ("SHA1..:" . $sha1a . "\n");
echo ("SHA1..:" . $sha1b . "\n");
echo ("SHA256:" . $sha256 . "\n");
echo ("SHA384:" . $sha384 . "\n");  echo ("SHA512:" . $sha512 . "\n"); 

microtime(1);

echo ("</pre>");

//select sha1('TheOriginalText');

//

function GRS_Encrypt($key, $string)
{
}

function GRS_Decrypt($key, $string)
{
}

?>