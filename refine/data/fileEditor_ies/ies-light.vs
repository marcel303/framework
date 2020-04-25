include engine/ShaderVS.txt

uniform mat4x4 viewToWorld;

shader_out vec3 v_position;

void main()
{
	vec4 position = unpackPosition();

	gl_Position = objectToProjection(position);

	v_position = (viewToWorld * objectToView(position)).xyz;
}
