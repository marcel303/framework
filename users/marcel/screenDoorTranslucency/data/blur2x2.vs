include engine/ShaderVS.txt

shader_out vec2 texcoord;

void main()
{
	gl_Position = objectToProjection(unpackPosition());

	texcoord = unpackTexcoord(0);
}