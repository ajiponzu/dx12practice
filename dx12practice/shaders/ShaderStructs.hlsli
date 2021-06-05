struct Inform
{
  float4 pos : SV_Position;
  float2 uv : TEXCOORD;
};

struct Information
{
  float4 pos : SV_Position;
  float4 normal : NORMAL0;
  float4 vnormal : NORMAL1;
  float2 uv : TEXCOORD;
  float3 ray : VECTOR;
};
