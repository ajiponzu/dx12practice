#include "Core.h"
#include "Window.h"
#include "Scene.h"
#include "Utility.h"

using WindowPtr = std::shared_ptr<Window>;
using WindowMap = std::unordered_map<HWND, WindowPtr>;

/*staticグローバル変数*/
static  WindowMap gWindows;
static std::unique_ptr<Core> g_pSingleton = nullptr;
/*end*/

/*staticクラスメンバ*/
HWND Core::sHwnd = nullptr;
/*end*/

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WindowPtr pWindow;
	{
		WindowMap::iterator iter = gWindows.find(hwnd);
		if (iter != gWindows.end())
			pWindow = iter->second;
	}

	if (pWindow)
	{
		switch (msg)
		{
		case WM_PAINT:
			pWindow->Update();
			pWindow->Render();
			break;
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			break;
		case WM_SYSKEYUP:
		case WM_KEYUP:
			break;
		case WM_SYSCHAR:
			break;
		case WM_MOUSEMOVE:
			break;
		case WM_SIZE:
			break;
		case WM_DESTROY:
			gWindows.erase(hwnd);
			::DestroyWindow(hwnd);
			//終了時，タイミングが悪いとGPU上に未処理の命令が残ることがある
			//強引に掃除すると危ないので，実行してから片づける
			g_pSingleton->ExecuteAppCommandLists();
			g_pSingleton->ResetGPUCommand();
			if (gWindows.empty())
				PostQuitMessage(0);
			break;
		default:
			return DefWindowProcW(hwnd, msg, wParam, lParam);
		}
	}
	else
		return DefWindowProcW(hwnd, msg, wParam, lParam);

	return 0;
}

void Core::MakeInstance(HINSTANCE hInst, const std::wstring title, const int& wid, const int& high)
{
	//スマートポインタでシングルトン
	// 	普通に書くと，別のクラスメンバであるmake_uniqueでprivateコンストラクタを呼び出そうとしてエラー
	// 	よって下記参考
	//「http://msty.hatenablog.jp/entry/2017/05/14/172653」
	if (!g_pSingleton)
	{
		//Coreを継承した一時的な構造体を作成
		//構造体なら基本がpublicメンバなので, 綺麗に書ける
		struct Instantiate : public Core
		{
			//親コンストラクタを呼び出し
			//メンバ関数内なので, privateコンストラクタも呼び出せる
			Instantiate(HINSTANCE hInst, const std::wstring& title, const int& wid = 1280, const int& high = 720)
				: Core(hInst, title, wid, high)
			{}
		};
		//あくまでmake_uniqueが直接呼び出すのはpublicなInstantiateコンストラクタ
		g_pSingleton = std::make_unique<Instantiate>(hInst, title, wid, high);
	}
}

void Core::SetWindow(const int& wid, const int& high, UINT bufferCount)
{
	auto pWindow = std::make_shared<Window>(sHwnd, bufferCount, wid, high);
	gWindows[sHwnd] = pWindow;
}

Core& Core::GetInstance()
{
	assert(g_pSingleton);
	return *g_pSingleton;
}

void Core::Run()
{
	Run(nullptr);
}

