#pragma once
#include "../corelib/Actor.h"

class PMDActor
	: public Actor
{
public:
	PMDActor(const float& angle = 0.0f) : Actor(angle) {}

	virtual void LoadContents() override;
};
