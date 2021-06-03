#include "Texture.h"
#include "Utility.h"
#include "Core.h"

ComPtr<ID3D12Resource> Texture::LoadTextureFromFile(ComPtr<ID3D12Resource>& uploadbuff, CD3DX12_TEXTURE_COPY_LOCATION locations[2], std::string texPath)
{
	if (gResourceTable.find(texPath) != gResourceTable.end())
		return gResourceTable[texPath];

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

	//CPUとGPU間のバッファのディスクリプタ
	auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD, 0, 0);

	//上記のバッファと異なる
	//シェーダから見えるバッファのディスクリプタ
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(imgAlignSize * img->height);

	ThrowIfFailed(Core::GetInstance().GetDevice()->CreateCommittedResource(
		&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadbuff))
	);

	auto texHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT, 0, 0);

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

	uint8_t* pMapforImg = nullptr;
	ThrowIfFailed(uploadbuff->Map(0, nullptr, (void**)&pMapforImg));
	auto srcAddress = img->pixels;
	auto& rowPitch = imgAlignSize;
	for (size_t y = 0; y < img->height; y++)
	{
		std::copy_n(srcAddress, rowPitch, pMapforImg);
		srcAddress += img->rowPitch;
		pMapforImg += rowPitch;
	}
	uploadbuff->Unmap(0, nullptr);

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
	auto desc = texbuff->GetDesc();
	Core::GetInstance().GetDevice()->GetCopyableFootprints(
		&desc, 0, 1, 0, &footprint, nullptr, nullptr, nullptr
	);
	auto subResFootprint = CD3DX12_SUBRESOURCE_FOOTPRINT(resDesc, static_cast<UINT>(imgAlignSize));
	footprint.Footprint = subResFootprint;

	locations[0] = CD3DX12_TEXTURE_COPY_LOCATION(texbuff.Get(), 0);
	locations[1] = CD3DX12_TEXTURE_COPY_LOCATION(uploadbuff.Get(), footprint);

	Core::GetInstance().GetCommandList()->CopyTextureRegion(
		&locations[0], 0, 0, 0, &locations[1], nullptr
	);

	gResourceTable[texPath] = texbuff;
	return texbuff;
}

ComPtr<ID3D12Resource> Texture::CreateWhiteTexture(ComPtr<ID3D12Resource>& uploadbuff, CD3DX12_TEXTURE_COPY_LOCATION locations[2])
{
	if (gResourceTable.find("white") != gResourceTable.end())
		return gResourceTable["white"];

	size_t dataWid = 4, dataHigh = 4, dataSizeInByte = 4;
	std::vector<uint8_t> texData(dataWid * dataHigh * dataSizeInByte);
	std::fill(texData.begin(), texData.end(), 0xff);
	auto imgAlignSize = Utility::AlignmentedSize(dataWid * dataSizeInByte, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

	//CPUとGPU間のバッファのディスクリプタ
	auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD, 0, 0);

	//上記のバッファと異なる
	//シェーダから見えるバッファのディスクリプタ
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(imgAlignSize * dataHigh);

	ThrowIfFailed(Core::GetInstance().GetDevice()->CreateCommittedResource(
		&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadbuff))
	);

	auto texHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT, 0, 0);
	resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, dataWid, dataHigh);

	ComPtr<ID3D12Resource> texbuff;
	ThrowIfFailed(Core::GetInstance().GetDevice()->CreateCommittedResource(
		&texHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texbuff))
	);

	uint8_t* pMapforImg = nullptr;
	ThrowIfFailed(uploadbuff->Map(0, nullptr, (void**)&pMapforImg));
	auto srcOffset = dataWid * dataSizeInByte;
	auto& rowPitch = imgAlignSize;
	decltype(srcOffset) srcAddress = 0, dstAddress = 0;
	while (srcAddress <= texData.size() - srcOffset)
	{
		std::copy_n(&texData[srcAddress], rowPitch, &pMapforImg[dstAddress]);
		srcAddress += srcOffset;
		dstAddress += rowPitch;
	}
	uploadbuff->Unmap(0, nullptr);

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
	auto desc = texbuff->GetDesc();
	Core::GetInstance().GetDevice()->GetCopyableFootprints(
		&desc, 0, 1, 0, &footprint, nullptr, nullptr, nullptr
	);
	auto subResFootprint = CD3DX12_SUBRESOURCE_FOOTPRINT(resDesc, static_cast<UINT>(imgAlignSize));
	footprint.Footprint = subResFootprint;

	locations[0] = CD3DX12_TEXTURE_COPY_LOCATION(texbuff.Get(), 0);
	locations[1] = CD3DX12_TEXTURE_COPY_LOCATION(uploadbuff.Get(), footprint);

	Core::GetInstance().GetCommandList()->CopyTextureRegion(
		&locations[0], 0, 0, 0, &locations[1], nullptr
	);

	gResourceTable["white"] = texbuff;
	return texbuff;
}

