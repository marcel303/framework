static const char * s_hqStrokedRoundedRectVs = R"SHADER(

include engine/ShaderVS.txt
include engine/builtin-hq-common-vs.txt

shader_out vec2 v_p;
shader_out vec3 v_edgePlane1;
shader_out vec3 v_edgePlane2;
shader_out vec3 v_edgePlane3;
shader_out vec3 v_edgePlane4;
shader_out vec2 v_corner1;
shader_out vec2 v_corner2;
shader_out vec2 v_corner3;
shader_out vec2 v_corner4;
shader_out float v_radius;
shader_out float v_strokeSize;
shader_out vec4 v_color;

vec3 calculatePlane(vec2 p1, vec2 p2)
{
	vec2 pd = p2 - p1;
	vec2 planeNormal = normalize(vec2(-pd.y, pd.x));
	float planeDistance = dot(planeNormal, p1);
	return vec3(planeNormal, -planeDistance);
}

void main()
{
	vec4 params1 = in_position4;
	vec4 params2 = unpackNormal();
	
	// unpack
	
	vec2 p11 = min(params1.xy, params1.zw);
	vec2 p12 = max(params1.xy, params1.zw);
	float radius = min(min(p12.x - p11.x, p12.y - p11.y)/2.01, params2.x);
	float strokeSize = params2.y * 0.5;

	float scale = min(length(ModelViewMatrix[0].xyz), length(ModelViewMatrix[1].xyz));

	float borderSize = strokeSize + 1.0;

	if (useScreenSize == 1.0)
	{
		vec2 s = (p12 - p11) * 0.5;
		vec2 m = (p11 + p12) * 0.5;
		p11 = m - s / scale;
		p12 = m + s / scale;

		borderSize /= scale;
		radius /= scale;
	}
	
	vec2 p1 = vec2(p11.x, p11.y);
	vec2 p2 = vec2(p12.x, p11.y);
	vec2 p3 = vec2(p12.x, p12.y);
	vec2 p4 = vec2(p11.x, p12.y);

	vec2 corner1 = vec2(p11.x + radius, p11.y + radius);
	vec2 corner2 = vec2(p12.x - radius, p11.y + radius);
	vec2 corner3 = vec2(p12.x - radius, p12.y - radius);
	vec2 corner4 = vec2(p11.x + radius, p12.y - radius);
	
	// transform
	
	p1 = (ModelViewMatrix * vec4(p1, 0.0, 1.0)).xy;
	p2 = (ModelViewMatrix * vec4(p2, 0.0, 1.0)).xy;
	p3 = (ModelViewMatrix * vec4(p3, 0.0, 1.0)).xy;
	p4 = (ModelViewMatrix * vec4(p4, 0.0, 1.0)).xy;

	corner1 = (ModelViewMatrix * vec4(corner1, 0.0, 1.0)).xy;
	corner2 = (ModelViewMatrix * vec4(corner2, 0.0, 1.0)).xy;
	corner3 = (ModelViewMatrix * vec4(corner3, 0.0, 1.0)).xy;
	corner4 = (ModelViewMatrix * vec4(corner4, 0.0, 1.0)).xy;
	
	// calculate plane equations along the edge and perpendicular to it
	
	vec3 edgePlane1 = calculatePlane(corner1, corner2);
	vec3 edgePlane2 = calculatePlane(corner2, corner3);
	vec3 edgePlane3 = calculatePlane(corner3, corner4);
	vec3 edgePlane4 = calculatePlane(corner4, corner1);
	
	vec2 mid = (corner1 + corner2 + corner3 + corner4) / 4.0;

	if (dot(edgePlane1, vec3(mid, 1.0)) > 0.0 && false)
	{
		edgePlane1 = -edgePlane1;
		edgePlane2 = -edgePlane2;
		edgePlane3 = -edgePlane3;
		edgePlane4 = -edgePlane4;
	}

	// determine vertex coord, stroke offset and stroke size based on vertex ID
	
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
	else if (index == 3)
	{
		basePosition = p4;
		
		basePosition -= normalize(p1 - p4) * borderSize;
		basePosition += normalize(p4 - p3) * borderSize;
	}
	
	vec4 position = vec4(basePosition, 0.0, 1.0);
	
	gl_Position = ProjectionMatrix * position;
	
	// output the rest
	
	v_p = position.xy;
	v_edgePlane1 = edgePlane1;
	v_edgePlane2 = edgePlane2;
	v_edgePlane3 = edgePlane3;
	v_edgePlane4 = edgePlane4;
	v_radius = radius * scale;
	v_strokeSize = strokeSize;
	v_color = unpackColor();
}

)SHADER";
