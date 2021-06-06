#include "Scene.h"
#include "Window.h"
#include "Renderer.h"
#include "Actor.h"
#include "Utility.h"
#include "Core.h"

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
	mUploadLocations = std::make_shared<std::vector<UploadLocation>>();

	for (auto& actor : mActors)
		actor->LoadContents(*this, window);

	SendGPUResources(window);

	//終了処理
	mUploadLocations->clear();
	mUploadLocations = nullptr;
}

void Scene::SendGPUResources(Window& window)
{
	window.UseBarrier();
	auto& core = Core::GetInstance();
	core.ExecuteAppCommandLists();
	core.ResetGPUCommand();
}
