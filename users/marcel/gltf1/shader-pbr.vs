include engine/ShaderVS.txt

uniform float time;

shader_out vec3 v_position_world;
shader_out vec2 v_texcoord0;
shader_out vec2 v_texcoord1;
shader_out vec3 v_normal;

void main()
{
  vec4 position = unpackPosition();

  v_position_world = objectToWorld(position).xyz;

  position = objectToProjection(position);
  
  vec2 texcoord0 = unpackTexcoord(0);
  vec2 texcoord1 = unpackTexcoord(1);
  
  gl_Position = position;
  
  v_texcoord0 = texcoord0;
  v_texcoord1 = texcoord1;

  v_normal = objectToWorld(unpackNormal()).xyz;
}
