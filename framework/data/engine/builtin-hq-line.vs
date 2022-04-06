include engine/ShaderVS.txt
include engine/builtin-hq-common-vs.txt

shader_out vec2 v_p;
shader_out vec2 v_g;
shader_out vec2 v_p1;
shader_out vec2 v_p2;
shader_out float v_strokeSize;
shader_out vec3 v_edgePlane;
shader_out vec3 v_perpPlane;
shader_out vec4 v_color;

void main()
{
	vec2 params = unpackTexcoord(0);
	vec4 positions = in_position4;
	
	// unpack
	
	vec2 p1 = positions.xy;
	vec2 p2 = positions.zw;
	
	float strokeSize1 = params.x;
	float strokeSize2 = params.y;

	mat2x2 ModelViewMatrix_rot = mat2x2(
		ModelViewMatrix[0].xy,
		ModelViewMatrix[1].xy);

	vec2 translation = vec2(ModelViewMatrix[3].x, ModelViewMatrix[3].y);

	// transform - scale

	if (useScreenSize == 1.0 && false)
	{
		float scale_rcp = inversesqrt(min(
			dot(ModelViewMatrix_rot[0], ModelViewMatrix_rot[0]),
			dot(ModelViewMatrix_rot[1], ModelViewMatrix_rot[1])));

		p1 *= scale_rcp;
		p2 *= scale_rcp;
	}
	
	// transform
	
	p1 = ModelViewMatrix_rot * p1 + translation;
	p2 = ModelViewMatrix_rot * p2 + translation;
	
	// determine vertex coord, stroke offset and stroke size based on vertex ID
	
	vec2 basePosition;
	vec2 strokeOffset;
	float strokeSize;
	
	int index = gl_VertexID % 4;
	
	if (index == 0)
	{
		basePosition = p1;
		strokeOffset = vec2(-1.0, -1.0);
		strokeSize = strokeSize1;
	}
	else if (index == 1)
	{
		basePosition = p2;
		strokeOffset = vec2(+1.0, -1.0);
		strokeSize = strokeSize2;
	}
	else if (index == 2)
	{
		basePosition = p2;
		strokeOffset = vec2(+1.0, +1.0);
		strokeSize = strokeSize2;
	}
	else // if (index == 3) // commented to avoid compiler warning about basePosition possibly being left uninitialized
	{
		basePosition = p1;
		strokeOffset = vec2(-1.0, +1.0);
		strokeSize = strokeSize1;
	}
	
	if (useScreenSize == 0.0)
	{
		float scale = sqrt(min(
			dot(ModelViewMatrix_rot[0], ModelViewMatrix_rot[0]),
			dot(ModelViewMatrix_rot[1], ModelViewMatrix_rot[1])));
		
		strokeSize1 *= scale;
		strokeSize2 *= scale;
		strokeSize *= scale;
	}
	
	// calculate plane equations along the edge and perpendicular to it
	
		// directions
	vec2 dEdge = (p2 - p1).xy;
	vec2 dPerp = vec2(+dEdge.y, -dEdge.x);
	
		// plane normals
	vec2 pnEdge = dEdge / dot(dEdge, dEdge);
	vec2 pnPerp = normalize(dPerp);
	
		// plane distances
	float pdEdge = dot(pnEdge, p1.xy);
	float pdPerp = dot(pnPerp, p2.xy);
	
	// calculate stroke offset for this vertex, taking into consideration rotation of the quad and stroke size
	
	{
		// rotate the stroke offset
		
		vec2 xAxis = normalize(dEdge);
		vec2 yAxis = pnPerp;
		mat2x2 rotation = mat2x2(xAxis, yAxis);
		
		strokeOffset = rotation * strokeOffset;
		
		// increase stroke size by one since anti-aliasing samples +1 extra pixel
		// the actual stroke is used in the pixel shader to calculate the final color
		
		strokeOffset *= max(strokeSize1, strokeSize2) + 1.0;
	}
	
	// final position is base vertex position + stroke offset
	
	vec4 position = vec4(basePosition + strokeOffset, 0.0, 1.0);
	
	gl_Position = ProjectionMatrix * position;
	
	// output the rest
	
	v_p = position.xy;
	v_g = hqGradientCoord(v_p);
	v_p1 = p1;
	v_p2 = p2;
	v_strokeSize = strokeSize;
	v_edgePlane = vec3(pnEdge, -pdEdge);
	v_perpPlane = vec3(pnPerp, -pdPerp);
	v_color = unpackColor();
}
