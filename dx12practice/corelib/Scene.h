#pragma once
#include "Renderer.h"

class Window;

class Scene
{
protected:
	UINT mRenderTargetsNum = 1;
	std::shared_ptr<Renderer> mRenderer;

public:
	Scene(UINT renderTargetsNum = 1, std::unique_ptr<Renderer> renderer = nullptr)
		: mRenderTargetsNum(renderTargetsNum), mRenderer(std::move(renderer)) {}

	virtual void Update(Window& window);
	virtual void Render(Window& window);
	virtual void LoadContents(Window& window);

	const UINT& GetRenderTargetsNum() const { return mRenderTargetsNum; }
};
