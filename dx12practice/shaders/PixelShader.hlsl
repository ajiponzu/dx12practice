#include "ShaderStructs.hlsli"

Texture2D<float4> tex:register(t0);
SamplerState smp:register(s0);

float4 main(Infomation inf) : SV_TARGET
{
    float3 light = normalize(float3(1, -1, 1)); //���ˌ��̍쐬 �x�N�g�� :(0, 0, ?) -> (1, -1, 1) == �E������(���ォ����˂��Ă���)
    float brightness = dot(-light, inf.normal.xyz); //���ˌ��̋t�x�N�g����p����C�@���x�N�g���Ǝn�_������
    return float4(brightness, brightness, brightness, 1.);
}