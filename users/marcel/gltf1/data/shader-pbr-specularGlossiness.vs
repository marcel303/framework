include engine/ShaderVS.txt

uniform float time;

shader_out vec3 v_position_view;
shader_out vec3 v_color;
shader_out vec2 v_texcoord0;
shader_out vec2 v_texcoord1;
shader_out vec3 v_normal;

void main()
{
  vec4 position = unpackPosition();

  gl_Position = objectToProjection(position);
  
  v_position_view = objectToView(position).xyz;

  v_color = unpackColor().rgb;
  
  v_texcoord0 = unpackTexcoord(0);
  v_texcoord1 = unpackTexcoord(1);

  v_normal = objectToView(unpackNormal()).xyz;
}
