#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlwapi.h>
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
#include <vector>

//directx系のlibをリンクする必要がある
//shlwapi.libをリンクする必要がある

/*グローバル変数*/

//ウィンドウまわり
HINSTANCE gHInstance = nullptr;
HWND gHwnd = nullptr;
WNDCLASSEX gWindowClass{};
constexpr auto gWindowWidth = 1280;
constexpr auto gWindowHeight = 720;
RECT gWindowRect{};

//定数
constexpr UINT gBufferCount = 2;

//directx必須まわり
ComPtr<IDXGIFactory6> gDxgiFactory;
ComPtr<IDXGIAdapter> gDxgiAdapter;
ComPtr<ID3D12Device> gDevice;
ComPtr<ID3D12CommandAllocator> gCommandAllocator;
ComPtr<ID3D12GraphicsCommandList> gCommandList; //グラフィクスコマンドリストを使用することに注意
ComPtr<ID3D12CommandQueue> gCommandQ;
ComPtr<IDXGISwapChain4> gSwapChain;
ComPtr<ID3D12DescriptorHeap> gRTVHeaps;
ComPtr<ID3D12Resource> gBackBuffers[gBufferCount];
ComPtr<ID3D12Resource> gDepthBuffer;
ComPtr<ID3D12DescriptorHeap> gDSVHeaps;
ComPtr<ID3D12Fence> gFence;
UINT64 gFenceValue = 0;
UINT gCurrentBackBufferIdx = 0;
float gClearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f};
D3D_FEATURE_LEVEL gFeatureLevel{};
CD3DX12_RESOURCE_BARRIER gBarrier{};

/*GPUリソース(パイプライン外)まわり*/
constexpr UINT gTextureNum = 1;
constexpr UINT gConstantBufferNum = 1;
ComPtr<ID3D12DescriptorHeap> gResourceDescHeap;
ComPtr<ID3D12Resource> gTexBuffer;
//Render内で呼ばれないが，ディスクリプタ(あるいはビュー)はバッファの命令インターフェイスみたいなもの
//本体が生存していなければエラー
ComPtr<ID3D12Resource> gConstantBuffer;
struct MatrixData
{
	XMMATRIX mWorld;
	XMMATRIX mViewProjection;
};
MatrixData gMatrix;
MatrixData* gMapMatrix = nullptr; //マップ用，アップデートで変換するためにスコープを拡張
float gAngle = 0.0f;
//MMDまわり
struct PMDHeader
{
	float mVersion;
	char mModelName[20];
	char mComment[256];
};
PMDHeader gPMDHeader{};

struct PMDVertex
{
	XMFLOAT3 mPos;
	XMFLOAT3 normal;
	XMFLOAT2 uv;
	uint16_t boneNo[2];
	uint8_t boneWeight;
	uint8_t edgeFlag;
};
PMDVertex gPMDVertex{};

std::vector<uint8_t> gVertices{};
std::vector<uint16_t> gIndices{};
constexpr size_t gPMDVertexSize = 38;

//グラフィクスパイプラインステートまわり
ComPtr<ID3D12Resource> gVertBuffer;
D3D12_VERTEX_BUFFER_VIEW gVertBufferView;
ComPtr<ID3D12Resource> gIdxBuffer;
D3D12_INDEX_BUFFER_VIEW gIdxBufferView;
constexpr UINT gRenderTargetsNum = 1; // 1 ~ 8の範囲から指定する
ComPtr<ID3D12RootSignature> gRootSignature;
ComPtr<ID3D12PipelineState> gPipelineState;

//ビューポートとシザー矩形
//これらはCOMではない
CD3DX12_VIEWPORT gViewport;
CD3DX12_RECT gScissorRect;

/*end*/

/// <summary>
/// デバッグ用
/// </summary>
void EnableDebugLayer() {
	ID3D12Debug* debugLayer = nullptr;
	auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
	debugLayer->EnableDebugLayer();
	debugLayer->Release();
}

