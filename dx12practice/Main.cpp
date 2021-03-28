#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlwapi.h>
#include <tchar.h>

#include <d3dx12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <d3dx12.h>

#include <DirectXMath.h>
using namespace DirectX;

//#include <DirectXTex.h>

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

//directx�n��lib�������N����K�v������
//shlwapi.lib�������N����K�v������

/*�O���[�o���ϐ�*/

//�E�B���h�E�܂��
HINSTANCE gHInstance = nullptr;
HWND gHwnd = nullptr;
WNDCLASSEX gWindowClass{};
constexpr auto gWindowWidth = 1280;
constexpr auto gWindowHeight = 720;
RECT gWindowRect{};

//�萔
constexpr UINT gBufferCount = 2;

//directx�K�{�܂��
ComPtr<IDXGIFactory6> gDxgiFactory;
ComPtr<IDXGIAdapter> gDxgiAdapter;
ComPtr<ID3D12Device> gDevice;
ComPtr<ID3D12CommandAllocator> gCommandAllocator;
ComPtr<ID3D12GraphicsCommandList> gCommandList; //�O���t�B�N�X�R�}���h���X�g���g�p���邱�Ƃɒ���
ComPtr<ID3D12CommandQueue> gCommandQ;
ComPtr<IDXGISwapChain4> gSwapChain;
ComPtr<ID3D12DescriptorHeap> gRTVHeaps;
ComPtr<ID3D12Resource> gBackBuffers[gBufferCount];
ComPtr<ID3D12Fence> gFence;
UINT64 gFenceValue = 0;
UINT gCurrentBackBufferIdx = 0;
float gClearColor[] = { 0.5f, 0.9f, 0.8f, 1.0f };
D3D_FEATURE_LEVEL gFeatureLevel{};

//GPU���\�[�X�܂��
ComPtr<ID3D12Resource> gVertBuffer;
D3D12_VERTEX_BUFFER_VIEW gVertBufferView;
ComPtr<ID3D12Resource> gIdxBuffer;
D3D12_INDEX_BUFFER_VIEW gIdxBufferView;

//�O���t�B�N�X�p�C�v���C���X�e�[�g�܂��
UINT gRenderTargetsNum = 1; // 1 ~ 8�͈̔͂���w�肷��
ComPtr<ID3D12RootSignature> gRootSignature;
ComPtr<ID3D12PipelineState> gPipelineState;

//�r���[�|�[�g�ƃV�U�[��`
//������COM�ł͂Ȃ�
std::shared_ptr<CD3DX12_VIEWPORT> gViewport;
std::shared_ptr<CD3DX12_RECT> gScissorRect;

/*end*/

/*�v���g�^�C�v�錾*/
void Update();
void Render();

/// <summary>
/// �f�o�b�O�p
/// </summary>
void EnableDebugLayer() {
	ID3D12Debug* debugLayer = nullptr;
	auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
	debugLayer->EnableDebugLayer();
	debugLayer->Release();
}

/// <summary>
/// �C�x���g���b�Z�[�W�����R�[���o�b�N
/// ������Ƀ��C�����[�v�̏��������Ă��悢 ==> �|�[���C�x���g����̃��[�v�����Ɠ���
/// </summary>
/// �ȉ��e���v���[�g
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
/// �E�B���h�E�̊�{���o�^
/// </summary>
/// <returns>�e���v���[�g�ł��������Ă�����</returns>
ATOM RegisterWindowParam()
{
	gWindowClass.cbSize = sizeof(WNDCLASSEX);
	gWindowClass.lpfnWndProc = (WNDPROC)AppWndProc;
	gWindowClass.lpszClassName = _T("DirectX12Sample");
	gWindowClass.hInstance = gHInstance;
	return RegisterClassEx(&gWindowClass);
}