void Core::Run(std::unique_ptr<Scene>&& scene)
{
	if (!g_pSingleton)
		return;

	auto& pWindow = gWindows[sHwnd];

	if (!scene)
		scene = std::make_unique<Scene>();
	pWindow->SetScene(std::move(scene));

	::ShowWindow(sHwnd, SW_SHOW);

	pWindow->MakeGraphicsPipeline();

	MSG msg{ 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

void Core::Init(const int& wid, const int& high)
{
	//ウィンドウ作成
	sHwnd = MakeWindow(wid, high);

#ifdef _DEBUG
	Utility::EnableDebugLayer();
#endif

	MakeDxgiFactory();
	MakeDxDevice();
	MakeGPUCommandTools();
	MakeFence();
}

ATOM Core::RegisterWindowParam()
{
	mWndClass.cbSize = sizeof(WNDCLASSEX);
	mWndClass.lpfnWndProc = (WNDPROC)WndProc;
	mWndClass.lpszClassName = mWndTitle.c_str();
	mWndClass.hInstance = mHInstance;
	return RegisterClassEx(&mWndClass);
}

/// <summary>
/// ウィンドウ矩形の初期化
/// </summary>
void Core::InitWindowRect(const int& wid, const int& high)
{
	mWndRect.left = 0;
	mWndRect.right = wid;
	mWndRect.top = 0;
	mWndRect.bottom = high;
}

HWND Core::MakeWindow(const int& wid, const int& high)
{
	RegisterWindowParam();
	InitWindowRect(wid, high);
	::AdjustWindowRect(&mWndRect, WS_OVERLAPPEDWINDOW, false);

	return ::CreateWindow(
		mWndClass.lpszClassName, mWndClass.lpszClassName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		wid, high, nullptr, nullptr, mWndClass.hInstance, nullptr
	);
}

void Core::MakeDxgiFactory()
{
	//GPUとのアダプタを得るために必要なファクトリを作成
	ThrowIfFailed(::CreateDXGIFactory1(IID_PPV_ARGS(&mDxgiFactory)));
}

void Core::MakeDxDevice()
{
	//NVIDIA製GPUを使用するための処理
	int i = 0;
	while (mDxgiFactory->EnumAdapters(i, &mDxgiAdapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC adapterDesc{};
		ThrowIfFailed(mDxgiAdapter->GetDesc(&adapterDesc));
		std::wstring wstr = adapterDesc.Description;
		if (wstr.find(L"NVIDIA") != std::string::npos)
			break;
		i++;
	}

	//バージョン格納
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	//D3Dデバイスの作成, gFeatureLevelにはlevelsのどのバージョンにてデバイスを作成したか保存される
	for (auto& level : levels)
	{
		if (SUCCEEDED(D3D12CreateDevice(mDxgiAdapter.Get(), level, IID_PPV_ARGS(&mDxDevice))))
		{
			mFeatureLevel = level;
			break;
		}
	}
}

void Core::MakeGPUCommandTools()
{
	//コマンドアロケータとコマンドリストの生成
	ThrowIfFailed(mDxDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator)));
	ThrowIfFailed(mDxDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&mCommandList)));

	/*コマンドキューの作成*/
	//デスクリプション構築
	D3D12_COMMAND_QUEUE_DESC commandQDesc{};
	commandQDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQDesc.NodeMask = 0;
	commandQDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; //コマンドアロケータ・コマンドリストのものと合わせる
	//作成
	ThrowIfFailed(mDxDevice->CreateCommandQueue(&commandQDesc, IID_PPV_ARGS(&mCommandQ)));
}

void Core::MakeFence()
{
	//フェンスの作成
	ThrowIfFailed(mDxDevice->CreateFence(mFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
}

void Core::ExecuteAppCommandLists()
{
	//このフレームにおける命令は以上です，という意思表示
	//これより上で呼び出したコマンドリストの命令が，コマンドキューによって後に実行される．
	mCommandList->Close();

	//コマンドキューによるコマンドリストの実行
	ID3D12CommandList* const commandLists[] = { mCommandList.Get() }; //コマンドリストを親クラスの配列に格納
	mCommandQ->ExecuteCommandLists(_countof(commandLists), commandLists); //後々，複数のコマンドリストを渡すことになると予想

	//GPUへ命令実行が終了したか確認するための信号を送る
	mFenceValue++; //fenceからの連絡番号
	mCommandQ->Signal(mFence.Get(), mFenceValue); //命令実行が終了したら，この番号で送り返してという信号を送る

	//フェンスによってGPUの命令実行終了を連絡してもらう
	//待たないと都合が悪いので連絡が来るまで待つ
	if (mFence->GetCompletedValue() != mFenceValue) //フェンスから連絡がなかったらビジーウェイト
	{
		auto fenceEvent = ::CreateEvent(nullptr, false, false, nullptr);
		assert(fenceEvent && "failed to create fence event."); //これをいれないと警告がうるさい. 空のイベントを使用するため
		mFence->SetEventOnCompletion(mFenceValue, fenceEvent);
		::WaitForSingleObject(fenceEvent, INFINITE);
		::CloseHandle(fenceEvent);
	}
}

/// <summary>
/// ためておいた命令をクリア
/// GPUとの通信を終了するごとにクリア
/// コマンドリストからパイプラインを登録解除する
/// </summary>
/// <param name="pipelineState"></param>
void Core::ResetGPUCommand(ComPtr<ID3D12PipelineState>& pipelineState)
{
	mCommandAllocator->Reset();
	mCommandList->Reset(mCommandAllocator.Get(), pipelineState.Get());
}

/// <summary>
/// パイプラインを登録していない場合
/// </summary>
void Core::ResetGPUCommand()
{
	mCommandAllocator->Reset();
	mCommandList->Reset(mCommandAllocator.Get(), nullptr);
}