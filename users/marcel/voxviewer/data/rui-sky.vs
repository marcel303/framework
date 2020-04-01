include engine/ShaderVS.txt

shader_out vec3 v_position;
shader_out vec2 v_texcoord;

void main()
{
	vec4 position = unpackPosition();
	vec2 texcoord = unpackTexcoord(0);

	gl_Position = objectToProjection(position);

	v_position = position.xyz;
	v_texcoord = texcoord;
}