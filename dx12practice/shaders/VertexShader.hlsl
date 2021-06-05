#include "ShaderStructs.hlsli"

struct Input
{
  float4 pos : POSITION;
  float2 uv : TEXCOORD;
};

cbuffer cbuff0 : register(b0)
{
  matrix world;
  matrix view;
  matrix proj;
  float3 eye;
};

Inform main(Input input)
{
  Inform inf;
  inf.pos = mul(mul(proj, mul(view, world)), input.pos);
  inf.uv = input.uv;
	return inf;
}