#include "VoidScene.h"
#include "Window.h"

void VoidScene::Update(Window* pWindow)
{
}

void VoidScene::Render(Window* pWindow)
{
	pWindow->ClearAppRenderTargetView(mRenderTargetsNum);
}

void VoidScene::LoadContents(Window* pWindow)
{
}
