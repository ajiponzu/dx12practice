#pragma once
#include "Helpers.h"

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
	size_t AlignmentedSize(size_t size, size_t alignment);
};
