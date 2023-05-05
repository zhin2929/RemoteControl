// ControlClientView.cpp: CControlClientView 类的实现
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS 可以在实现预览、缩略图和搜索筛选器句柄的
// ATL 项目中进行定义，并允许与该项目共享文档代码。
#ifndef SHARED_HANDLERS
#include "ControlClient.h"
#endif

#include "ControlClientDoc.h"
#include "ControlClientView.h"
#include "CmdDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//定时id：发送包获取屏幕
#define TIMER_SEND_SCREEN 1

using namespace zh;

IMPLEMENT_DYNCREATE(CControlClientView, CView)

BEGIN_MESSAGE_MAP(CControlClientView, CView)
  ON_COMMAND(MN_CONNECT, &CControlClientView::OnConnect)
  ON_COMMAND(MN_SCREEN, &CControlClientView::OnScreen)
  ON_COMMAND(MN_CMD, &CControlClientView::OnCmd)
  ON_WM_TIMER()
  ON_COMMAND(MN_STOPSCREEN, &CControlClientView::OnStopScreen)
  ON_COMMAND(MN_FILEBROWSE, &CControlClientView::OnFileBrowse)
  ON_WM_SIZE()
  ON_COMMAND(MN_CONNECTLOCAL, &CControlClientView::OnConnectLocal)
  ON_WM_CHAR()
  ON_COMMAND(ID_SETTING, &CControlClientView::OnSetting)
  ON_COMMAND(MN_MOUSEKEY, &CControlClientView::OnMousekey)
END_MESSAGE_MAP()

// CControlClientView 构造/析构

CControlClientView::CControlClientView() noexcept {
  // TODO: 在此处添加构造代码
  m_szIp = "127.0.0.1";
}

CControlClientView::~CControlClientView() {
}

BOOL CControlClientView::PreCreateWindow(CREATESTRUCT& cs) {
  // TODO: 在此处通过修改
  //  CREATESTRUCT cs 来修改窗口类或样式

  return CView::PreCreateWindow(cs);
}

// CControlClientView 绘图

void CControlClientView::OnDraw(CDC* /*pDC*/) {
  CControlClientDoc* pDoc = GetDocument();
  ASSERT_VALID(pDoc);
  if (!pDoc)
    return;

  // TODO: 在此处为本机数据添加绘制代码
}

// CControlClientView 诊断

#ifdef _DEBUG
void CControlClientView::AssertValid() const {
  CView::AssertValid();
}

void CControlClientView::Dump(CDumpContext& dc) const {
  CView::Dump(dc);
}

CControlClientDoc* CControlClientView::GetDocument() const // 非调试版本是内联的
{
  ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CControlClientDoc)));
  return (CControlClientDoc*)m_pDocument;
}
#endif //_DEBUG

// CControlClientView 消息处理程序

void CControlClientView::OnConnectLocal() {
  m_bIsThreadRunning = true;
  //连接服务端 192.168.43.128   127.0.0.1
  m_szIp = "127.0.0.1";
  bool bConnected = m_screenTcp.ConnectServer(10086, m_szIp.GetBuffer());
  if (bConnected) {
    std::thread(&CControlClientView::WorkScreenThread, this).detach();
  }
  else {
    AfxMessageBox("连接不成功");
  }
}

void CControlClientView::OnConnect() {
  m_bIsThreadRunning = true;
  //连接服务端 192.168.43.128   127.0.0.1
  //m_szIp = "192.168.43.128";
  if (m_szIp == "127.0.0.1") {
    AfxMessageBox("未设置远程IP");
    return;
  }
  bool bConnected = m_screenTcp.ConnectServer(10086, m_szIp.GetBuffer());
  if (bConnected) {
    std::thread(&CControlClientView::WorkScreenThread, this).detach();
  }
  else {
    AfxMessageBox("连接不成功");
  }
}



