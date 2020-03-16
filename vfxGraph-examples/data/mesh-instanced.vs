include engine/ShaderVS.txt

uniform mat4 u_worldMatrices[64] buffer transforms; // todo : 256 again. 64 now due to 4k limit Metal

shader_out vec3 v_position_view;
shader_out vec3 v_normal_view;
shader_out vec2 v_texcoord;
shader_out vec4 v_color;

void main()
{
	vec4 position = unpackPosition();
	vec4 normal = unpackNormal();
	vec2 texcoord = unpackTexcoord(0);
	vec4 color = unpackColor();

	int id = gl_InstanceID;

	gl_Position = objectToProjection(u_worldMatrices[id] * position);

	v_position_view = objectToView(position).xyz;
	v_normal_view = objectToView(normal).xyz;
	v_texcoord = texcoord;
	v_color = color;
}