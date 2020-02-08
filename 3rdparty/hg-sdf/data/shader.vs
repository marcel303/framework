include engine/ShaderVS.txt

shader_out vec2 v_texcoord;

void main()
{
	vec4 position = unpackPosition();

	gl_Position = objectToProjection(position);

	v_texcoord = unpackTexcoord(0);
}
