include engine/ShaderVS.txt

void main()
{
	vec4 position = unpackPosition();
	
	gl_Position = objectToProjection(position);
}
