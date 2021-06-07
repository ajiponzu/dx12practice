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
	InitCameraPos(window);

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

void Scene::InitCameraPos(Window& window)
{
	XMFLOAT3 eye(0.0f, 0.0f, -5.0f), target(0.0f, 0.0f, 0.0f), up(0.0f, 1.0f, 0.0f);

	auto view = XMMatrixLookAtLH(
		XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up)
	);
	auto projection = XMMatrixPerspectiveFovLH(
		XM_PIDIV4, static_cast<float>(window.GetWidth()) / static_cast<float>(window.GetHeight()),
		1.0f, 100.0f
	);

	mCameraPos = CameraPos{ view, projection, eye, target, up };
}