#include "Renderer.h"
#include "Core.h"
#include "Window.h"
#include "Scene.h"
#include "Utility.h"
#include "Actor.h"
#include "Texture.h"

/// <summary>
/// ルートシグネチャの作成
/// </summary>
/// <param name="scene"></param>
/// <param name="window"></param>
/// <param name="descTblRange"></param>
void Renderer::CreateAppRootSignature(Scene& scene, Window& window, std::vector<CD3DX12_DESCRIPTOR_RANGE>& descTblRange)
{
	//テクスチャ用
	descTblRange.push_back(CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, mTextureNum, 0));
	//行列用
	descTblRange.push_back(CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, mConstantBufferNum, 0));

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
		1, &rootParam, mTextureNum, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);
	ComPtr<ID3DBlob> rootSignatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	ThrowIfFailed(::D3D12SerializeRootSignature(
		&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSignatureBlob, &errorBlob
	));

	ThrowIfFailed(Core::GetInstance().GetDevice()->CreateRootSignature(
		0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)
	));
}

/// <summary>
/// オブジェクトの行列とGPUのバッファとの関連付け
/// </summary>
/// <param name="scene"></param>
/// <param name="window"></param>
void Renderer::LinkMatrixAndCBuffer(Scene& scene, Window& window)
{ 
	auto& actors = scene.GetActors();
	for (auto& actor : actors)
	{
		actor->SetWorldMat(std::move(XMMatrixRotationY(XM_PI)));
		XMFLOAT3 eye(0.0f, 0.0f, -5.0f);
		XMFLOAT3 target(0.0f, 0.0f, 0.0f);
		XMFLOAT3 up(0.0f, 1.0f, 0.0f);
		actor->SetViewMat(std::move(XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up))));
		actor->SetProjectionMat(std::move(XMMatrixPerspectiveFovLH(
			XM_PIDIV4, static_cast<float>(window.GetWidth()) / static_cast<float>(window.GetHeight()),
			1.0f, 10.0f
		)));

		auto pTempHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto pTempResourceDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(XMMATRIX) + 0xff) & ~0xff); //アラインメント
		ThrowIfFailed(Core::GetInstance().GetDevice()->CreateCommittedResource(
			&pTempHeapProps, D3D12_HEAP_FLAG_NONE, &pTempResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mConstantBuffer)
		));

		ThrowIfFailed(mConstantBuffer->Map(0, nullptr, (void**)actor->GetPMapMatrix()));
		//ループ内で変換させる場合はマップしたままにしておく
	}
}

/// <summary>
/// パイプライン外リソースの作成・送信
/// </summary>
void Renderer::CreateAppResources(Scene& scene, Window& window, std::vector<CD3DX12_DESCRIPTOR_RANGE>& descTblRange)
{
	ComPtr<ID3D12Resource> uploadbuff; //copytexureregionがexecuteされるまでライフタイムがあればよい
	CD3DX12_TEXTURE_COPY_LOCATION locations[2];
	//mTexBuffer = Texture::LoadTextureFromFile(uploadbuff, locations, "textest.png");
	mTexBuffer = Texture::CreateGradationTexture();
	LinkMatrixAndCBuffer(scene, window);

	/*リソース作成の仕上げ*/

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = static_cast<UINT>(descTblRange.size());
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ThrowIfFailed(Core::GetInstance().GetDevice()->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&mResourceDescHeap)));

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	auto resourceDescHeapHandle = mResourceDescHeap->GetCPUDescriptorHandleForHeapStart();

	Core::GetInstance().GetDevice()->CreateShaderResourceView(
		mTexBuffer.Get(), &srvDesc, resourceDescHeapHandle
	);

	resourceDescHeapHandle.ptr += Core::GetInstance().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	cbvDesc.BufferLocation = mConstantBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = static_cast<UINT>(mConstantBuffer->GetDesc().Width);
	//定数バッファビューの作成
	Core::GetInstance().GetDevice()->CreateConstantBufferView(&cbvDesc, resourceDescHeapHandle);

	/*end*/

	/*GPUへ送信*/

	window.SetBarrier(CD3DX12_RESOURCE_BARRIER::Transition(
		mTexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	));
	window.UseBarrier();

	auto& core = Core::GetInstance();
	core.ExecuteAppCommandLists(); //パイプライン外なのでfalse
	core.ResetGPUCommand();

	/*end*/
}

