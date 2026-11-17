#version 450
layout(location=0) in vec2 textureUV;
layout(location=1) in vec4 tint;
layout(location=0) out vec4 result;
layout(set=2,binding=0) uniform sampler2D imageTexture;
void main(){result=texture(imageTexture,textureUV)*tint;}
