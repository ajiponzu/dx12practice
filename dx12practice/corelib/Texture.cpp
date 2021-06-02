#include "Texture.h"
#include "Utility.h"
#include "Core.h"

ComPtr<ID3D12Resource> Texture::LoadTextureFromFile(ComPtr<ID3D12Resource>& uploadbuff, CD3DX12_TEXTURE_COPY_LOCATION locations[2], std::string texPath)
{
	if (gResourceTable.find(texPath) != gResourceTable.end())
		return gResourceTable[texPath];

	if (gLoadLamdaTable.empty())
		MakeLoadLamdaTable();

	ComPtr<ID3D12Resource> texbuff;

	//WICテクスチャのロード
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};
	auto wtexpath = Utility::GetWideStringFromString(texPath);//テクスチャのファイルパス
	auto ext = Utility::GetExtension(texPath);//拡張子を取得
	ThrowIfFailed(gLoadLamdaTable[ext](
		wtexpath, &metadata, scratchImg
		));
	auto img = scratchImg.GetImage(0, 0, 0);//生データ抽出

	//CPUとGPU間のバッファ
	D3D12_HEAP_PROPERTIES uploadHeapProp{};
	uploadHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	uploadHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	uploadHeapProp.CreationNodeMask = 0;
	uploadHeapProp.VisibleNodeMask = 0;

	//上記のバッファと異なる
	//シェーダから見えるバッファ
	D3D12_RESOURCE_DESC resDesc{};
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	auto pixelsize = scratchImg.GetPixelsSize();
	resDesc.Width = Utility::AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * img->height;
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;

	ThrowIfFailed(Core::GetInstance().GetDevice()->CreateCommittedResource(
		&uploadHeapProp, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadbuff))
	);

	D3D12_HEAP_PROPERTIES texHeapProp{};
	texHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	texHeapProp.CreationNodeMask = 0;
	texHeapProp.VisibleNodeMask = 0;

	resDesc.Format = metadata.format;
	resDesc.Width = static_cast<UINT>(metadata.width);
	resDesc.Height = static_cast<UINT>(metadata.height);
	resDesc.DepthOrArraySize = static_cast<UINT16>(metadata.arraySize);
	resDesc.MipLevels = static_cast<UINT16>(metadata.mipLevels);
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	ThrowIfFailed(Core::GetInstance().GetDevice()->CreateCommittedResource(
		&texHeapProp, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texbuff))
	);
	uint8_t* pMapforImg = nullptr;
	ThrowIfFailed(uploadbuff->Map(0, nullptr, (void**)&pMapforImg));
	auto srcAddress = img->pixels;
	auto rowPitch = Utility::AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
	for (size_t y = 0; y < img->height; y++)
	{
		std::copy_n(srcAddress, rowPitch, pMapforImg);
		srcAddress += img->rowPitch;
		pMapforImg += rowPitch;
	}
	uploadbuff->Unmap(0, nullptr);

	locations[0].pResource = texbuff.Get();
	locations[0].Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	locations[0].SubresourceIndex = 0;

	locations[1].pResource = uploadbuff.Get();
	locations[1].Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
	UINT nrow;
	UINT64 rowsize, size;
	auto desc = texbuff->GetDesc();
	Core::GetInstance().GetDevice()->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, &nrow, &rowsize, &size);
	locations[1].PlacedFootprint = footprint;
	locations[1].PlacedFootprint.Offset = 0;
	locations[1].PlacedFootprint.Footprint.Width = static_cast<UINT>(metadata.width);
	locations[1].PlacedFootprint.Footprint.Height = static_cast<UINT>(metadata.height);
	locations[1].PlacedFootprint.Footprint.Depth = static_cast<UINT>(metadata.depth);
	locations[1].PlacedFootprint.Footprint.RowPitch = static_cast<UINT>(Utility::AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT));
	locations[1].PlacedFootprint.Footprint.Format = img->format;

	Core::GetInstance().GetCommandList()->CopyTextureRegion(&locations[0], 0, 0, 0, &locations[1], nullptr);

	gResourceTable[texPath] = texbuff;
	return texbuff;
}

ComPtr<ID3D12Resource> Texture::CreateWhiteTexture()
{
	auto texHeapProps = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0, 0, 0
	);

	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4, 1, 1, 1, 0
	);

	ComPtr<ID3D12Resource> whiteBuffer;
	ThrowIfFailed(Core::GetInstance().GetDevice()->CreateCommittedResource(
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

ComPtr<ID3D12Resource> Texture::CreateBlackTexture()
{
	auto texHeapProps = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0, 0, 0
	);

	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4, 1, 1, 1, 0
	);

	ComPtr<ID3D12Resource> blackBuffer;
	ThrowIfFailed(Core::GetInstance().GetDevice()->CreateCommittedResource(
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

ComPtr<ID3D12Resource> Texture::CreateGradationTexture()
{
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 4, 256);
	auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0, 0, 0);

	ComPtr<ID3D12Resource> gradBuff;
	ThrowIfFailed(Core::GetInstance().GetDevice()->CreateCommittedResource(
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