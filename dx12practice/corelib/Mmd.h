#pragma once

struct PMDHeader
{
	float mVersion;
	char mModelName[20];
	char mComment[256];
};

struct PMDVertex
{
	XMFLOAT3 mPos;
	XMFLOAT3 normal;
	XMFLOAT2 uv;
	uint16_t boneNo[2];
	uint8_t boneWeight;
	uint8_t edgeFlag;
};

constexpr size_t mPMDVertexSize = 38;

#pragma pack(1)
struct PMDMaterial
{
	XMFLOAT3 diffuse;
	float alpha;
	float specularity;
	XMFLOAT3 specular;
	XMFLOAT3 ambient;
	uint8_t toonIdx;
	uint8_t edgeFlag;
	uint32_t indicesNum;
	char texFilePath[20];
};
#pragma pack()
//pragma packで囲われた部分ではアラインメントが無効になる
//よってメンバ間にフラグメントは生じない
//構造体のポインタを渡して，freadで丸ごと読み込むような場合，フラグメントはネックとなる

struct MaterialForHlsl
{
	XMFLOAT3 diffuse;
	float alpha;
	XMFLOAT3 specular;
	float specularity;
	XMFLOAT3 ambient;
};

struct AdditionalMaterial
{
	std::string texPath;
	int toonIdx;
	bool edgeFlg;
};

struct Material
{
	uint32_t indicesNum;
	MaterialForHlsl material{};
	AdditionalMaterial additional{};
};

struct MMDTextures
{
	ComPtr<ID3D12Resource> texBuffer;
	ComPtr<ID3D12Resource> sphere;
	ComPtr<ID3D12Resource> sphereAdder;
	ComPtr<ID3D12Resource> toon;
};

struct UploadLocation;

namespace MMD
{
	FILE* LoadPMD(PMDHeader& header, std::vector<uint8_t>& vertices, std::vector<uint16_t>& indices, const std::string& path);
	void LoadPMD(FILE* fp, std::vector<PMDMaterial>& pmdMaterials, std::vector<Material>& materials);
	void LoadPMDMaterialResources(
		const int& idx, std::vector<MMDTextures>& mmdTextureList, std::vector<UploadLocation>& uploadLocation,
		const std::string& resourcePath, const std::string& toonFilePath, std::string texFileName
	);
};
