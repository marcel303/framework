static const char * s_msdfTextVs = R"SHADER(

include engine/ShaderVS.txt

shader_out vec4 v_color;
shader_out vec2 v_texcoord;

void main()
{
	vec4 position = unpackPosition();
	
	position = objectToProjection(position);
	
	vec4 color = unpackColor();

	vec2 texcoord = unpackTexcoord(0);
	
	gl_Position = position;
	
	v_color = color;
	v_texcoord = texcoord;
}

)SHADER";