/// <summary>
/// イベントメッセージ処理コールバック
/// こちらにメインループの処理を入れてもよい ==> ポールイベントからのループ処理と同一
/// </summary>
/// 以下テンプレート
/// <param name="hwnd"></param>
/// <param name="msg"></param>
/// <param name="wparam"></param>
/// <param name="lparam"></param>
/// <returns></returns>
LRESULT CALLBACK AppWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProc(hwnd, msg, wparam, lparam);
}

/// <summary>
/// ウィンドウの基本情報登録
/// </summary>
/// <returns>テンプレートでそう書いてあった</returns>
ATOM RegisterWindowParam()
{
	gWindowClass.cbSize = sizeof(WNDCLASSEX);
	gWindowClass.lpfnWndProc = (WNDPROC)AppWndProc;
	gWindowClass.lpszClassName = _T("DirectX12Sample");
	gWindowClass.hInstance = gHInstance;
	return RegisterClassEx(&gWindowClass);
}

/// <summary>
/// ウィンドウのベース矩形の初期化
/// </summary>
void InitWindowRect()
{
	gWindowRect.left = 0;
	gWindowRect.right = gWindowWidth;
	gWindowRect.top = 0;
	gWindowRect.bottom = gWindowHeight;
}

/// <summary>
/// ウィンドウサイズ情報を矩形から作成，その後種々の情報からウィンドウを作成し返す
/// </summary>
void CreateAppWindow()
{
	RegisterWindowParam();

	InitWindowRect();
	::AdjustWindowRect(&gWindowRect, WS_OVERLAPPEDWINDOW, false);

	auto windowWidth = gWindowRect.right - gWindowRect.left,
		windowHeight = gWindowRect.bottom - gWindowRect.top;

	gHwnd = ::CreateWindow(
		gWindowClass.lpszClassName, gWindowClass.lpszClassName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		windowWidth, windowHeight,
		nullptr, nullptr, gWindowClass.hInstance, nullptr
	);
}

/// <summary>
///directxのリソース等を作る要のデバイスを作成
/// </summary>
void MakeAppDevice()
{
	/*バージョン合わせ*/
	//バージョン格納
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	//GPUとのアダプタを得るために必要なファクトリを作成
	ThrowIfFailed(::CreateDXGIFactory1(IID_PPV_ARGS(&gDxgiFactory)));

	//NVIDIA製GPUを使用するための処理
	int i = 0;
	while (gDxgiFactory->EnumAdapters(i, &gDxgiAdapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC adapterDesc{};
		ThrowIfFailed(gDxgiAdapter->GetDesc(&adapterDesc));
		std::wstring wstr = adapterDesc.Description;
		if (wstr.find(L"NVIDIA") != std::string::npos)
			break;
		i++;
	}

	//D3Dデバイスの作成, gFeatureLevelにはlevelsのどのバージョンにてデバイスを作成したか保存される
	for (auto& level : levels)
	{
		if (SUCCEEDED(D3D12CreateDevice(gDxgiAdapter.Get(), level, IID_PPV_ARGS(&gDevice))))
		{
			gFeatureLevel = level;
			break;
		}
	}
}

/// <summary>
/// コマンドアロケータ・コマンドリスト・コマンドキューの作成
/// </summary>
void CreateAppGPUCommandTools()
{
	//コマンドアロケータとコマンドリストの生成
	ThrowIfFailed(gDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&gCommandAllocator)));
	ThrowIfFailed(gDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, gCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&gCommandList)));

	/*コマンドキューの作成*/
	//デスクリプション構築
	D3D12_COMMAND_QUEUE_DESC commandQDesc{};
	commandQDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQDesc.NodeMask = 0;
	commandQDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; //コマンドアロケータ・コマンドリストのものと合わせる
	//作成
	ThrowIfFailed(gDevice->CreateCommandQueue(&commandQDesc, IID_PPV_ARGS(&gCommandQ)));
}

/// <summary>
/// スワップチェインの作成
/// </summary>
void CreateAppSwapChain()
{
	//デスクリプション構築
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = gWindowWidth;
	swapChainDesc.Height = gWindowHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapChainDesc.BufferCount = gBufferCount; //ダブルバッファなら2
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	//作成
	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(gDxgiFactory->CreateSwapChainForHwnd(gCommandQ.Get(), gHwnd, &swapChainDesc, nullptr, nullptr, &swapChain1));
	//キャスト, idxgi~4はcreate~hwndの引数に渡せないから
	swapChain1.As(&gSwapChain);
}

