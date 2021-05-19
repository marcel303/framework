include engine/ShaderVS.txt

shader_out vec2 v_texcoord;
shader_out float v_distance;
shader_out vec3 v_normal;
shader_out float v_opacity;

void main()
{
	vec4 position = unpackPosition();

	vec4 position_proj = objectToProjection(position);

	vec4 position_view = objectToView(position);

	gl_Position = position_proj;

	v_texcoord = unpackTexcoord(0);

	v_distance = position_view.z;
	v_normal = unpackNormal().xyz;
	v_opacity = unpackColor().a;
}

