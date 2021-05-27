#include "Scene.h"
#include "Window.h"
#include "Renderer.h"
#include "Actor.h"
#include "Utility.h"

ComPtr<ID3D12PipelineState>& Scene::GetPipelineState()
{
	return mPipelineState;
}

Scene::Scene(std::unique_ptr<Renderer>& renderer, UINT renderTargetsNum)
	: mRenderTargetsNum(renderTargetsNum)
{
	mRenderer = std::move(renderer);
	mPipelineState = nullptr;
}

Scene::Scene(UINT renderTargetsNum)
	: mRenderTargetsNum(renderTargetsNum)
{
	mRenderer = nullptr;
	mPipelineState = nullptr;
}

void Scene::Update(Window& window)
{
	for (auto& actor : mActors)
		actor->Update(*this);
}

void Scene::Render(Window& window)
{
	window.ClearAppRenderTargetView(mRenderTargetsNum);
	if (mRenderer)
		mRenderer->SetCommandsForGraphicsPipeline(*this, window);
}

void Scene::LoadContents(Window& window)
{
	for (auto& actor : mActors)
		actor->LoadContents(*this);

	if (mRenderer)
		mRenderer->LoadContents(*this, window);
}