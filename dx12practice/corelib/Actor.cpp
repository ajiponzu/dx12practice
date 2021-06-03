#include "Actor.h"
#include "Texture.h"
#include "Utility.h"

void Actor::Update(Scene& scene)
{
	mAngle += 0.06f;
	mWorldMat = XMMatrixRotationY(mAngle);
	*m_pMapMatrix = mWorldMat * mViewMat * mProjectionMat;
}

void Actor::Render(Scene& scene)
{
}

void Actor::LoadContents(Scene& scene)
{
	//Texture::RegistResource("textest.png");
	Texture::RegistResource("white");
}