/// <summary>
/// �E�B���h�E�̃x�[�X��`�̏�����
/// </summary>
void InitWindowRect()
{
	gWindowRect.left = 0;
	gWindowRect.right = gWindowWidth;
	gWindowRect.top = 0;
	gWindowRect.bottom = gWindowHeight;
}

/// <summary>
/// �E�B���h�E�T�C�Y������`����쐬�C���̌��X�̏�񂩂�E�B���h�E���쐬���Ԃ�
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
///directx�̃��\�[�X�������v�̃f�o�C�X���쐬
/// </summary>
void MakeAppDevice()
{
	/*�o�[�W�������킹*/
	//�o�[�W�����i�[
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	//GPU�Ƃ̃A�_�v�^�𓾂邽�߂ɕK�v�ȃt�@�N�g�����쐬
	ThrowIfFailed(::CreateDXGIFactory1(IID_PPV_ARGS(&gDxgiFactory)));

	//NVIDIA��GPU���g�p���邽�߂̏���
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

	//D3D�f�o�C�X�̍쐬, gFeatureLevel�ɂ�levels�̂ǂ̃o�[�W�����ɂăf�o�C�X���쐬�������ۑ������
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
/// �R�}���h�A���P�[�^�E�R�}���h���X�g�E�R�}���h�L���[�̍쐬
/// </summary>
void CreateAppGPUCommandTools()
{
	//�R�}���h�A���P�[�^�ƃR�}���h���X�g�̐���
	ThrowIfFailed(gDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&gCommandAllocator)));
	ThrowIfFailed(gDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, gCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&gCommandList)));

	/*�R�}���h�L���[�̍쐬*/
	//�f�X�N���v�V�����\�z
	D3D12_COMMAND_QUEUE_DESC commandQDesc{};
	commandQDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQDesc.NodeMask = 0;
	commandQDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; //�R�}���h�A���P�[�^�E�R�}���h���X�g�̂��̂ƍ��킹��
	//�쐬
	ThrowIfFailed(gDevice->CreateCommandQueue(&commandQDesc, IID_PPV_ARGS(&gCommandQ)));
}

/// <summary>
/// �X���b�v�`�F�C���̍쐬
/// </summary>
void CreateAppSwapChain()
{
	//�f�X�N���v�V�����\�z
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = gWindowWidth;
	swapChainDesc.Height = gWindowHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapChainDesc.BufferCount = gBufferCount; //�_�u���o�b�t�@�Ȃ�2
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	//�쐬
	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(gDxgiFactory->CreateSwapChainForHwnd(gCommandQ.Get(), gHwnd, &swapChainDesc, nullptr, nullptr, &swapChain1));
	//�L���X�g, idxgi~4��create~hwnd�̈����ɓn���Ȃ�����
	swapChain1.As(&gSwapChain);
}

/// <summary>
/// �o�b�N�o�b�t�@�ƃ����_�[�^�[�Q�b�g�r���[�̍쐬
/// ����ъ֘A�t���H
/// </summary>
void CreateBackBufferAndRTV()
{
	//�����_�[�^�[�Q�b�g�r���[�̃f�B�X�N���v�V�����q�[�v�̃f�B�X�N���v�V�����\�z
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descriptorHeapDesc.NodeMask = 0;
	descriptorHeapDesc.NumDescriptors = gBufferCount; //�_�u���o�b�t�@�Ȃ�2
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	//�����_�[�^�[�Q�b�g�r���[�̃f�B�X�N���v�V�����q�[�v�쐬
	ThrowIfFailed(gDevice->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&gRTVHeaps)));

	//�f�B�X�N���v�V�����q�[�v�̊J�n�A�h���X���擾
	auto rtvHeapsHandle = gRTVHeaps->GetCPUDescriptorHandleForHeapStart();

	//�����_�[�^�[�Q�b�g�r���[�̃f�B�X�N���v�V�����\�z
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	//�o�b�N�o�b�t�@�ƃ����_�[�^�[�Q�b�g�r���[�̊֘A�t��
	for (UINT idx = 0; idx < gBufferCount; idx++)
	{
		ThrowIfFailed(gSwapChain->GetBuffer(idx, IID_PPV_ARGS(&(gBackBuffers[idx])))); //�o�b�N�o�b�t�@���X���b�v�`�F�C������擾
		gDevice->CreateRenderTargetView(gBackBuffers[idx].Get(), &rtvDesc, rtvHeapsHandle); //�����_�[�^�[�Q�b�g�r���[�쐬�C�����idx�̃o�b�N�o�b�t�@�ƑΉ�
		rtvHeapsHandle.ptr += gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV); //�f�B�X�N���v�V�����q�[�v�̑Ή��A�h���X���X�V�C�I�t�Z�b�g�T�C�Y���Z�o���邽�߂Ɋ֐���p����
	}
}