/// <summary>
/// グラフィクスパイプラインステートを作成
/// </summary>
void Renderer::CreateAppGraphicsPipelineState(Scene& scene, Window& window)
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
	graphicsPipelineStateDesc.DepthStencilState.DepthEnable = false;
	//インプットレイアウト
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayouts
	{
		{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	};
	graphicsPipelineStateDesc.InputLayout.pInputElementDescs = inputLayouts.data();
	graphicsPipelineStateDesc.InputLayout.NumElements = static_cast<UINT>(inputLayouts.size());
	//幾何情報
	graphicsPipelineStateDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//レンダーターゲットの設定
	graphicsPipelineStateDesc.NumRenderTargets = scene.GetRenderTargetsNum();
	for (UINT idx = 0; idx < graphicsPipelineStateDesc.NumRenderTargets; idx++)
		graphicsPipelineStateDesc.RTVFormats[idx] = DXGI_FORMAT_R8G8B8A8_UNORM;
	//サンプリング
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleDesc.Quality = 0;
	//ルートシグネチャの登録
	graphicsPipelineStateDesc.pRootSignature = mRootSignature.Get();
	//パイプラインステート作成
	ThrowIfFailed(Core::GetInstance().GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&scene.GetPipelineState())));
}

/// <summary>
///	 インプットアセンブラステージで使用するリソースを作成
/// </summary>
void Renderer::CreateInputAssembly(Scene& scene, Window& window)
{
	auto& device = Core::GetInstance().GetDevice();

	struct Vertex
	{
		XMFLOAT3 pos;//XYZ座標
		XMFLOAT2 uv;//UV座標
	};
	Vertex vertices[] =
	{
		{{-1.f,-1.f,0.0f},{0.0f,1.0f} },//左下
		{{-1.f,1.f,0.0f} ,{0.0f,0.0f}},//左上
		{{1.f,-1.f,0.0f} ,{1.0f,1.0f}},//右下
		{{1.f,1.f,0.0f} ,{1.0f,0.0f}},//右上
	};

	unsigned short indices[] =
	{
		0,1,2,
		2,1,3
	};

	auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices));
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mVertBuffer)
	));

	Vertex* pVertMap = nullptr;
	ThrowIfFailed(mVertBuffer->Map(0, nullptr, (void**)&pVertMap));
	std::copy(std::begin(vertices), std::end(vertices), pVertMap);
	mVertBuffer->Unmap(0, nullptr);

	mVertBufferView.BufferLocation = mVertBuffer->GetGPUVirtualAddress(); //GPU側のバッファの仮想アドレス
	mVertBufferView.SizeInBytes = sizeof(vertices); //頂点情報全体のサイズ
	mVertBufferView.StrideInBytes = sizeof(Vertex); //頂点情報一つ当たりのサイズ

	resourceDesc.Width = sizeof(indices);
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mIdxBuffer)
	));

	unsigned short* pIdxMap = nullptr;
	ThrowIfFailed(mIdxBuffer->Map(0, nullptr, (void**)&pIdxMap));
	std::copy(std::begin(indices), std::end(indices), pIdxMap);
	mIdxBuffer->Unmap(0, nullptr);

	mIdxBufferView.BufferLocation = mIdxBuffer->GetGPUVirtualAddress();
	mIdxBufferView.Format = DXGI_FORMAT_R16_UINT;
	mIdxBufferView.SizeInBytes = sizeof(indices);
}

