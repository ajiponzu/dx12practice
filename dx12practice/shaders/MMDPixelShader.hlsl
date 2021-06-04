#include "ShaderStructs.hlsli"

Texture2D<float4> tex : register(t0);
Texture2D<float4> sph : register(t1);
Texture2D<float4> spa : register(t2);
Texture2D<float4> toon : register(t3);

SamplerState smp : register(s0);
SamplerState smpToon : register(s1);

cbuffer Material : register(b1)
{
    float4 diffuse;
    float4 specular;
    float3 ambient;
};

float4 main(Infomation inf) : SV_TARGET
{
	float3 light = normalize(float3(1., -1., 1.));
	float3 lightColor = float3(1., 1., 1.);

	float diffuseB = saturate(dot(-light, inf.normal.xyz));
	float4 toonDif =  toon.Sample(smpToon, float2(0., 1. - diffuseB));

	float3 refLight= normalize(reflect(light, inf.normal.xyz));
	float specularB = pow(saturate(dot(refLight, -inf.ray)),specular.a);
	
	float2 sphereMapUV = inf.vnormal.xy;
	sphereMapUV = (sphereMapUV + float2(1., -1.)) * float2(.5, -.5);

	float4 texColor = tex.Sample(smp, inf.uv);

	return max(saturate(toonDif
		* diffuse
		*texColor
		*sph.Sample(smp, sphereMapUV))
		+ saturate(spa.Sample(smp, sphereMapUV)*texColor
		+ float4(specularB *specular.rgb, 1.))
		, float4(texColor.rgb * ambient,1.));
}