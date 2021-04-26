#include "Core.h"
#include "Window.h"

#include "Utility.h"

using WindowPtr = std::shared_ptr<Window>;
using WindowMap = std::unordered_map<HWND, WindowPtr>;

/*static�ϐ�*/
static  WindowMap gWindows;
static std::unique_ptr<Core> g_pSingleton = nullptr;
static HWND gHwnd;
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

void Core::MakeInstance(HINSTANCE hInst, std::wstring title, const int& wid, const int& high)
{
	//�X�}�[�g�|�C���^�ŃV���O���g��
	// 	���ʂɏ����ƁC�ʂ̃N���X�����o�ł���make_unique��private�R���X�g���N�^���Ăяo�����Ƃ��ăG���[
	// 	����ĉ��L�Q�l
	//�uhttp://msty.hatenablog.jp/entry/2017/05/14/172653�v
	if (!g_pSingleton)
	{
		//Core���p�������ꎞ�I�ȍ\���̂��쐬
		//�\���̂Ȃ��{��public�����o�Ȃ̂�, �Y��ɏ�����
		struct Instantiate : public Core
		{
			//�e�R���X�g���N�^���Ăяo��
			//�����o�֐����Ȃ̂�, private�R���X�g���N�^���Ăяo����
			Instantiate(HINSTANCE hInst, std::wstring& title, const int& wid = 1280, const int& high = 720)
			: Core(hInst, title, wid, high)
			{}
		};
		//�����܂�make_unique�����ڌĂяo���̂�public��Instantiate�R���X�g���N�^
		g_pSingleton = std::make_unique<Instantiate>(hInst, title, wid, high);
	}
}

void Core::SetWindow(const int& wid, const int& high, UINT bufferCount)
{
	auto pWindow = std::make_shared<Window>(gHwnd, bufferCount, wid, high);
	gWindows[gHwnd] = pWindow;
}

Core& Core::GetInstance()
{
	assert(g_pSingleton);
	return *g_pSingleton;
}

void Core::Run(std::shared_ptr<Scene> pScene)
{
	if (!g_pSingleton)
		return;

	auto hwnd = g_pSingleton->mHwnd;
	auto &pWindow = gWindows[hwnd];
	pWindow->SetScene(pScene);

	::ShowWindow(hwnd, SW_SHOW);

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
	//�E�B���h�E�쐬
	gHwnd = mHwnd = MakeWindow(wid, high);

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
/// �E�B���h�E��`�̏�����
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
		wid, high, 	nullptr, nullptr, mWndClass.hInstance, nullptr
	);
}

void Core::MakeDxgiFactory()
{
	//GPU�Ƃ̃A�_�v�^�𓾂邽�߂ɕK�v�ȃt�@�N�g�����쐬
	ThrowIfFailed(::CreateDXGIFactory1(IID_PPV_ARGS(&mDxgiFactory)));
}

void Core::MakeDxDevice()
{
	//NVIDIA��GPU���g�p���邽�߂̏���
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

	//�o�[�W�����i�[
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	//D3D�f�o�C�X�̍쐬, gFeatureLevel�ɂ�levels�̂ǂ̃o�[�W�����ɂăf�o�C�X���쐬�������ۑ������
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
	//�R�}���h�A���P�[�^�ƃR�}���h���X�g�̐���
	ThrowIfFailed(mDxDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator)));
	ThrowIfFailed(mDxDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&mCommandList)));

	/*�R�}���h�L���[�̍쐬*/
	//�f�X�N���v�V�����\�z
	D3D12_COMMAND_QUEUE_DESC commandQDesc{};
	commandQDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQDesc.NodeMask = 0;
	commandQDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; //�R�}���h�A���P�[�^�E�R�}���h���X�g�̂��̂ƍ��킹��
	//�쐬
	ThrowIfFailed(mDxDevice->CreateCommandQueue(&commandQDesc, IID_PPV_ARGS(&mCommandQ)));
}

void Core::MakeFence()
{
	//�t�F���X�̍쐬
	ThrowIfFailed(mDxDevice->CreateFence(mFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
}

void Core::ExecuteAppCommandLists(bool isPipelineUsed)
{
	//���̃t���[���ɂ����閽�߂͈ȏ�ł��C�Ƃ����ӎv�\��
	//�������ŌĂяo�����R�}���h���X�g�̖��߂��C�R�}���h�L���[�ɂ���Č�Ɏ��s�����D
	mCommandList->Close();

	//�R�}���h�L���[�ɂ��R�}���h���X�g�̎��s
	ID3D12CommandList* const commandLists[] = { mCommandList.Get() }; //�R�}���h���X�g��e�N���X�̔z��Ɋi�[
	mCommandQ->ExecuteCommandLists(_countof(commandLists), commandLists); //��X�C�����̃R�}���h���X�g��n�����ƂɂȂ�Ɨ\�z

	//GPU�֖��ߎ��s���I���������m�F���邽�߂̐M���𑗂�
	mFenceValue++; //fence����̘A���ԍ�
	mCommandQ->Signal(mFence.Get(), mFenceValue); //���ߎ��s���I��������C���̔ԍ��ő���Ԃ��ĂƂ����M���𑗂�

	//�t�F���X�ɂ����GPU�̖��ߎ��s�I����A�����Ă��炤
	//�҂��Ȃ��Ɠs���������̂ŘA��������܂ő҂�
	if (mFence->GetCompletedValue() != mFenceValue) //�t�F���X����A�����Ȃ�������r�W�[�E�F�C�g
	{
		auto fenceEvent = ::CreateEvent(nullptr, false, false, nullptr);
		assert(fenceEvent && "failed to create fence event."); //���������Ȃ��ƌx�������邳��. ��̃C�x���g���g�p���邽��
		mFence->SetEventOnCompletion(mFenceValue, fenceEvent);
		::WaitForSingleObject(fenceEvent, INFINITE);
		::CloseHandle(fenceEvent);
	}

	//���߂Ă��������߂��N���A
	//GPU�Ƃ̒ʐM���I�����邲�ƂɃN���A(���t���[���ł��N���A���邱�Ƃ�����)
	//�p�C�v���C�����g�p����ꍇ�C�R�}���h���X�g�̃N���A�ɓn���K�v������
	//auto pTemp = isPipelineUsed ? mPipelineState.Get() : nullptr;
	auto pTemp = nullptr;
	mCommandAllocator->Reset();
	mCommandList->Reset(mCommandAllocator.Get(), pTemp);
}