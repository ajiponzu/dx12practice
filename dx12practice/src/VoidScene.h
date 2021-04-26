#pragma once
#include "Scene.h"

class VoidScene :
    public Scene
{
public:
private:
    UINT mRenderTargetsNum = 1;

public:
    VoidScene() : Scene() {}
    virtual void Update(Window& window) override;
    virtual void Render(Window& window) override;
    virtual void LoadContents(Window& window) override;
};