ComPtr<ID3D12Resource> Texture::CreateBlackTexture(ComPtr<ID3D12Resource>& uploadbuff, CD3DX12_TEXTURE_COPY_LOCATION locations[2])
{
	if (gResourceTable.find("black") != gResourceTable.end())
		return gResourceTable["black"];

	size_t dataWid = 4, dataHigh = 4, dataSizeInByte = 4;
	std::vector<uint8_t> texData(dataWid * dataHigh * dataSizeInByte);
	auto imgAlignSize = Utility::AlignmentedSize(dataWid * dataSizeInByte, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

	//CPUとGPU間のバッファのディスクリプタ
	auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD, 0, 0);

	//上記のバッファと異なる
	//シェーダから見えるバッファのディスクリプタ
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(imgAlignSize * dataHigh);

	ThrowIfFailed(Core::GetInstance().GetDevice()->CreateCommittedResource(
		&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadbuff))
	);

	auto texHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT, 0, 0);
	resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, dataWid, dataHigh);

	ComPtr<ID3D12Resource> texbuff;
	ThrowIfFailed(Core::GetInstance().GetDevice()->CreateCommittedResource(
		&texHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texbuff))
	);

	uint8_t* pMapforImg = nullptr;
	ThrowIfFailed(uploadbuff->Map(0, nullptr, (void**)&pMapforImg));
	auto srcOffset = dataWid * dataSizeInByte;
	auto& rowPitch = imgAlignSize;
	decltype(srcOffset) srcAddress = 0, dstAddress = 0;
	while (srcAddress <= texData.size() - srcOffset)
	{
		std::copy_n(&texData[srcAddress], rowPitch, &pMapforImg[dstAddress]);
		srcAddress += srcOffset;
		dstAddress += rowPitch;
	}
	uploadbuff->Unmap(0, nullptr);

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
	auto desc = texbuff->GetDesc();
	Core::GetInstance().GetDevice()->GetCopyableFootprints(
		&desc, 0, 1, 0, &footprint, nullptr, nullptr, nullptr
	);
	auto subResFootprint = CD3DX12_SUBRESOURCE_FOOTPRINT(resDesc, static_cast<UINT>(imgAlignSize));
	footprint.Footprint = subResFootprint;

	locations[0] = CD3DX12_TEXTURE_COPY_LOCATION(texbuff.Get(), 0);
	locations[1] = CD3DX12_TEXTURE_COPY_LOCATION(uploadbuff.Get(), footprint);

	Core::GetInstance().GetCommandList()->CopyTextureRegion(
		&locations[0], 0, 0, 0, &locations[1], nullptr
	);

	gResourceTable["black"] = texbuff;
	return texbuff;
}

ComPtr<ID3D12Resource> Texture::CreateGradationTexture(ComPtr<ID3D12Resource>& uploadbuff, CD3DX12_TEXTURE_COPY_LOCATION locations[2])
{
	if (gResourceTable.find("grad") != gResourceTable.end())
		return gResourceTable["grad"];

	size_t dataWid = 4, dataHigh = 64, dataSizeInByte = 4;
	std::vector<uint8_t> texData(dataWid * dataHigh * dataSizeInByte);
	uint32_t c = 0xff;
	for (auto itr = texData.begin(); itr != texData.end(); itr += dataWid, c--)
	{
		auto col = (c << 24) | (c << 16) | (c << 8) | c;
		std::fill(itr, itr + dataWid, col);
	}
	auto imgAlignSize = Utility::AlignmentedSize(dataWid * dataSizeInByte, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

	//CPUとGPU間のバッファのディスクリプタ
	auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD, 0, 0);

	//上記のバッファと異なる
	//シェーダから見えるバッファのディスクリプタ
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(imgAlignSize * dataHigh);

	ThrowIfFailed(Core::GetInstance().GetDevice()->CreateCommittedResource(
		&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadbuff))
	);

	auto texHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT, 0, 0);
	resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, dataWid, dataHigh);

	ComPtr<ID3D12Resource> texbuff;
	ThrowIfFailed(Core::GetInstance().GetDevice()->CreateCommittedResource(
		&texHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texbuff))
	);

	uint8_t* pMapforImg = nullptr;
	ThrowIfFailed(uploadbuff->Map(0, nullptr, (void**)&pMapforImg));
	auto srcOffset = dataWid * dataSizeInByte;
	auto& rowPitch = imgAlignSize;
	decltype(srcOffset) srcAddress = 0, dstAddress = 0;
	while (srcAddress <= texData.size() - srcOffset)
	{
		std::copy_n(&texData[srcAddress], rowPitch, &pMapforImg[dstAddress]);
		srcAddress += srcOffset;
		dstAddress += rowPitch;
	}
	uploadbuff->Unmap(0, nullptr);

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
	auto desc = texbuff->GetDesc();
	Core::GetInstance().GetDevice()->GetCopyableFootprints(
		&desc, 0, 1, 0, &footprint, nullptr, nullptr, nullptr
	);
	auto subResFootprint = CD3DX12_SUBRESOURCE_FOOTPRINT(resDesc, static_cast<UINT>(imgAlignSize));
	footprint.Footprint = subResFootprint;

	locations[0] = CD3DX12_TEXTURE_COPY_LOCATION(texbuff.Get(), 0);
	locations[1] = CD3DX12_TEXTURE_COPY_LOCATION(uploadbuff.Get(), footprint);

	Core::GetInstance().GetCommandList()->CopyTextureRegion(
		&locations[0], 0, 0, 0, &locations[1], nullptr
	);

	gResourceTable["grad"] = texbuff;
	return texbuff;
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