//将图片字节流显示到指定DC
void DrawPicture(HDC hdc, char* buf, int len) {
  HGLOBAL hMem = GlobalAlloc(GMEM_FIXED, len);
  BYTE* pMem = (BYTE*)GlobalLock(hMem);
  memcpy(pMem, buf, len);
  IStream* pStream;
  HRESULT hr = CreateStreamOnHGlobal(pMem, FALSE, &pStream);

  CImage img;
  img.Load(pStream);
  img.Draw(hdc, CPoint(0, 0));

  img.Destroy();
  pStream->Release();
  GlobalUnlock(hMem);
  GlobalFree(hMem);
}

//CMD工作线程
void CControlClientView::WorkCmdThread() {
  int count2 = 0;
  while (true) {
    std::shared_ptr<Packet> pPacket = m_cmdTcp.RecvPacket();
    switch (pPacket->m_packetType) {
      case PACKET_TYPE::S2C_CMD:
      {
        //解包
        S2CCmdPacket* pCmdPacket = (S2CCmdPacket*)pPacket.get();
        CString szRet;
        std::memcpy(szRet.GetBufferSetLength(pCmdPacket->m_nDataLen),
          pCmdPacket->m_nDataBuf, pCmdPacket->m_nDataLen);

        if (m_cmdDlg) {
          int len = m_cmdDlg.m_editShowCmd.GetWindowTextLength(); //文本框长度
          m_cmdDlg.m_editShowCmd.SetSel(len, len, false);
          m_cmdDlg.m_editShowCmd.ReplaceSel(szRet);
          m_cmdDlg.m_editShowCmd.SetFocus();
        }

        break;
      }
      case PACKET_TYPE::NO_CONNECT:
      {
        m_cmdTcp.ExitConnect();
        return;
      }
      default:
        break;
    }
  }
}
//文件浏览、传输 线程
void CControlClientView::WorkFileThread() {
  int countError = 0;
  while (true) {
    std::shared_ptr<Packet> pPacket = m_fileTcp.RecvPacket();
    switch (pPacket->m_packetType) {
      case PACKET_TYPE::S2C_DOWNLOAD:
      {
        S2CDownloadPacket* pDownPacket = (S2CDownloadPacket*)pPacket.get();

        size_t nTotalLen = pDownPacket->m_nTotalLen;
        size_t nTotalSend = pDownPacket->m_nTotalSendLen;
        size_t nDataLen = pDownPacket->m_nDataLen; //此次发送的数据长度
        if (nTotalLen == 0 && nTotalSend == 0 && nDataLen == 0) {
          //发送完毕
          if (m_pDownFile) {
            fclose(m_pDownFile);
          }
          m_pDownFile = nullptr;
          printf("下载结束，总大小：%lld 字节  %d KB  %d MB\n", m_nRecvSize, m_nRecvSize / 1024, m_nRecvSize / 1024 / 1024);
          m_nRecvSize = 0;
          break;
        }

        CString szSavePath = m_fileDlg.m_szDownPath;
        if (!m_pDownFile) {
          m_pDownFile = fopen(szSavePath.GetBuffer(), "wb");
          if (!m_pDownFile) {
            printf("打开下载文件失败\n");
            return;
          }
        }
        if (m_pDownFile) {
          fwrite(pDownPacket->m_pDataBuf, 1, nDataLen, m_pDownFile);
          m_nRecvSize += nDataLen;
          int pos = m_fileDlg.m_proDownload.GetPos();
          m_fileDlg.m_proDownload.SetPos(pos+1);
        }
        break;
      }
      case PACKET_TYPE::S2C_FILE:
      {
        S2CFilePacket* filePacket = (S2CFilePacket*)pPacket.get();
        if (filePacket->m_nDataSize == 0 && filePacket->m_nDataLen > 0) { //获取盘符
          char* pDriver = filePacket->m_nDataBuf;
          while (*pDriver != '\0') {
            CString path = pDriver;
            pDriver += strlen(pDriver) + 1;//定位到下一个字符串
            m_fileDlg.SetTreeData(path);
          }
        }
        else if(filePacket->m_nDataSize > 0) {
          //共有多少个文件、目录
          std::vector<FileInfoStruct> vct(filePacket->m_nDataSize);
          //复制发送过来vector数据
          memcpy((char*)vct.data(), filePacket->m_nDataBuf, filePacket->m_nDataLen);

          //设置数据
          m_fileDlg.SetData(vct);
        }

        break;
      }
    
      case PACKET_TYPE::NO_CONNECT:
      {
        m_fileTcp.ExitConnect();
        return;
      }
    }
  }
}
//屏幕，鼠标工作线程
void CControlClientView::WorkScreenThread() {
  int countError = 0;
  while (true) {
    std::shared_ptr<Packet> pPacket = m_screenTcp.RecvPacket();
    switch (pPacket.get()->m_packetType) {
      case PACKET_TYPE::S2C_SCREEN:
      {
        S2CScreenPacket* pScreen = (S2CScreenPacket*)pPacket.get();

        //显示到界面
        CDC* pDC = GetDC();

        int nSize = pPacket->m_nLen; //BYTE*指向的数据的长度
        if (pScreen->m_nDataLen > 3000000) {
          countError++;
          break;
        }
        DrawPicture(pDC->GetSafeHdc(), pScreen->m_nDataBuf, nSize);

        break;
      }
      case PACKET_TYPE::S2C_KEYBOARD:
      {
        break;
      }
      case PACKET_TYPE::S2C_MOUSE:
      {
        break;
      }
      case PACKET_TYPE::S2C_CMD:
      {
        //解包
        S2CCmdPacket* pCmdPacket = (S2CCmdPacket*)pPacket.get();
        CString szRet;
        std::memcpy(szRet.GetBufferSetLength(pCmdPacket->m_nDataLen),
          pCmdPacket->m_nDataBuf, pCmdPacket->m_nDataLen);

        if (m_cmdDlg) {
          int len = m_cmdDlg.m_editShowCmd.GetWindowTextLength(); //文本框长度
          m_cmdDlg.m_editShowCmd.SetSel(len, len, false);
          m_cmdDlg.m_editShowCmd.ReplaceSel(szRet);
          m_cmdDlg.m_editShowCmd.SetFocus();
        }

        break;
      }
      case PACKET_TYPE::NO_CONNECT:
      {
        m_screenTcp.ExitConnect();
        return;
      }
      default:
        break;
    }
  }
}
//定时发送获取屏幕
void CControlClientView::OnTimer(UINT_PTR nIDEvent) {
  switch (nIDEvent) {
    case TIMER_SEND_SCREEN:
    {
      //控制端向被控端发送命令，要求查看屏幕
      //第一个0是柔性数组的长度，第二个0是子类的长度，这个包没用子类，也填0
      m_screenTcp.SendPacket<Packet>(0, 0, PACKET_TYPE::C2S_SCREEN);
      
      break;
    }
    default:
      break;
  }
  CView::OnTimer(nIDEvent);
}

