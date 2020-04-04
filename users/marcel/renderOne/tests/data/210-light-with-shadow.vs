include engine/ShaderVS.txt

shader_out vec3 v_position;

void main()
{
	vec4 position = unpackPosition();
	
	gl_Position = objectToProjection(position);
	v_position = objectToView(position).xyz;
}
