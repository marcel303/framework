include engine/ShaderVS.txt
include engine/builtin-hq-common-vs.txt

shader_out vec2 v_p;
shader_out vec2 v_g;
shader_out vec2 v_center;
shader_out float v_radius;
shader_out float v_strokeSize;
shader_out vec4 v_color;

void main()
{
	vec2 params = unpackTexcoord(0);
	vec4 positions = in_position4;
	
	// unpack
	
	vec2 p = positions.xy;
	float radius = params.x;
	float strokeSize = params.y * 0.5;
	
	// transform
	
	p = (ModelViewMatrix * vec4(p, 0.0, 1.0)).xy;
	
	float scaledRadius = radius;
	
	if (useScreenSize == 0.0)
	{
		float scale = min(length(ModelViewMatrix[0].xyz), length(ModelViewMatrix[1].xyz));
		
		scaledRadius = radius * scale;

		strokeSize *= scale;
	}
	
	// determine vertex coord, stroke offset and stroke size based on vertex ID
	
	vec2 offset;
	
	int index = gl_VertexID % 4;
	
	if (index == 0)
	{
		offset = vec2(-1.0, -1.0);
	}
	else if (index == 1)
	{
		offset = vec2(+1.0, -1.0);
	}
	else if (index == 2)
	{
		offset = vec2(+1.0, +1.0);
	}
	else // if (index == 3) // commented to avoid compiler warning about offset possibly being left uninitialized
	{
		offset = vec2(-1.0, +1.0);
	}
	
	// final position
	
	vec4 position = vec4(p + offset * (scaledRadius + strokeSize + 1.0), 0.0, 1.0);
	
	gl_Position = ProjectionMatrix * position;
	
	// output the rest
	
	v_p = position.xy;
	v_g = hqGradientCoord(v_p);
	v_center = p;
	v_radius = scaledRadius;
	v_strokeSize = strokeSize;
	v_color = unpackColor();
}