//点击查看屏幕
void CControlClientView::OnScreen() {
  if (m_screenTcp.GetClientSocket() == INVALID_SOCKET) {
    AfxMessageBox("未连接被控端");
    return;
  }
  //SetTimer(TIMER_SEND_SCREEN, 100, NULL);
  m_screenTcp.SendPacket<C2SScreenPacket>(0, true, 100);
}


//停止获取屏幕
void CControlClientView::OnStopScreen() {
  if (m_screenTcp.GetClientSocket() == INVALID_SOCKET) {
    AfxMessageBox("未连接被控端");
    return;
  }
  KillTimer(TIMER_SEND_SCREEN);
  m_screenTcp.SendPacket<C2SScreenPacket>(0, false, 0);
}

//点击菜单栏CMD
void CControlClientView::OnCmd() {
#if 0 //单TCP
  if (m_itcp.GetClientSocket() == INVALID_SOCKET) {
    AfxMessageBox("未连接被控端");
    return;
  }
  m_cmdDlg.SetTcp(m_itcp);
  bool nRet = m_cmdDlg.ShowWindow(SW_SHOW);

#endif // 1


#if 1 //多TCP
  if (m_cmdTcp.GetClientSocket() == INVALID_SOCKET) {
    bool bRet = m_cmdTcp.ConnectServer(10087, m_szIp.GetBuffer());
    if (!bRet) {
      AfxMessageBox("连接不成功");
      return;
      
    }
    std::thread(&CControlClientView::WorkCmdThread, this).detach();
  }
  m_cmdDlg.SetTcp(m_cmdTcp);
  bool nRet = m_cmdDlg.ShowWindow(SW_SHOW);

#endif // 0 //多TCP

}

