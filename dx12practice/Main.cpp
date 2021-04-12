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
#include <unordered_map>
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
ComPtr<ID3D12Resource> gDepthBuffer;
ComPtr<ID3D12DescriptorHeap> gDSVHeaps;
ComPtr<ID3D12Fence> gFence;
UINT64 gFenceValue = 0;
UINT gCurrentBackBufferIdx = 0;
float gClearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
D3D_FEATURE_LEVEL gFeatureLevel{};
CD3DX12_RESOURCE_BARRIER gBarrier{};

/*GPU���\�[�X(�p�C�v���C���O)�܂��*/
constexpr UINT gTextureNum = 1;
constexpr UINT gConstantBufferNum = 1;
ComPtr<ID3D12DescriptorHeap> gResourceDescHeap;
//Render���ŌĂ΂�Ȃ����C�f�B�X�N���v�^(���邢�̓r���[)�̓o�b�t�@�̖��߃C���^�[�t�F�C�X�݂����Ȃ���
//�{�̂��������Ă��Ȃ���΃G���[
ComPtr<ID3D12Resource> gConstantBuffer;
struct MatrixData
{
	XMMATRIX mWorld;
	XMMATRIX mView;
	XMMATRIX mProjection;
	XMFLOAT3 mEye;
};
MatrixData gMatrix;
MatrixData* gMapMatrix = nullptr; //�}�b�v�p�C�A�b�v�f�[�g�ŕϊ����邽�߂ɃX�R�[�v���g��
float gAngle = 0.0f;
//MMD�܂��

//const std::string gModelPath = "assets/�����~�N.pmd";
const std::string gModelPath = "assets/�������J.pmd";
//const std::string gModelPath = "assets/��������.pmd";
//const std::string gModelPath = "assets/��������_act2.pmd";
//const std::string gModelPath = "assets/��������.pmd";
//const std::string gModelPath = "assets/�㉹�n�N.pmd";
//const std::string gModelPath = "assets/�特���C�R.pmd";
//const std::string gModelPath = "assets/�����~�Nmetal.pmd";
//const std::string gModelPath = "assets/�����~�NVer2.pmd";
//const std::string gModelPath = "assets/���k�l��.pmd";
//const std::string gModelPath = "assets/MEIKO.pmd";
//const std::string gModelPath = "assets/�J�C�g.pmd";
//const std::string gModelPath = "assets/�_�~�[�{�[��.pmd";

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

//�}�e���A������
#pragma pack(1)
struct PMDMaterial
{
	XMFLOAT3 diffuse;
	float alpha;
	float specularity;
	XMFLOAT3 specular;
	XMFLOAT3 ambient;
	uint8_t toonIdx;
	uint8_t edgeFlag;
	uint32_t indicesNum;
	char texFilePath[20];
};
#pragma pack()
//pragma pack�ň͂�ꂽ�����ł̓A���C�������g�������ɂȂ�
//����ă����o�ԂɃt���O�����g�͐����Ȃ�
//�\���̂̃|�C���^��n���āCfread�Ŋۂ��Ɠǂݍ��ނ悤�ȏꍇ�C�t���O�����g�̓l�b�N�ƂȂ�
std::vector<PMDMaterial> gPMDMaterials{};

struct MaterialForHlsl
{
	XMFLOAT3 diffuse;
	float alpha;
	XMFLOAT3 specular;
	float specularity;
	XMFLOAT3 ambient;
};

struct AdditionalMaterial
{
	std::string texPath;
	int toonIdx;
	bool edgeFlg;
};

struct Material
{
	uint32_t indicesNum;
	MaterialForHlsl material{};
	AdditionalMaterial additional{};
};

std::vector<Material> gMaterials{};
ComPtr<ID3D12Resource> gMaterialBuffer;
ComPtr<ID3D12DescriptorHeap> gMaterialDescHeap;
std::vector<ComPtr<ID3D12Resource>> gTexBuffers;
std::vector<ComPtr<ID3D12Resource>> gSphereResources;
std::vector<ComPtr<ID3D12Resource>> gSphereAdderResources;
std::vector<ComPtr<ID3D12Resource>> gToonResources;
std::unordered_map<std::string, ComPtr<ID3D12Resource>> gResourceTable;
using LoadLamda = std::function<HRESULT(const std::wstring& path, TexMetadata*, ScratchImage&)>;
std::unordered_map<std::string, LoadLamda> gLoadLamdaTable;

//�O���t�B�N�X�p�C�v���C���X�e�[�g�܂��
ComPtr<ID3D12Resource> gVertBuffer;
D3D12_VERTEX_BUFFER_VIEW gVertBufferView;
ComPtr<ID3D12Resource> gIdxBuffer;
D3D12_INDEX_BUFFER_VIEW gIdxBufferView;
constexpr UINT gRenderTargetsNum = 1; // 1 ~ 8�͈̔͂���w�肷��
ComPtr<ID3D12RootSignature> gRootSignature;
ComPtr<ID3D12PipelineState> gPipelineState;

