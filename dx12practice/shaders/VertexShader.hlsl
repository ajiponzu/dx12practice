#include "ShaderStructs.hlsli"

struct Input
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
};

Infomation main(Input input)
{
    Infomation inf;
    inf.pos = float4(input.pos, 1.);
    inf.uv = input.uv;
	return inf;
}