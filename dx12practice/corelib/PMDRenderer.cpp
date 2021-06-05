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

	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc{};
	matCBVDesc.BufferLocation = mMaterialBuffer->GetGPUVirtualAddress();
	matCBVDesc.SizeInBytes = static_cast<UINT>(materialBuffSize);

	/*テクスチャ*/
	mTexBuffers.resize(mPMDMaterials.size());
	mSphereResources.resize(mPMDMaterials.size());
	mSphereAdderResources.resize(mPMDMaterials.size());
	mToonResources.resize(mPMDMaterials.size());

	std::vector<ComPtr<ID3D12Resource>> uploadbuff(mPMDMaterials.size()); //copytexureregionがexecuteされるまでライフタイムがあればよい
	std::vector<CD3DX12_TEXTURE_COPY_LOCATION[2]> locationses(mPMDMaterials.size());

	//マテリアルリソース読み込みループ
	for (int idx = 0; idx < mPMDMaterials.size(); idx++)
	{
		std::string toonFilePath = "toon/";
		char toonFileName[16]{};
		sprintf_s(toonFileName, "toon%02d.bmp", (mPMDMaterials[idx].toonIdx + 1) % 11);
		toonFilePath += toonFileName;
#ifdef _DEBUG
		std::cout << std::endl << toonFilePath << ": ";
#endif

		try 
		{
			mToonResources[idx] = Texture::LoadTexture(uploadbuff[idx], locationses[idx], toonFilePath);
		}
		catch (std::exception e)
		{
			mToonResources[idx] = nullptr;
		}

		mSphereResources[idx] = nullptr;
		mSphereAdderResources[idx] = nullptr;
		std::string texFileName = mPMDMaterials[idx].texFilePath;
		if (texFileName == "")
		{
			mTexBuffers[idx] = nullptr;
			continue;
		}

		auto namePair = std::make_pair(texFileName, texFileName);
#ifdef _DEBUG
		std::cout << namePair.first << ", " << namePair.second;
#endif
		std::string firstNameExtension = Utility::GetExtension(texFileName),
			secondNameExtension = Utility::GetExtension(texFileName);
		if (std::count(texFileName.begin(), texFileName.end(), '*') > 0)
		{
			namePair = Utility::SplitFileName(texFileName);
			firstNameExtension = Utility::GetExtension(namePair.first);
			secondNameExtension = Utility::GetExtension(namePair.second);

			if (firstNameExtension == "spa")
			{
				texFileName = namePair.second;
				auto tmp = Utility::GetTexturePathFromModelAndTexPath(
					gModelPath, namePair.first.c_str()
				);
				mSphereAdderResources[idx] = Texture::LoadTexture(uploadbuff[idx], locationses[idx], tmp);
			}
			else if (firstNameExtension == "sph")
			{
				texFileName = namePair.second;
				auto tmp = Utility::GetTexturePathFromModelAndTexPath(
					gModelPath, namePair.first.c_str()
				);
				mSphereResources[idx] = Texture::LoadTexture(uploadbuff[idx], locationses[idx], tmp);
			}

			if (secondNameExtension == "spa")
			{
				texFileName = namePair.first;
				auto tmp = Utility::GetTexturePathFromModelAndTexPath(
					gModelPath, namePair.second.c_str()
				);
				mSphereAdderResources[idx] = Texture::LoadTexture(uploadbuff[idx], locationses[idx], tmp);
			}
			else if (secondNameExtension == "sph")
			{
				texFileName = namePair.first;
				auto tmp = Utility::GetTexturePathFromModelAndTexPath(
					gModelPath, namePair.second.c_str()
				);
				mSphereResources[idx] = Texture::LoadTexture(uploadbuff[idx], locationses[idx], tmp);
			}

			auto texFilePath = Utility::GetTexturePathFromModelAndTexPath(
				gModelPath, texFileName.c_str()
			);
			mTexBuffers[idx] = Texture::LoadTexture(uploadbuff[idx], locationses[idx], texFilePath);
		}
		else if (Utility::GetExtension(texFileName) == "spa")
		{
			auto texFilePath = Utility::GetTexturePathFromModelAndTexPath(
				gModelPath, texFileName.c_str()
			);
			mSphereAdderResources[idx] = Texture::LoadTexture(uploadbuff[idx], locationses[idx], texFilePath);
		}
		else if (Utility::GetExtension(texFileName) == "sph")
		{
			auto texFilePath = Utility::GetTexturePathFromModelAndTexPath(
				gModelPath, texFileName.c_str()
			);
			mSphereResources[idx] = Texture::LoadTexture(uploadbuff[idx], locationses[idx], texFilePath);
		}
		else
		{
			auto texFilePath = Utility::GetTexturePathFromModelAndTexPath(
				gModelPath, texFileName.c_str()
			);
			mTexBuffers[idx] = Texture::LoadTexture(uploadbuff[idx], locationses[idx], texFilePath);
		}
	}
	/*end*/

	/*end*/

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

	/*マテリアルテクスチャの作成*/
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	auto matDescHeapHandle = mMaterialDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto incOffset = Core::GetInstance().GetDevice()->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);

	std::vector<ComPtr<ID3D12Resource>> uploadbuff2(mPMDMaterials.size()); 
	std::vector<CD3DX12_TEXTURE_COPY_LOCATION[2]> locationses2(3);
	std::string path = "white";
	auto whiteTex = Texture::LoadTexture(uploadbuff2[0], locationses2[0], path);
	path = "black";
	auto blackTex = Texture::LoadTexture(uploadbuff2[1], locationses2[1], path);
	path = "grad";
	auto gradTex =  Texture::LoadTexture(uploadbuff2[2], locationses2[2], path);

	UINT ptr_idx = 0;
	for (int idx = 0; idx < mPMDMaterials.size(); idx++)
	{
		Core::GetInstance().GetDevice()->CreateConstantBufferView(&matCBVDesc, matDescHeapHandle);
		matDescHeapHandle.ptr += incOffset;
		matCBVDesc.BufferLocation += materialBuffSize;

		if (mTexBuffers[idx] == nullptr)
			mTexBuffers[idx] = whiteTex;

		srvDesc.Format = mTexBuffers[idx]->GetDesc().Format;
		Core::GetInstance().GetDevice()->CreateShaderResourceView(
			mTexBuffers[idx].Get(), &srvDesc, matDescHeapHandle
		);

		matDescHeapHandle.ptr += incOffset;

		if (!mSphereResources[idx])
			mSphereResources[idx] = whiteTex;

		srvDesc.Format = mSphereResources[idx]->GetDesc().Format;
		Core::GetInstance().GetDevice()->CreateShaderResourceView(
			mSphereResources[idx].Get(), &srvDesc, matDescHeapHandle
		);

		matDescHeapHandle.ptr += incOffset;

		if (!mSphereAdderResources[idx])
			mSphereAdderResources[idx] = blackTex;

		srvDesc.Format = mSphereAdderResources[idx]->GetDesc().Format;
		Core::GetInstance().GetDevice()->CreateShaderResourceView(
			mSphereAdderResources[idx].Get(), &srvDesc, matDescHeapHandle
		);

		matDescHeapHandle.ptr += incOffset;

		if (!mToonResources[idx])
			mToonResources[idx] = gradTex;

		srvDesc.Format = mToonResources[idx]->GetDesc().Format;
		Core::GetInstance().GetDevice()->CreateShaderResourceView(
			mToonResources[idx].Get(), &srvDesc, matDescHeapHandle
		);

		matDescHeapHandle.ptr += incOffset;
	}

	/*end*/

	/*GPUへ送信*/
	for (auto& texBuffer : mTexBuffers)
		window.SetBarrier(CD3DX12_RESOURCE_BARRIER::Transition(
			texBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		));

	window.UseBarrier();

	auto& core = Core::GetInstance();
	core.ExecuteAppCommandLists();
	core.ResetGPUCommand();

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
	LoadMMD(scene, window);
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

