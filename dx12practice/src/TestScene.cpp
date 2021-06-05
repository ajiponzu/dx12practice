#include "TestScene.h"
#include "../corelib/PMDRenderer.h"
#include "../corelib/PMDActor.h"

TestScene::TestScene(UINT renderTargetsNum)
	: Scene(std::move(std::make_unique<PMDRenderer>()), renderTargetsNum)
{
	//mActors.push_back(std::make_shared<Actor>());
	mActors.push_back(std::make_shared<PMDActor>());
}