/// <summary>
/// 深度がらみの設定
/// </summary>
void CreateDepthTools()
{
	auto depthResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_D32_FLOAT, gWindowWidth, gWindowHeight
	);
	depthResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	auto depthHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	auto depthClearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0	);

	ThrowIfFailed(gDevice->CreateCommittedResource(
		&depthHeapProps, D3D12_HEAP_FLAG_NONE, &depthResourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue, IID_PPV_ARGS(&gDepthBuffer)
	));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	
	ThrowIfFailed(gDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&gDSVHeaps)));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	gDevice->CreateDepthStencilView(
		gDepthBuffer.Get(), &dsvDesc, gDSVHeaps->GetCPUDescriptorHandleForHeapStart()
	);
}

/// <summary>
/// バックバッファとレンダーターゲットビューの作成
/// および関連付け？
/// </summary>
void CreateBackBufferAndRTV()
{
	//レンダーターゲットビューのディスクリプションヒープのディスクリプション構築
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descriptorHeapDesc.NodeMask = 0;
	descriptorHeapDesc.NumDescriptors = gBufferCount; //ダブルバッファなら2
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	//レンダーターゲットビューのディスクリプションヒープ作成
	ThrowIfFailed(gDevice->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&gRTVHeaps)));

	//ディスクリプションヒープの開始アドレスを取得
	auto rtvHeapsHandle = gRTVHeaps->GetCPUDescriptorHandleForHeapStart();

	//レンダーターゲットビューのディスクリプション構築
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
#ifdef _DEBUG
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
#else
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
#endif
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	//バックバッファとレンダーターゲットビューの関連付け
	for (UINT idx = 0; idx < gBufferCount; idx++)
	{
		ThrowIfFailed(gSwapChain->GetBuffer(idx, IID_PPV_ARGS(&(gBackBuffers[idx])))); //バックバッファをスワップチェインから取得
		gDevice->CreateRenderTargetView(gBackBuffers[idx].Get(), &rtvDesc, rtvHeapsHandle); //レンダーターゲットビュー作成，同一のidxのバックバッファと対応
		rtvHeapsHandle.ptr += gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV); //ディスクリプションヒープの対応アドレスを更新，オフセットサイズを算出するために関数を用いる
	}
}

/// <summary>
/// 画面クリアに必要な最低限の準備
/// </summary>
void PrepareClearWindow()
{
	CreateAppWindow();

#ifdef _DEBUG
	EnableDebugLayer();
#endif

	MakeAppDevice();
	CreateAppGPUCommandTools();
	CreateAppSwapChain();
	CreateBackBufferAndRTV();
	CreateDepthTools(); //必須ではないが，呼び出し場所としてはここがふさわしい

	//フェンスの作成
	ThrowIfFailed(gDevice->CreateFence(gFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gFence)));
}

/// <summary>
/// コマンドリストの実行とフェンスからの連絡待ち
/// </summary>
void ExecuteAppCommandLists(bool isPipelineUsed)
{
	//このフレームにおける命令は以上です，という意思表示
	//これより上で呼び出したコマンドリストの命令が，コマンドキューによって後に実行される．
	gCommandList->Close();

	//コマンドキューによるコマンドリストの実行
	ID3D12CommandList* const commandLists[] = { gCommandList.Get() }; //コマンドリストを親クラスの配列に格納
	gCommandQ->ExecuteCommandLists(_countof(commandLists), commandLists); //後々，複数のコマンドリストを渡すことになると予想

	//GPUへ命令実行が終了したか確認するための信号を送る
	gFenceValue++; //fenceからの連絡番号
	gCommandQ->Signal(gFence.Get(), gFenceValue); //命令実行が終了したら，この番号で送り返してという信号を送る

	//フェンスによってGPUの命令実行終了を連絡してもらう
	//待たないと都合が悪いので連絡が来るまで待つ
	if (gFence->GetCompletedValue() != gFenceValue) //フェンスから連絡がなかったらビジーウェイト
	{
		auto fenceEvent = ::CreateEvent(nullptr, false, false, nullptr);
		assert(fenceEvent && "failed to create fence event."); //これをいれないと警告がうるさい. 空のイベントを使用するため
		gFence->SetEventOnCompletion(gFenceValue, fenceEvent);
		::WaitForSingleObject(fenceEvent, INFINITE);
		::CloseHandle(fenceEvent);
	}

	//ためておいた命令をクリア
	//GPUとの通信を終了するごとにクリア(同フレームでもクリアすることがある)
	//パイプラインを使用する場合，コマンドリストのクリアに渡す必要がある
	auto pTetmp = isPipelineUsed ? gPipelineState.Get() : nullptr;
	gCommandAllocator->Reset();
	gCommandList->Reset(gCommandAllocator.Get(), pTetmp);
}

