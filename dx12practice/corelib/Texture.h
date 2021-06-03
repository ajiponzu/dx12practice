#pragma once

namespace Texture
{
	using LoadLamda = std::function<HRESULT(const std::wstring& path, TexMetadata*, ScratchImage&)>;

	static std::unordered_map<std::string, ComPtr<ID3D12Resource>> gResourceTable;
	static std::unordered_map<std::string, LoadLamda> gLoadLamdaTable;
	static void MakeLoadLamdaTable();

	ComPtr<ID3D12Resource> LoadTexture(ComPtr<ID3D12Resource>& uploadbuff, CD3DX12_TEXTURE_COPY_LOCATION locations[2], std::string texPath);
	static ComPtr<ID3D12Resource> LoadTextureFromFile(ComPtr<ID3D12Resource>& uploadbuff, CD3DX12_TEXTURE_COPY_LOCATION locations[2], std::string& texPath);
	static ComPtr<ID3D12Resource> CreateWhiteTexture(ComPtr<ID3D12Resource>& uploadbuff, CD3DX12_TEXTURE_COPY_LOCATION locations[2]);
	static ComPtr<ID3D12Resource> CreateBlackTexture(ComPtr<ID3D12Resource>& uploadbuff, CD3DX12_TEXTURE_COPY_LOCATION locations[2]);
	static ComPtr<ID3D12Resource> CreateGradationTexture(ComPtr<ID3D12Resource>& uploadbuff, CD3DX12_TEXTURE_COPY_LOCATION locations[2]);

	static ComPtr<ID3D12Resource> CreateTexture(ComPtr<ID3D12Resource>& uploadbuff, std::vector<uint8_t>& texData, CD3DX12_TEXTURE_COPY_LOCATION locations[2], size_t params[4]);
	static void SetBufferProps(CD3DX12_HEAP_PROPERTIES& upload, CD3DX12_HEAP_PROPERTIES& texHeap);
	static void MapTexture(ComPtr<ID3D12Resource>& uploadbuff, uint8_t* pImgData, size_t params[3]);
	static D3D12_PLACED_SUBRESOURCE_FOOTPRINT MakeFootprint(ComPtr<ID3D12Resource>& texbuff, CD3DX12_RESOURCE_DESC& resDesc, size_t imgAlignSize);
	static void SetCopyTextureCommand(ComPtr<ID3D12Resource>& uploadbuff, ComPtr<ID3D12Resource>& texbuff, CD3DX12_TEXTURE_COPY_LOCATION locations[2], D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint);
};