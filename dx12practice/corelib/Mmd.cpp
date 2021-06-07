#include "../pch.h"
#include "Mmd.h"
#include "Utility.h"
#include "Texture.h"
#include "Core.h"

FILE* MMD::LoadPMD(PMDHeader& header, std::vector<uint8_t>& vertices, std::vector<uint16_t>& indices, const std::string& path)
{
	FILE* fp = nullptr;
	auto err = ::fopen_s(&fp, path.c_str(), "rb");
	if (fp == nullptr)
	{
		char strerr[256];
		strerror_s(strerr, 256, err);
		const std::string err_str = strerr;
		::MessageBox(::GetActiveWindow(), Utility::GetWideStringFromString(err_str).c_str(), L"Open Error", MB_ICONERROR);
		return nullptr;
	}

	char magicNum[3]{};
	fread_s(magicNum, sizeof(magicNum), sizeof(char), sizeof(magicNum) / sizeof(char), fp);

	fread_s(&(header.mVersion), sizeof(header.mVersion), sizeof(float), sizeof(header.mVersion) / sizeof(float), fp);
	fread_s(header.mModelName, sizeof(header.mModelName), sizeof(char), sizeof(header.mModelName) / sizeof(char), fp);
	fread_s(header.mComment, sizeof(header.mComment), sizeof(char), sizeof(header.mComment) / sizeof(char), fp);

	uint32_t vertNum = 0;
	fread_s(&vertNum, sizeof(vertNum), sizeof(uint32_t), 1, fp);
	vertices.resize(vertNum * mPMDVertexSize);
	fread_s(vertices.data(), vertices.size() * sizeof(uint8_t), sizeof(uint8_t), vertices.size(), fp);

	uint32_t indicesNum = 0;
	fread_s(&indicesNum, sizeof(indicesNum), sizeof(uint32_t), 1, fp);
	indices.resize(indicesNum);
	fread_s(indices.data(), indices.size() * sizeof(uint16_t), sizeof(uint16_t), indices.size(), fp);

	return fp;
}

void MMD::LoadPMD(FILE* const fp, std::vector<PMDMaterial>& pmdMaterials, std::vector<Material>& materials)
{
	uint32_t materialNum = 0;
	fread_s(&materialNum, sizeof(materialNum), sizeof(uint32_t), sizeof(materialNum) / sizeof(materialNum), fp);
	pmdMaterials.resize(materialNum);
	fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(PMDMaterial), 1, fp);

	materials.resize(pmdMaterials.size());
	for (int idx = 0; idx < pmdMaterials.size(); idx++)
	{
		materials[idx].indicesNum = pmdMaterials[idx].indicesNum;
		materials[idx].material.diffuse = pmdMaterials[idx].diffuse;
		materials[idx].material.alpha = pmdMaterials[idx].alpha;
		materials[idx].material.specular = pmdMaterials[idx].specular;
		materials[idx].material.specularity = pmdMaterials[idx].specularity;
		materials[idx].material.ambient = pmdMaterials[idx].ambient;
	}

	fclose(fp);
}

