#pragma once
#include "VideodecodeDlg.h"
#include "AudiodecodeDlg.h"
#include "SysinfoDlg.h"

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual BOOL OnInitDialog();

protected:
	DECLARE_MESSAGE_MAP()
public:
	enum { IDD = IDD_ABOUTBOX };// �Ի�������
	CEdit m_editconfig;
};

// CffplaymfcDlg �Ի���
class CffplaymfcDlg : public CDialogEx
{
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��
	virtual BOOL OnInitDialog(); // ���ɵ���Ϣӳ�亯��
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CffplaymfcDlg(CWnd* pParent = NULL);

	void ResetBtn();
	void ActiveBtn();
	void SystemClear();
	void CreateSubWindow();
	void FreeSubWindow();
public:
	afx_msg void OnBnClickedStart();//Main Button
	afx_msg void OnBnClickedSeekB();
	afx_msg void OnBnClickedPause();
	afx_msg void OnBnClickedSeekF();
	afx_msg void OnBnClickedStop();
	afx_msg void OnBnClickedSeekStep();
	afx_msg void OnBnClickedFullscreen();
	afx_msg void OnBnClickedInputurlButton();
	afx_msg void OnBnClickedVideodecode();
	afx_msg void OnBnClickedAudiodecode();
	afx_msg void OnDropFiles(HDROP hDropInfo);//
	afx_msg void OnAbout();//Menu
	afx_msg void OnSysinfo();
	afx_msg void OnSeekF60();
	afx_msg void OnSeekB60();
	afx_msg void OnSeekF600();
	afx_msg void OnSeekB600();
	afx_msg void OnWindowYuv();
	afx_msg void OnWindowWave();
	afx_msg void OnWindowRdft();
	afx_msg void OnSize50();
	afx_msg void OnSize75();
	afx_msg void OnSize100();
	afx_msg void OnSize125();
	afx_msg void OnSize150();
	afx_msg void OnSize200();
	afx_msg void OnSize400();
	afx_msg void OnAspect11();
	afx_msg void OnAspect43();
	afx_msg void OnAspect169();
	afx_msg void OnAspect1610();
	afx_msg void OnAspect235100();
	afx_msg void OnLangCn();
	afx_msg void OnLangEn();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnWebsite();
	afx_msg void OnIdcancel();
	afx_msg void OnWindowstretchKeepratio();
	afx_msg void OnWindowstretchResize();

protected:
	HICON m_hIcon;
	CWinThread* pThreadPlay;

public:
	enum { IDD = IDD_FFPLAYMFC_DIALOG };// �Ի�������
	CEdit m_codecachannels;//Control
	CEdit m_codecaname;
	CEdit m_codecasamplerate;
	CEdit m_codecvname;
	CEdit m_codecvframerate;
	CEdit m_codecvpixfmt;
	CEdit m_currentclock;
	CEdit m_duration;
	CEdit m_formatbitrate;
	CEdit m_formatduration;
	CEdit m_formatinputformat;
	CEdit m_formatmetadata;
	CEdit m_formatprotocol;
	CEdit m_codecvresolution;
	CSliderCtrl m_playprogress;
	CEdit m_inputurl;

	VideodecodeDlg* vddlg;
	AudiodecodeDlg* addlg;
	SysinfoDlg* sidlg;
};
