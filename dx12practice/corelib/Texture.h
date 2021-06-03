#pragma once

namespace Texture
{
	using LoadLamda = std::function<HRESULT(const std::wstring& path, TexMetadata*, ScratchImage&)>;

	static std::unordered_map<std::string, ComPtr<ID3D12Resource>> gResourceTable;
	static std::unordered_map<std::string, LoadLamda> gLoadLamdaTable;

	static void MakeLoadLamdaTable();

	ComPtr<ID3D12Resource> LoadTextureFromFile(ComPtr<ID3D12Resource>& uploadbuff, CD3DX12_TEXTURE_COPY_LOCATION locations[2], std::string texPath);
	ComPtr<ID3D12Resource> CreateWhiteTexture(ComPtr<ID3D12Resource>& uploadbuff, CD3DX12_TEXTURE_COPY_LOCATION locations[2]);
	ComPtr<ID3D12Resource> CreateBlackTexture(ComPtr<ID3D12Resource>& uploadbuff, CD3DX12_TEXTURE_COPY_LOCATION locations[2]);
	ComPtr<ID3D12Resource> CreateGradationTexture(ComPtr<ID3D12Resource>& uploadbuff, CD3DX12_TEXTURE_COPY_LOCATION locations[2]);
};