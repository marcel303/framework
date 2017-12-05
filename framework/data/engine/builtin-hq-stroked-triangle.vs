static const char * s_hqStrokedTriangleVs = R"SHADER(

include engine/ShaderVS.txt
include engine/builtin-hq-common-vs.txt

shader_out vec2 v_p;
shader_out vec3 v_edgePlane1;
shader_out vec3 v_edgePlane2;
shader_out vec3 v_edgePlane3;
shader_out vec4 v_color;
shader_out float v_strokeSize;

vec3 calculatePlane(vec2 p1, vec2 p2)
{
	vec2 pd = p2 - p1;
	vec2 planeNormal = normalize(vec2(+pd.y, -pd.x));
	float planeDistance = dot(planeNormal, p1);
	return vec3(-planeNormal, planeDistance);
}

void main()
{
	vec4 params1 = in_position4;
	vec4 params2 = unpackNormal();

	// unpack
	
	vec2 p1 = params1.xy;
	vec2 p2 = params1.zw;
	vec2 p3 = params2.xy;
	float strokeSize = params2.z * 0.5;
	
	// transform
	
	p1 = (ModelViewMatrix * vec4(p1, 0.0, 1.0)).xy;
	p2 = (ModelViewMatrix * vec4(p2, 0.0, 1.0)).xy;
	p3 = (ModelViewMatrix * vec4(p3, 0.0, 1.0)).xy;
	
	// calculate plane equations along the edge and perpendicular to it
	
	vec3 edgePlane1 = calculatePlane(p1, p2);
	vec3 edgePlane2 = calculatePlane(p2, p3);
	vec3 edgePlane3 = calculatePlane(p3, p1);
	
	vec2 mid = (p1 + p2 + p3) / 3.0;

	if (dot(edgePlane1, vec3(mid, 1.0)) < 0.0)
	{
		edgePlane1 = -edgePlane1;
		edgePlane2 = -edgePlane2;
		edgePlane3 = -edgePlane3;
	}

	// determine vertex coord, stroke offset and stroke size based on vertex ID
	
	float borderSize = strokeSize + 1.0;
	
	vec2 basePosition;
	
	int index = gl_VertexID % 3;
	
	if (index == 0)
	{
		basePosition = p1;
		
		basePosition -= normalize(p2 - p1) * borderSize;
		basePosition += normalize(p1 - p3) * borderSize;
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
		
		basePosition -= normalize(p1 - p3) * borderSize;
		basePosition += normalize(p3 - p2) * borderSize;
	}
	
	if (1 == 0)
	{
		// expand triangle aabb by 1px in all directions for anti-aliasing to work	
		
		vec2 pmid = (p1 + p2 + p3) / 3.0;
		
		float borderSize = 20.0;
		
		if (basePosition.x < pmid.x)
			basePosition.x -= borderSize;
		if (basePosition.y < pmid.y)
			basePosition.y -= borderSize;
		if (basePosition.x > pmid.x)
			basePosition.x += borderSize;
		if (basePosition.y > pmid.y)
			basePosition.y += borderSize;
	}
	
	vec4 position = vec4(basePosition, 0.0, 1.0);
	
	gl_Position = ProjectionMatrix * position;
	
	// output the rest
	
	v_p = position.xy;
	v_edgePlane1 = edgePlane1;
	v_edgePlane2 = edgePlane2;
	v_edgePlane3 = edgePlane3;
	v_strokeSize = strokeSize;
	v_color = unpackColor();
}

)SHADER";
