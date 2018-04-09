include engine/ShaderVS.txt

shader_out vec2 texcoord;

void main()
{
	vec4 position = in_position4;

	//

	vec2 uv = in_texcoord.xy;

	vec2 xz = (uv - vec2(0.5)) * 2.0;

	float s = dot(xz, xz);

	if (s < 1.0)
	{
		float h = sqrt(1.0 - s);

		position.z = -h;
	}

	position.xyz = position.xzy;
	position.x = -position.x;

	gl_Position = ModelViewProjectionMatrix * position;

	texcoord = vec2(in_texcoord);
}
