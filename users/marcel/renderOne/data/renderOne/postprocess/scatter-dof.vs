include engine/ShaderVS.txt
include renderOne/depth-functions.txt

uniform sampler2D colorTexture;
uniform sampler2D depthTexture;

uniform mat4x4 projectionToView;

uniform float strength;
uniform float focusDistance;

shader_out vec4 color;

float computeRadius(vec2 texcoord)
{
	float depth = texture(depthTexture, texcoord).x;

	float viewZ = depthToViewZ(depth, texcoord, projectionToView);

	float dof_begin = focusDistance;
	float dof_range = dof_begin;
	
	float dof_back = clamp((viewZ - dof_begin) / dof_range, -1.0, 1.0);

	return dof_back;
}

void main()
{
	int quad = gl_VertexID/6;
	int vert = gl_VertexID%6;
	if (vert == 0) vert = 0;
	else if (vert == 1) vert = 1;
	else if (vert == 2) vert = 2;
	else if (vert == 3) vert = 0;
	else if (vert == 4) vert = 2;
	else if (vert == 5) vert = 3;

	int x = quad % 800;
	int y = quad / 800;

	vec2 texcoord = vec2(
		(x + 0.5) / 800.0,
		(y + 0.5) / 600.0);

	float radius = computeRadius(texcoord);

	vec4 position = vec4(texcoord, 0.0, 1.0);

	vec2 offset;
	if (vert == 0)
		offset = vec2(-1, -1);
	if (vert == 1)
		offset = vec2(+1, -1);
	if (vert == 2)
		offset = vec2(+1, +1);
	if (vert == 3)
		offset = vec2(-1, +1);
	
	position.xy *= vec2(800, 600);
	radius *= 3.0;
	radius += 0.5;

	position.xy += offset * radius;

	gl_Position = ModelViewProjectionMatrix * position;

	color = texture(colorTexture, texcoord);

	//color *= radius;
	//color /= radius;
	color /= max(1.0, radius * radius);
}