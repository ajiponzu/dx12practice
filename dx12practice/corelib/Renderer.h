#pragma once

class Window;
class Scene;
class Actor;

class Renderer
{
protected:
	//グラフィクスパイプラインステートまわり
	ComPtr<ID3D12Resource> mVertBuffer;
	D3D12_VERTEX_BUFFER_VIEW mVertBufferView{};
	ComPtr<ID3D12Resource> mIdxBuffer;
	D3D12_INDEX_BUFFER_VIEW mIdxBufferView{};
	ComPtr<ID3D12RootSignature> mRootSignature;

	//GPUリソース(パイプライン外)まわり
	UINT mTextureNum = 1U;
	UINT mConstantBufferNum = 1U;
	ComPtr<ID3D12DescriptorHeap> mResourceDescHeap;
	ComPtr<ID3D12Resource> mTexBuffer;
	ComPtr<ID3D12Resource> mConstantBuffer;

	//ビューポートとシザー矩形
	//これらはCOMではない
	CD3DX12_VIEWPORT mViewport{};
	CD3DX12_RECT mScissorRect{};

public:
	Renderer(UINT textureNum = 1U, UINT constantBufferNum = 1U)
		: mTextureNum(textureNum), mConstantBufferNum(constantBufferNum) {}

	virtual ~Renderer() { mConstantBuffer->Unmap(0, nullptr); }

	virtual void LoadContents(Actor& actor, Scene& scene, Window& window);
	virtual void SetCommandsForGraphicsPipeline(Scene& scene, Window& window);

protected:
	virtual void CreateAppRootSignature(Scene& scene, Window& window, std::vector<std::vector<CD3DX12_DESCRIPTOR_RANGE>>& descTblRanges);
	virtual void LinkMatrixAndCBuffer(Actor& actor, Scene& scene, Window& window);
	virtual void CreateAppResources(Actor& actor, Scene& scene, Window& window, std::vector<std::vector<CD3DX12_DESCRIPTOR_RANGE>>& descTblRanges);
	virtual void CreateAppGraphicsPipelineState(Scene& scene, Window& window);
	virtual void CreateInputAssembly(Scene& scene, Window& window);
	virtual void CreateViewportAndScissorRect(Scene& scene, Window& window);
	virtual void ConstructGraphicsPipeline(Scene& scene, Window& window);

	virtual void SetAppGPUResources(Scene& scene, Window& window, ComPtr<ID3D12GraphicsCommandList>& commandList);
	virtual void SetCommandsOnIAStage(Scene& scene, Window& window, ComPtr<ID3D12GraphicsCommandList>& commandList);
	virtual void SetCommandsOnRStage(Scene& scene, Window& window, ComPtr<ID3D12GraphicsCommandList>& commandList);
};
