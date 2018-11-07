include engine/ShaderVS.txt

uniform mat4x4 transform;

shader_out vec3 normal;

void main()
{
	vec4 position = ModelViewProjectionMatrix * in_position4;

	gl_Position = position;
	
	normal = normalize(transform * vec4(in_position4.xy, 0.0, 1.0)).xyz;
}