//点击菜单栏文件浏览、传输
void CControlClientView::OnFileBrowse() {
  if (m_fileTcp.GetClientSocket() == INVALID_SOCKET) {
    bool bRet = m_fileTcp.ConnectServer(10088, m_szIp.GetBuffer());
    if (!bRet) {
      AfxMessageBox("连接不成功");
      return;
    }
    std::thread(&CControlClientView::WorkFileThread, this).detach();
  }

  m_fileDlg.SetTcp(m_fileTcp);
  //非模态立即发送一个文件浏览包
  if (!bFileDlgIsModal) {//非模态构造
    m_fileDlg.ShowWindow(SW_SHOW);
    CString szPath = "C:\\";
    int nLen = szPath.GetLength();
    m_fileTcp.SendPacket<C2SFilePacket>(nLen, nLen, szPath.GetBuffer());
    //获取盘符   非模态对话框走这里发送
    szPath = "获取盘符";
    nLen = szPath.GetLength();
    m_fileTcp.SendPacket<C2SFilePacket>(nLen, nLen, szPath.GetBuffer());
  }
  else {
    m_fileDlg.DoModal();
  }

}


void CControlClientView::OnSize(UINT nType, int cx, int cy) {
  CView::OnSize(nType, cx, cy);
}

BOOL CControlClientView::PreTranslateMessage(MSG* pMsg) {
  ScreenToClient(&(pMsg->pt));
  if (!m_bIsControlMouseKey) { //若为false，则不控制键鼠
    return CView::PreTranslateMessage(pMsg);
  }
  switch (pMsg->message) {
    case WM_KEYUP:
    {
      //WM_KEYDOWN、WM_KEYUP、WM_SYSKEYDOWN和WM_SYSKEYUP 的虚拟键码
      m_screenTcp.SendPacket<C2SKeyBoardPacket>(0, *pMsg);
      break;
    }
    case WM_KEYDOWN:
    {
      // pMsg->wParam 虚拟键码
      m_screenTcp.SendPacket<C2SKeyBoardPacket>(0, *pMsg);
      break;
    }
    case WM_CHAR:
    {
      //键盘消息 //键盘中可输入的字符：0 ~ 127
      //m_screenTcp.SendPacket<C2SKeyBoardPacket>(0, *pMsg);
      break;
    }
    case WM_MOUSEMOVE:
    case WM_MOUSEWHEEL:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    {
      //第一个0是柔性数组的长度
      m_screenTcp.SendPacket<C2SMousePacket>(0, *pMsg);
      break;
    }
    default:
      break;
  }
  m_lastMousePoint = pMsg->pt;
  return CView::PreTranslateMessage(pMsg);
}

// View 初始化
void CControlClientView::OnInitialUpdate() {
  CView::OnInitialUpdate();
  if (!m_cmdDlg) {
    m_cmdDlg.Create(DLG_CMD, this);
  }
  if (!bFileDlgIsModal) {//非模态构造
    m_fileDlg.Create(DLG_BROWSE, this);
  }
  
}




void CControlClientView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) {

  CView::OnChar(nChar, nRepCnt, nFlags);
}

//打开设置对话框
void CControlClientView::OnSetting() {
  SettingDlg dlg;
  int nRet = dlg.DoModal();
  if (nRet == IDOK) {
    in_addr addr;
    addr.s_addr = htonl(dlg.m_nIp);
    m_szIp = inet_ntoa(addr);
  }
}


//控制键鼠
void CControlClientView::OnMousekey()
{
  m_bIsControlMouseKey = !m_bIsControlMouseKey;
}
