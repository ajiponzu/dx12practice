#include "TestScene.h"
#include "../corelib/PMDRenderer.h"
#include "../corelib/PMDActor.h"

TestScene::TestScene(const UINT& renderTargetsNum)
	: Scene(renderTargetsNum)
{
	//mActors.push_back(std::make_shared<Actor>());
	mActors.push_back(std::make_shared<PMDActor>());
}