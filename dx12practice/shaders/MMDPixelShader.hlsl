#include "ShaderStructs.hlsli"

Texture2D<float4> tex : register(t0);
Texture2D<float4> sph : register(t1);
Texture2D<float4> spa : register(t2);
Texture2D<float4> toon : register(t3);

SamplerState smp : register(s0);
SamplerState smpToon : register(s1);

cbuffer SceneData : register(b0) {
	matrix world;//ワールド変換行列
	matrix view;
	matrix proj;//ビュープロジェクション行列
	float3 eye;
};

cbuffer Material : register(b1)
{
    float4 diffuse;
    float4 specular;
    float3 ambient;
};

float4 main(Information input) : SV_TARGET
{

	
	
		float3 light = normalize(float3(1,-1,1));//光の向かうベクトル(平行光線)
	float3 lightColor = float3(1,1,1);//ライトのカラー(1,1,1で真っ白)

	//ディフューズ計算
	float diffuseB = saturate(dot(-light, input.normal));
	float4 toonDif =  toon.Sample(smpToon, float2(0, 1.0 - diffuseB));

	//光の反射ベクトル
	float3 refLight= normalize(reflect(light, input.normal.xyz));
	float specularB = pow(saturate(dot(refLight, -input.ray)),specular.a);
	
	//スフィアマップ用UV
	float2 sphereMapUV = input.vnormal.xy;
	sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5, -0.5);

	float4 texColor = tex.Sample(smp, input.uv); //テクスチャカラー

	return max(saturate(toonDif//輝度(トゥーン)
		* diffuse//ディフューズ色
		*texColor//テクスチャカラー
		*sph.Sample(smp, sphereMapUV))//スフィアマップ(乗算)
		+ saturate(spa.Sample(smp, sphereMapUV)*texColor//スフィアマップ(加算)
		+ float4(specularB *specular.rgb, 1))//スペキュラー
		, texColor * float4(ambient,1));//アンビエント
}