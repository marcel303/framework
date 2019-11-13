include engine/ShaderVS.txt

shader_out vec2 texcoord;

void main()
{
	vec4 position = unpackPosition();

	gl_Position = ModelViewProjectionMatrix * position;

	texcoord = unpackTexcoord(0);
}