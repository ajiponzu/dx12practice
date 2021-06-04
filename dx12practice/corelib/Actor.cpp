#include "Actor.h"
#include "Texture.h"
#include "Utility.h"

void Actor::Update(Scene& scene)
{
	mAngle += 0.06f;
	mMatrix.world = XMMatrixRotationY(mAngle);
	*m_pMapMatrix = mMatrix;
}

void Actor::Render(Scene& scene)
{
}

void Actor::LoadContents(Scene& scene)
{
	Texture::RegistResource("textest.png");
	//Texture::RegistResource("white");
}

InitCameraPos&& Actor::GetInitCameraPos()
{
	XMFLOAT3 eye(0.0f, 0.0f, -5.0f), target(0.0f, 0.0f, 0.0f), up(0.0f, 1.0f, 0.0f);
	InitCameraPos initCameraPos{std::move(eye), std::move(target), std::move(up)};
	return std::move(initCameraPos);
}
