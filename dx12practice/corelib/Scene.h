#pragma once

class Window;
class Renderer;
class Actor;

struct UploadLocation;

struct CameraPos
{
	XMMATRIX mViewMat;
	XMMATRIX mProjMat;
	XMFLOAT3 eye;
	XMFLOAT3 target;
	XMFLOAT3 up;
};

class Scene
{
protected:
	UINT mRenderTargetsNum = 1;
	std::vector<std::shared_ptr<Actor>> mActors;
	ComPtr<ID3D12PipelineState> mPipelineState;

	CameraPos mCameraPos;

	std::shared_ptr<std::vector<UploadLocation>> mUploadLocations;

public:
	Scene(const UINT& renderTargetsNum = 1U);

	virtual void Update(Window& window);
	virtual void Render(Window& window);
	virtual void LoadContents(Window& window);

	const UINT& GetRenderTargetsNum() const { return mRenderTargetsNum; }
	const CameraPos& GetCameraPos() const { return mCameraPos; }
	const std::vector<std::shared_ptr<Actor>>& GetActors() const { return mActors; }
	ComPtr<ID3D12PipelineState>& GetPipelineState() { return mPipelineState; }
	std::vector<UploadLocation>& GetUploadLocations() { return *mUploadLocations; }

protected:
	void SendGPUResources(Window& window);
	virtual void InitCameraPos(Window& window);
};
