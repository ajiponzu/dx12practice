#pragma once

class Window;
class Scene;

class Renderer
{
protected:
	//グラフィクスパイプラインステートまわり
	ComPtr<ID3D12Resource> mVertBuffer;
	D3D12_VERTEX_BUFFER_VIEW mVertBufferView;
	ComPtr<ID3D12Resource> mIdxBuffer;
	D3D12_INDEX_BUFFER_VIEW mIdxBufferView;
	ComPtr<ID3D12RootSignature> mRootSignature;

	//GPUリソース(パイプライン外)まわり
	UINT mTextureNum = 1;
	UINT mConstantBufferNum = 1;
	ComPtr<ID3D12DescriptorHeap> mResourceDescHeap;
	ComPtr<ID3D12Resource> mTexBuffer;
	ComPtr<ID3D12Resource> mConstantBuffer;

	//ビューポートとシザー矩形
	//これらはCOMではない
	CD3DX12_VIEWPORT mViewport;
	CD3DX12_RECT mScissorRect;

	//テクスチャ関連(あとでクラス化する)
	TexMetadata metadata{};
	ScratchImage scratchImg{};

public:
	Renderer(UINT texureNum = 1, UINT constantBufferNum = 1)
		: mTextureNum(texureNum), mConstantBufferNum(constantBufferNum) {}

	virtual void LoadContents(Scene& scene, Window& window);
	virtual void SetCommandsForGraphicsPipeline(Scene& scene, Window& window);

protected:
	virtual void CreateAppRootSignature(Scene& scene, Window& window, std::vector<CD3DX12_DESCRIPTOR_RANGE>& descTblRange);
	virtual void LinkMatrixAndCBuffer(Scene& scene, Window& window);
	virtual void CreateAppResources(Scene& scene, Window& window, std::vector<CD3DX12_DESCRIPTOR_RANGE> &descTblRange);
	virtual void CreateAppGraphicsPipelineState(Scene& scene, Window& window);
	virtual void CreateInputAssembly(Scene& scene, Window& window);
	virtual void CreateViewportAndScissorRect(Scene& scene, Window& window);
	virtual void ConstructGraphicsPipeline(Scene& scene, Window& window);

	virtual void SetAppGPUResources(Scene& scene, Window& window, ComPtr<ID3D12GraphicsCommandList>& commandList);
	virtual void SetCommandsOnIAStage(Scene& scene, Window& window, ComPtr<ID3D12GraphicsCommandList>& commandList);
	virtual void SetCommandsOnRStage(Scene& scene, Window& window, ComPtr<ID3D12GraphicsCommandList>& commandList);

};
