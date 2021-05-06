#pragma once

class Window;

class Scene
{
private:
	UINT mRenderTargetsNum = 1;

public:
	virtual void Update(Window& window);
	virtual void Render(Window& window);
	virtual void LoadContents(Window& window);
};
