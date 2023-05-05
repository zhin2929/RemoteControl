// DlgSetting.cpp: 实现文件
//

#include "pch.h"
#include "ControlClient.h"
#include "afxdialogex.h"
#include "SettingDlg.h"


// DlgSetting 对话框

IMPLEMENT_DYNAMIC(SettingDlg, CDialogEx)

SettingDlg::SettingDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(DLG_SETTING, pParent)
	, m_nIp(0) {

}

SettingDlg::~SettingDlg()
{
}

void SettingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS1, m_nIp);
	DDX_Control(pDX, IDC_IPADDRESS1, m_ip);
}


BEGIN_MESSAGE_MAP(SettingDlg, CDialogEx)
END_MESSAGE_MAP()


// DlgSetting 消息处理程序


BOOL SettingDlg::OnInitDialog() {
	CDialogEx::OnInitDialog();
	m_ip.SetWindowText("192.168.43.128");
	//m_nIp = inet_addr("192.168.43.128");
	UpdateData(true);
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}
