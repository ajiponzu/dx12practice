#include "Window.h"
#include "Core.h"
#include "Scene.h"

void Window::Update()
{
	mScene->Update(*this);
}

void Window::Render()
{
	//このフレームの表示バッファを取得
	mCurrentBackBufferIdx = mSwapChain->GetCurrentBackBufferIndex(); //おそらく，swapchain->presentで切り替わるものと思われる
	auto& backBuffer = mBackBuffers[mCurrentBackBufferIdx]; //インデックスが毎フレーム交互に入れ替わる

	auto& core = Core::GetInstance();
	auto& commandList = core.GetCommandList();
	auto& device = core.GetDevice();

	//バッファの操作を安全に行うための処理
	mBarrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ResourceBarrier(1, &mBarrier);

	/*GPUに対する命令追加*/
	mScene->Render(*this);
	/*end*/

	//バッファの操作を安全に終了するための処理，画面に表示する準備ができた
	mBarrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	commandList->ResourceBarrier(1, &mBarrier);

	//コマンドキューによるコマンドリストの命令実行，およびフェンスからの連絡待ち
	//パイプラインを使用するのでtrueを渡す
	core.ExecuteAppCommandLists();
	core.ResetGPUCommand(mScene->GetPipelineState());

	//カレントバッファを表示，その後，カレントバッファが切り替わる
	mSwapChain->Present(1, 0);
}

void Window::MakeGraphicsPipeline()
{
	mScene->LoadContents(*this);
}

/// <summary>
/// Windowクラス外からバリア展開命令を発行する場合
/// </summary>
void Window::UseBarrier() const
{
	Core::GetInstance().GetCommandList()->ResourceBarrier(1, &mBarrier);
}

void Window::Init()
{
	mBackBuffers.resize(mBufferCount);
	PrepareClearWindow();
}

void Window::PrepareClearWindow()
{
	auto& core = Core::GetInstance();
	auto& factory = core.GetFactory();
	auto& commandQ = core.GetCommandQ();
	auto& device = core.GetDevice();

	MakeSwapChain(factory, commandQ);
	MakeBackBufferAndRTV(device);
	MakeDepthTools(device);
}

void Window::MakeSwapChain(ComPtr<IDXGIFactory6>& factory, ComPtr<ID3D12CommandQueue>& commandQ)
{
	//デスクリプション構築
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = mWidth;
	swapChainDesc.Height = mHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapChainDesc.BufferCount = mBufferCount; //ダブルバッファなら2
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	//作成
	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(commandQ.Get(), mHwnd, &swapChainDesc, nullptr, nullptr, &swapChain1));
	//キャスト, idxgi~4はcreate~hwndの引数に渡せないから
	swapChain1.As(&mSwapChain);
}

/// <summary>
/// バックバッファとレンダーターゲットビューの作成
/// および関連付け？
/// </summary>
void Window::MakeBackBufferAndRTV(ComPtr<ID3D12Device>& device)
{
	//レンダーターゲットビューのディスクリプションヒープのディスクリプション構築
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descriptorHeapDesc.NodeMask = 0;
	descriptorHeapDesc.NumDescriptors = mBufferCount; //ダブルバッファなら2
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	//レンダーターゲットビューのディスクリプションヒープ作成
	ThrowIfFailed(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&mRTVHeaps)));

	//ディスクリプションヒープの開始アドレスを取得
	auto rtvHeapsHandle = mRTVHeaps->GetCPUDescriptorHandleForHeapStart();

	//レンダーターゲットビューのディスクリプション構築
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
#ifdef _DEBUG
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
#else
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
#endif
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	//バックバッファとレンダーターゲットビューの関連付け
	for (UINT idx = 0; idx < mBufferCount; idx++)
	{
		ThrowIfFailed(mSwapChain->GetBuffer(idx, IID_PPV_ARGS(&(mBackBuffers[idx])))); //バックバッファをスワップチェインから取得
		device->CreateRenderTargetView(mBackBuffers[idx].Get(), &rtvDesc, rtvHeapsHandle); //レンダーターゲットビュー作成，同一のidxのバックバッファと対応
		rtvHeapsHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV); //ディスクリプションヒープの対応アドレスを更新，オフセットサイズを算出するために関数を用いる
	}
}

/// <summary>
/// 深度がらみの設定
/// </summary>
void Window::MakeDepthTools(ComPtr<ID3D12Device>& device)
{
	auto depthResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_D32_FLOAT, mWidth, mHeight
	);
	depthResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	auto depthHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	auto depthClearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);

	ThrowIfFailed(device->CreateCommittedResource(
		&depthHeapProps, D3D12_HEAP_FLAG_NONE, &depthResourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue, IID_PPV_ARGS(&mDepthBuffer)
	));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDSVHeaps)));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	device->CreateDepthStencilView(
		mDepthBuffer.Get(), &dsvDesc, mDSVHeaps->GetCPUDescriptorHandleForHeapStart()
	);
}

/// <summary>
/// Renderから呼び出される
/// レンダーターゲットビューのクリア
/// </summary>
void Window::ClearAppRenderTargetView(UINT renderTargetsNum)
{
	auto& core = Core::GetInstance();
	auto& device = core.GetDevice();
	auto& commandList = core.GetCommandList();

	//レンダーターゲットの指定
	auto rtvHeapsHandle = mRTVHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvHeapsHandle.ptr += static_cast<SIZE_T>(mCurrentBackBufferIdx) * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//深度バッファの指定
	auto dsvHandle = mDSVHeaps->GetCPUDescriptorHandleForHeapStart();
	//パイプラインのOMステージの対象はこのRTVだよーということ
	//ないと画面クリアされたものしか表示されない(パイプラインの結果は表示されない)
	commandList->OMSetRenderTargets(renderTargetsNum, &rtvHeapsHandle, false, &dsvHandle);
	//レンダーターゲットビューを一色で塗りつぶしてクリア → 表示すると塗りつぶされていることがわかる
	commandList->ClearRenderTargetView(rtvHeapsHandle, mClearColor, 0, nullptr);
	//深度バッファをクリア
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}