//�r���[�|�[�g�ƃV�U�[��`
//������COM�ł͂Ȃ�
CD3DX12_VIEWPORT gViewport;
CD3DX12_RECT gScissorRect;

/*end*/

/// <summary>
/// �f�o�b�O�p
/// </summary>
/// <param name="format"></param>
/// <param name=""></param>
void DebugOutputFormatString(const char* format , ...) {
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf_s(format, valist);
	va_end(valist);
#endif
}

/// <summary>
/// �f�o�b�O�p
/// </summary>
void EnableDebugLayer()
{
	ID3D12Debug* debugLayer = nullptr;
	auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
	debugLayer->EnableDebugLayer();
	debugLayer->Release();
}

std::string GetTexturePathFromModelAndTexPath(const std::string& modelPath, const char* texPath)
{
	auto pathIndex1 = modelPath.rfind('/');
	auto pathIndex2 = modelPath.rfind('\\');
	auto pathIndex = max(pathIndex1, pathIndex2);
	auto folderPath = modelPath.substr(0, pathIndex + 1);
	return folderPath + texPath;
}

std::string GetExtension(const std::string& path)
{
	auto idx = path.rfind('.');
	return path.substr(idx + 1, path.length() - idx - 1);
}

std::wstring GetExtension(const std::wstring& path)
{
	auto idx = path.rfind(L'.');
	return path.substr(idx + 1, path.length() - idx - 1);
}

std::pair<std::string, std::string> SplitFileName(const std::string& path, const char splitter = '*')
{
	auto idx = path.find(splitter);
	std::pair<std::string, std::string> ret;
	ret.first = path.substr(0, idx);
	ret.second = path.substr(idx + 1, path.length() - idx - 1);
	return ret;
}

std::wstring GetWideStringFromString(const std::string& str)
{
	auto num1 = MultiByteToWideChar(CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(), -1, nullptr, 0);

	std::wstring wstr;
	wstr.resize(num1);

	auto num2 = MultiByteToWideChar(CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(), -1, &wstr[0], num1);

	assert(num1 == num2);
	wstr = L"assets/" + wstr;
	return wstr;
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
/// �[�x����݂̐ݒ�
/// </summary>
void CreateDepthTools()
{
	auto depthResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_D32_FLOAT, gWindowWidth, gWindowHeight
	);
	depthResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	auto depthHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	auto depthClearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);

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
#ifdef _DEBUG
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
#else
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
#endif
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
	CreateDepthTools(); //�K�{�ł͂Ȃ����C�Ăяo���ꏊ�Ƃ��Ă͂������ӂ��킵��

	//�t�F���X�̍쐬
	ThrowIfFailed(gDevice->CreateFence(gFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gFence)));
}

/// <summary>
/// �R�}���h���X�g�̎��s�ƃt�F���X����̘A���҂�
/// </summary>
void ExecuteAppCommandLists(bool isPipelineUsed)
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

	//���߂Ă��������߂��N���A
	//GPU�Ƃ̒ʐM���I�����邲�ƂɃN���A(���t���[���ł��N���A���邱�Ƃ�����)
	//�p�C�v���C�����g�p����ꍇ�C�R�}���h���X�g�̃N���A�ɓn���K�v������
	auto pTetmp = isPipelineUsed ? gPipelineState.Get() : nullptr;
	gCommandAllocator->Reset();
	gCommandList->Reset(gCommandAllocator.Get(), pTetmp);
}

size_t AlignmentedSize(size_t size, size_t alignment) {
	return size + alignment - size % alignment;
}

/// <summary>
/// �C���v�b�g�A�Z���u���X�e�[�W�ő���3D���f���̒��_���쐬
/// </summary>
void LoadMMD()
{
	FILE* fp = nullptr;
	auto err = ::fopen_s(&fp, gModelPath.c_str(), "rb");
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

	uint32_t materialNum = 0;
	fread_s(&materialNum, sizeof(materialNum), sizeof(uint32_t), sizeof(materialNum) / sizeof(materialNum), fp);
	gPMDMaterials.resize(materialNum);
	fread(gPMDMaterials.data(), gPMDMaterials.size() * sizeof(PMDMaterial), 1, fp);

	gMaterials.resize(gPMDMaterials.size());
	for (int idx = 0; idx < gPMDMaterials.size(); idx++)
	{
		gMaterials[idx].indicesNum = gPMDMaterials[idx].indicesNum;
		gMaterials[idx].material.diffuse = gPMDMaterials[idx].diffuse;
		gMaterials[idx].material.alpha = gPMDMaterials[idx].alpha;
		gMaterials[idx].material.specular = gPMDMaterials[idx].specular;
		gMaterials[idx].material.specularity = gPMDMaterials[idx].specularity;
		gMaterials[idx].material.ambient = gPMDMaterials[idx].ambient;
	}
}

