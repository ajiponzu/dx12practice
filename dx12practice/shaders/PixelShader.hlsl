#include "ShaderStructs.hlsli"

Texture2D<float4> tex:register(t0);
SamplerState smp:register(s0);

float4 main(Infomation inf) : SV_TARGET
{
    float3 light = normalize(float3(1, -1, 1)); //入射光の作成 ベクトル :(0, 0, ?) -> (1, -1, 1) == 右下向き(左上から入射してくる)
    float brightness = dot(-light, inf.normal.xyz); //入射光の逆ベクトルを用いる，法線ベクトルと始点が同じ
    return float4(brightness, brightness, brightness, 1.);
}