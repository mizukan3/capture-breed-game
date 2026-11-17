#version 450
layout(location=0) in vec2 position;
layout(location=1) in vec2 uv;
layout(location=2) in vec4 color;
layout(location=0) out vec2 textureUV;
layout(location=1) out vec4 tint;
layout(set=1,binding=0,std140) uniform Screen {vec4 size;} screen;
void main(){gl_Position=vec4(position.x*2./screen.size.x-1.,1.-position.y*2./screen.size.y,0,1);textureUV=uv;tint=color;}
