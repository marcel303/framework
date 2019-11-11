include engine/ShaderVS.txt

uniform sampler2D p;

void main()
{
	int local_index = gl_VertexID % 6;

	vec2 position;
	
	if (local_index == 0)
		position = vec2(-1.0, -1.0);
	else if (local_index == 1)
		position = vec2(+1.0, -1.0);
	else if (local_index == 2)
		position = vec2(-1.0, +1.0);
	else if (local_index == 3)
		position = vec2(+1.0, -1.0);
	else if (local_index == 4)
		position = vec2(+1.0, +1.0);
	else if (local_index == 5)
		position = vec2(-1.0, +1.0);

	position *= 4.0; // todo : make the size a uniform

	//

	int particle_index = gl_VertexID / 6;
	vec2 sizeRcp = vec2(1.0) / textureSize(p, 0);
	position += texture(p, sizeRcp * (particle_index + 0.5)).xy;

	gl_Position = ModelViewProjectionMatrix * vec4(position, 0.0, 1.0);
}