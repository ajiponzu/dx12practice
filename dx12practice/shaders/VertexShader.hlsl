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
    matrix viewproj;
};

Infomation main(Input input)
{
    Infomation inf;
    inf.pos = mul(mul(viewproj, world), input.pos); //シェーダにおける行列の積は数学の順序 : (変換行列: 左引数) * (変換対象: 右引数)
    input.normal.w = 0;
    inf.normal = mul(world, input.normal);
    inf.uv = input.uv;
	return inf;
}