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
class Window;
class Renderer;

class Actor
{
protected:
	std::shared_ptr<Renderer> mRenderer;

	float mAngle = 0.0f;
	InitCameraPos mInitCameraPos{};
	MatrixData mMatrix{};
	MatrixData* m_pMapMatrix = nullptr;

	std::string mResourcePath;

public:
	Actor(const float& angle = 0.0f)
		: mAngle(angle)
	{}

	virtual void Update(Scene& scene, Window& window);
	virtual void Render(Scene& scene, Window& window);

	void LoadContents(Scene& scene, Window& window);

	std::string& GetResourcePath() { return mResourcePath; }
	InitCameraPos& GetInitCameraPos() { return mInitCameraPos; }
	void SetMatrix(MatrixData& matrixData) { mMatrix = matrixData; mMatrix.eye = mInitCameraPos.eye; }
	MatrixData* const* GetPMapMatrix() const { return &m_pMapMatrix; }
	void SendMatrixDataToMap(MatrixData& matrixData) { *m_pMapMatrix = matrixData; }
	void SendMatrixDataToMap() { *m_pMapMatrix = mMatrix; }

protected:
	virtual void SetInitCameraPos();
	virtual void LoadContents();
};
