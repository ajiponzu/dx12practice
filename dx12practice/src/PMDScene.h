#pragma once
#include "../corelib/Scene.h"

class PMDScene
	: public Scene
{
protected:

public:
	PMDScene(const UINT& renderTargetsNum = 1U);

protected:
	virtual void InitCameraPos(Window& window) override;
};