/// <summary>
/// ��ʃN���A�ɕK�v�ȍŒ���̏���
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

	//�t�F���X�̍쐬
	ThrowIfFailed(gDevice->CreateFence(gFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gFence)));
}

using ShaderResources = std::pair<ComPtr<ID3DBlob>, ComPtr<ID3DBlob>>;

/// <summary>
///	 GPU�֑��郊�\�[�X���쐬����
/// </summary>
ShaderResources CreateGPUResources()
{
	struct Vertex 
	{
		XMFLOAT3 pos;//XYZ���W
		XMFLOAT2 uv;//UV���W
	};
	Vertex vertices[] = 
	{
		{{-0.5f,-0.9f,0.0f},{0.0f,1.0f} },//����
		{{-0.5f,0.9f,0.0f} ,{0.0f,0.0f}},//����
		{{0.5f,-0.9f,0.0f} ,{1.0f,1.0f}},//�E��
		{{0.5f,0.9f,0.0f} ,{1.0f,0.0f}},//�E��
	};
	
	unsigned short indices[] = 
	{ 
		0,1,2, 
		2,1,3 
	};

	auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices));
	ThrowIfFailed(gDevice->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&gVertBuffer)
	));

	Vertex* vertMap = nullptr;
	ThrowIfFailed(gVertBuffer->Map(0, nullptr, (void**)&vertMap));
	std::copy(std::begin(vertices), std::end(vertices), vertMap);
	gVertBuffer->Unmap(0, nullptr);

	gVertBufferView.BufferLocation = gVertBuffer->GetGPUVirtualAddress(); //GPU���̃o�b�t�@�̉��z�A�h���X
	gVertBufferView.SizeInBytes = sizeof(vertices); //���_���S�̂̃T�C�Y
	gVertBufferView.StrideInBytes = sizeof(vertices[0]); //���_���������̃T�C�Y

	resourceDesc.Width = sizeof(indices);
	ThrowIfFailed(gDevice->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&gIdxBuffer)
	));

	unsigned short* idxMap = nullptr;
	ThrowIfFailed(gIdxBuffer->Map(0, nullptr, (void**)&idxMap));
	std::copy(std::begin(indices), std::end(indices), idxMap);
	gIdxBuffer->Unmap(0, nullptr);

	gIdxBufferView.BufferLocation = gIdxBuffer->GetGPUVirtualAddress();
	gIdxBufferView.Format = DXGI_FORMAT_R16_UINT;
	gIdxBufferView.SizeInBytes = sizeof(indices);

	ComPtr<ID3DBlob> vsBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"VertexShader.cso", &vsBlob));

	ComPtr<ID3DBlob> psBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"PixelShader.cso", &psBlob));

	return std::make_pair(vsBlob, psBlob);
}

