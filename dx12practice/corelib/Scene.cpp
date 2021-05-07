#include "Scene.h"
#include "Window.h"

void Scene::Update(Window& window)
{
	mAngle += 0.06f;
	mWorldMat = XMMatrixRotationY(mAngle);
	*m_pMapMatrix = mWorldMat * mViewMat * mProjectionMat;
}

void Scene::Render(Window& window)
{
	window.ClearAppRenderTargetView(mRenderTargetsNum);
	if (mRenderer)
		mRenderer->SetCommandsForGraphicsPipeline(*this, window);
}

void Scene::LoadContents(Window& window)
{
	if (mRenderer)
		mRenderer->LoadContents(*this, window);
}