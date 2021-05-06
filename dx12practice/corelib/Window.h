#pragma once

class Scene;

/// <summary>
/// ウィンドウと描画の統括
/// </summary>
class Window
{
private:
	HWND mHwnd;
	UINT mBufferCount = 0;
	UINT mWidth = 0;
	UINT mHeight = 0;

	std::unique_ptr<Scene> mScene;

	/*directx12周り*/
	ComPtr<IDXGISwapChain4> mSwapChain;
	std::vector<ComPtr<ID3D12Resource>> mBackBuffers;
	ComPtr<ID3D12DescriptorHeap> mRTVHeaps;
	ComPtr<ID3D12Resource> mDepthBuffer;
	ComPtr<ID3D12DescriptorHeap> mDSVHeaps;
	CD3DX12_RESOURCE_BARRIER mBarrier{};
	UINT mCurrentBackBufferIdx = 0;
	float mClearColor[4] = { 0.1f, 0.5f, 1.0f, 1.0f };

public:
	Window(HWND hwnd, UINT bufferCount, UINT width, UINT height)
		: mHwnd(hwnd)
		, mBufferCount(bufferCount)
		, mWidth(width)
		, mHeight(height)
	{
		Init();
	}

	void Update();
	void Render();
	void MakeGraphicsPipeline();
	void ClearAppRenderTargetView(UINT renderTargetsNum);

	void SetScene(std::unique_ptr<Scene>& pScene) { mScene = std::move(pScene); }

private:
	void Init();

	/*directx12周り*/
	void PrepareClearWindow();
	void MakeSwapChain(ComPtr<IDXGIFactory6>& factory, ComPtr<ID3D12CommandQueue>& commandQ);
	void MakeBackBufferAndRTV(ComPtr<ID3D12Device>& device);
	void MakeDepthTools(ComPtr<ID3D12Device>& device);
};