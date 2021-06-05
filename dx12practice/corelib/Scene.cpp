#include "Scene.h"
#include "Window.h"
#include "Renderer.h"
#include "Actor.h"
#include "Utility.h"

Scene::Scene(const UINT& renderTargetsNum)
	: mRenderTargetsNum(renderTargetsNum)
{
	mPipelineState = nullptr;
}

void Scene::Update(Window& window)
{
	for (auto& actor : mActors)
		actor->Update(*this, window);
}

void Scene::Render(Window& window)
{
	window.ClearAppRenderTargetView(mRenderTargetsNum);
	for (auto& actor : mActors)
		actor->Render(*this, window);
}

void Scene::LoadContents(Window& window)
{
	for (auto& actor : mActors)
	{
		actor->Init();
		actor->LoadContents(*this, window);
	}
}