/// <summary>
///	 �C���v�b�g�A�Z���u���X�e�[�W�Ŏg�p���郊�\�[�X���쐬
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

	gVertBufferView.BufferLocation = gVertBuffer->GetGPUVirtualAddress(); //GPU���̃o�b�t�@�̉��z�A�h���X
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

ComPtr<ID3D12Resource> LoadTextureFromFile(std::string& texPath)
{
	if (gResourceTable.find(texPath) != gResourceTable.end())
		return gResourceTable[texPath];

	//WIC�e�N�X�`���̃��[�h
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};
	auto wtexpath = GetWideStringFromString(texPath);//�e�N�X�`���̃t�@�C���p�X
	auto ext = GetExtension(texPath);//�g���q���擾
	ThrowIfFailed(gLoadLamdaTable[ext](
		wtexpath, &metadata, scratchImg
		));
	auto img = scratchImg.GetImage(0, 0, 0);//���f�[�^���o

	//WriteToSubresource�œ]������p�̃q�[�v�ݒ�
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;//����Ȑݒ�Ȃ̂�default�ł�upload�ł��Ȃ�
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;//���C�g�o�b�N��
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;//�]����L0�܂�CPU�����璼��
	texHeapProp.CreationNodeMask = 0;//�P��A�_�v�^�̂���0
	texHeapProp.VisibleNodeMask = 0;//�P��A�_�v�^�̂���0

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = metadata.format;
	resDesc.Width = static_cast<UINT>(metadata.width);//��
	resDesc.Height = static_cast<UINT>(metadata.height);//����
	resDesc.DepthOrArraySize = static_cast<UINT16>(metadata.arraySize);
	resDesc.SampleDesc.Count = 1;//�ʏ�e�N�X�`���Ȃ̂ŃA���`�F�����Ȃ�
	resDesc.SampleDesc.Quality = 0;//
	resDesc.MipLevels = static_cast<UINT16>(metadata.mipLevels);//�~�b�v�}�b�v���Ȃ��̂Ń~�b�v���͂P��
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;//���C�A�E�g�ɂ��Ă͌��肵�Ȃ�
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;//�Ƃ��Ƀt���O�Ȃ�

	ComPtr<ID3D12Resource> texbuff = nullptr;
	ThrowIfFailed(gDevice->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//���Ɏw��Ȃ�
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&texbuff)
	));

	ThrowIfFailed(texbuff->WriteToSubresource(
		0,
		nullptr,//�S�̈�փR�s�[
		img->pixels,//���f�[�^�A�h���X
		static_cast<UINT>(img->rowPitch),//1���C���T�C�Y
		static_cast<UINT>(img->slicePitch)//�S�T�C�Y
	));

	gResourceTable[texPath] = texbuff;
	return texbuff;
}

ComPtr<ID3D12Resource> CreateWhiteTexture()
{
	auto texHeapProps = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0, 0, 0
	);

	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4, 1, 1, 1, 0
	);

	ComPtr<ID3D12Resource> whiteBuffer;
	ThrowIfFailed(gDevice->CreateCommittedResource(
		&texHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr, IID_PPV_ARGS(&whiteBuffer)
	));

	std::vector<uint8_t> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff);

	ThrowIfFailed(whiteBuffer->WriteToSubresource(
		0, nullptr, data.data(), 4 * 4, data.size()
	));

	return whiteBuffer;
}

ComPtr<ID3D12Resource> CreateBlackTexture()
{
	auto texHeapProps = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0, 0, 0
	);

	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4, 1, 1, 1, 0
	);

	ComPtr<ID3D12Resource> blackBuffer;
	ThrowIfFailed(gDevice->CreateCommittedResource(
		&texHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr, IID_PPV_ARGS(&blackBuffer)
	));

	std::vector<uint8_t> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0x00);

	ThrowIfFailed(blackBuffer->WriteToSubresource(
		0, nullptr, data.data(), 4 * 4, data.size()
	));

	return blackBuffer;
}

ComPtr<ID3D12Resource> CreateGradationTexture()
{
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 4, 256);
	auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0, 0, 0);

	ComPtr<ID3D12Resource> gradBuff;
	ThrowIfFailed(gDevice->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr, IID_PPV_ARGS(&gradBuff)
	));

	std::vector<uint32_t> data(4 * 256);
	uint32_t c = 0xff;
	for (auto itr = data.begin(); itr != data.end(); itr += 4)
	{
		auto col = (c << 24) | (c << 16) | (c << 8) | c;
		std::fill(itr, itr + 4, col);
		c--;
	}

	ThrowIfFailed(gradBuff->WriteToSubresource(
		0, nullptr, data.data(), 4 * sizeof(uint32_t), sizeof(uint32_t) * data.size()
	));

	return gradBuff;
}

