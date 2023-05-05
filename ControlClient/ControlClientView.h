// ControlClientView.h: CControlClientView 类的接口
//

#pragma once
#include "ITCP.h"
#include "CmdDlg.h"
#include "FileBrowseDlg.h"
#include "SettingDlg.h"
#include <thread>
class CControlClientView : public CView {
private:
  bool bFileDlgIsModal{ false };
  zh::ITCP m_screenTcp;
  zh::ITCP m_cmdTcp;
  zh::ITCP m_fileTcp;
  CmdDlg m_cmdDlg;
  FileBrowseDlg m_fileDlg;
  bool m_bIsThreadRunning{ false };
  POINT m_lastMousePoint{ 0,0 };
  CString m_szIp{ "127.0.0.1" };
  FILE* m_pDownFile{ nullptr };
  size_t m_nRecvSize{ 0 };
  bool m_bIsControlMouseKey = false;
protected: // 仅从序列化创建
  CControlClientView() noexcept;
  DECLARE_DYNCREATE(CControlClientView)

    // 特性
public:
  CControlClientDoc* GetDocument() const;
  void WorkScreenThread();
  void WorkCmdThread();
  void WorkFileThread();
  // 操作
public:

  // 重写
public:
  virtual void OnDraw(CDC* pDC);  // 重写以绘制该视图
  virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:

  // 实现
public:
  virtual ~CControlClientView();
#ifdef _DEBUG
  virtual void AssertValid() const;
  virtual void Dump(CDumpContext& dc) const;
#endif

protected:

  // 生成的消息映射函数
protected:
  DECLARE_MESSAGE_MAP()

public:
  afx_msg void OnConnect();
  afx_msg void OnScreen();
  afx_msg void OnCmd();
  afx_msg void OnTimer(UINT_PTR nIDEvent);
  afx_msg void OnStopScreen();
  afx_msg void OnFileBrowse();
  afx_msg void OnSize(UINT nType, int cx, int cy);
  afx_msg void OnConnectLocal();
  virtual BOOL PreTranslateMessage(MSG* pMsg);
  virtual void OnInitialUpdate();
  afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
  afx_msg void OnSetting();
  afx_msg void OnClose();
  afx_msg void OnMousekey();
};

#ifndef _DEBUG  // ControlClientView.cpp 中的调试版本
inline CControlClientDoc* CControlClientView::GetDocument() const {
  return reinterpret_cast<CControlClientDoc*>(m_pDocument);
}
#endif
