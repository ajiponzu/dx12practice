#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlwapi.h>

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

#include <tchar.h>

#include <d3dx12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include <DirectXTex.h>
#include <DirectXMath.h>
using namespace DirectX;

#include <wrl.h>
using namespace Microsoft::WRL;

#include <Helpers.h>

#include <algorithm>
#include <cstdio>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace Utility
{
	void DebugOutputFormatString(const char* format, ...);
	void EnableDebugLayer();
	void DisplayConsole();
	void SetBasePath();
	std::string GetTexturePathFromModelAndTexPath(const std::string& modelPath, const char* texPath);
	std::string GetExtension(const std::string& path);
	std::wstring GetExtension(const std::wstring& path);
	std::pair<std::string, std::string> SplitFileName(const std::string& path, const char splitter = '*');
	std::wstring GetWideStringFromString(const std::string& str);
};
