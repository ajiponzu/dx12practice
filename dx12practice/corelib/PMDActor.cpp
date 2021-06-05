#include "../pch.h"
#include "PMDActor.h"
#include "PMDRenderer.h"

//const std::string gModelPath = "assets/初音ミク.pmd";
//const std::string gModelPath = "assets/鏡音リン.pmd";
//const std::string gModelPath = "assets/鏡音リン_act2.pmd";
//const std::string gModelPath = "assets/鏡音レン.pmd";
//const std::string gModelPath = "assets/弱音ハク.pmd";
//const std::string gModelPath = "assets/咲音メイコ.pmd";
//const std::string gModelPath = "assets/初音ミクVer2.pmd";
//const std::string gModelPath = "assets/亞北ネル.pmd";
//const std::string gModelPath = "assets/MEIKO.pmd";
//const std::string gModelPath = "assets/カイト.pmd";

void PMDActor::LoadContents()
{
	SetInitCameraPos();
	mResourceList.push_back("assets/咲音メイコ.pmd");
	mRenderer = std::make_shared<PMDRenderer>();
}

void PMDActor::SetInitCameraPos()
{
	XMFLOAT3 eye(0.0f, 15.0f, -20.0f), target(0.0f, 15.0f, 0.0f), up(0.0f, 1.0f, 0.0f);
	mInitCameraPos = InitCameraPos{ eye, target, up };
}
