static const char * s_colorMultiplyVs = R"SHADER(

include engine/ShaderVS.txt

shader_out vec2 texcoord;

void main()
{
	gl_Position = ModelViewProjectionMatrix * in_position4;
	
	texcoord = vec2(in_texcoord);
}

)SHADER";
