#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlwapi.h>

#if defined(min)
#undef min
#endif
 
#if defined(max)
#undef max
#endif

#include <tchar.h>

#include <d3dx12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include <DirectXTex.h>
#include <DirectXMath.h>
using namespace DirectX;

#include <wrl.h>
using namespace Microsoft::WRL;

#include <Helpers.h>

#include <cstdio>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

class Scene;

/// <summary>
/// �E�B���h�E�ƕ`��̓���
/// </summary>
class Window
{
private:
	HWND mHwnd;
	UINT mBufferCount = 0;
	UINT mWidth = 0;
	UINT mHeight = 0;

	std::unique_ptr<Scene> mScene;

	/*directx12����*/
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

	/*directx12����*/
	void PrepareClearWindow();
	void MakeSwapChain(ComPtr<IDXGIFactory6>& factory, ComPtr<ID3D12CommandQueue>& commandQ);
	void MakeBackBufferAndRTV(ComPtr<ID3D12Device>& device);
	void MakeDepthTools(ComPtr<ID3D12Device>& device);
};
