#include "stdafx.h"
#include "ffplaymfc.h"
#include "ffplaymfcDlg.h"
#include <io.h>

CffplaymfcApp theApp;	// Ψһ����

// ��ں���
BOOL CffplaymfcApp::InitInstance()
{
	CffplaymfcDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	return FALSE;
}