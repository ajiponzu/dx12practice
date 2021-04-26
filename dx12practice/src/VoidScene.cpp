#include "VoidScene.h"
#include "Window.h"

void VoidScene::Update(Window& window)
{
}

void VoidScene::Render(Window& window)
{
	window.ClearAppRenderTargetView(mRenderTargetsNum);
}

void VoidScene::LoadContents(Window& window)
{
}
