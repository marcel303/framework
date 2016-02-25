varying vec2 texcoord;
varying vec3 position;
varying vec3 normal;

void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	
	texcoord = vec2(gl_MultiTexCoord0);
	position = (gl_ModelViewMatrix * gl_Vertex).xyz;
	normal = (gl_ModelViewMatrix * vec4(gl_Normal, 0.f)).xyz;
}
