include engine/ShaderVS.txt

uniform vec3 viewOrigin;

shader_out vec3 v_position;

void main()
{
	vec4 position = unpackPosition();

	gl_Position = objectToProjection(vec4(viewOrigin, 0.0) + position);

	v_position = position.xyz;
}