size_t AlignmentedSize(size_t size, size_t alignment) {
	return size + alignment - size % alignment;
}

/// <summary>
/// ルートシグネチャの作成, パイプライン外リソースの作成・送信
/// </summary>
void CreateAppResources()
{
	std::vector<CD3DX12_DESCRIPTOR_RANGE> descTblRange{}; //ベクターにすれば，汎用性が向上する？
	//テクスチャ用
	descTblRange.push_back(CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, gTextureNum, 0));
	//行列用
	descTblRange.push_back(CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, gConstantBufferNum, 0));

	D3D12_ROOT_PARAMETER rootParam{};
	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam.DescriptorTable.pDescriptorRanges = &descTblRange[0];
	rootParam.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(descTblRange.size());
	rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	auto rootSignatureDesc = CD3DX12_ROOT_SIGNATURE_DESC(
		1, &rootParam, gTextureNum, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);
	ComPtr<ID3DBlob> rootSignatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	ThrowIfFailed(::D3D12SerializeRootSignature(
		&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSignatureBlob, &errorBlob
	));
	ThrowIfFailed(gDevice->CreateRootSignature(
		0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&gRootSignature)
	));

	/*テクスチャ*/

	TexMetadata metadata{};
	ScratchImage scratchImg{};
	//ThrowIfFailed(LoadFromWICFile(L"assets/textest200x200.png", WIC_FLAGS_NONE, &metadata, scratchImg));
	ThrowIfFailed(LoadFromWICFile(L"assets/textest.png", WIC_FLAGS_NONE, &metadata, scratchImg));
	auto img = scratchImg.GetImage(0, 0, 0);

	//CPUとGPU間のバッファ
	D3D12_HEAP_PROPERTIES uploadHeapProp{};
	uploadHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	uploadHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	uploadHeapProp.CreationNodeMask = 0;
	uploadHeapProp.VisibleNodeMask = 0;

	//上記のバッファと異なる
	//シェーダから見えるバッファ
	D3D12_RESOURCE_DESC resDesc{};
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	auto pixelsize = scratchImg.GetPixelsSize();
	resDesc.Width = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * img->height;
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;

	ComPtr<ID3D12Resource> uploadbuff = nullptr;
	ThrowIfFailed(gDevice->CreateCommittedResource(
		&uploadHeapProp, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadbuff))
	);

	D3D12_HEAP_PROPERTIES texHeapProp{};
	texHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	texHeapProp.CreationNodeMask = 0;
	texHeapProp.VisibleNodeMask = 0;

	resDesc.Format = metadata.format;
	resDesc.Width = static_cast<UINT>(metadata.width);
	resDesc.Height = static_cast<UINT>(metadata.height);
	resDesc.DepthOrArraySize = static_cast<UINT16>(metadata.arraySize);
	resDesc.MipLevels = static_cast<UINT16>(metadata.mipLevels);
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	ThrowIfFailed(gDevice->CreateCommittedResource(
		&texHeapProp, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&gTexBuffer))
	);
	uint8_t* mapforImg = nullptr;
	ThrowIfFailed(uploadbuff->Map(0, nullptr, (void**)&mapforImg));
	auto srcAddress = img->pixels;
	auto rowPitch = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
	for (size_t y = 0; y < img->height; y++)
	{
		std::copy_n(srcAddress, rowPitch, mapforImg);
		srcAddress += img->rowPitch;
		mapforImg += rowPitch;
	}
	uploadbuff->Unmap(0, nullptr);

	D3D12_TEXTURE_COPY_LOCATION src{}, dst{};
	dst.pResource = gTexBuffer.Get();
	dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dst.SubresourceIndex = 0;

	src.pResource = uploadbuff.Get();
	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
	UINT nrow;
	UINT64 rowsize, size;
	auto desc = gTexBuffer->GetDesc();
	gDevice->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, &nrow, &rowsize, &size);
	src.PlacedFootprint = footprint;
	src.PlacedFootprint.Offset = 0;
	src.PlacedFootprint.Footprint.Width = static_cast<UINT>(metadata.width);
	src.PlacedFootprint.Footprint.Height = static_cast<UINT>(metadata.height);
	src.PlacedFootprint.Footprint.Depth = static_cast<UINT>(metadata.depth);
	src.PlacedFootprint.Footprint.RowPitch = static_cast<UINT>(AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT));
	src.PlacedFootprint.Footprint.Format = img->format;

	/*end*/

	/*行列*/
	gMatrix.mWorld = XMMatrixRotationY(XM_PIDIV4);
	XMFLOAT3 eye(0.0f, 10.0f, -15.0f);
	XMFLOAT3 target(0.0f, 10.0f, 0.0f);
	XMFLOAT3 up(0.0f, 1.0f, 0.0f);
	gMatrix.mViewProjection = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	gMatrix.mViewProjection *= XMMatrixPerspectiveFovLH(
		XM_PIDIV2, static_cast<float>(gWindowWidth) / static_cast<float>(gWindowHeight),
		1.0f, 100.0f
	);

	auto pTempHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto pTempResourceDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(MatrixData) + 0xff) & ~0xff); //アラインメント
	ThrowIfFailed(gDevice->CreateCommittedResource(
		&pTempHeapProps, D3D12_HEAP_FLAG_NONE, &pTempResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&gConstantBuffer)
	));

	ThrowIfFailed(gConstantBuffer->Map(0, nullptr, (void**)&gMapMatrix));
	gMapMatrix->mWorld = gMatrix.mWorld;
	gMapMatrix->mViewProjection = gMatrix.mViewProjection;
	//ループ内で変換させる場合はマップしたままにしておく

	/*end*/

	/*リソース作成の仕上げ*/

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = static_cast<UINT>(descTblRange.size());
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ThrowIfFailed(gDevice->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&gResourceDescHeap)));

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	auto resourceDescHeapHandle = gResourceDescHeap->GetCPUDescriptorHandleForHeapStart();

	gDevice->CreateShaderResourceView(
		gTexBuffer.Get(), &srvDesc, resourceDescHeapHandle
	);

	resourceDescHeapHandle.ptr += gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	cbvDesc.BufferLocation = gConstantBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = static_cast<UINT>(gConstantBuffer->GetDesc().Width);
	//定数バッファビューの作成
	gDevice->CreateConstantBufferView(&cbvDesc, resourceDescHeapHandle);

	/*end*/

	/*GPUへ送信*/

	gCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
	gBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		gTexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	gCommandList->ResourceBarrier(1, &gBarrier);
	ExecuteAppCommandLists(false); //パイプライン外なのでfalse

	/*end*/
}

