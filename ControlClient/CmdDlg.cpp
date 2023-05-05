// CmdDlg.cpp: 实现文件
//

#include "pch.h"
#include "ControlClient.h"
#include "afxdialogex.h"
#include "CmdDlg.h"

// CmdDlg 对话框

IMPLEMENT_DYNAMIC(CmdDlg, CDialogEx)

CmdDlg::CmdDlg(CWnd* pParent /*=nullptr*/)
  : CDialogEx(DLG_CMD, pParent)
  , m_editCmd(_T("dir")) {
}

CmdDlg::~CmdDlg() {
}

void CmdDlg::SetTcp(zh::ITCP& tcp) {
  m_itcp = tcp;
}

void CmdDlg::DoDataExchange(CDataExchange* pDX) {
  CDialogEx::DoDataExchange(pDX);
  DDX_Text(pDX, EDIT_CMD, m_editCmd);
  DDX_Control(pDX, EDIT_SHOW, m_editShowCmd);
  DDX_Control(pDX, EDIT_CMD, m_editInput);
}

BEGIN_MESSAGE_MAP(CmdDlg, CDialogEx)
  ON_BN_CLICKED(IDOK, &CmdDlg::OnBnClickedOk)
END_MESSAGE_MAP()

// CmdDlg 消息处理程序

void CmdDlg::OnBnClickedOk() {
  UpdateData(true);
  m_editCmd += "\r\n"; //加上回车换行符
  int nLen = m_editCmd.GetLength();
  //控制端向被控端发送执行CMD命令，C2SCmdPacket
  int nSendCount = m_itcp.SendPacket<C2SCmdPacket>(
    nLen, nLen, m_editCmd.GetBuffer());
  chASSERT(nSendCount == (sizeof(C2SCmdPacket) + nLen));
  m_editCmd = "";
  m_editInput.SetFocus();
  
  UpdateData(false);
}

BOOL CmdDlg::OnInitDialog() {
  CDialogEx::OnInitDialog();

  // 设置最大文本长度
  m_editShowCmd.SetLimitText(UINT32_MAX);
  return TRUE;  // return TRUE unless you set the focus to a control
  // 异常: OCX 属性页应返回 FALSE
}
