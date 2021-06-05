#pragma once
#include "../corelib/Actor.h"

class PMDActor
	: public Actor
{
public:
	PMDActor(float angle = 0.0f) : Actor(angle) {}

	//virtual void LoadContents(Scene& scene);
	//virtual InitCameraPos GetInitCameraPos() override;
};
