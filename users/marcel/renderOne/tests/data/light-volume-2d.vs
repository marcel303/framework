include engine/ShaderVS.txt

shader_out vec3 v_position;

void main()
{
	vec4 position = unpackPosition();
	vec2 texcoord = unpackTexcoord(0);

	gl_Position = objectToProjection(position);
	v_position.xz = ((texcoord - vec2(0.5)) * 2.0) * 16.0;
	v_position.y = 0.0;
}