void PMDRenderer::LoadMMD(Scene& scene, Window& window)
{
	FILE* fp = nullptr;
	auto err = ::fopen_s(&fp, gModelPath.c_str(), "rb");
	if (fp == nullptr)
	{
		char strerr[256];
		strerror_s(strerr, 256, err);
		const std::string err_str = strerr;
		::MessageBox(::GetActiveWindow(), Utility::GetWideStringFromString(err_str).c_str(), L"Open Error", MB_ICONERROR);
		return;
	}

	char magicNum[3]{};
	fread_s(magicNum, sizeof(magicNum), sizeof(char), sizeof(magicNum) / sizeof(char), fp);

	fread_s(&(mPMDHeader.mVersion), sizeof(mPMDHeader.mVersion), sizeof(float), sizeof(mPMDHeader.mVersion) / sizeof(float), fp);
	fread_s(mPMDHeader.mModelName, sizeof(mPMDHeader.mModelName), sizeof(char), sizeof(mPMDHeader.mModelName) / sizeof(char), fp);
	fread_s(mPMDHeader.mComment, sizeof(mPMDHeader.mComment), sizeof(char), sizeof(mPMDHeader.mComment) / sizeof(char), fp);

	uint32_t vertNum = 0;
	fread_s(&vertNum, sizeof(vertNum), sizeof(uint32_t), 1, fp);
	mVertices.resize(vertNum * mPMDVertexSize);
	fread_s(mVertices.data(), mVertices.size() * sizeof(uint8_t), sizeof(uint8_t), mVertices.size(), fp);

	uint32_t indicesNum = 0;
	fread_s(&indicesNum, sizeof(indicesNum), sizeof(uint32_t), 1, fp);
	mIndices.resize(indicesNum);
	fread_s(mIndices.data(), mIndices.size() * sizeof(uint16_t), sizeof(uint16_t), mIndices.size(), fp);

	uint32_t materialNum = 0;
	fread_s(&materialNum, sizeof(materialNum), sizeof(uint32_t), sizeof(materialNum) / sizeof(materialNum), fp);
	mPMDMaterials.resize(materialNum);
	fread(mPMDMaterials.data(), mPMDMaterials.size() * sizeof(PMDMaterial), 1, fp);

	mMaterials.resize(mPMDMaterials.size());
	for (int idx = 0; idx < mPMDMaterials.size(); idx++)
	{
		mMaterials[idx].indicesNum = mPMDMaterials[idx].indicesNum;
		mMaterials[idx].material.diffuse = mPMDMaterials[idx].diffuse;
		mMaterials[idx].material.alpha = mPMDMaterials[idx].alpha;
		mMaterials[idx].material.specular = mPMDMaterials[idx].specular;
		mMaterials[idx].material.specularity = mPMDMaterials[idx].specularity;
		mMaterials[idx].material.ambient = mPMDMaterials[idx].ambient;
	}
}
