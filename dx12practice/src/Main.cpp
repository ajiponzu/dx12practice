#include "pch.h"
#include "Core.h"
#include "Utility.h"

/// <summary>
/// �G���g���|�C���g
/// </summary>
/// <param name="hInst"></param>
/// <param name="_hInst"></param>
/// <param name="_lpstr"></param>
/// <param name="_intnum"></param>
/// <returns></returns>
int APIENTRY WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE _hInst, _In_ LPSTR _lpstr, _In_ int _intnum)
{
	//�g�p���Ȃ��p�����[�^�ɂ���
	UNREFERENCED_PARAMETER(_hInst);
	UNREFERENCED_PARAMETER(_lpstr);
	UNREFERENCED_PARAMETER(_intnum);

	//��DPI�Ή�
	::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	//���s�t�@�C���p�X�𑊑΃p�X�̊�_�ɂ���
	Utility::SetBasePath();

#ifdef _DEBUG
	Utility::DisplayConsole(); //�f�o�b�O�R���\�[���\��
#endif

	Core::MakeInstance(hInst, L"�Q�[��", 2560, 1440);
	Core::SetWindow();
	Core::Run();

	return 0;
}