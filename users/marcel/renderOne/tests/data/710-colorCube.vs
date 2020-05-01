include engine/ShaderVS.txt

uniform float scale;

shader_out vec3 v_color;

void main()
{
	v_color = (unpackPosition().xyz + vec3(1.0)) / 2.0;

	gl_Position = objectToProjection(unpackPosition() * vec4(scale, scale, scale, 1.0));
}
