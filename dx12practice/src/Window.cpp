#include "Window.h"
#include "Core.h"
#include "VoidScene.h"

void Window::Update()
{
	mScene->Update(this);
}

void Window::Render()
{
	//���̃t���[���̕\���o�b�t�@���擾
	mCurrentBackBufferIdx = mSwapChain->GetCurrentBackBufferIndex(); //�����炭�Cswapchain->present�Ő؂�ւ����̂Ǝv����
	auto& backBuffer = mBackBuffers[mCurrentBackBufferIdx]; //�C���f�b�N�X�����t���[�����݂ɓ���ւ��

	auto& core = Core::GetInstance();
	auto commandList = core.GetCommandList();
	auto device = core.GetDevice();

	//�o�b�t�@�̑�������S�ɍs�����߂̏���
	mBarrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ResourceBarrier(1, &mBarrier);

	/*GPU�ɑ΂��閽�ߒǉ�*/
	mScene->Render(this);
	/*end*/

	//�o�b�t�@�̑�������S�ɏI�����邽�߂̏����C��ʂɕ\�����鏀�����ł���
	mBarrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	commandList->ResourceBarrier(1, &mBarrier);

	//�R�}���h�L���[�ɂ��R�}���h���X�g�̖��ߎ��s�C����уt�F���X����̘A���҂�
	//�p�C�v���C�����g�p����̂�true��n��
	core.ExecuteAppCommandLists(false);

	//�J�����g�o�b�t�@��\���C���̌�C�J�����g�o�b�t�@���؂�ւ��
	mSwapChain->Present(1, 0);
}

void Window::MakeGraphicsPipeline()
{
	mScene->LoadContents(this);
}

void Window::Init()
{
	mBackBuffers.resize(mBufferCount);
	PrepareClearWindow();
}

void Window::PrepareClearWindow()
{
	auto& core = Core::GetInstance();
	auto factory = core.GetFactory();
	auto commandQ = core.GetCommandQ();
	auto device = core.GetDevice();

	MakeSwapChain(factory, commandQ);
	MakeBackBufferAndRTV(device);
	MakeDepthTools(device);
}

void Window::MakeSwapChain(IDXGIFactory6* factory, ID3D12CommandQueue* commandQ)
{
	//�f�X�N���v�V�����\�z
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = mWidth;
	swapChainDesc.Height = mHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapChainDesc.BufferCount = mBufferCount; //�_�u���o�b�t�@�Ȃ�2
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	//�쐬
	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(commandQ, mHwnd, &swapChainDesc, nullptr, nullptr, &swapChain1));
	//�L���X�g, idxgi~4��create~hwnd�̈����ɓn���Ȃ�����
	swapChain1.As(&mSwapChain);
}

/// <summary>
/// �o�b�N�o�b�t�@�ƃ����_�[�^�[�Q�b�g�r���[�̍쐬
/// ����ъ֘A�t���H
/// </summary>
void Window::MakeBackBufferAndRTV(ID3D12Device* device)
{
	//�����_�[�^�[�Q�b�g�r���[�̃f�B�X�N���v�V�����q�[�v�̃f�B�X�N���v�V�����\�z
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descriptorHeapDesc.NodeMask = 0;
	descriptorHeapDesc.NumDescriptors = mBufferCount; //�_�u���o�b�t�@�Ȃ�2
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	//�����_�[�^�[�Q�b�g�r���[�̃f�B�X�N���v�V�����q�[�v�쐬
	ThrowIfFailed(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&mRTVHeaps)));

	//�f�B�X�N���v�V�����q�[�v�̊J�n�A�h���X���擾
	auto rtvHeapsHandle = mRTVHeaps->GetCPUDescriptorHandleForHeapStart();

	//�����_�[�^�[�Q�b�g�r���[�̃f�B�X�N���v�V�����\�z
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
#ifdef _DEBUG
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
#else
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
#endif
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	//�o�b�N�o�b�t�@�ƃ����_�[�^�[�Q�b�g�r���[�̊֘A�t��
	for (UINT idx = 0; idx < mBufferCount; idx++)
	{
		ThrowIfFailed(mSwapChain->GetBuffer(idx, IID_PPV_ARGS(&(mBackBuffers[idx])))); //�o�b�N�o�b�t�@���X���b�v�`�F�C������擾
		device->CreateRenderTargetView(mBackBuffers[idx].Get(), &rtvDesc, rtvHeapsHandle); //�����_�[�^�[�Q�b�g�r���[�쐬�C�����idx�̃o�b�N�o�b�t�@�ƑΉ�
		rtvHeapsHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV); //�f�B�X�N���v�V�����q�[�v�̑Ή��A�h���X���X�V�C�I�t�Z�b�g�T�C�Y���Z�o���邽�߂Ɋ֐���p����
}
}

/// <summary>
/// �[�x����݂̐ݒ�
/// </summary>
void Window::MakeDepthTools(ID3D12Device* device)
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
/// Render����Ăяo�����
/// �����_�[�^�[�Q�b�g�r���[�̃N���A
/// </summary>
void Window::ClearAppRenderTargetView(UINT renderTargetsNum)
{
	auto& core = Core::GetInstance();
	auto	device = core.GetDevice();
	auto	commandList = core.GetCommandList();

	//�����_�[�^�[�Q�b�g�̎w��
	auto rtvHeapsHandle = mRTVHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvHeapsHandle.ptr += static_cast<SIZE_T>(mCurrentBackBufferIdx) * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//�[�x�o�b�t�@�̎w��
	auto dsvHandle = mDSVHeaps->GetCPUDescriptorHandleForHeapStart();
	//�p�C�v���C����OM�X�e�[�W�̑Ώۂ͂���RTV����[�Ƃ�������
	//�Ȃ��Ɖ�ʃN���A���ꂽ���̂����\������Ȃ�(�p�C�v���C���̌��ʂ͕\������Ȃ�)
	commandList->OMSetRenderTargets(renderTargetsNum, &rtvHeapsHandle, false, &dsvHandle);
	//�����_�[�^�[�Q�b�g�r���[����F�œh��Ԃ��ăN���A �� �\������Ɠh��Ԃ���Ă��邱�Ƃ��킩��
	commandList->ClearRenderTargetView(rtvHeapsHandle, mClearColor, 0, nullptr);
	//�[�x�o�b�t�@���N���A
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}