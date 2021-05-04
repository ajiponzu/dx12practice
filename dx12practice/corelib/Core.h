#pragma once

#include "Scene.h"

/// <summary>
/// API�A�g�C�E�B���h�E�\��
/// </summary>
class Core
{
public:
private:
	/*win32����*/
	HINSTANCE mHInstance = nullptr;
	HWND mHwnd = nullptr;
	RECT mWndRect{};
	WNDCLASSEX mWndClass{};
	std::wstring mWndTitle{};

	/*directx12����*/
	D3D_FEATURE_LEVEL mFeatureLevel{};
	ComPtr<IDXGIFactory6> mDxgiFactory;
	ComPtr<IDXGIAdapter> mDxgiAdapter;
	ComPtr<ID3D12Device> mDxDevice;
	ComPtr<ID3D12CommandQueue> mCommandQ;
	ComPtr<ID3D12CommandAllocator> mCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> mCommandList;
	ComPtr<ID3D12Fence> mFence;
	UINT64 mFenceValue = 0;

public:
	Core() {}

	~Core()
	{
		UnregisterClass(mWndClass.lpszClassName, mWndClass.hInstance);
	}

	static void MakeInstance(HINSTANCE hInst, const std::wstring title, const int& wid = 1280, const int& high = 720);
	static void SetWindow(const int& wid = 1280, const int& high = 720, UINT bufferCount = 2);
	static Core& GetInstance();
	static void Run(std::unique_ptr<Scene> pScene = nullptr);

	void ExecuteAppCommandLists(bool isPipelineUsed);

	ComPtr<IDXGIFactory6>& GetFactory() { return mDxgiFactory; }
	ComPtr<ID3D12Device>& GetDevice() { return mDxDevice; }
	ComPtr<ID3D12CommandQueue>& GetCommandQ() { return mCommandQ; }
	ComPtr<ID3D12GraphicsCommandList>& GetCommandList() { return mCommandList; }

private:
	Core(HINSTANCE hInst, const std::wstring& title, const int& wid = 1280, const int& high = 720)
		: mHInstance(hInst)
		, mWndTitle(title)
	{
		Init(wid, high);
	}

	Core(const Core& copy) = delete;
	Core& operator=(const Core& other) = delete;

	void Init(const int& wid, const int& high);

	/*win32����*/
	ATOM RegisterWindowParam();
	void InitWindowRect(const int& wid, const int& high);
	HWND MakeWindow(const int& wid, const int& high);

	/*directx12����*/
	void MakeDxgiFactory();
	void MakeDxDevice();
	void MakeGPUCommandTools();
	void MakeFence();
};