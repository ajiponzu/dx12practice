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
    inf.pos = mul(mul(viewproj, world), input.pos); //�V�F�[�_�ɂ�����s��̐ς͐��w�̏��� : (�ϊ��s��: ������) * (�ϊ��Ώ�: �E����)
    input.normal.w = 0;
    inf.normal = mul(world, input.normal);
    inf.uv = input.uv;
	return inf;
}