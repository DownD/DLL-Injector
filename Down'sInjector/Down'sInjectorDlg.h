
// Down'sInjectorDlg.h : header file
//

#pragma once
#include <thread>

// CDownsInjectorDlg dialog
class CDownsInjectorDlg : public CDialogEx
{
// Construction
public:
	CDownsInjectorDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DOWNSINJECTOR_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

public:
	bool inline threadRunning() { return isThreadRunning; };
	void endThreadUnsucessfull();
	void endThreadSucessfull();
// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnChooseFileBnClickedButton();


private:
	bool isThreadRunning;
	std::thread th;
public:
	afx_msg void OnBnClickedCheckMapSxs();
};