/// <summary>
/// �O���t�B�N�X�p�C�v���C���X�e�[�g���쐬
/// </summary>
/// <param name="shaders">shader�A�Z���u���R�[�h���ɂ��^�v��</param>
void CreateAppGraphicsPipelineState(ShaderResources& shaders)
{
	auto& vsBlob = shaders.first, & psBlob = shaders.second;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = nullptr;
	//�V�F�[�_���\�[�X�̓o�^
	graphicsPipelineStateDesc.VS.pShaderBytecode = vsBlob->GetBufferPointer();
	graphicsPipelineStateDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
	graphicsPipelineStateDesc.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());
	//�T���v���}�X�N
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	//�u�����h�X�e�[�g
	//D3D12_DEFAULT��n���Ȃ��ƁC����ȃR���X�g���N�^���Ă΂�Ȃ� �� �f�t�H���g�R���X�g���N�^�͂�������0�N���A���炢�̋@�\�����Ȃ�
	graphicsPipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	//���X�^���C�U�X�e�[�g
	graphicsPipelineStateDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//�[�x�X�e���V���X�e�[�g
	graphicsPipelineStateDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	graphicsPipelineStateDesc.DepthStencilState.DepthEnable = false;
	//�C���v�b�g���C�A�E�g
	D3D12_INPUT_ELEMENT_DESC inputLayouts[] =
	{
		{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	};
	graphicsPipelineStateDesc.InputLayout.pInputElementDescs = inputLayouts;
	graphicsPipelineStateDesc.InputLayout.NumElements = _countof(inputLayouts);
	//�􉽏��
	graphicsPipelineStateDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//�����_�[�^�[�Q�b�g�̐ݒ�
	graphicsPipelineStateDesc.NumRenderTargets = gRenderTargetsNum;
	for (UINT idx = 0; idx < gRenderTargetsNum; idx++)
		graphicsPipelineStateDesc.RTVFormats[idx] = DXGI_FORMAT_R8G8B8A8_UNORM;
	//�T���v�����O
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleDesc.Quality = 0;
	//���[�g�V�O�l�`��
	auto rootSignatureDesc = CD3DX12_ROOT_SIGNATURE_DESC(
		0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);
	ComPtr<ID3DBlob> rootSignatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	ThrowIfFailed(::D3D12SerializeRootSignature(
		&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSignatureBlob, &errorBlob
	));
	ThrowIfFailed(gDevice->CreateRootSignature(
		0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&gRootSignature)
	));
	graphicsPipelineStateDesc.pRootSignature = gRootSignature.Get();
	//�p�C�v���C���X�e�[�g�쐬
	ThrowIfFailed(gDevice->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&gPipelineState)));
	auto i = 0;
}

void CreateViewportAndScissorRect()
{
	//�r���[�|�[�g�쐬
	gViewport = std::make_shared<CD3DX12_VIEWPORT>(
		static_cast<FLOAT>(0), static_cast<FLOAT>(0),
		static_cast<FLOAT>(gWindowWidth), static_cast<FLOAT>(gWindowHeight)
		);
	//�V�U�[��`�쐬�C�r���[�|�[�g�ƈႢ�C�p�����[�^����`��4���_�̍��W�ł��邱�Ƃɒ���
	gScissorRect = std::make_shared<CD3DX12_RECT>(
		static_cast<LONG>(0), static_cast<LONG>(0),
		static_cast<LONG>(gWindowWidth), static_cast<LONG>(gWindowHeight)
		);
	gScissorRect->right += gScissorRect->left;
	gScissorRect->bottom += gScissorRect->top;
}

/// <summary>
/// ���C�����[�v
/// </summary>
void MainLoop()
{
	MSG msg = {};
	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (msg.message == WM_QUIT)
			break;

		Update();
		Render();
	}
}

/// <summary>
/// ���s�t�@�C���̏ꏊ����ʒu�ɐݒ�
/// </summary>
void SetBasePath()
{
	WCHAR path[MAX_PATH];
	HMODULE hModule = GetModuleHandle(NULL);
	if (GetModuleFileNameW(hModule, path, MAX_PATH) > 0)
	{
		PathRemoveFileSpecW(path);
		SetCurrentDirectoryW(path);
	}
}

