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

Infomation main(Input input)
{
    Infomation inf;
    inf.pos = mul(mul(proj, mul(view, world)), input.pos); //�V�F�[�_�ɂ�����s��̐ς͐��w�̏��� : (�ϊ��s��: ������) * (�ϊ��Ώ�: �E����)
    input.normal.w = 0;
    inf.normal = mul(world, input.normal);
    inf.vnormal = mul(view, inf.normal);
    inf.uv = input.uv;
    float4 sight = mul(view, float4(eye, 0.));
    inf.ray = normalize(inf.pos.xyz - sight.xyz);
	return inf;
}