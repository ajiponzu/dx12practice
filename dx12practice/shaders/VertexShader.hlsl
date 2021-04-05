#include "ShaderStructs.hlsli"

struct Input
{
    float4 pos : POSITION;
    float2 uv : TEXCOORD;
};

cbuffer cbuff0 : register(b0)
{
    matrix mat;
};

Infomation main(Input input)
{
    Infomation inf;
    inf.pos = mul(mat, input.pos);
    //inf.pos = input.pos;
    inf.uv = input.uv;
	return inf;
}