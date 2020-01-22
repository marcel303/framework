include engine/ShaderVS.txt

shader_out vec4 color;
shader_out vec2 texcoord;

void main()
{
	gl_Position = objectToProjection(unpackPosition());

	color = unpackColor();
	texcoord = unpackTexcoord(0);
}