#pragma once

namespace Texture
{
	using LoadLamda = std::function<HRESULT(const std::wstring& path, TexMetadata*, ScratchImage&)>;
	std::unordered_map<std::string, ComPtr<ID3D12Resource>> gResourceTable;
	std::unordered_map<std::string, LoadLamda> gLoadLamdaTable;

	static void MakeLoadLamdaTable();

	ComPtr<ID3D12Resource> Texture::LoadTextureFromFile(std::string texPath);
	ComPtr<ID3D12Resource> Texture::CreateWhiteTexture();
	ComPtr<ID3D12Resource> Texture::CreateBlackTexture();
	ComPtr<ID3D12Resource> Texture::CreateGradationTexture();
};