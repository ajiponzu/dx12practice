#pragma once

class Window;
class Renderer;
class Actor;

class Scene
{
protected:
	UINT mRenderTargetsNum = 1;
	std::shared_ptr<Renderer> mRenderer;
	std::vector<std::shared_ptr<Actor>> mActors;

public:
	Scene(UINT renderTargetsNum = 1);

	virtual void Update(Window& window);
	virtual void Render(Window& window);
	virtual void LoadContents(Window& window);

	const UINT& GetRenderTargetsNum() const { return mRenderTargetsNum; }
	const std::vector<std::shared_ptr<Actor>>& GetActors() const { return mActors; }

protected:
	Scene(std::unique_ptr<Renderer> renderer, UINT renderTargetsNum = 1);
};
