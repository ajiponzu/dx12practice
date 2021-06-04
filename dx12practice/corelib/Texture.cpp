#include "Texture.h"
#include "Utility.h"
#include "Core.h"

ComPtr<ID3D12Resource> Texture::LoadTexture(ComPtr<ID3D12Resource>& uploadbuff, CD3DX12_TEXTURE_COPY_LOCATION locations[2], const std::string& texPath)
{
	if (gResourceTable.find(texPath) != gResourceTable.end())
		return gResourceTable[texPath];

	if (texPath == "white")
		gResourceTable[texPath] = CreateWhiteTexture(uploadbuff, locations);
	else if (texPath == "black")
		gResourceTable[texPath] = CreateBlackTexture(uploadbuff, locations);
	else if (texPath == "grad")
		gResourceTable[texPath] = CreateGradationTexture(uploadbuff, locations);
	else
		gResourceTable[texPath] = LoadTextureFromFile(uploadbuff, locations, texPath);

	return gResourceTable[texPath];
}

ComPtr<ID3D12Resource> Texture::LoadTextureFromFile(ComPtr<ID3D12Resource>& uploadbuff, CD3DX12_TEXTURE_COPY_LOCATION locations[2], const std::string& texPath)
{
	if (gLoadLamdaTable.empty())
		MakeLoadLamdaTable();

	//WICテクスチャのロード
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};
	auto wtexpath = Utility::GetWideStringFromString(texPath);//テクスチャのファイルパス
	auto ext = Utility::GetExtension(texPath);//拡張子を取得
	ThrowIfFailed(gLoadLamdaTable[ext](wtexpath, &metadata, scratchImg));
	auto img = scratchImg.GetImage(0, 0, 0);//生データ抽出
	auto imgAlignSize = Utility::AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

	CD3DX12_HEAP_PROPERTIES uploadHeapProps, texHeapProps;
	SetBufferProps(uploadHeapProps, texHeapProps);

	//シェーダから見えるバッファのディスクリプタ
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(imgAlignSize * img->height);

	ThrowIfFailed(Core::GetInstance().GetDevice()->CreateCommittedResource(
		&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadbuff))
	);

	resDesc.Format = metadata.format;
	resDesc.Width = static_cast<UINT>(metadata.width);
	resDesc.Height = static_cast<UINT>(metadata.height);
	resDesc.DepthOrArraySize = static_cast<UINT16>(metadata.arraySize);
	resDesc.MipLevels = static_cast<UINT16>(metadata.mipLevels);
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	ComPtr<ID3D12Resource> texbuff;
	ThrowIfFailed(Core::GetInstance().GetDevice()->CreateCommittedResource(
		&texHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texbuff))
	);

	size_t params[3] = { img->rowPitch, imgAlignSize, img->slicePitch };
	MapTexture(uploadbuff, img->pixels, params);

	auto footprint = MakeFootprint(texbuff, resDesc, imgAlignSize);
	SetCopyTextureCommand(uploadbuff, texbuff, locations, footprint);

	return texbuff;
}

ComPtr<ID3D12Resource> Texture::CreateWhiteTexture(ComPtr<ID3D12Resource>& uploadbuff, CD3DX12_TEXTURE_COPY_LOCATION locations[2])
{
	size_t dataWid = 4, dataHigh = 4, dataSizeInByte = 4, rowPitch = dataWid * dataSizeInByte;
	std::vector<uint8_t> texData(dataWid * dataHigh * dataSizeInByte);
	std::fill(texData.begin(), texData.end(), 0xff);

	size_t params[4] = { dataWid, dataHigh, dataSizeInByte, rowPitch };
	return CreateTexture(uploadbuff, texData, locations, params);
}

ComPtr<ID3D12Resource> Texture::CreateBlackTexture(ComPtr<ID3D12Resource>& uploadbuff, CD3DX12_TEXTURE_COPY_LOCATION locations[2])
{
	size_t dataWid = 4, dataHigh = 4, dataSizeInByte = 4, rowPitch = dataWid * dataSizeInByte;
	std::vector<uint8_t> texData(dataWid * dataHigh * dataSizeInByte);

	size_t params[4] = { dataWid, dataHigh, dataSizeInByte, rowPitch };
	return CreateTexture(uploadbuff, texData, locations, params);
}

ComPtr<ID3D12Resource> Texture::CreateGradationTexture(ComPtr<ID3D12Resource>& uploadbuff, CD3DX12_TEXTURE_COPY_LOCATION locations[2])
{
	size_t dataWid = 4, dataHigh = 64, dataSizeInByte = 4, rowPitch = dataWid * dataSizeInByte;
	std::vector<uint8_t> texData(dataWid * dataHigh * dataSizeInByte);
	uint32_t c = 0xff;
	for (auto itr = texData.begin(); itr != texData.end(); itr += dataWid, c--)
	{
		auto col = (c << 24) | (c << 16) | (c << 8) | c;
		std::fill(itr, itr + dataWid, col);
	}

	size_t params[4] = { dataWid, dataHigh, dataSizeInByte, rowPitch };
	return CreateTexture(uploadbuff, texData, locations, params);
}

