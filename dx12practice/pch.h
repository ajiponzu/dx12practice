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

#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include <directx/d3dx12.h>
#include <DirectXMath.h>
using namespace DirectX;

#include <wrl.h>
using namespace Microsoft::WRL;

#include <cstdio>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <set>
#include <unordered_map>
#include <vector>
