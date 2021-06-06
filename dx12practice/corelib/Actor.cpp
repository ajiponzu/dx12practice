#include "Actor.h"
#include "Texture.h"
#include "Utility.h"
#include "Renderer.h"

void Actor::Update(Scene& scene, Window& window)
{
	mAngle += 0.06f;
	mMatrix.world = XMMatrixRotationY(mAngle);
	
	//マッピングしているのはMatrixData型のメモリ領域
	// 節約のためにメンバだけコピーしても反映されない
	SendMatrixDataToMap();
}

void Actor::Render(Scene& scene, Window& window)
{
	if (mRenderer)
		mRenderer->SetCommandsForGraphicsPipeline(scene, window);
}

void Actor::LoadContents(Scene& scene, Window& window)
{
	LoadContents();
	mRenderer->LoadContents(*this, scene, window);
}

void Actor::SetInitCameraPos()
{
	XMFLOAT3 eye(0.0f, 0.0f, -5.0f), target(0.0f, 0.0f, 0.0f), up(0.0f, 1.0f, 0.0f);
	mInitCameraPos = InitCameraPos{ eye, target, up };
}

void Actor::LoadContents()
{
	SetInitCameraPos();
	mResourcePath = "textest.png";
	mRenderer = std::make_shared<Renderer>();
}
