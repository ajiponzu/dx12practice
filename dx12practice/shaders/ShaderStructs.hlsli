struct Infomation
{
    float4 pos : SV_Position;
    float4 normal : NORMAL0;
    float4 vnormal : NORMAL1;
    float2 uv : TEXCOORD;
    float3 ray : VECTOR;
};

//uv���g���΃s�N�Z���V�F�[�_�݂̂ŕ`�悷�邱�Ƃ��\
//�萔�o�b�t�@���̃��\�[�X���g���΁Ctime, mouse���g����Ɨ\�z