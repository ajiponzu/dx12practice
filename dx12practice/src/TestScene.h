#pragma once
#include "../corelib/Scene.h"

class Renderer;
class Window;

class TestScene : public Scene
{
protected:

public:
	TestScene(const UINT& renderTargetsNum = 1U);

protected:
	virtual void InitCameraPos(Window& window) override;
};