/// <summary>
/// ���[�g�V�O�l�`���̍쐬, �p�C�v���C���O���\�[�X�̍쐬�E���M
/// </summary>
void CreateAppResources()
{
	std::vector<CD3DX12_DESCRIPTOR_RANGE> descTblRanges{}; //�x�N�^�[�ɂ���΁C�ėp�������シ��H
	//�s��p
	descTblRanges.push_back(CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, gConstantBufferNum, 0));

	std::vector<CD3DX12_DESCRIPTOR_RANGE> materialDescTblRanges{};
	//�}�e���A���p
	materialDescTblRanges.push_back(CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1));
	//�}�e���A���e�N�X�`���p
	materialDescTblRanges.push_back(CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0));

	std::vector<D3D12_ROOT_PARAMETER> rootParams(2);
	//�f�B�X�N���v�^�q�[�v�����ʂȂ��̂̓��[�g�p�����[�^������
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[0].DescriptorTable.pDescriptorRanges = descTblRanges.data();
	rootParams[0].DescriptorTable.NumDescriptorRanges = static_cast<UINT>(descTblRanges.size());
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[1].DescriptorTable.pDescriptorRanges = materialDescTblRanges.data();
	rootParams[1].DescriptorTable.NumDescriptorRanges = static_cast<UINT>(materialDescTblRanges.size());
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	std::vector<D3D12_STATIC_SAMPLER_DESC> samplerDescs(2);
	samplerDescs[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDescs[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDescs[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDescs[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplerDescs[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplerDescs[0].MaxLOD = D3D12_FLOAT32_MAX;
	samplerDescs[0].MinLOD = 0.0f;
	samplerDescs[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDescs[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	samplerDescs[0].ShaderRegister = 0;

	samplerDescs[1] = samplerDescs[0];
	samplerDescs[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDescs[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDescs[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDescs[1].ShaderRegister = 1;

	auto rootSignatureDesc = CD3DX12_ROOT_SIGNATURE_DESC(
		rootParams.size(), rootParams.data(), samplerDescs.size(), samplerDescs.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);
	ComPtr<ID3DBlob> rootSignatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	ThrowIfFailed(::D3D12SerializeRootSignature(
		&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSignatureBlob, &errorBlob
	));
	ThrowIfFailed(gDevice->CreateRootSignature(
		0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&gRootSignature)
	));

	/*�s��*/
	gMatrix.mWorld = XMMatrixRotationY(XM_PIDIV4);
	XMFLOAT3 eye(0.0f, 15.0f, -15.0f);
	XMFLOAT3 target(0.0f, 15.0f, 0.0f);
	XMFLOAT3 up(0.0f, 1.0f, 0.0f);
	gMatrix.mView = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	gMatrix.mProjection = XMMatrixPerspectiveFovLH(
		XM_PIDIV4, static_cast<float>(gWindowWidth) / static_cast<float>(gWindowHeight),
		1.0f, 100.0f
	);

	auto pTempHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto pTempResourceDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(MatrixData) + 0xff) & ~0xff); //�A���C�������g
	ThrowIfFailed(gDevice->CreateCommittedResource(
		&pTempHeapProps, D3D12_HEAP_FLAG_NONE, &pTempResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&gConstantBuffer)
	));

	ThrowIfFailed(gConstantBuffer->Map(0, nullptr, (void**)&gMapMatrix));
	gMapMatrix->mWorld = gMatrix.mWorld;
	gMapMatrix->mView = gMatrix.mView;
	gMapMatrix->mProjection = gMatrix.mProjection;
	gMapMatrix->mEye = eye;
	//���[�v���ŕϊ�������ꍇ�̓}�b�v�����܂܂ɂ��Ă���
	/*end*/

	/*�}�e���A��*/
	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize + 0xff) & ~0xff;
	pTempResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(materialBuffSize * gPMDMaterials.size());
	ThrowIfFailed(gDevice->CreateCommittedResource(
		&pTempHeapProps, D3D12_HEAP_FLAG_NONE, &pTempResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&gMaterialBuffer)
	));

	uint8_t* mapMaterial = nullptr;
	ThrowIfFailed(gMaterialBuffer->Map(0, nullptr, (void**)&mapMaterial));
	for (auto& m : gMaterials)
	{
		*((MaterialForHlsl*)(mapMaterial)) = m.material;
		mapMaterial += materialBuffSize;
	}
	gMaterialBuffer->Unmap(0, nullptr);

	D3D12_DESCRIPTOR_HEAP_DESC matDescHeapDesc{};
	matDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matDescHeapDesc.NodeMask = 0;
	matDescHeapDesc.NumDescriptors = static_cast<UINT>(gPMDMaterials.size()) * 5;
	matDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ThrowIfFailed(gDevice->CreateDescriptorHeap(&matDescHeapDesc, IID_PPV_ARGS(&gMaterialDescHeap)));

	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc{};
	matCBVDesc.BufferLocation = gMaterialBuffer->GetGPUVirtualAddress();
	matCBVDesc.SizeInBytes = static_cast<UINT>(materialBuffSize);

	/*�e�N�X�`��*/
	gTexBuffers.resize(gPMDMaterials.size());
	gSphereResources.resize(gPMDMaterials.size());
	gSphereAdderResources.resize(gPMDMaterials.size());
	gToonResources.resize(gPMDMaterials.size());

	gLoadLamdaTable["sph"]
		= gLoadLamdaTable["spa"]
		= gLoadLamdaTable["bmp"]
		= gLoadLamdaTable["png"]
		= gLoadLamdaTable["jpg"]
		=
		[](const std::wstring& path, TexMetadata* meta, ScratchImage& img) -> HRESULT
	{
		return LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, meta, img);
	};
	gLoadLamdaTable["tga"] = [](const std::wstring& path, TexMetadata* meta, ScratchImage& img) -> HRESULT
	{
		return LoadFromTGAFile(path.c_str(), meta, img);
	};
	gLoadLamdaTable["dds"] = [](const std::wstring& path, TexMetadata* meta, ScratchImage& img) -> HRESULT
	{
		return LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, meta, img);
	};

	//�}�e���A�����\�[�X�ǂݍ��݃��[�v
	for (int idx = 0; idx < gPMDMaterials.size(); idx++)
	{
		std::string toonFilePath = "toon/";
		char toonFileName[16]{};
		sprintf_s(toonFileName, "toon%02d.bmp", gPMDMaterials[idx].toonIdx + 1);
		toonFilePath += toonFileName;

		try {
			gToonResources[idx] = LoadTextureFromFile(toonFilePath);
		}
		catch (std::exception e)
		{
			gToonResources[idx] = nullptr;
		}

		gSphereResources[idx] = nullptr;
		gSphereAdderResources[idx] = nullptr;
		std::string texFileName = gPMDMaterials[idx].texFilePath;
		if (texFileName == "")
		{
			gTexBuffers[idx] = nullptr;
			continue;
		}

		auto namePair = std::make_pair(texFileName, texFileName);
		std::string firstNameExtension = GetExtension(texFileName),
			secondNameExtension = GetExtension(texFileName);
		if (std::count(texFileName.begin(), texFileName.end(), '*') > 0)
		{
			namePair = SplitFileName(texFileName);
			firstNameExtension = GetExtension(namePair.first);
			secondNameExtension = GetExtension(namePair.second);

			if (firstNameExtension == "spa")
			{
				texFileName = namePair.second;
				auto tmp = GetTexturePathFromModelAndTexPath(
					gModelPath, namePair.first.c_str()
				);
				gSphereAdderResources[idx] = LoadTextureFromFile(tmp);
			}
			else if (firstNameExtension == "sph")
			{
				texFileName = namePair.second;
				auto tmp = GetTexturePathFromModelAndTexPath(
					gModelPath, namePair.first.c_str()
				);
				gSphereResources[idx] = LoadTextureFromFile(tmp);
			}

			if (secondNameExtension == "spa")
			{
				texFileName = namePair.first;
				auto tmp = GetTexturePathFromModelAndTexPath(
					gModelPath, namePair.second.c_str()
				);
				gSphereAdderResources[idx] = LoadTextureFromFile(tmp);
			}
			else if (secondNameExtension == "sph")
			{
				texFileName = namePair.first;
				auto tmp = GetTexturePathFromModelAndTexPath(
					gModelPath, namePair.second.c_str()
				);
				gSphereResources[idx] = LoadTextureFromFile(tmp);
			}

			auto texFilePath = GetTexturePathFromModelAndTexPath(
				gModelPath, texFileName.c_str()
			);
			gTexBuffers[idx] = LoadTextureFromFile(texFilePath);
		}
		else if (GetExtension(texFileName) == "spa")
		{
			auto texFilePath = GetTexturePathFromModelAndTexPath(
				gModelPath, texFileName.c_str()
			);
			gSphereAdderResources[idx] = LoadTextureFromFile(texFilePath);
		}
		else if (GetExtension(texFileName) == "sph")
		{
			auto texFilePath = GetTexturePathFromModelAndTexPath(
				gModelPath, texFileName.c_str()
			);
			gSphereResources[idx] = LoadTextureFromFile(texFilePath);
		}
		else
		{
			auto texFilePath = GetTexturePathFromModelAndTexPath(
				gModelPath, texFileName.c_str()
			);
			gTexBuffers[idx] = LoadTextureFromFile(texFilePath);
		}
	}
	/*end*/

	/*end*/

	/*���f���S�̂̍��W�ϊ��s��̓o�^*/
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = static_cast<UINT>(descTblRanges.size());
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ThrowIfFailed(gDevice->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&gResourceDescHeap)));

	auto resourceDescHeapHandle = gResourceDescHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	cbvDesc.BufferLocation = gConstantBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = static_cast<UINT>(gConstantBuffer->GetDesc().Width);
	//�萔�o�b�t�@�r���[�̍쐬
	gDevice->CreateConstantBufferView(&cbvDesc, resourceDescHeapHandle);
	/*end*/

	/*�}�e���A���e�N�X�`���̍쐬*/
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	auto matDescHeapHandle = gMaterialDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto incOffset = gDevice->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);

	auto whiteTex = CreateWhiteTexture();
	auto blackTex = CreateBlackTexture();
	auto gradTex = CreateGradationTexture();
	for (int idx = 0; idx < gPMDMaterials.size(); idx++)
	{
		gDevice->CreateConstantBufferView(&matCBVDesc, matDescHeapHandle);
		matDescHeapHandle.ptr += incOffset;
		matCBVDesc.BufferLocation += materialBuffSize;

		if (gTexBuffers[idx] == nullptr)
			gTexBuffers[idx] = whiteTex;

		srvDesc.Format = gTexBuffers[idx]->GetDesc().Format;
		gDevice->CreateShaderResourceView(
			gTexBuffers[idx].Get(), &srvDesc, matDescHeapHandle
		);

		matDescHeapHandle.ptr += incOffset;

		if (gSphereResources[idx] == nullptr)
			gSphereResources[idx] = whiteTex;

		srvDesc.Format = gSphereResources[idx]->GetDesc().Format;
		gDevice->CreateShaderResourceView(
			gSphereResources[idx].Get(), &srvDesc, matDescHeapHandle
		);

		matDescHeapHandle.ptr += incOffset;

		if (gSphereAdderResources[idx] == nullptr)
			gSphereAdderResources[idx] = blackTex;

		srvDesc.Format = gSphereAdderResources[idx]->GetDesc().Format;
		gDevice->CreateShaderResourceView(
			gSphereAdderResources[idx].Get(), &srvDesc, matDescHeapHandle
		);

		matDescHeapHandle.ptr += incOffset;

		if (gToonResources[idx] == nullptr)
			gToonResources[idx] = gradTex;

		srvDesc.Format = gToonResources[idx]->GetDesc().Format;
		gDevice->CreateShaderResourceView(
			gToonResources[idx].Get(), &srvDesc, matDescHeapHandle
		);

		matDescHeapHandle.ptr += incOffset;
	}
	/*end*/
}

