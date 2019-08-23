static const char * s_textureSwizzleVs = R"SHADER(

include engine/ShaderVS.txt

shader_out vec4 v_color;
shader_out vec2 v_texcoord;

void main()
{
	gl_Position = ModelViewProjectionMatrix * in_position4;

	v_color = unpackColor();
	v_texcoord = unpackTexcoord(0);
}

)SHADER";
