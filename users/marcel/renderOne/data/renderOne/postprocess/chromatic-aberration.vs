include engine/ShaderVS.txt

shader_out vec2 v_texcoord;

void main()
{
	vec4 position = unpackPosition();
	vec2 texcoord = unpackTexcoord(0);

	gl_Position = objectToProjection(position);
	v_texcoord = texcoord;
}