/// <summary>
/// �r���[�|�[�g�ƃV�U�[��`���쐬
/// </summary>
void CreateViewportAndScissorRect()
{
	//�r���[�|�[�g�쐬
	gViewport = CD3DX12_VIEWPORT(
		static_cast<FLOAT>(0), static_cast<FLOAT>(0),
		static_cast<FLOAT>(gWindowWidth), static_cast<FLOAT>(gWindowHeight)
	);
	//�V�U�[��`�쐬�C�r���[�|�[�g�ƈႢ�C�p�����[�^����`��4���_�̍��W�ł��邱�Ƃɒ���
	gScissorRect = CD3DX12_RECT(
		static_cast<LONG>(0), static_cast<LONG>(0),
		static_cast<LONG>(gWindowWidth), static_cast<LONG>(gWindowHeight)
	);
	gScissorRect.right += gScissorRect.left;
	gScissorRect.bottom += gScissorRect.top;
}

/// <summary>
/// �O���t�B�N�X�p�C�v���C���X�e�[�g���쐬
/// </summary>
void CreateAppGraphicsPipelineState()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = nullptr;
	//�V�F�[�_�̃R���p�C���ƃA�Z���u���̓o�^
	ComPtr<ID3DBlob> vsBlob, psBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"VertexShader.cso", &vsBlob));
	ThrowIfFailed(D3DReadFileToBlob(L"PixelShader.cso", &psBlob));
	graphicsPipelineStateDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
	graphicsPipelineStateDesc.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());
	//�T���v���}�X�N
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	//�u�����h�X�e�[�g
	//D3D12_DEFAULT��n���Ȃ��ƁC����ȃR���X�g���N�^���Ă΂�Ȃ� �� �f�t�H���g�R���X�g���N�^�͂�������0�N���A���炢�̋@�\�����Ȃ�
	graphicsPipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	//���X�^���C�U�X�e�[�g
	graphicsPipelineStateDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	graphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	//�[�x�X�e���V���X�e�[�g
	graphicsPipelineStateDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	//�[�x�r���[�̐ݒ�
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	//�C���v�b�g���C�A�E�g
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
	//���[�g�V�O�l�`���̓o�^
	graphicsPipelineStateDesc.pRootSignature = gRootSignature.Get();
	//�p�C�v���C���X�e�[�g�쐬
	ThrowIfFailed(gDevice->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&gPipelineState)));
}

