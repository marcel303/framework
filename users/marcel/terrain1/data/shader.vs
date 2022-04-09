include engine/ShaderVS.txt

void main()
{
	vec4 position = unpackPosition();
	
	position = objectToProjection(position);
	
	gl_Position = position;
}
