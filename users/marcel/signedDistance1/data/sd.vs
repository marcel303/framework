include engine/ShaderVS.txt

void main()
{
	gl_Position = ModelViewProjectionMatrix * unpackPosition();
}
