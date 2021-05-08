#pragma once

class Scene;

class Actor
{
protected:
	XMMATRIX mWorldMat{};
	XMMATRIX mViewMat{};
	XMMATRIX mProjectionMat{};
	XMMATRIX* m_pMapMatrix = nullptr;
	float mAngle = 0.0f;

public:
	Actor(float angle = 0.0f) : mAngle(angle) {}

	virtual void Update(Scene& scene);
	virtual void Render(Scene& scene);
	virtual void LoadContents(Scene& scene);

	void SetWorldMat(XMMATRIX&& worldMat) { mWorldMat = worldMat; }
	void SetViewMat(XMMATRIX&& viewMat) { mViewMat = viewMat; }
	void SetProjectionMat(XMMATRIX&& projectionMat) { mProjectionMat = projectionMat; }
	XMMATRIX* const* GetPMapMatrix() const { return &m_pMapMatrix; }
};

