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

#include <cstdio>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

class Window;

class Scene
{
public:
private:

public:
	virtual void Update(Window* pWindow) = 0;
	virtual void Render(Window* pWindow) = 0;
	virtual void LoadContents(Window* pWindow) = 0;

private:
};