/// <summary>
/// �G���g���|�C���g
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

	//���΃p�X��L���ɂ���
	SetBasePath();

	//�C���X�^���X�擾
	gHInstance = hInst;

	//��ʃN���A�̏���
	PrepareClearWindow();

	//GPU���\�[�X�̍쐬
	auto shaders = CreateGPUResources();

	//�O���t�B�N�X�p�C�v���C���X�e�[�g�̍쐬
	CreateAppGraphicsPipelineState(shaders);

	//�\����ʂ̍��W�n���w��
	CreateViewportAndScissorRect();

	//�E�B���h�E�\��
	::ShowWindow(gHwnd, SW_SHOW);

	//���C�����[�v
	MainLoop();

	//�I������
	UnregisterClass(gWindowClass.lpszClassName, gWindowClass.hInstance);
	return 0;
}

/// <summary>
/// ���t���[���̃��f���̍X�V����
/// </summary>
void Update()
{
}

/// <summary>
/// Render����Ăяo�����
/// �����_�[�^�[�Q�b�g�r���[�̃N���A
/// </summary>
/// <param name="backBuffer">���̃t���[���̉�ʕ\���o�b�t�@</param>
void ClearAppRenderTargetView(ComPtr<ID3D12Resource>& backBuffer)
{
	//�����_�[�^�[�Q�b�g�r���[�ɂ��o�b�t�@�̑�������S�ɍs�����߂̏���
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	gCommandList->ResourceBarrier(1, &barrier);

	//�����_�[�^�[�Q�b�g�̎w��
	auto rtvHeapsHandle = gRTVHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvHeapsHandle.ptr += static_cast<unsigned long long>(gCurrentBackBufferIdx) * gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	gCommandList->OMSetRenderTargets(1, &rtvHeapsHandle, false, nullptr);

	//�����_�[�^�[�Q�b�g�r���[����F�œh��Ԃ��ăN���A �� �\������Ɠh��Ԃ���Ă��邱�Ƃ��킩��
	gCommandList->ClearRenderTargetView(rtvHeapsHandle, gClearColor, 0, nullptr);

	//�����_�[�^�[�Q�b�g�r���[�ɂ��o�b�t�@�̑�������S�ɏI�����邽�߂̏����C��ʂɕ\�����鏀�����ł���
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	gCommandList->ResourceBarrier(1, &barrier);
}

/// <summary>
/// �C���v�b�g�A�Z���u���X�e�[�W�ɂ�����R�}���h���Z�b�g
/// </summary>
void SetCommandsOnIAStage()
{
	gCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	gCommandList->IASetVertexBuffers(0, gRenderTargetsNum, &gVertBufferView);
	gCommandList->IASetIndexBuffer(&gIdxBufferView);
}

/// <summary>
/// ���X�^���C�U�X�e�[�W�ɂ�����R�}���h���Z�b�g
/// �r���[�|�[�g�ƃV�U�[��`���Z�b�g����̂�����
/// </summary>
void SetCommandsOnRStage()
{
	//�r���[�|�[�g�ƃV�U�[��`���R�}���h���X�g�ɃZ�b�g �� ��ʂɑ΂��ăI�u�W�F�N�g���ǂ��`�悷�邩�C���̕`�挋�ʂ̕\���̈���ǂ����邩�����肳���
	gCommandList->RSSetViewports(gRenderTargetsNum, gViewport.get());
	gCommandList->RSSetScissorRects(gRenderTargetsNum, gScissorRect.get());
}

