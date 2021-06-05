#include "ShaderStructs.hlsli"

Texture2D<float4> tex:register(t0);
SamplerState smp:register(s0);

float3 SetCircle(in float3 color, in float radius, in float2 st)
{
  float2 center = float2(.5, .5);
  float pct = distance(center, st);
  return color * smoothstep(radius + .1, radius, pct);
}

float4 CircleTutorial(in Inform inf)
{
  float3 color = float3(.93, .54, .54);
  return float4(SetCircle(color, .3, inf.uv), 1.);
}

float4 main(Inform inf) : SV_TARGET
{
  float4 output = tex.Sample(smp, inf.uv);
  output.rgb = lerp(output.rgb, SetCircle(float3(1., 1., 1.), .5, inf.uv), float3(inf.uv, 0.));
  return output;
}