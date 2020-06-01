include engine/ShaderVS.txt

void main()
{
	vec3 position = unpackPosition().xyz;

	vec3 normal = position.xyz;
	vec3 tangent1 = normal.yzx;
	float distance = dot(normal, tangent1);
	//tangent1 += distance * normal;
	tangent1 = normalize(tangent1);
	vec3 tangent2 = cross(tangent1, normal);

	tangent1 = vec3(1, 0, 0);
	tangent2 = vec3(0, 1, 0);

	float s = 0.1;
	tangent1 *= s;
	tangent2 *= s;

	int id = gl_VertexID % 6;
	if (id == 0) position += - tangent1 - tangent2;
	if (id == 1) position +=   tangent1 - tangent2;
	if (id == 2) position += - tangent1 + tangent2;
	if (id == 3) position += - tangent1 + tangent2;
	if (id == 4) position +=   tangent1 - tangent2;
	if (id == 5) position +=   tangent1 + tangent2;

	gl_Position = objectToProjection(vec4(position, 1.0));
}