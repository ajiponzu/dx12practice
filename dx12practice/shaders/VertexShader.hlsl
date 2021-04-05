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
    matrix mat;
};

Infomation main(Input input)
{
    Infomation inf;
    inf.pos = mul(mat, input.pos);
    inf.uv = input.uv;
	return inf;
}