/// <summary>
/// グラフィクスパイプラインステートを作成
/// </summary>
void CreateAppGraphicsPipelineState()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = nullptr;
	//シェーダのコンパイルとアセンブリの登録
	ComPtr<ID3DBlob> vsBlob, psBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"VertexShader.cso", &vsBlob));
	ThrowIfFailed(D3DReadFileToBlob(L"PixelShader.cso", &psBlob));
	graphicsPipelineStateDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
	graphicsPipelineStateDesc.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());
	//サンプルマスク
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	//ブレンドステート
	//D3D12_DEFAULTを渡さないと，特殊なコンストラクタが呼ばれない → デフォルトコンストラクタはせいぜい0クリアぐらいの機能しかない
	graphicsPipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	//ラスタライザステート
	graphicsPipelineStateDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	graphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	//深度ステンシルステート
	graphicsPipelineStateDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	//深度ビューの設定
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	//インプットレイアウト
	D3D12_INPUT_ELEMENT_DESC inputLayouts[] =
	{
		{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "BONE_NO",0,DXGI_FORMAT_R16G16_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "WEIGHT",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "EDGE_FLG",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	};
	graphicsPipelineStateDesc.InputLayout.pInputElementDescs = inputLayouts;
	graphicsPipelineStateDesc.InputLayout.NumElements = _countof(inputLayouts);
	//幾何情報
	graphicsPipelineStateDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//レンダーターゲットの設定
	graphicsPipelineStateDesc.NumRenderTargets = gRenderTargetsNum;
	for (UINT idx = 0; idx < gRenderTargetsNum; idx++)
		graphicsPipelineStateDesc.RTVFormats[idx] = DXGI_FORMAT_R8G8B8A8_UNORM;
	//サンプリング
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleDesc.Quality = 0;
	//ルートシグネチャの登録
	graphicsPipelineStateDesc.pRootSignature = gRootSignature.Get();
	//パイプラインステート作成
	ThrowIfFailed(gDevice->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&gPipelineState)));
}

