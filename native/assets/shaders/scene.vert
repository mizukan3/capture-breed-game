#version 450
layout(location=0) in vec3 position;
layout(location=1) in vec3 color;
layout(location=0) out vec3 vertex_color;
layout(location=1) out float fog_amount;
layout(set=1,binding=0,std140) uniform Scene {
 mat4 matrix;
 vec4 model;
 vec4 center;
 vec4 torches[8];
} scene;
void main() {
 float co=cos(scene.model.w),si=sin(scene.model.w);
 vec3 local=position*scene.center.z;
 vec3 p=vec3(local.x*co+local.z*si,local.y,-local.x*si+local.z*co)+scene.model.xyz;
 gl_Position=scene.matrix*vec4(p,1);
 float warm=0;
 for(int i=0;i<8;i++){float d=distance(p,scene.torches[i].xyz);warm+=max(0.,1.-d/2.5)*.65;}
 vertex_color=min(vec3(1),color+vec3(.032,.027,.017)+vec3(.24,.12,.035)*min(warm,1.5));
 fog_amount=smoothstep(8.,18.,distance(p.xz,scene.center.xy));
}
