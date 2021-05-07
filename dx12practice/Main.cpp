#include "corelib/Core.h"
#include "corelib/Scene.h"
#include "corelib/Renderer.h"
#include "corelib/Utility.h"

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
	//使用しないパラメータについて
	UNREFERENCED_PARAMETER(_hInst);
	UNREFERENCED_PARAMETER(_lpstr);
	UNREFERENCED_PARAMETER(_intnum);

	//高DPI対応
	::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	//実行ファイルパスを相対パスの基点にする
	Utility::SetBasePath();

#ifdef _DEBUG
	Utility::DisplayConsole(); //デバッグコンソール表示
#endif

	Core::MakeInstance(hInst, L"ゲーム", 2560, 1440);
	Core::SetWindow();
	Core::Run(std::move(std::make_unique<Scene>(1, std::move(std::make_unique<Renderer>()))));

	return 0;
}