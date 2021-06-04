#pragma once

struct MatrixData
{
	XMMATRIX world;
	XMMATRIX view;
	XMMATRIX projection;
	XMFLOAT3 eye;
};

struct InitCameraPos
{
	XMFLOAT3 eye;
	XMFLOAT3 target;
	XMFLOAT3 up;
};

class Scene;

class Actor
{
protected:
	float mAngle = 0.0f;
	MatrixData mMatrix{};
	MatrixData* m_pMapMatrix = nullptr;

public:
	Actor(float angle = 0.0f) : mAngle(angle) {}

	virtual void Update(Scene& scene);
	virtual void Render(Scene& scene);
	virtual void LoadContents(Scene& scene);
	virtual InitCameraPos&& GetInitCameraPos();

	void SetMatrix(MatrixData&& matrixData) { mMatrix = matrixData; }
	MatrixData* const* GetPMapMatrix() const { return &m_pMapMatrix; }
};
