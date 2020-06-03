include engine/ShaderVS.txt

shader_out vec3 v_position_view;
shader_out vec2 v_texcoord0;
shader_out vec3 v_normal_view;

void main()
{
  vec4 position = unpackPosition();

  gl_Position = objectToProjection(position);
  
  v_position_view = objectToView(position).xyz;

  v_texcoord0 = unpackTexcoord(0);

  v_normal_view = objectToView3(unpackNormal().xyz);
}
