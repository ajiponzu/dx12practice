#include "Utility.h"

/// <summary>
/// デバッグ用
/// </summary>
/// <param name="format"></param>
/// <param name=""></param>
void Utility::DebugOutputFormatString(const char* format, ...) {
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf_s(format, valist);
	va_end(valist);
#endif
}

/// <summary>
/// デバッグ用
/// </summary>
void Utility::EnableDebugLayer()
{
	ID3D12Debug* debugLayer = nullptr;
	auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
	debugLayer->EnableDebugLayer();
	debugLayer->Release();
}

void Utility::DisplayConsole()
{
	FILE* fp = nullptr;
	::AllocConsole(); //コンソール作成
    ::freopen_s(&fp, "CONOUT$", "w", stdout) ; //コンソールと標準出力を接続
    ::freopen_s(&fp, "CONIN$", "r", stdin); //コンソールと標準入力を接続
}

void Utility::SetBasePath()
{
	WCHAR path[MAX_PATH];
	HMODULE hModule = ::GetModuleHandle(nullptr);
	if (::GetModuleFileNameW(hModule, path, MAX_PATH) > 0)
	{
		::PathRemoveFileSpecW(path);
		::SetCurrentDirectoryW(path);
	}
}

std::string Utility::GetTexturePathFromModelAndTexPath(const std::string& modelPath, const char* texPath)
{
	auto pathIndex1 = modelPath.rfind('/');
	auto pathIndex2 = modelPath.rfind('\\');
	auto pathIndex = std::max(pathIndex1, pathIndex2);
	auto folderPath = modelPath.substr(0, pathIndex + 1);
	return folderPath + texPath;
}

std::string Utility::GetExtension(const std::string& path)
{
	auto idx = path.rfind('.');
	return path.substr(idx + 1, path.length() - idx - 1);
}

std::wstring Utility::GetExtension(const std::wstring& path)
{
	auto idx = path.rfind(L'.');
	return path.substr(idx + 1, path.length() - idx - 1);
}

std::pair<std::string, std::string> Utility::SplitFileName(const std::string& path, const char splitter)
{
	auto idx = path.find(splitter);
	std::pair<std::string, std::string> ret;
	ret.first = path.substr(0, idx);
	ret.second = path.substr(idx + 1, path.length() - idx - 1);
	return ret;
}

std::wstring Utility::GetWideStringFromString(const std::string& str)
{
	auto num1 = MultiByteToWideChar(CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(), -1, nullptr, 0);

	std::wstring wstr;
	wstr.resize(num1);

	auto num2 = MultiByteToWideChar(CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(), -1, &wstr[0], num1);

	assert(num1 == num2);
	wstr = L"assets/" + wstr;
	return wstr;
}