/// <summary>
/// インプットアセンブラステージで送る3Dモデルの頂点情報作成
/// </summary>
void LoadMMD()
{
	FILE* fp = nullptr;
	auto err = ::fopen_s(&fp, "assets/初音ミク.pmd", "rb");
	if (fp == nullptr) 
	{
		char strerr[256];
		strerror_s(strerr, 256, err);
		::MessageBox(::GetActiveWindow(), strerr, "Open Error", MB_ICONERROR);
		return;
	}

	char magicNum[3]{};
	fread_s(magicNum, sizeof(magicNum), sizeof(char), sizeof(magicNum) / sizeof(char), fp);

	fread_s(&(gPMDHeader.mVersion), sizeof(gPMDHeader.mVersion), sizeof(float), sizeof(gPMDHeader.mVersion) / sizeof(float), fp);
	fread_s(gPMDHeader.mModelName, sizeof(gPMDHeader.mModelName), sizeof(char), sizeof(gPMDHeader.mModelName) / sizeof(char), fp);
	fread_s(gPMDHeader.mComment, sizeof(gPMDHeader.mComment), sizeof(char), sizeof(gPMDHeader.mComment) / sizeof(char), fp);

	uint32_t vertNum = 0;
	fread_s(&vertNum, sizeof(vertNum), sizeof(uint32_t), 1, fp);
	gVertices.resize(vertNum * gPMDVertexSize);
	fread_s(gVertices.data(), gVertices.size() * sizeof(uint8_t), sizeof(uint8_t), gVertices.size(), fp);

	uint32_t indicesNum = 0;
	fread_s(&indicesNum, sizeof(indicesNum), sizeof(uint32_t), 1, fp);
	gIndices.resize(indicesNum);
	fread_s(gIndices.data(), gIndices.size() * sizeof(uint16_t), sizeof(uint16_t), gIndices.size(), fp);
}

/// <summary>
///	 インプットアセンブラステージで使用するリソースを作成
/// </summary>
void CreateInputAssembly()
{
	LoadMMD();
	auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(gVertices.size() * sizeof(gVertices[0]));
	ThrowIfFailed(gDevice->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&gVertBuffer)
	));

	uint8_t* vertMap = nullptr;
	ThrowIfFailed(gVertBuffer->Map(0, nullptr, (void**)&vertMap));
	std::copy(gVertices.begin(), gVertices.end(), vertMap);
	gVertBuffer->Unmap(0, nullptr);

	gVertBufferView.BufferLocation = gVertBuffer->GetGPUVirtualAddress(); //GPU側のバッファの仮想アドレス
	gVertBufferView.SizeInBytes = static_cast<UINT>(gVertices.size() * sizeof(gVertices[0]));
	gVertBufferView.StrideInBytes = gPMDVertexSize;

	resourceDesc.Width = static_cast<UINT64>(gIndices.size() * sizeof(gIndices[0]));
	ThrowIfFailed(gDevice->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&gIdxBuffer)
	));

	uint16_t* idxMap = nullptr;
	ThrowIfFailed(gIdxBuffer->Map(0, nullptr, (void**)&idxMap));
	std::copy(gIndices.begin(), gIndices.end(), idxMap);
	gIdxBuffer->Unmap(0, nullptr);

	gIdxBufferView.BufferLocation = gIdxBuffer->GetGPUVirtualAddress();
	gIdxBufferView.Format = DXGI_FORMAT_R16_UINT;
	gIdxBufferView.SizeInBytes = static_cast<UINT>(gIndices.size() * sizeof(gIndices[0]));	
}

