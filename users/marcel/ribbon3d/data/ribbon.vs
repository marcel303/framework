include engine/ShaderVS.txt

shader_out vec3 v_position;
shader_out vec3 v_normal;

void main()
{
	vec4 position = unpackPosition();

	gl_Position = objectToProjection(position);

	v_position = position.xyz;
	v_normal = unpackNormal().xyz;

	//gl_Position.xyz = mix(gl_Position.xyz, gl_Position.xyz * v_normal * 1.0, 0.8);
}