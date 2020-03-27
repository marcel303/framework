include engine/ShaderVS.txt

shader_out vec3 v_position;

void main()
{
	vec4 position = unpackPosition();
	vec2 texcoord = unpackTexcoord(0);

	gl_Position = objectToProjection(position);
	v_position = objectToView(position).xyz;
}
