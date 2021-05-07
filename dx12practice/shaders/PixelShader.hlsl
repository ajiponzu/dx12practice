#include "ShaderStructs.hlsli"

Texture2D<float4> tex:register(t0);
SamplerState smp:register(s0);

float3 SetCircle(in float3 color, in float radius, in float2 st)
{
    float2 center = float2(.5, .5);
    float pct = distance(center, st);
    return color * smoothstep(radius + .1, radius, pct);
}

float4 CircleTutorial(in Infomation inf)
{
    float3 color = float3(.93, .54, .54);
    return float4(SetCircle(color, .3, inf.uv), 1.);
}

float4 main(Infomation inf) : SV_TARGET
{
    return float4(tex.Sample(smp, inf.uv));
}