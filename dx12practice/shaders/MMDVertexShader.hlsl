#include "ShaderStructs.hlsli"

struct Input
{
    float4 pos : POSITION;
    float4 normal : NORMAL;
    float2 uv : TEXCOORD;
    min16uint2 boneNo : BONE_NO;
    min16uint weight : WEIGHT;
};

cbuffer cbuff0 : register(b0)
{
    matrix world;
    matrix view;
    matrix proj;
    float3 eye;
};

Information main(Input input)
{
    Information inf;
    inf.pos = mul(mul(proj, mul(view, world)), input.pos); //シェーダにおける行列の積は数学の順序 : (変換行列: 左引数) * (変換対象: 右引数)
    input.normal.w = 0;
    inf.normal = mul(world, input.normal);
    inf.vnormal = mul(view, inf.normal);
    inf.uv = input.uv;
    float4 sight = mul(view, float4(eye, 0.));
    inf.ray = normalize(inf.pos.xyz - sight.xyz);
	return inf;
}