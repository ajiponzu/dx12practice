#pragma once
#include "../corelib/Scene.h"

class Renderer;

class TestScene : public Scene
{
protected:

public:
	TestScene(const UINT& renderTargetsNum = 1U);
};
