#pragma once
#include "../corelib/Scene.h"

class Renderer;

class TestScene : public Scene
{
protected:

public:
	TestScene(std::unique_ptr<Renderer>& renderer, UINT renderTargetsNum);
};
