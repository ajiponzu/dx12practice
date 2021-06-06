#include "../pch.h"
#include "PMDRenderer.h"
#include "Core.h"
#include "Utility.h"
#include "Window.h"
#include "Scene.h"
#include "Actor.h"
#include "Texture.h"

void PMDRenderer::CreateAppRootSignature(Scene& scene, Window& window, std::vector<std::vector<CD3DX12_DESCRIPTOR_RANGE>>& descTblRanges)
{
	descTblRanges.resize(2);
	auto& constantbuffDescTblRange = descTblRanges[0];
	//行列用
	constantbuffDescTblRange.push_back(CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, mConstantBufferNum, 0));

	auto& materialDescTblRange = descTblRanges[1];
	//マテリアル用
	materialDescTblRange.push_back(CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1));
	//マテリアルテクスチャ用
	materialDescTblRange.push_back(CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0));

	std::vector<D3D12_ROOT_PARAMETER> rootParams(2);
	//ディスクリプタヒープが共通なものはルートパラメータも共通
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[0].DescriptorTable.pDescriptorRanges = constantbuffDescTblRange.data();
	rootParams[0].DescriptorTable.NumDescriptorRanges = static_cast<UINT>(constantbuffDescTblRange.size());
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[1].DescriptorTable.pDescriptorRanges = materialDescTblRange.data();
	rootParams[1].DescriptorTable.NumDescriptorRanges = static_cast<UINT>(materialDescTblRange.size());
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
		static_cast<UINT>(rootParams.size()), rootParams.data(), static_cast<UINT>(samplerDescs.size()), samplerDescs.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
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

void PMDRenderer::CreateAppResources(Actor& actor, Scene& scene, Window& window,  std::vector<std::vector<CD3DX12_DESCRIPTOR_RANGE>>& descTblRanges)
{
	/*マテリアル*/
	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize + 0xff) & ~0xff;
	auto pTempResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(materialBuffSize * mPMDMaterials.size());
	auto pTempHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	ThrowIfFailed(Core::GetInstance().GetDevice()->CreateCommittedResource(
		&pTempHeapProps, D3D12_HEAP_FLAG_NONE, &pTempResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mMaterialBuffer)
	));

	uint8_t* mapMaterial = nullptr;
	ThrowIfFailed(mMaterialBuffer->Map(0, nullptr, (void**)&mapMaterial));
	for (auto& m : mMaterials)
	{
		*((MaterialForHlsl*)(mapMaterial)) = m.material;
		mapMaterial += materialBuffSize;
	}
	mMaterialBuffer->Unmap(0, nullptr);

	D3D12_DESCRIPTOR_HEAP_DESC matDescHeapDesc{};
	matDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matDescHeapDesc.NodeMask = 0;
	matDescHeapDesc.NumDescriptors = static_cast<UINT>(mPMDMaterials.size()) * 5;
	matDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ThrowIfFailed(Core::GetInstance().GetDevice()->CreateDescriptorHeap(&matDescHeapDesc, IID_PPV_ARGS(&mMaterialDescHeap)));

	/*マテリアルリソース読み込みループ*/
	mMMDTexturesList.resize(mPMDMaterials.size());

	auto& uploadLocations = scene.GetUploadLocations();
	uploadLocations.resize(mPMDMaterials.size());

	auto& resourcePath = actor.GetResourcePath();

	for (int idx = 0; idx < mPMDMaterials.size(); idx++)
	{
		std::string toonFilePath = "toon/";
		char toonFileName[16]{};
		sprintf_s(toonFileName, "toon%02d.bmp", (mPMDMaterials[idx].toonIdx + 1) % 11);
		toonFilePath += toonFileName;
#ifdef _DEBUG
		std::cout << std::endl << toonFilePath << ": ";
#endif
		MMD::LoadPMDMaterialResources(idx, mMMDTexturesList, uploadLocations, resourcePath, toonFilePath, mPMDMaterials[idx].texFilePath);
	}
	/*end*/

	/*マテリアル座標登録*/
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc{};
	matCBVDesc.BufferLocation = mMaterialBuffer->GetGPUVirtualAddress();
	matCBVDesc.SizeInBytes = static_cast<UINT>(materialBuffSize);
	/*end*/

	//マテリアルテクスチャ生成
	MMD::MakePMDMaterialTexture(mMMDTexturesList, uploadLocations, mMaterialDescHeap, matCBVDesc);

	/*GPUへ送信*/
	for (auto& mmdTexture : mMMDTexturesList)
		window.SetBarrier(CD3DX12_RESOURCE_BARRIER::Transition(
			mmdTexture.texBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		));

	/*モデル全体の座標変換行列の登録*/
	auto& constanBuffDescTblRange = descTblRanges[0];
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = static_cast<UINT>(constanBuffDescTblRange.size());
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ThrowIfFailed(Core::GetInstance().GetDevice()->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&mResourceDescHeap)));

	auto resourceDescHeapHandle = mResourceDescHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	cbvDesc.BufferLocation = mConstantBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = static_cast<UINT>(mConstantBuffer->GetDesc().Width);
	//定数バッファビューの作成
	Core::GetInstance().GetDevice()->CreateConstantBufferView(&cbvDesc, resourceDescHeapHandle);
	/*end*/
}

