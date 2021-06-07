#include "../pch.h"
#include "PMDActor.h"
#include "PMDRenderer.h"

//const std::string gModelPath = "assets/初音ミク.pmd";
//const std::string gModelPath = "assets/鏡音リン.pmd";
//const std::string gModelPath = "assets/鏡音リン_act2.pmd";
//const std::string gModelPath = "assets/鏡音レン.pmd";
//const std::string gModelPath = "assets/弱音ハク.pmd";
const std::string gModelPath = "assets/咲音メイコ.pmd";
//const std::string gModelPath = "assets/初音ミクVer2.pmd";
//const std::string gModelPath = "assets/亞北ネル.pmd";
//const std::string gModelPath = "assets/MEIKO.pmd";
//const std::string gModelPath = "assets/カイト.pmd";

void PMDActor::LoadContents()
{
	mMatrix.world = XMMatrixRotationX(XM_PI);
	mResourcePath = gModelPath;
	mRenderer = std::make_shared<PMDRenderer>();
}
