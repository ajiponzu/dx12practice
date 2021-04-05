#include "ShaderStructs.hlsli"

Texture2D<float4> tex:register(t0);
SamplerState smp:register(s0);

float4 main(Infomation inf) : SV_TARGET
{
    return float4(0., 0., 0., 1.);
}