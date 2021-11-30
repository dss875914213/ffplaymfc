#include "stdafx.h"
#include "ffplaymfc.h"
#include "ffplaymfcDlg.h"
#include <io.h>

CffplaymfcApp theApp;	// 唯一对象

// 入口函数
BOOL CffplaymfcApp::InitInstance()
{
	CffplaymfcDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	return FALSE;
}