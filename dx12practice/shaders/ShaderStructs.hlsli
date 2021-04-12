struct Infomation
{
    float4 pos : SV_Position;
    float4 normal : NORMAL0;
    float4 vnormal : NORMAL1;
    float2 uv : TEXCOORD;
    float3 ray : VECTOR;
};

//uvを使えばピクセルシェーダのみで描画することが可能
//定数バッファ等のリソースを使えば，time, mouseも使えると予想