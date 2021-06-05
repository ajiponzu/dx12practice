#include "../pch.h"
#include "PMDActor.h"

void PMDActor::LoadContents(Scene& scene)
{
}

InitCameraPos PMDActor::GetInitCameraPos()
{
	XMFLOAT3 eye(0.0f, 15.0f, -20.0f), target(0.0f, 15.0f, 0.0f), up(0.0f, 1.0f, 0.0f);
	InitCameraPos initCameraPos{eye, target, up};
	return initCameraPos;
}
