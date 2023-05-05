#pragma once
#include "afxdialogex.h"
#include "resource.h"		// 主符号
#include "ITCP.h"

// CmdDlg 对话框

class CmdDlg : public CDialogEx {
  DECLARE_DYNAMIC(CmdDlg)

public:
  CmdDlg(CWnd* pParent = nullptr);   // 标准构造函数
  virtual ~CmdDlg();
  void SetTcp(zh::ITCP& tcp);
  // 对话框数据
#ifdef AFX_DESIGN_TIME
  enum { IDD = DLG_CMD };
#endif

protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

  DECLARE_MESSAGE_MAP()
public:
  CString m_editCmd;
  CEdit m_editShowCmd;
  zh::ITCP m_itcp;
  afx_msg void OnBnClickedOk();
  virtual BOOL OnInitDialog();
  CEdit m_editInput;
};