/// <summary>
/// �O���t�B�N�X�p�C�v���C���̍\�z�E�g�p����
/// </summary>
void ConstructGraphicsPipeline()
{
	//�C���v�b�g�A�Z���u�����쐬
	CreateInputAssembly();
	//���[�g�V�O�l�`���̍쐬
	//���\�[�X��GPU�֑���
	CreateAppResources();
	//�r���[�|�[�g�ƃV�U�[��`���쐬
	CreateViewportAndScissorRect();
	//�O���t�B�N�X�p�C�v���C���X�e�[�g�̍쐬
	CreateAppGraphicsPipelineState();
}

/// <summary>
/// ���t���[���̃��f���̍X�V����
/// </summary>
void Update()
{
	gAngle += 0.06f;
	gMatrix.mWorld = XMMatrixRotationY(gAngle);
	gMapMatrix->mWorld = gMatrix.mWorld;
}

/// <summary>
/// Render����Ăяo�����
/// �����_�[�^�[�Q�b�g�r���[�̃N���A
/// </summary>
void ClearAppRenderTargetView()
{
	//�����_�[�^�[�Q�b�g�̎w��
	auto rtvHeapsHandle = gRTVHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvHeapsHandle.ptr += static_cast<SIZE_T>(gCurrentBackBufferIdx) * gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//�[�x�o�b�t�@�̎w��
	auto dsvHandle = gDSVHeaps->GetCPUDescriptorHandleForHeapStart();
	//�p�C�v���C����OM�X�e�[�W�̑Ώۂ͂���RTV����[�Ƃ�������
	//�Ȃ��Ɖ�ʃN���A���ꂽ���̂����\������Ȃ�(�p�C�v���C���̌��ʂ͕\������Ȃ�)
	gCommandList->OMSetRenderTargets(gRenderTargetsNum, &rtvHeapsHandle, false, &dsvHandle);

	//�����_�[�^�[�Q�b�g�r���[����F�œh��Ԃ��ăN���A �� �\������Ɠh��Ԃ���Ă��邱�Ƃ��킩��
	gCommandList->ClearRenderTargetView(rtvHeapsHandle, gClearColor, 0, nullptr);
	//�[�x�o�b�t�@���N���A
	gCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

/// <summary>
/// �p�C�v���C���O�Ń��\�[�X���Z�b�g
/// </summary>
void SetAppGPUResources()
{
	gCommandList->SetGraphicsRootSignature(gRootSignature.Get()); //���[�g�V�O�l�`��(�V�F�[�_���爵���郌�W�X�^����`���ꂽ�f�B�X�N���v�^�e�[�u��)���R�}���h���X�g�ɃZ�b�g
	gCommandList->SetDescriptorHeaps(gTextureNum, gResourceDescHeap.GetAddressOf());
	gCommandList->SetGraphicsRootDescriptorTable(0, gResourceDescHeap->GetGPUDescriptorHandleForHeapStart());
	//�R�}���h���X�g�ɐς񂾏��Ɏ��s�����D�f�B�X�N���v�^�q�[�v���Z�b�g���Ă���C�Ή������e�[�u�����Z�b�g����
	//�S�Ẵf�B�X�N���v�^�q�[�v��ς�ł���e�[�u�����C�Ƃ���ƁC�u�Ή������A�h���X�ƐH���Ⴄ�v�Ƃ������G���[��������

	gCommandList->SetDescriptorHeaps(1, gMaterialDescHeap.GetAddressOf());
	//�}�e���A���ؑւ̂��߂̏����C�ؑ֌�Ƀe�[�u���Z�b�g�ƕ`��R�}���h��ς�(�n���h���܂�|�C���^���Ⴄ�̂�)
	//�f�B�X�N���v�^�q�[�v�͓���Ȃ̂ŁC��L�̃f�B�X�N���v�^�q�[�v�ƑΉ��e�[�u����ςޏ��Ɉᔽ���Ȃ�
	auto materialHandle = gMaterialDescHeap->GetGPUDescriptorHandleForHeapStart();
	uint32_t materialIdxOffset = 0;
	auto ptrIncOffset = gDevice->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	) * 5;
	for (auto& m : gMaterials)
	{
		gCommandList->SetGraphicsRootDescriptorTable(1, materialHandle);
		//�`��R�}���h�ɂď��߂ăs�N�Z���V�F�[�_���g�p�����
		//����āC�}�e���A�����Ƃɏ�L�̏����Œ萔�o�b�t�@�̓��e���ς��̂ŁC�������ƂɐF���ς��D
		gCommandList->DrawIndexedInstanced(m.indicesNum, 1, materialIdxOffset, 0, 0);
		materialHandle.ptr += ptrIncOffset;
		materialIdxOffset += m.indicesNum;
	}
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
	gCommandList->RSSetViewports(gRenderTargetsNum, &gViewport);
	gCommandList->RSSetScissorRects(gRenderTargetsNum, &gScissorRect);
}

