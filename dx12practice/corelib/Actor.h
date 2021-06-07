#pragma once

struct MatrixData
{
	XMMATRIX world;
	XMMATRIX view;
	XMMATRIX projection;
	XMFLOAT3 eye;
};

class Scene;
class Window;
class Renderer;

struct CameraPos;

class Actor
{
protected:
	std::shared_ptr<Renderer> mRenderer;

	float mAngle = 0.0f;
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

	const std::string& GetResourcePath() const { return mResourcePath; }
	void SetMatrix(const CameraPos& camera);
	MatrixData* const* GetPMapMatrix() const { return &m_pMapMatrix; }
	void SendMatrixDataToMap() { *m_pMapMatrix = mMatrix; }

protected:
	virtual void LoadContents();
};
