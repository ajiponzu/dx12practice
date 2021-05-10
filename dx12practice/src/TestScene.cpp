#include "TestScene.h"
#include "../corelib/Renderer.h"
#include "../corelib/Actor.h"

TestScene::TestScene(std::unique_ptr<Renderer>& renderer, UINT renderTargetsNum)
	: Scene(renderer, renderTargetsNum)
{
	mActors.push_back(std::make_shared<Actor>());
}