/// <summary>
/// �O���t�B�N�X�p�C�v���C���̂��߂̃R�}���h���R�}���h���X�g�ɃZ�b�g
/// </summary>
void SetCommandsForGraphicsPipeline()
{
	//�p�C�v���C���X�e�[�g���R�}���h���X�g�ɃZ�b�g
	gCommandList->SetPipelineState(gPipelineState.Get());

	//�C���v�b�g�A�Z���u���X�e�[�W
	SetCommandsOnIAStage();
	//���X�^���C�U�X�e�[�W
	SetCommandsOnRStage();

	//�p�C�v���C���O���\�[�X���Z�b�g�C�ꍇ�ɂ���Ă͕`��R�}���h��ς�
	//�`��R�}���h��ςޏꍇ��, �p�C�v���C���Ɋւ���S�ẴR�}���h��
	//�Ăяo���ȑO�Ɏ��s����Ă���K�v������D
	SetAppGPUResources();
}

/// <summary>
/// ���t���[���̃r���[�̕`�揈��
/// </summary>
void Render()
{
	//���̃t���[���̕\���o�b�t�@���擾
	gCurrentBackBufferIdx = gSwapChain->GetCurrentBackBufferIndex(); //�����炭�Cswapchain->present�Ő؂�ւ����̂Ǝv����
	auto& backBuffer = gBackBuffers[gCurrentBackBufferIdx]; //�C���f�b�N�X�����t���[�����݂ɓ���ւ��

	//�o�b�t�@�̑�������S�ɍs�����߂̏���
	gBarrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	gCommandList->ResourceBarrier(1, &gBarrier);

	/*GPU�ɑ΂��閽�ߒǉ�*/
	//�����_�[�^�[�Q�b�g�r���[�̃N���A
	ClearAppRenderTargetView();
	//�O���t�B�N�X�p�C�v���C��(���s�ł͂Ȃ��o�^)
	SetCommandsForGraphicsPipeline();
	/*end*/

	//�o�b�t�@�̑�������S�ɏI�����邽�߂̏����C��ʂɕ\�����鏀�����ł���
	gBarrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	gCommandList->ResourceBarrier(1, &gBarrier);

	//�R�}���h�L���[�ɂ��R�}���h���X�g�̖��ߎ��s�C����уt�F���X����̘A���҂�
	//�p�C�v���C�����g�p����̂�true��n��
	ExecuteAppCommandLists(true);

	//�J�����g�o�b�t�@��\���C���̌�C�J�����g�o�b�t�@���؂�ւ��
	gSwapChain->Present(1, 0);
}

