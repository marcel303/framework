include engine/ShaderVS.txt

uniform mat4x4 viewToWorld;

shader_out vec3 v_position;
shader_out vec3 v_origin;

shader_out vec4 v_debug;

void main()
{
	vec4 position = unpackPosition();

	gl_Position = objectToProjection(position);

	vec2 position2 = unpackTexcoord(0);
	position2 = position2 * 2.0 - vec2(1.0);
	position2.y = -position2.y;

	v_position = (viewToWorld * vec4(position2, 1, 1)).xyz;
	v_origin = (viewToWorld * vec4(0, 0, 0, 1)).xyz;

	//v_debug = vec4(v_origin, 0);
	//v_debug = vec4(v_position, 0);
	v_debug = vec4(v_position - v_origin, 0);
}
