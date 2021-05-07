#pragma once
#include "Renderer.h"

class Window;

class Scene
{
protected:
	UINT mRenderTargetsNum = 1;
	std::shared_ptr<Renderer> mRenderer;

	XMMATRIX mWorldMat{};
	XMMATRIX mViewMat{};
	XMMATRIX mProjectionMat{};
	XMMATRIX* m_pMapMatrix = nullptr;
	float mAngle = 0.0f;

public:
	Scene(UINT renderTargetsNum = 1, std::unique_ptr<Renderer> renderer = nullptr)
		: mRenderTargetsNum(renderTargetsNum), mRenderer(std::move(renderer)) {}

	virtual void Update(Window& window);
	virtual void Render(Window& window);
	virtual void LoadContents(Window& window);

	void SetWorldMat(XMMATRIX&& worldMat) { mWorldMat = worldMat; }
	void SetViewMat(XMMATRIX&& viewMat) { mViewMat = viewMat; }
	void SetProjectionMat(XMMATRIX&& projectionMat) { mProjectionMat = projectionMat; }
	XMMATRIX* const* GetPMapMatrix() const { return &m_pMapMatrix; }

	const UINT& GetRenderTargetsNum() const { return mRenderTargetsNum; }
};
