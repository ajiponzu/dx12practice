#pragma once
#include "Renderer.h"

//const std::string gModelPath = "assets/初音ミク.pmd";
//const std::string gModelPath = "assets/鏡音リン.pmd";
//const std::string gModelPath = "assets/鏡音リン_act2.pmd";
//const std::string gModelPath = "assets/鏡音レン.pmd";
//const std::string gModelPath = "assets/弱音ハク.pmd";
const std::string gModelPath = "assets/咲音メイコ.pmd";
//const std::string gModelPath = "assets/初音ミクVer2.pmd";
//const std::string gModelPath = "assets/亞北ネル.pmd";
//const std::string gModelPath = "assets/MEIKO.pmd";
//const std::string gModelPath = "assets/カイト.pmd";

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

class PMDRenderer :
	public Renderer
{
protected:
	PMDHeader mPMDHeader{};

	PMDVertex mPMDVertex{};

	//頂点情報
	std::vector<uint8_t> mVertices{};
	std::vector<uint16_t> mIndices{};

	std::vector<PMDMaterial> mPMDMaterials{};

	std::vector<Material> mMaterials{};
	ComPtr<ID3D12Resource> mMaterialBuffer;
	ComPtr<ID3D12DescriptorHeap> mMaterialDescHeap;
	std::vector<ComPtr<ID3D12Resource>> mTexBuffers;
	std::vector<ComPtr<ID3D12Resource>> mSphereResources;
	std::vector<ComPtr<ID3D12Resource>> mSphereAdderResources;
	std::vector<ComPtr<ID3D12Resource>> mToonResources;

public:
	PMDRenderer(UINT textureNum = 1U, UINT constantBufferNum = 1U)
		: Renderer(textureNum, constantBufferNum) {}

protected:
	//virtual void CreateAppRootSignature(Scene& scene, Window& window, std::vector<std::vector<CD3DX12_DESCRIPTOR_RANGE>>& descTblRanges) override;
	//virtual void CreateAppResources(Scene& scene, Window& window, std::vector<std::vector<CD3DX12_DESCRIPTOR_RANGE>>& descTblRanges) override;
	//virtual void CreateAppGraphicsPipelineState(Scene& scene, Window& window) override;
	//virtual void CreateInputAssembly(Scene& scene, Window& window) override;

	//virtual void SetAppGPUResources(Scene& scene, Window& window, ComPtr<ID3D12GraphicsCommandList>& commandList) override;

	//virtual void LoadMMD(Scene& scene, Window& window);
};