/// <summary>
/// �O���t�B�N�X�p�C�v���C���̂��߂̃R�}���h���R�}���h���X�g�ɃZ�b�g
/// </summary>
void SetCommandsForGraphicsPipeline()
{
	//�p�C�v���C���X�e�[�g���R�}���h���X�g�ɃZ�b�g
	gCommandList->SetPipelineState(gPipelineState.Get());
	//���[�g�V�O�l�`��(�V�F�[�_���爵���郌�W�X�^����`���ꂽ�f�B�X�N���v�^�e�[�u��)���R�}���h���X�g�ɃZ�b�g
	gCommandList->SetGraphicsRootSignature(gRootSignature.Get());
	//�C���v�b�g�A�Z���u���X�e�[�W
	SetCommandsOnIAStage();
	//���X�^���C�U�X�e�[�W
	SetCommandsOnRStage();
	//�`��
	gCommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

/// <summary>
/// �R�}���h���X�g�̎��s�ƃt�F���X����̘A���҂�
/// </summary>
void ExecuteAppCommandLists()
{
	//���̃t���[���ɂ����閽�߂͈ȏ�ł��C�Ƃ����ӎv�\��
	//�������ŌĂяo�����R�}���h���X�g�̖��߂��C�R�}���h�L���[�ɂ���Č�Ɏ��s�����D
	gCommandList->Close();

	//�R�}���h�L���[�ɂ��R�}���h���X�g�̎��s
	ID3D12CommandList* const commandLists[] = { gCommandList.Get() }; //�R�}���h���X�g��e�N���X�̔z��Ɋi�[
	gCommandQ->ExecuteCommandLists(_countof(commandLists), commandLists); //��X�C�����̃R�}���h���X�g��n�����ƂɂȂ�Ɨ\�z

	//GPU�֖��ߎ��s���I���������m�F���邽�߂̐M���𑗂�
	gFenceValue++; //fence����̘A���ԍ�
	gCommandQ->Signal(gFence.Get(), gFenceValue); //���ߎ��s���I��������C���̔ԍ��ő���Ԃ��ĂƂ����M���𑗂�

	//�t�F���X�ɂ����GPU�̖��ߎ��s�I����A�����Ă��炤
	//�҂��Ȃ��Ɠs���������̂ŘA��������܂ő҂�
	if (gFence->GetCompletedValue() != gFenceValue) //�t�F���X����A�����Ȃ�������r�W�[�E�F�C�g
	{
		auto fenceEvent = ::CreateEvent(nullptr, false, false, nullptr);
		assert(fenceEvent && "failed to create fence event."); //���������Ȃ��ƌx�������邳��. ��̃C�x���g���g�p���邽��
		gFence->SetEventOnCompletion(gFenceValue, fenceEvent);
		::WaitForSingleObject(fenceEvent, INFINITE);
		::CloseHandle(fenceEvent);
	}
}

/// <summary>
/// ���t���[���̃r���[�̕`�揈��
/// </summary>
void Render()
{
	//���̃t���[���̕\���o�b�t�@���擾
	gCurrentBackBufferIdx = gSwapChain->GetCurrentBackBufferIndex(); //�����炭�Cswapchain->present�Ő؂�ւ����̂Ǝv����
	auto& backBuffer = gBackBuffers[gCurrentBackBufferIdx]; //�C���f�b�N�X�����t���[�����݂ɓ���ւ��

	//�����_�[�^�[�Q�b�g�r���[�̃N���A
	ClearAppRenderTargetView(backBuffer);

	//�O���t�B�N�X�p�C�v���C��(���s�ł͂Ȃ��o�^)
	SetCommandsForGraphicsPipeline();

	//�R�}���h�L���[�ɂ��R�}���h���X�g�̖��ߎ��s�C����уt�F���X����̘A���҂�
	ExecuteAppCommandLists();

	//���̃t���[���̖��߂��N���A�C���t���[���̂��߂ɋ󂯂�
	gCommandAllocator->Reset();
	gCommandList->Reset(gCommandAllocator.Get(), nullptr);

	//�J�����g�o�b�t�@��\���C���̌�C�J�����g�o�b�t�@���؂�ւ��
	gSwapChain->Present(1, 0);
}