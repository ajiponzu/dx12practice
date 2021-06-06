#include "../pch.h"
#include "Mmd.h"
#include "Utility.h"

FILE* MMD::LoadPMD(PMDHeader& header, 	std::vector<uint8_t>& vertices, std::vector<uint16_t>& indices, const std::string& path)
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

void MMD::LoadPMD(FILE* fp, std::vector<PMDMaterial>& pmdMaterials, std::vector<Material>& materials)
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