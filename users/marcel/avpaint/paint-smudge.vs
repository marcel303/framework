include engine/ShaderVS.txt

uniform sampler2D colormap;

shader_out vec2 texcoordBrush;
shader_out vec2 texcoordColor;
shader_out vec2 movementDelta;

void main()
{
	vec4 position = vec4(in_position4.xy, 0.0, 1.0);

	gl_Position = ModelViewProjectionMatrix * position;
	
	texcoordBrush = vec2(in_texcoord);
	texcoordColor = position.xy / textureSize(colormap, 0);
	movementDelta = in_position4.zw;
}
