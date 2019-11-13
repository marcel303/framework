include engine/ShaderVS.txt

shader_out float texcoord;

void main()
{
	vec4 position = unpackPosition();

	gl_Position = ModelViewProjectionMatrix * position;

	texcoord = unpackTexcoord(0).x;
}