/// <summary>
/// ���C�����[�v
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
/// ���s�t�@�C���̏ꏊ����ʒu�ɐݒ�
/// </summary>
void SetBasePath()
{
	WCHAR path[MAX_PATH];
	HMODULE hModule = ::GetModuleHandle(nullptr);
	if (::GetModuleFileNameW(hModule, path, MAX_PATH) > 0)
	{
		::PathRemoveFileSpecW(path);
		::SetCurrentDirectoryW(path);
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

	//��DPI�Ή�
	::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	//�R���\�[���̎g�p
	FILE* fp = nullptr;
	::AllocConsole(); //�R���\�[���쐬
    ::freopen_s(&fp, "CONOUT$", "w", stdout) ; //�R���\�[���ƕW���o�͂�ڑ�
    ::freopen_s(&fp, "CONIN$", "r", stdin); //�R���\�[���ƕW�����͂�ڑ�

	//���΃p�X��L���ɂ���
	SetBasePath();

	//�C���X�^���X�擾
	gHInstance = hInst;

	//��ʃN���A�̏���
	PrepareClearWindow();

	//�E�B���h�E�\��
	::ShowWindow(gHwnd, SW_SHOW);

	//�O���t�B�N�X�p�C�v���C���̍\�z, �쐬�������[�g�V�O�l�`���̓o�^�������ōs��
	ConstructGraphicsPipeline();

	for (auto& t : gResourceTable)
	{
		auto ptr = t.second->GetDesc();
		DebugOutputFormatString("%lu\n", ptr.Height);
	}

	//���C�����[�v
	MainLoop();

	//�I������
	UnregisterClass(gWindowClass.lpszClassName, gWindowClass.hInstance);
	return 0;
}