void PMDRenderer::CreateAppGraphicsPipelineState(Scene& scene, Window& window)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = nullptr;
	//シェーダのコンパイルとアセンブリの登録
	ComPtr<ID3DBlob> vsBlob, psBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"MMDVertexShader.cso", &vsBlob));
	ThrowIfFailed(D3DReadFileToBlob(L"MMDPixelShader.cso", &psBlob));
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
	graphicsPipelineStateDesc.NumRenderTargets = scene.GetRenderTargetsNum();
	for (UINT idx = 0; idx < scene.GetRenderTargetsNum(); idx++)
		graphicsPipelineStateDesc.RTVFormats[idx] = DXGI_FORMAT_R8G8B8A8_UNORM;
	//サンプリング
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleDesc.Quality = 0;
	//ルートシグネチャの登録
	graphicsPipelineStateDesc.pRootSignature = mRootSignature.Get();
	//パイプラインステート作成
	ThrowIfFailed(Core::GetInstance().GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&scene.GetPipelineState())));
}

void PMDRenderer::CreateInputAssembly(Scene& scene, Window& window)
{
	auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(mVertices.size() * sizeof(mVertices[0]));
	ThrowIfFailed(Core::GetInstance().GetDevice()->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mVertBuffer)
	));

	uint8_t* vertMap = nullptr;
	ThrowIfFailed(mVertBuffer->Map(0, nullptr, (void**)&vertMap));
	std::copy(mVertices.begin(), mVertices.end(), vertMap);
	mVertBuffer->Unmap(0, nullptr);

	mVertBufferView.BufferLocation = mVertBuffer->GetGPUVirtualAddress(); //GPU側のバッファの仮想アドレス
	mVertBufferView.SizeInBytes = static_cast<UINT>(mVertices.size() * sizeof(mVertices[0]));
	mVertBufferView.StrideInBytes = mPMDVertexSize;

	resourceDesc.Width = static_cast<UINT64>(mIndices.size() * sizeof(mIndices[0]));
	ThrowIfFailed(Core::GetInstance().GetDevice()->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mIdxBuffer)
	));

	uint16_t* idxMap = nullptr;
	ThrowIfFailed(mIdxBuffer->Map(0, nullptr, (void**)&idxMap));
	std::copy(mIndices.begin(), mIndices.end(), idxMap);
	mIdxBuffer->Unmap(0, nullptr);

	mIdxBufferView.BufferLocation = mIdxBuffer->GetGPUVirtualAddress();
	mIdxBufferView.Format = DXGI_FORMAT_R16_UINT;
	mIdxBufferView.SizeInBytes = static_cast<UINT>(mIndices.size() * sizeof(mIndices[0]));
}

void PMDRenderer::LoadContents(Actor& actor, Scene& scene, Window& window)
{
	std::vector<std::vector<CD3DX12_DESCRIPTOR_RANGE>> descTblRanges{};
	//ルートシグネチャの作成
	CreateAppRootSignature(scene, window, descTblRanges);
	//pmd読み込み
	auto fp = MMD::LoadPMD(mPMDHeader, mVertices, mIndices, actor.GetResourcePath());
	MMD::LoadPMD(fp, mPMDMaterials, mMaterials);
	//actor行列の登録
	LinkMatrixAndCBuffer(actor, scene, window);
	//外部リソース読み込み・登録
	CreateAppResources(actor, scene, window, descTblRanges);
	//グラフィクスパイプラインの構築
	ConstructGraphicsPipeline(scene, window);
}

void PMDRenderer::SetAppGPUResources(Scene& scene, Window& window, ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	commandList->SetGraphicsRootSignature(mRootSignature.Get()); //ルートシグネチャ(シェーダから扱えるレジスタが定義されたディスクリプタテーブル)をコマンドリストにセット
	commandList->SetDescriptorHeaps(mTextureNum, mResourceDescHeap.GetAddressOf());
	commandList->SetGraphicsRootDescriptorTable(0, mResourceDescHeap->GetGPUDescriptorHandleForHeapStart());
	//コマンドリストに積んだ順に実行される．ディスクリプタヒープをセットしてから，対応したテーブルをセットする
	//全てのディスクリプタヒープを積んでからテーブルを，とすると，「対応したアドレスと食い違う」といったエラーが生じる

	commandList->SetDescriptorHeaps(1, mMaterialDescHeap.GetAddressOf());
	//マテリアル切替のための処理，切替後にテーブルセットと描画コマンドを積む(ハンドルつまりポインタが違うので)
	//ディスクリプタヒープは同一なので，上記のディスクリプタヒープと対応テーブルを積む順に違反しない
	auto materialHandle = mMaterialDescHeap->GetGPUDescriptorHandleForHeapStart();
	uint32_t materialIdxOffset = 0;
	auto ptrIncOffset = Core::GetInstance().GetDevice()->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	) * 5;
	for (auto& m : mMaterials)
	{
		commandList->SetGraphicsRootDescriptorTable(1, materialHandle);
		//描画コマンドにて初めてピクセルシェーダが使用される
		//よって，マテリアルごとに上記の処理で定数バッファの内容が変わるので，部分ごとに色も変わる．
		commandList->DrawIndexedInstanced(m.indicesNum, 1, materialIdxOffset, 0, 0);
		materialHandle.ptr += ptrIncOffset;
		materialIdxOffset += m.indicesNum;
	}
}
