//#include "pch.h"
#include "Scene.h"
#include "Window.h"

void Scene::Update(Window& window)
{
}

void Scene::Render(Window& window)
{
	window.ClearAppRenderTargetView(mRenderTargetsNum);
}

void Scene::LoadContents(Window& window)
{
}