/// <summary>
/// ビューポートとシザー矩形を作成
/// </summary>
void Renderer::CreateViewportAndScissorRect(Scene& scene, Window& window)
{
	auto& width = window.GetWidth();
	auto& height = window.GetHeight();

	//ビューポート作成
	mViewport = CD3DX12_VIEWPORT(
		static_cast<FLOAT>(0), static_cast<FLOAT>(0),
		static_cast<FLOAT>(width), static_cast<FLOAT>(height)
	);
	//シザー矩形作成，ビューポートと違い，パラメータが矩形の4頂点の座標であることに注意
	mScissorRect = CD3DX12_RECT(
		static_cast<LONG>(0), static_cast<LONG>(0),
		static_cast<LONG>(width), static_cast<LONG>(height)
	);
	mScissorRect.right += mScissorRect.left;
	mScissorRect.bottom += mScissorRect.top;
}

/// <summary>
/// グラフィクスパイプラインの構築
/// </summary>
void Renderer::ConstructGraphicsPipeline(Scene& scene, Window& window)
{
	//グラフィクスパイプラインステートの作成
	CreateAppGraphicsPipelineState(scene, window);
	//インプットアセンブリを作成
	CreateInputAssembly(scene, window);
	//ビューポートとシザー矩形を作成
	CreateViewportAndScissorRect(scene, window);
}

/// <summary>
/// パイプライン外でリソースをセット
/// </summary>
void Renderer::SetAppGPUResources(Scene& scene, Window& window, ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	commandList->SetGraphicsRootSignature(mRootSignature.Get()); //ルートシグネチャ(シェーダから扱えるレジスタが定義されたディスクリプタテーブル)をコマンドリストにセット
	commandList->SetDescriptorHeaps(mTextureNum, mResourceDescHeap.GetAddressOf());
	commandList->SetGraphicsRootDescriptorTable(0, mResourceDescHeap->GetGPUDescriptorHandleForHeapStart());
}

/// <summary>
/// インプットアセンブラステージにおけるコマンドをセット
/// </summary>
void Renderer::SetCommandsOnIAStage(Scene& scene, Window& window, ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, scene.GetRenderTargetsNum(), &mVertBufferView);
	commandList->IASetIndexBuffer(&mIdxBufferView);
}

/// <summary>
/// ラスタライザステージにおけるコマンドをセット
/// ビューポートとシザー矩形をセットするのもここ
/// </summary>
void Renderer::SetCommandsOnRStage(Scene& scene, Window& window, ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	auto& renderTargetsNum = scene.GetRenderTargetsNum();
	//ビューポートとシザー矩形をコマンドリストにセット → 画面に対してオブジェクトをどう描画するか，その描画結果の表示領域をどうするかが決定される
	commandList->RSSetViewports(renderTargetsNum, &mViewport);
	commandList->RSSetScissorRects(renderTargetsNum, &mScissorRect);
}

/// <summary>
/// リソース読み込み
/// </summary>
/// <param name="scene"></param>
/// <param name="window"></param>
void Renderer::LoadContents(Scene& scene, Window& window)
{
	std::vector<CD3DX12_DESCRIPTOR_RANGE> descTblRange{};
	//ルートシグネチャの作成
	CreateAppRootSignature(scene, window, descTblRange);
	//外部リソース読み込み・登録
	CreateAppResources(scene, window, descTblRange);
	//グラフィクスパイプラインの構築
	ConstructGraphicsPipeline(scene, window);
}

/// <summary>
/// グラフィクスパイプラインのためのコマンドをコマンドリストにセット
/// </summary>
void Renderer::SetCommandsForGraphicsPipeline(Scene& scene, Window& window)
{
	auto& commandList = Core::GetInstance().GetCommandList();

	//リソースをセット
	SetAppGPUResources(scene, window, commandList);
	//パイプラインステートをコマンドリストにセット
	commandList->SetPipelineState(scene.GetPipelineState().Get());

	//インプットアセンブラステージ
	SetCommandsOnIAStage(scene, window, commandList);
	//ラスタライザステージ
	SetCommandsOnRStage(scene, window, commandList);
	//描画
	commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}