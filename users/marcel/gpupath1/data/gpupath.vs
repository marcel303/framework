include engine/ShaderVS.txt

uniform vec2 position1;
uniform vec2 position2;
uniform float numVertices;

uniform float halfThickness;

shader_out float v_radialPosition;

vec2 interp(vec2 v1, vec2 v4, float t)
{
	vec2 c1 = vec2(+100.0, 0.0);
	vec2 c2 = vec2(-100.0, 0.0);

	vec2 v2 = v1 + c1;
	vec2 v3 = v4 + c2;

	float t1 = 1.0 - t;
	float t2 =       t;

	float a = t1 * t1 * t1;
	float b = t1 * t1 * t2 * 3.0;
	float c = t1 * t2 * t2 * 3.0;
	float d = t2 * t2 * t2;

	return
		v1 * a +
		v2 * b +
		v3 * c +
		v4 * d;
}

void main()
{
	// calculate vertex position

	float t = gl_VertexID / numVertices;
	vec2 position = interp(position1, position2, t);

	// calculate tangent. this requires us knowing the derivative for the position
	// to stay compatible with legacy GLSL we calculate the position twice, and
	// use a partial derivative, instead of using dFdx etc
	vec2 nextPosition = interp(position1, position2, t + 1e-3);

	vec2 tangent = nextPosition - position;
	tangent = vec2(-tangent.y, +tangent.x);
	tangent = normalize(tangent);

	// apply thickness

	tangent *= halfThickness;

	// calculate vertex offset

	vec2 offset;

	if ((gl_VertexID & 1) == 0)
	{
		offset = -tangent;
		v_radialPosition = -halfThickness;
	}
	else
	{
		offset = +tangent;
		v_radialPosition = +halfThickness;
	}

	// calculate the final position

	position += offset;

	gl_Position = objectToProjection(vec4(position, 0.0, 1.0));
}
