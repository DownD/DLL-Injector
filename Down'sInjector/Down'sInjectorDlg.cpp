
// Down'sInjectorDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "Down'sInjector.h"
#include "Down'sInjectorDlg.h"
#include "afxdialogex.h"
#include "BlackBone/Process/Process.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

struct DllArgs {
	wchar_t path[256];
};

void waitForProcess(CDownsInjectorDlg* dlg) {

	while (dlg->threadRunning()) {
		DWORD targetId;
		DWORD currId = GetCurrentProcessId();
		HWND targetWindow = GetForegroundWindow();
		GetWindowThreadProcessId(targetWindow, &targetId);
		if (targetId != currId) {
			CButton* radio = (CButton*)dlg->GetDlgItem(IDC_RADIO1);

			if (radio->GetCheck() == BST_CHECKED) {
				//If is MMap
				//Try Atach Process
				blackbone::Process prc;
				NTSTATUS statOpen = prc.Attach(targetId);
				if (STATUS_SUCCESS != statOpen) {
					std::wstring str = L"Fail to open process with id " + std::to_wstring(targetId) + L"\nError Code: " + std::to_wstring(statOpen);
					MessageBox(NULL, str.c_str(), L"Error on Injection", MB_OK);
					dlg->endThreadUnsucessfull();
					return;
				}

				CEdit* fileEditBox = (CEdit*)dlg->GetDlgItem(IDC_EDIT1);
				CString cPath;
				fileEditBox->GetWindowText(cPath);
				std::wstring wPath(cPath);

				//MM options
				auto flags = blackbone::NoFlags;

				CButton* sxs = (CButton*)dlg->GetDlgItem(IDC_CHECK_MAP_SXS);
				if (sxs->GetCheck() == BST_CHECKED) {
					flags |= blackbone::NoSxS;
				}
				CButton* exceptions = (CButton*)dlg->GetDlgItem(IDC_CHECK_MAP_EXCEPTION);
				if (exceptions->GetCheck() == BST_CHECKED) {
					flags |= blackbone::NoExceptions;
				}

				CButton* threadh = (CButton*)dlg->GetDlgItem(IDC_CHECK_MAP_THREAD_HIJACK);
				if (threadh->GetCheck() == BST_CHECKED) {
					flags |= blackbone::NoThreads;
				}
				DllArgs arg = { 0 };
				memcpy(arg.path, wPath.data(), wPath.size()*sizeof(wchar_t));
				blackbone::CustomArgs_t args;
				args.push_back(&arg);
				auto addr = prc.mmap().MapImage(wPath,flags,nullptr,nullptr,&args);
				if (!addr) {
					std::wstring str = L"Fail to inject" + wPath + L" into process id " + std::to_wstring(targetId) + L"\n";
					str += L"Status Code = " + std::to_wstring(addr.status) + L"\n";
					str += blackbone::Utils::GetErrorDescription(addr.status);
					MessageBox(NULL, str.c_str(), L"Error on Injection", MB_OK);
					dlg->endThreadUnsucessfull();
					return;
				}

				//MessageBox(NULL, L"SUCESS", L"SUCESS", MB_OK);
			}
			else if (((CButton*)dlg->GetDlgItem(IDC_LOADLIB))->GetCheck() == BST_CHECKED) {
				//If is LoadLibrary

			}

			CButton* close = (CButton*)dlg->GetDlgItem(IDC_CLOSE_CHECK);
			if (close->GetCheck() == BST_CHECKED) {
				ExitProcess(0);
			}
			dlg->endThreadSucessfull();


		}
	}
}


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnChooseFileBnClickedButton();
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON1, &CAboutDlg::OnChooseFileBnClickedButton)
END_MESSAGE_MAP()


// CDownsInjectorDlg dialog



CDownsInjectorDlg::CDownsInjectorDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DOWNSINJECTOR_DIALOG, pParent)
{
	isThreadRunning = false;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDownsInjectorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}



void CDownsInjectorDlg::endThreadUnsucessfull()
{
	isThreadRunning = false;
	CStatic* status = (CStatic*)GetDlgItem(IDC_STATIC_STATUS_MSG);
	CButton* bt = (CButton*)GetDlgItem(IDOK);
	bt->SetWindowTextW(TEXT("Inject"));
	CString statusMsg;
	statusMsg.LoadString(IDS_STRING105);
	status->SetWindowText(statusMsg);
	th.detach();
}

void CDownsInjectorDlg::endThreadSucessfull()
{
	isThreadRunning = false;
	CStatic* status = (CStatic*)GetDlgItem(IDC_STATIC_STATUS_MSG);
	CButton* bt = (CButton*)GetDlgItem(IDOK);
	bt->SetWindowTextW(TEXT("Inject"));
	CString statusMsg;
	statusMsg.LoadString(IDS_STRING104);
	status->SetWindowText(statusMsg);
	th.detach();
}

BEGIN_MESSAGE_MAP(CDownsInjectorDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CDownsInjectorDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON1, &CDownsInjectorDlg::OnChooseFileBnClickedButton)
END_MESSAGE_MAP()


// CDownsInjectorDlg message handlers

BOOL CDownsInjectorDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDownsInjectorDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CDownsInjectorDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDownsInjectorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CDownsInjectorDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	//CDialogEx::OnOK();
	CStatic* status = (CStatic*)GetDlgItem(IDC_STATIC_STATUS_MSG);
	if (isThreadRunning) {
		isThreadRunning = false;
		if (th.joinable()) {
			th.join();
		}
		CButton* bt = (CButton*)GetDlgItem(IDOK);
		bt->SetWindowTextW(TEXT("Inject"));
		CString statusMsg;
		statusMsg.LoadString(IDS_STRING102);
		status->SetWindowText(statusMsg);
	}
	else {
		isThreadRunning = true;
		th = std::thread(waitForProcess, this);
		CButton* bt = (CButton*)GetDlgItem(IDOK);
		bt->SetWindowTextW(TEXT("Cancel Injection"));
		CString statusMsg;
		statusMsg.LoadString(IDS_STRING103);
		status->SetWindowText(statusMsg);
	}
}


void CAboutDlg::OnChooseFileBnClickedButton()
{
	// TODO: Add your control notification handler code here

}


void CDownsInjectorDlg::OnChooseFileBnClickedButton()
{
	CEdit* fileEditBox = (CEdit*)GetDlgItem(IDC_EDIT1);
	CString initText;
	fileEditBox->GetWindowText(initText);

	const TCHAR szFilter[] = _T("DLL Files (*.dll)|*.dll|");
	CFileDialog dlg(TRUE, _T("Select Dll File"), initText, OFN_NOCHANGEDIR, szFilter, this);
	if (dlg.DoModal() == IDOK)
	{
		CString sFilePath = dlg.GetPathName();
		fileEditBox->SetWindowText(sFilePath);
		
		//Set Status
		CStatic* status = (CStatic*)GetDlgItem(IDC_STATIC_STATUS_MSG);
		CString statusMsg;
		statusMsg.LoadString(IDS_STRING102);
		status->SetWindowText(statusMsg);
	}

	//IDS_STRING102

}