/// <summary>
/// ビューポートとシザー矩形を作成
/// </summary>
void CreateViewportAndScissorRect()
{
	//ビューポート作成
	gViewport = CD3DX12_VIEWPORT(
		static_cast<FLOAT>(0), static_cast<FLOAT>(0),
		static_cast<FLOAT>(gWindowWidth), static_cast<FLOAT>(gWindowHeight)
	);
	//シザー矩形作成，ビューポートと違い，パラメータが矩形の4頂点の座標であることに注意
	gScissorRect = CD3DX12_RECT(
		static_cast<LONG>(0), static_cast<LONG>(0),
		static_cast<LONG>(gWindowWidth), static_cast<LONG>(gWindowHeight)
	);
	gScissorRect.right += gScissorRect.left;
	gScissorRect.bottom += gScissorRect.top;
}

/// <summary>
/// グラフィクスパイプラインの構築
/// </summary>
void ConstructGraphicsPipeline()
{
	//グラフィクスパイプラインステートの作成
	CreateAppGraphicsPipelineState();
	//インプットアセンブリを作成
	CreateInputAssembly();
	//ビューポートとシザー矩形を作成
	CreateViewportAndScissorRect();
}

/// <summary>
/// 毎フレームのモデルの更新処理
/// </summary>
void Update()
{
	gAngle += 0.06f;
	gMatrix.mWorld = XMMatrixRotationY(gAngle);
	gMapMatrix->mWorld = gMatrix.mWorld;
}

