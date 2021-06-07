#include "TestScene.h"
#include "../corelib/PMDRenderer.h"
#include "../corelib/PMDActor.h"
#include "../corelib/Window.h"

TestScene::TestScene(const UINT& renderTargetsNum)
	: Scene(renderTargetsNum)
{
	//mActors.push_back(std::make_shared<Actor>());
	mActors.push_back(std::make_shared<PMDActor>());
}

void TestScene::InitCameraPos(Window& window)
{
	XMFLOAT3 eye(0.0f, 15.0f, -20.0f), target(0.0f, 15.0f, 0.0f), up(0.0f, 1.0f, 0.0f);

	auto view = XMMatrixLookAtLH(
		XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up)
	);
	auto projection = XMMatrixPerspectiveFovLH(
		XM_PIDIV4, static_cast<float>(window.GetWidth()) / static_cast<float>(window.GetHeight()),
		1.0f, 100.0f
	);

	mCameraPos = CameraPos{ view, projection, eye, target, up };

	//Scene::InitCameraPos(window);
}
