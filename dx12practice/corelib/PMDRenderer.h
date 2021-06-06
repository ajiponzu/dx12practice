#pragma once
#include "Renderer.h"
#include "Mmd.h"

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
	std::vector<MMDTextures> mMMDTextureList;

public:
	PMDRenderer(const UINT& textureNum = 1U, const UINT& constantBufferNum = 1U)
		: Renderer(textureNum, constantBufferNum) {}

protected:
	virtual void CreateAppRootSignature(Scene& scene, Window& window, std::vector<std::vector<CD3DX12_DESCRIPTOR_RANGE>>& descTblRanges) override;
	virtual void CreateAppResources(Actor& actor, Scene& scene, Window& window, std::vector<std::vector<CD3DX12_DESCRIPTOR_RANGE>>& descTblRanges) override;
	virtual void CreateAppGraphicsPipelineState(Scene& scene, Window& window) override;
	virtual void CreateInputAssembly(Scene& scene, Window& window) override;

	virtual void LoadContents(Actor& actor, Scene& scene, Window& window) override;

	virtual void SetAppGPUResources(Scene& scene, Window& window, ComPtr<ID3D12GraphicsCommandList>& commandList) override;
};
