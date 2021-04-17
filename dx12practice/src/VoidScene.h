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
    virtual void Update(Window* pWindow) override;
    virtual void Render(Window* pWindow) override;
    virtual void LoadContents(Window* pWindow) override;
};