void MMD::LoadPMDMaterialResources(
	const int& idx, std::vector<MMDTextures>& mmdTexturesList, std::vector<UploadLocation>& uploadLocations,
	const std::string& resourcePath, const std::string& toonFilePath, std::string texFileName)
{
	try
	{
		mmdTexturesList[idx].toon = Texture::LoadTexture(uploadLocations[idx].uploadbuff, uploadLocations[idx].locations, toonFilePath);
	}
	catch (std::exception e)
	{
		mmdTexturesList[idx].toon = nullptr;
	}

	mmdTexturesList[idx].sphere = nullptr;
	mmdTexturesList[idx].sphereAdder = nullptr;
	if (texFileName == "")
	{
		mmdTexturesList[idx].texBuffer = nullptr;
		return;
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
				resourcePath, namePair.first.c_str()
			);
			mmdTexturesList[idx].sphereAdder = Texture::LoadTexture(uploadLocations[idx].uploadbuff, uploadLocations[idx].locations, tmp);
		}
		else if (firstNameExtension == "sph")
		{
			texFileName = namePair.second;
			auto tmp = Utility::GetTexturePathFromModelAndTexPath(
				resourcePath, namePair.first.c_str()
			);
			mmdTexturesList[idx].sphere = Texture::LoadTexture(uploadLocations[idx].uploadbuff, uploadLocations[idx].locations, tmp);
		}

		if (secondNameExtension == "spa")
		{
			texFileName = namePair.first;
			auto tmp = Utility::GetTexturePathFromModelAndTexPath(
				resourcePath, namePair.second.c_str()
			);
			mmdTexturesList[idx].sphereAdder = Texture::LoadTexture(uploadLocations[idx].uploadbuff, uploadLocations[idx].locations, tmp);
		}
		else if (secondNameExtension == "sph")
		{
			texFileName = namePair.first;
			auto tmp = Utility::GetTexturePathFromModelAndTexPath(
				resourcePath, namePair.second.c_str()
			);
			mmdTexturesList[idx].sphere = Texture::LoadTexture(uploadLocations[idx].uploadbuff, uploadLocations[idx].locations, tmp);
		}

		auto texFilePath = Utility::GetTexturePathFromModelAndTexPath(
			resourcePath, texFileName.c_str()
		);
		mmdTexturesList[idx].texBuffer = Texture::LoadTexture(uploadLocations[idx].uploadbuff, uploadLocations[idx].locations, texFilePath);
	}
	else if (Utility::GetExtension(texFileName) == "spa")
	{
		auto texFilePath = Utility::GetTexturePathFromModelAndTexPath(
			resourcePath, texFileName.c_str()
		);
		mmdTexturesList[idx].sphereAdder = Texture::LoadTexture(uploadLocations[idx].uploadbuff, uploadLocations[idx].locations, texFilePath);
	}
	else if (Utility::GetExtension(texFileName) == "sph")
	{
		auto texFilePath = Utility::GetTexturePathFromModelAndTexPath(
			resourcePath, texFileName.c_str()
		);
		mmdTexturesList[idx].sphere = Texture::LoadTexture(uploadLocations[idx].uploadbuff, uploadLocations[idx].locations, texFilePath);
	}
	else
	{
		auto texFilePath = Utility::GetTexturePathFromModelAndTexPath(
			resourcePath, texFileName.c_str()
		);
		mmdTexturesList[idx].texBuffer = Texture::LoadTexture(uploadLocations[idx].uploadbuff, uploadLocations[idx].locations, texFilePath);
	}
}

void MMD::MakePMDMaterialTexture(
	std::vector<MMDTextures>& mmdTexturesList, std::vector<UploadLocation>& uploadLocations, 
	ComPtr<ID3D12DescriptorHeap>& materialDescHeap, D3D12_CONSTANT_BUFFER_VIEW_DESC& matCBVDesc)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	auto matDescHeapHandle = materialDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto incOffset = Core::GetInstance().GetDevice()->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);

	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize + 0xff) & ~0xff;

	ComPtr<ID3D12Resource> tex[3];
	std::string pathes[3] = { "white", "black", "grad" };
	for (int idx = 0; idx < 3; idx++)
	{
		uploadLocations.push_back(UploadLocation());
		auto& uploadLocation = *uploadLocations.rbegin();
		tex[idx] = Texture::LoadTexture(uploadLocation.uploadbuff, uploadLocation.locations, pathes[idx]);
	}
	auto& whiteTex = tex[0], blackTex = tex[1], gradTex = tex[2];

	UINT ptr_idx = 0;
	for (auto& mmdTextures : mmdTexturesList)
	{
		Core::GetInstance().GetDevice()->CreateConstantBufferView(&matCBVDesc, matDescHeapHandle);
		matDescHeapHandle.ptr += incOffset;
		matCBVDesc.BufferLocation += materialBuffSize;

		if (mmdTextures.texBuffer == nullptr)
			mmdTextures.texBuffer = whiteTex;

		srvDesc.Format = mmdTextures.texBuffer->GetDesc().Format;
		Core::GetInstance().GetDevice()->CreateShaderResourceView(
			mmdTextures.texBuffer.Get(), &srvDesc, matDescHeapHandle
		);

		matDescHeapHandle.ptr += incOffset;

		if (!mmdTextures.sphere)
			mmdTextures.sphere = whiteTex;

		srvDesc.Format = mmdTextures.sphere->GetDesc().Format;
		Core::GetInstance().GetDevice()->CreateShaderResourceView(
			mmdTextures.sphere.Get(), &srvDesc, matDescHeapHandle
		);

		matDescHeapHandle.ptr += incOffset;

		if (!mmdTextures.sphereAdder)
			mmdTextures.sphereAdder = blackTex;

		srvDesc.Format = mmdTextures.sphereAdder->GetDesc().Format;
		Core::GetInstance().GetDevice()->CreateShaderResourceView(
			mmdTextures.sphereAdder.Get(), &srvDesc, matDescHeapHandle
		);

		matDescHeapHandle.ptr += incOffset;

		if (!mmdTextures.toon)
			mmdTextures.toon = gradTex;

		srvDesc.Format = mmdTextures.toon->GetDesc().Format;
		Core::GetInstance().GetDevice()->CreateShaderResourceView(
			mmdTextures.toon.Get(), &srvDesc, matDescHeapHandle
		);

		matDescHeapHandle.ptr += incOffset;
	}
}
