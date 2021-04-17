#include "src/Core.h"
#include "src/Utility.h"
#include "src/VoidScene.h"

/// <summary>
/// エントリポイント
/// </summary>
/// <param name="hInst"></param>
/// <param name="_hInst"></param>
/// <param name="_lpstr"></param>
/// <param name="_intnum"></param>
/// <returns></returns>
int APIENTRY WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE _hInst, _In_ LPSTR _lpstr, _In_ int _intnum)
{
	UNREFERENCED_PARAMETER(_hInst);
	UNREFERENCED_PARAMETER(_lpstr);
	UNREFERENCED_PARAMETER(_intnum);

	//高DPI対応
	::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	//相対パスを有効にする
	Utility::SetBasePath();

#ifdef _DEBUG
	Utility::DisplayConsole();
#endif

	Core::MakeInstance(hInst, L"ゲーム", 2560, 1440);
	Core::SetWindow();
	{
		auto scene = std::make_unique<VoidScene>();
		Core::Run(std::move(scene));
	}

	return 0;
}