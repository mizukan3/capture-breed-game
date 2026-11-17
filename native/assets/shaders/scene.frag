#version 450
layout(location=0) in vec3 vertex_color;
layout(location=1) in float fog_amount;
layout(location=0) out vec4 result;
layout(set=3,binding=0,std140) uniform Fog {vec4 color;} fog;
void main(){result=vec4(mix(vertex_color,fog.color.rgb,fog_amount*.65),1);}
