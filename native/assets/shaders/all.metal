#include <metal_stdlib>
using namespace metal;
struct SceneIn {float3 position [[attribute(0)]];float3 color [[attribute(1)]];};
struct SceneOut {float4 position [[position]];float3 color;float fog;};
struct SceneUniform {float4x4 matrix;float4 model;float4 center;float4 torches[8];};
vertex SceneOut scene_vertex(SceneIn v [[stage_in]],constant SceneUniform& u [[buffer(0)]]) {
 float co=cos(u.model.w),si=sin(u.model.w);
 float3 local=v.position*u.center.z;
 float3 p=float3(local.x*co+local.z*si,local.y,-local.x*si+local.z*co)+u.model.xyz;
 float warm=0;for(int i=0;i<8;i++)warm+=max(0.f,1.f-distance(p,u.torches[i].xyz)/2.5f)*.65f;
 SceneOut o;o.position=u.matrix*float4(p,1);o.color=min(float3(1),v.color+float3(.032,.027,.017)+float3(.24,.12,.035)*min(warm,1.5f));o.fog=smoothstep(8.f,18.f,distance(p.xz,u.center.xy));return o;
}
fragment float4 scene_fragment(SceneOut v [[stage_in]],constant float4& fog [[buffer(0)]]) {return float4(mix(v.color,fog.rgb,v.fog*.65f),1);}
struct UiIn {float2 position [[attribute(0)]];float2 uv [[attribute(1)]];float4 color [[attribute(2)]];};
struct UiOut {float4 position [[position]];float2 uv;float4 color;};
vertex UiOut ui_vertex(UiIn v [[stage_in]],constant float4& size [[buffer(0)]]) {UiOut o;o.position=float4(v.position.x*2/size.x-1,1-v.position.y*2/size.y,0,1);o.uv=v.uv;o.color=v.color;return o;}
fragment float4 ui_fragment(UiOut v [[stage_in]],texture2d<float> image [[texture(0)]],sampler smp [[sampler(0)]]) {return image.sample(smp,v.uv)*v.color;}
