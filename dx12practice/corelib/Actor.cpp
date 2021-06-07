#include "Actor.h"
#include "Texture.h"
#include "Utility.h"
#include "Renderer.h"
#include "Scene.h"

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

void Actor::SetMatrix(const CameraPos& camera)
{
	mMatrix.view = camera.mViewMat;
	mMatrix.projection = camera.mProjMat;
	mMatrix.eye = camera.eye;
}

void Actor::LoadContents()
{
	mMatrix.world = XMMatrixRotationY(XM_PI);
	//リソースパスを登録しない場合, 黒画像が表示される
	//mResourcePath = "textest.png";
	mRenderer = std::make_shared<Renderer>();
}
