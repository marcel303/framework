static const char * s_liceGradientVs = R"SHADER(

include engine/ShaderVS.txt

shader_out vec2 position;

void main()
{
	gl_Position = ModelViewProjectionMatrix * in_position4;

	position = in_position4.xy;
}

)SHADER";