/// <summary>
/// Renderから呼び出される
/// レンダーターゲットビューのクリア
/// </summary>
void ClearAppRenderTargetView()
{
	//レンダーターゲットの指定
	auto rtvHeapsHandle = gRTVHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvHeapsHandle.ptr += static_cast<unsigned long long>(gCurrentBackBufferIdx) * gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//深度バッファの指定
	auto dsvHandle = gDSVHeaps->GetCPUDescriptorHandleForHeapStart();
	//パイプラインのOMステージの対象はこのRTVだよーということ
	//ないと画面クリアされたものしか表示されない(パイプラインの結果は表示されない)
	gCommandList->OMSetRenderTargets(gRenderTargetsNum, &rtvHeapsHandle, false, &dsvHandle);

	//レンダーターゲットビューを一色で塗りつぶしてクリア → 表示すると塗りつぶされていることがわかる
	gCommandList->ClearRenderTargetView(rtvHeapsHandle, gClearColor, 0, nullptr);
	//深度バッファをクリア
	gCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

/// <summary>
/// パイプライン外でリソースをセット
/// </summary>
void SetAppGPUResources()
{
	gCommandList->SetGraphicsRootSignature(gRootSignature.Get()); //ルートシグネチャ(シェーダから扱えるレジスタが定義されたディスクリプタテーブル)をコマンドリストにセット
	gCommandList->SetDescriptorHeaps(gTextureNum, gResourceDescHeap.GetAddressOf());
	gCommandList->SetGraphicsRootDescriptorTable(0, gResourceDescHeap->GetGPUDescriptorHandleForHeapStart());
}

/// <summary>
/// インプットアセンブラステージにおけるコマンドをセット
/// </summary>
void SetCommandsOnIAStage()
{
	gCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	gCommandList->IASetVertexBuffers(0, gRenderTargetsNum, &gVertBufferView);
	gCommandList->IASetIndexBuffer(&gIdxBufferView);
}

/// <summary>
/// ラスタライザステージにおけるコマンドをセット
/// ビューポートとシザー矩形をセットするのもここ
/// </summary>
void SetCommandsOnRStage()
{
	//ビューポートとシザー矩形をコマンドリストにセット → 画面に対してオブジェクトをどう描画するか，その描画結果の表示領域をどうするかが決定される
	gCommandList->RSSetViewports(gRenderTargetsNum, &gViewport);
	gCommandList->RSSetScissorRects(gRenderTargetsNum, &gScissorRect);
}

/// <summary>
/// グラフィクスパイプラインのためのコマンドをコマンドリストにセット
/// </summary>
void SetCommandsForGraphicsPipeline()
{
	//リソースをセット
	SetAppGPUResources();

	//パイプラインステートをコマンドリストにセット
	gCommandList->SetPipelineState(gPipelineState.Get());

	//インプットアセンブラステージ
	SetCommandsOnIAStage();
	//ラスタライザステージ
	SetCommandsOnRStage();
	//描画
	//gCommandList->DrawInstanced(gVertices.size() / gPMDVertexSize, 1, 0, 0);
	gCommandList->DrawIndexedInstanced(static_cast<UINT>(gIndices.size()), 1, 0, 0, 0);
}

/// <summary>
/// 毎フレームのビューの描画処理
/// </summary>
void Render()
{
	//このフレームの表示バッファを取得
	gCurrentBackBufferIdx = gSwapChain->GetCurrentBackBufferIndex(); //おそらく，swapchain->presentで切り替わるものと思われる
	auto& backBuffer = gBackBuffers[gCurrentBackBufferIdx]; //インデックスが毎フレーム交互に入れ替わる

	//バッファの操作を安全に行うための処理
	gBarrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	gCommandList->ResourceBarrier(1, &gBarrier);

	/*GPUに対する命令追加*/
	//レンダーターゲットビューのクリア
	ClearAppRenderTargetView();
	//グラフィクスパイプライン(実行ではなく登録)
	SetCommandsForGraphicsPipeline();
	/*end*/

	//バッファの操作を安全に終了するための処理，画面に表示する準備ができた
	gBarrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	gCommandList->ResourceBarrier(1, &gBarrier);

	//コマンドキューによるコマンドリストの命令実行，およびフェンスからの連絡待ち
	//パイプラインを使用するのでtrueを渡す
	ExecuteAppCommandLists(true);

	//カレントバッファを表示，その後，カレントバッファが切り替わる
	gSwapChain->Present(1, 0);
}

/// <summary>
/// メインループ
/// </summary>
void MainLoop()
{
	MSG msg = {};
	while (true)
	{
		if (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
		if (msg.message == WM_QUIT)
			break;

		Update();
		Render();
	}
}

/// <summary>
/// 実行ファイルの場所を基準位置に設定
/// </summary>
void SetBasePath()
{
	WCHAR path[MAX_PATH];
	HMODULE hModule = ::GetModuleHandle(NULL);
	if (::GetModuleFileNameW(hModule, path, MAX_PATH) > 0)
	{
		::PathRemoveFileSpecW(path);
		::SetCurrentDirectoryW(path);
	}
}

/// <summary>
/// エントリポイント
/// </summary>
/// <param name="hInst"></param>
/// <param name="_hInst"></param>
/// <param name="_lpstr"></param>
/// <param name="_intnum"></param>
/// <returns></returns>
int APIENTRY WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE _hInst, _In_ LPSTR _lpstr, _In_ int _intnum)
{
	UNREFERENCED_PARAMETER(_hInst);
	UNREFERENCED_PARAMETER(_lpstr);
	UNREFERENCED_PARAMETER(_intnum);

	//高DPI対応
	::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	//相対パスを有効にする
	SetBasePath();

	//インスタンス取得
	gHInstance = hInst;

	//画面クリアの準備
	PrepareClearWindow();

	//ウィンドウ表示
	::ShowWindow(gHwnd, SW_SHOW);

	//ルートシグネチャの作成
	//リソースをGPUへ送る
	CreateAppResources();
	//グラフィクスパイプラインの構築, 作成したルートシグネチャの登録もここで行う
	ConstructGraphicsPipeline();

	LoadMMD();

	//メインループ
	MainLoop();

	//終了処理
	UnregisterClass(gWindowClass.lpszClassName, gWindowClass.hInstance);
	return 0;
}