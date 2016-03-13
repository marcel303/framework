include engine/ShaderCommon.txt

shader_out vec2 texcoord;

void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	
	texcoord = vec2(gl_MultiTexCoord0);
}