ComPtr<ID3D12Resource> Texture::CreateTexture(ComPtr<ID3D12Resource>& uploadbuff, std::vector<uint8_t>& texData, CD3DX12_TEXTURE_COPY_LOCATION locations[2], size_t params[4])
{
	auto& dataWid = params[0], dataHigh = params[1], dataSizeInByte = params[2], rowPitch = params[3];

	auto imgAlignSize = Utility::AlignmentedSize(dataWid * dataSizeInByte, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

	CD3DX12_HEAP_PROPERTIES uploadHeapProps, texHeapProps;
	SetBufferProps(uploadHeapProps, texHeapProps);

	//シェーダから見えるバッファのディスクリプタ
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(imgAlignSize * dataHigh);

	ThrowIfFailed(Core::GetInstance().GetDevice()->CreateCommittedResource(
		&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadbuff))
	);

	resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, dataWid, dataHigh);

	ComPtr<ID3D12Resource> texbuff;
	ThrowIfFailed(Core::GetInstance().GetDevice()->CreateCommittedResource(
		&texHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texbuff))
	);

	size_t paramsForMap[3] = { rowPitch, imgAlignSize, texData.size() };
	MapTexture(uploadbuff, texData.data(), paramsForMap);

	auto footprint = MakeFootprint(texbuff, resDesc, imgAlignSize);
	SetCopyTextureCommand(uploadbuff, texbuff, locations, footprint);

	return texbuff;
}

/// <summary>
/// テクスチャ生成共通項その1
/// </summary>
/// <param name="heapPropses">[0]: upload, [1]: texheap</param>
/// <param name="resDesc"></param>
void Texture::SetBufferProps(CD3DX12_HEAP_PROPERTIES& upload, CD3DX12_HEAP_PROPERTIES& texHeap)
{
	//CPUとGPU間のバッファのプロパティ
	upload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD, 0, 0);

	//テクスチャのプロパティ
	texHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT, 0, 0);
}

/// <summary>
/// マップ
/// </summary>
/// <param name="uploadbuff"></param>
/// <param name="p_imgData"></param>
/// <param name="params">[0]: srcOffset, [1]: dstOffset, [2]: imgSize</param>
void Texture::MapTexture(ComPtr<ID3D12Resource>& uploadbuff, uint8_t* pImgData, size_t params[3])
{
	auto& srcOffset = params[0];
	auto& dstOffset = params[1];
	auto& imgSize = params[2];

	uint8_t* pMapforImg = nullptr;
	ThrowIfFailed(uploadbuff->Map(0, nullptr, (void**)&pMapforImg));

	size_t srcAddress = 0, dstAddress = 0;
	while (srcAddress < imgSize)
	{
		std::copy_n(&pImgData[srcAddress], dstOffset, &pMapforImg[dstAddress]);
		srcAddress += srcOffset;
		dstAddress += dstOffset;
	}
	uploadbuff->Unmap(0, nullptr);
}

D3D12_PLACED_SUBRESOURCE_FOOTPRINT Texture::MakeFootprint(ComPtr<ID3D12Resource>& texbuff, CD3DX12_RESOURCE_DESC& resDesc, size_t imgAlignSize)
{
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
	auto desc = texbuff->GetDesc();
	Core::GetInstance().GetDevice()->GetCopyableFootprints(
		&desc, 0, 1, 0, &footprint, nullptr, nullptr, nullptr
	);
	auto subResFootprint = CD3DX12_SUBRESOURCE_FOOTPRINT(resDesc, static_cast<UINT>(imgAlignSize));
	footprint.Footprint = subResFootprint;

	return footprint;
}

void Texture::SetCopyTextureCommand(ComPtr<ID3D12Resource>& uploadbuff, ComPtr<ID3D12Resource>& texbuff, CD3DX12_TEXTURE_COPY_LOCATION locations[2], D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint)
{
	auto& dst = locations[0], src = locations[1];

	dst = CD3DX12_TEXTURE_COPY_LOCATION(texbuff.Get(), 0);
	src = CD3DX12_TEXTURE_COPY_LOCATION(uploadbuff.Get(), footprint);

	Core::GetInstance().GetCommandList()->CopyTextureRegion(
		&dst, 0, 0, 0, &src, nullptr
	);
}

void Texture::MakeLoadLamdaTable()
{
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
}

void Texture::RegistResource(std::string texPath)
{
	gResourceRegistory.insert(texPath);
}

std::set<std::string>& Texture::GetResourceRegistory()
{
	return gResourceRegistory;
}