include engine/ShaderVS.txt
include engine/builtin-hq-common-vs.txt

shader_out vec2 v_p;
shader_out vec2 v_g;
shader_out vec3 v_edgePlane1;
shader_out vec3 v_edgePlane2;
shader_out vec3 v_edgePlane3;
shader_out vec3 v_edgePlane4;
shader_out float v_strokeSize;
shader_out vec4 v_color;

vec3 calculatePlane(vec2 p1, vec2 p2)
{
	vec2 pd = p2 - p1;
	vec2 planeNormal = normalize(vec2(+pd.y, -pd.x));
	float planeDistance = dot(planeNormal, p1);
	return vec3(planeNormal, -planeDistance);
}

void main()
{
	vec4 params1 = in_position4;
	vec4 params2 = unpackNormal();

	// unpack
	
	vec2 p11 = params1.xy;
	vec2 p12 = params1.zw;
	float strokeSize = params2.x * 0.5;
	
	float scale = min(length(ModelViewMatrix[0].xyz), length(ModelViewMatrix[1].xyz));
	
	if (useScreenSize == 1.0)
	{
		vec2 s = (p12 - p11) * 0.5;
		vec2 m = (p11 + p12) * 0.5;
		p11 = m - s / scale;
		p12 = m + s / scale;
	}
	else
	{
		strokeSize *= scale;
	}
	
	vec2 p1 = vec2(p11.x, p11.y);
	vec2 p2 = vec2(p12.x, p11.y);
	vec2 p3 = vec2(p12.x, p12.y);
	vec2 p4 = vec2(p11.x, p12.y);
	
	// transform
	
	p1 = (ModelViewMatrix * vec4(p1, 0.0, 1.0)).xy;
	p2 = (ModelViewMatrix * vec4(p2, 0.0, 1.0)).xy;
	p3 = (ModelViewMatrix * vec4(p3, 0.0, 1.0)).xy;
	p4 = (ModelViewMatrix * vec4(p4, 0.0, 1.0)).xy;
	
	// calculate plane equations along the edge and perpendicular to it
	
	vec3 edgePlane1 = calculatePlane(p1, p2);
	vec3 edgePlane2 = calculatePlane(p2, p3);
	vec3 edgePlane3 = calculatePlane(p3, p4);
	vec3 edgePlane4 = calculatePlane(p4, p1);
	
	vec2 mid = (p1 + p2 + p3 + p4) / 4.0;

	if (dot(edgePlane1, vec3(mid, 1.0)) > 0.0)
	{
		edgePlane1 = -edgePlane1;
		edgePlane2 = -edgePlane2;
		edgePlane3 = -edgePlane3;
		edgePlane4 = -edgePlane4;
	}

	// determine vertex coord, stroke offset and stroke size based on vertex ID
	
	float borderSize = strokeSize + 1.0;
	
	vec2 basePosition;
	
	int index = gl_VertexID % 4;
	
	if (index == 0)
	{
		basePosition = p1;
		
		basePosition -= normalize(p2 - p1) * borderSize;
		basePosition += normalize(p1 - p4) * borderSize;
	}
	else if (index == 1)
	{
		basePosition = p2;
		
		basePosition -= normalize(p3 - p2) * borderSize;
		basePosition += normalize(p2 - p1) * borderSize;
	}
	else if (index == 2)
	{
		basePosition = p3;
		
		basePosition -= normalize(p4 - p3) * borderSize;
		basePosition += normalize(p3 - p2) * borderSize;
	}
	else // if (index == 3) // commented to avoid compiler warning about basePosition possibly being left uninitialized
	{
		basePosition = p4;
		
		basePosition -= normalize(p1 - p4) * borderSize;
		basePosition += normalize(p4 - p3) * borderSize;
	}
	
	vec4 position = vec4(basePosition, 0.0, 1.0);
	
	gl_Position = ProjectionMatrix * position;
	
	// output the rest
	
	v_p = position.xy;
	v_g = hqGradientCoord(v_p);
	v_edgePlane1 = edgePlane1;
	v_edgePlane2 = edgePlane2;
	v_edgePlane3 = edgePlane3;
	v_edgePlane4 = edgePlane4;
	v_strokeSize = strokeSize;
	v_color = unpackColor();
}
