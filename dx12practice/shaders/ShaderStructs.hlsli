struct Infomation
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

//uvを使えばピクセルシェーダのみで描画することが可能
//定数バッファ等のリソースを使えば，time, mouseも使えると予想