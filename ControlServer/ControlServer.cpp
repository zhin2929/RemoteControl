// ControlServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <istream>
#include <fstream>
#include <vector>
#include <thread>

#include <afx.h>
#include <atlimage.h>
#include <thread>
#include "ITCP.h"
#include "ControlServer.h"

#pragma comment(lib,"gdiplus.lib")

using namespace zh;
using namespace Gdiplus;
#pragma warning(disable : 4996)
//////////////////////////// 断言/校验宏 /////////////////////////////

inline void chMB(PCSTR szMsg) {
  char szTitle[MAX_PATH];
  GetModuleFileNameA(NULL, szTitle, _countof(szTitle));
  MessageBoxA(GetActiveWindow(), szMsg, szTitle, MB_OK);
}

inline void chFAIL(PSTR szMsg) {
  chMB(szMsg);
  DebugBreak();
}

inline void chASSERTFAIL(LPCSTR file, int line, PCSTR expr) {
  char sz[2 * MAX_PATH];
  wsprintfA(sz, "File %s, line %d : %s", file, line, expr);
  chFAIL(sz);
}

#ifdef _DEBUG
#define chASSERT(x) if (!(x)) chASSERTFAIL(__FILE__, __LINE__, #x)
#else
#define chASSERT(x)
#endif

#if 1


//CMD服务
struct CmdServer {
  CmdServer() = default;

  HANDLE hRead = NULL;
  HANDLE hWrite = NULL;
  HANDLE hParentRead = NULL;
  HANDLE hChildWrite = NULL;
  CString szOutput; //读取到的数据
  std::shared_ptr<ITCP> m_cmdTcp;
  SOCKET m_sock;
  bool m_bIsRunning = false;
  HANDLE m_hProcess;

  void SetTcp(std::shared_ptr<ITCP> tcp) {
    m_cmdTcp = tcp;
  }

  void SetSocket(SOCKET sock) {
    m_sock = sock;
  }

  void ExitServer() {
    m_bIsRunning = false;
    TerminateProcess(m_hProcess, 0);
    hRead = NULL;
    hWrite = NULL;
    hParentRead = NULL;
    hChildWrite = NULL;
  }
  //从管道读取
  void CmdThreadReadFunc() {
    DWORD dwBytesAvail = 0;
    DWORD dwTotalReaded = 0;
    while (m_bIsRunning) {
      if (PeekNamedPipe(hParentRead,
        NULL, 0, NULL, &dwBytesAvail, NULL)) {
        DWORD dwBytesReaded = 0;
        if (dwBytesAvail > 0) {
          CString szOutput;
          ReadFile(hParentRead,
            //设置字符串变量的长度
            szOutput.GetBufferSetLength(dwBytesAvail),
            dwBytesAvail,
            &dwBytesReaded,
            NULL);

          //发包
          int szLen = szOutput.GetLength();
          if (m_cmdTcp) {
            m_cmdTcp->SendPacketFromServer<S2CCmdPacket>(m_sock, szLen,
              szLen, szOutput.GetBuffer());
          }
        }
      }
    }
  }

  //创建管道和CMD进程
  void CmdCreatePipeAndProcess() {
    //创建管道通信
    SECURITY_ATTRIBUTES sa = {};
    //设置子进程可继承父进程的句柄
    sa.bInheritHandle = TRUE;
    BOOL bRet = CreatePipe(&hRead, &hWrite, &sa, 0);
    chASSERT(bRet);
    bRet = CreatePipe(&hParentRead, &hChildWrite, &sa, 0);
    chASSERT(bRet);

    //启动CMD进程
    STARTUPINFO si = {};
    PROCESS_INFORMATION pi = {};
    si.cb = sizeof(si);

    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hRead;
    si.hStdOutput = hChildWrite;
    si.hStdError = hChildWrite;
    bRet = CreateProcess(NULL, // 进程名.
      (LPSTR)"cmd", // 命令行参数，
      NULL,             // 进程句柄不可继承.
      NULL,             // 线程句柄不可继承
      TRUE,            // 允许子进程继承句柄
      0, // 不显示窗口 CREATE_NO_WINDOW CREATE_SUSPENDED, 0立即运行
      NULL,             // 使用父进程的环境块
      NULL,             // 使用父进程的启动目录
      &si,              // 指向 STARTUPINFO 的结构体指针
      &pi);// 指向 PROCESS_INFORMATION 的结构体指针
    if (!bRet) {
      MessageBox(GetActiveWindow(), "创建进程失败", "创建进程失败", MB_OK);
    }
    m_bIsRunning = true;
    m_hProcess = pi.hProcess;
    //启动线程接收CMD回显数据
    std::thread(&CmdServer::CmdThreadReadFunc, this).detach();
  }
  //写入命令到管道
  bool CmdWrite(CString szCmd) {
    //写入数据
    DWORD dwBytesWrited = 0;
    bool bRet = WriteFile(hWrite,
      szCmd.GetBuffer(), // ""
      szCmd.GetLength(), // 0
      &dwBytesWrited,
      NULL);
    chASSERT(bRet);
    return bRet;
  }

  void C2SCmd(std::shared_ptr<Packet> ptr) {
    //解包
    C2SCmdPacket* cmdPacket = (C2SCmdPacket*)ptr.get();
    //获取命令
    CString szCmd;
    std::memcpy(szCmd.GetBufferSetLength(cmdPacket->m_nDataLen), cmdPacket->m_nDataBuf, cmdPacket->m_nDataLen);
    if (szCmd == "qqq\r\n") {
      ExitServer();
      return;
    }
    //如果是第一次就创建管道和CMD进程
    if (!hRead) {
      CmdCreatePipeAndProcess();
    }

    //写入命令
    CmdWrite(szCmd);
  }

};


//屏幕、键盘、鼠标服务
struct ScreenMouseKeyServer {
  std::shared_ptr<ITCP> m_screenTcp;
  SOCKET m_sock;
  bool m_bIsSendScreen = false;
  size_t m_nSleepTime = false;

  void SetTcp(std::shared_ptr<ITCP> tcp) {
    m_screenTcp = tcp;
  }
  void SetSocket(SOCKET sock) {
    m_sock = sock;
  }
  int GetMouseEvent(int nMsgType) {
    switch (nMsgType) {
      case WM_MOUSEMOVE: return MOUSEEVENTF_MOVE;
      case WM_MOUSEWHEEL: return MOUSEEVENTF_WHEEL;
      case WM_LBUTTONDOWN: return MOUSEEVENTF_LEFTDOWN;
      case WM_LBUTTONUP: return MOUSEEVENTF_LEFTUP;
      case WM_RBUTTONDOWN: return MOUSEEVENTF_RIGHTDOWN;
      case WM_RBUTTONUP: return MOUSEEVENTF_RIGHTUP;
      case WM_MBUTTONDOWN: return MOUSEEVENTF_MIDDLEDOWN;
      case WM_MBUTTONUP: return MOUSEEVENTF_MIDDLEUP;
      case WM_XBUTTONDOWN: return MOUSEEVENTF_XDOWN;
      case WM_XBUTTONUP: return MOUSEEVENTF_XUP;
      case WM_LBUTTONDBLCLK: return MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP;
      case WM_RBUTTONDBLCLK: return MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP;
      case WM_MBUTTONDBLCLK: return MOUSEEVENTF_MIDDLEDOWN | MOUSEEVENTF_MIDDLEUP;
      default: return 0;
    }
  }

  void SendScreen(std::shared_ptr<Packet> ptr) {
    C2SScreenPacket* pScreenPacket = (C2SScreenPacket*)ptr.get();
    m_bIsSendScreen = pScreenPacket->m_bIsRecvScreen;
    m_nSleepTime = pScreenPacket->m_nDelay;
    if (m_bIsSendScreen) {
      std::thread([this]() {
        while (m_bIsSendScreen) {
          //获取屏幕高宽
          int nWidth = GetSystemMetrics(SM_CXSCREEN);
          int nHight = GetSystemMetrics(SM_CYSCREEN);
          //创建屏幕DC，内存DC
          RAIIHDC hdcScreen = GetDC(NULL);
          RAIIHDC hdcMem = CreateCompatibleDC(hdcScreen.m_hdc);

          CImage img;
          img.Create(nWidth, nHight, 32);
          HDC hdcImg = img.GetDC();
          BitBlt(hdcImg, 0, 0, nWidth, nHight, hdcScreen.m_hdc, 0, 0, SRCCOPY);
          img.ReleaseDC();

          IStream* pStream = nullptr;
          HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);//可移动的缓冲区
          //BYTE* pMem = (BYTE*)GlobalLock(hMem);
          chASSERT(hMem);
          //创建一个流对象，该对象使用 HGLOBAL 内存句柄来存储流内容。
          CreateStreamOnHGlobal(hMem, true, &pStream);
          //以JPEG格式保存进 is 流中
          img.Save(pStream, ImageFormatJPEG);
          //得到缓冲区的起始地址
          BYTE* pbyBmp = (BYTE*)GlobalLock(hMem);
          chASSERT(pbyBmp);
          //锁住内存
          GlobalUnlock(hMem);
          //得到格式转换后图片的大小
          size_t nBitsCount = GlobalSize(hMem);

          //申请 nBitsCount 大小的缓冲区
          std::shared_ptr<char> pBitmapBit(new char[nBitsCount]);
          //将指定设备相关位图的位图位复制到缓冲区中
          memcpy(pBitmapBit.get(), pbyBmp, nBitsCount);
          printf("w=%d, h=%d, nBitsCount=%d\n", nWidth, nHight, nBitsCount);
          //发包
          m_screenTcp->SendPacketFromServer<S2CScreenPacket>(m_sock, nBitsCount,
            nBitsCount, nWidth, nHight, pBitmapBit.get());

          img.Destroy();
          pStream->Release();
          GlobalUnlock(hMem);
          GlobalFree(hMem);
          //休眠100秒
          std::this_thread::sleep_for(std::chrono::milliseconds(m_nSleepTime));
        }

      }).detach();
    }
  }

  void KeyEvent(std::shared_ptr<Packet> ptr) {
    C2SKeyBoardPacket* keyPacket = (C2SKeyBoardPacket*)ptr.get();
    MSG msg = keyPacket->m_msg;
    switch (msg.message) {
      case WM_KEYDOWN:
        keybd_event(msg.wParam, MapVirtualKey(msg.wParam, 0), 0, 0);
        break;
      case WM_KEYUP:
        keybd_event(msg.wParam, MapVirtualKey(msg.wParam, 0), KEYEVENTF_KEYUP, 0);
      default:
        break;
    }
  }

  void MouseEvent(std::shared_ptr<Packet> ptr) {
    C2SMousePacket* pMousePacket = (C2SMousePacket*)ptr.get();
    POINT pt = pMousePacket->m_msg.pt;
    POINT pt2;
    pt2.x = LOWORD(pMousePacket->m_msg.lParam);
    pt2.y = HIWORD(pMousePacket->m_msg.lParam);
    int nMsgType = pMousePacket->m_msg.message;
    int nMouseEvent = GetMouseEvent(nMsgType);

    if (nMouseEvent == MOUSEEVENTF_MOVE) { //移动的
      SetCursorPos(pt2.x, pt2.y);
      SetCapture(NULL);
    } else if (nMouseEvent != MOUSEEVENTF_MOVE) { //点击的
#if 0
      printf("GetMouseEvent=%d   ", nMouseEvent);
      printf("msgType=%d pt.x=%d,  pt.y=%d\n", nMsgType, pt.x, pt.y);

#endif // 0
      //mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
      SetCursorPos(pt.x, pt.y);
      mouse_event(nMouseEvent, pt.x, pt.y, 0, 0);
    }
  }
};


//文件信息，传输服务
struct FileServer {
  FileServer() = default;
  std::shared_ptr<ITCP> m_fileTcp;
  SOCKET m_sock;
  bool m_bIsRunning = false;
  CString m_szPath;
  size_t m_nRecvSize = 0; //用于接收文件上传，接收了多少字节
  size_t m_nSendSize = 0; //用于发送文件下载，发送了多少字节
  FILE* m_pUploadFile;
  FILE* m_pDownFile;
  CString path;
  void SetTcp(std::shared_ptr<ITCP> tcp) {
    m_fileTcp = tcp;
  }

  void SetSocket(SOCKET sock) {
    m_sock = sock;
  }

  void ExitServer() {
    m_bIsRunning = false;
  }

  //目录、文件信息处理函数
  void GetDirThreadFunc(CString szPath) {
    szPath += "\\*.*";
    CFileFind fileFind;
    bool bRet = fileFind.FindFile(szPath.GetBuffer());
    CString szResult;
    std::vector<FileInfoStruct> vctFileInfo;

    while (bRet) {
      bRet = fileFind.FindNextFile();
      CString fileName = fileFind.GetFileName();
      CString filePath = fileFind.GetFilePath();
      CTime time;
      fileFind.GetLastWriteTime(time);
      CString szType;
      CString szSize;
      ULONGLONG fileSize = fileFind.GetLength() / 1024; // KB

      if (fileSize < 1) {
        fileSize = 1;
      }
      if (fileFind.IsDirectory()) {
        szType = "文件夹";
        szSize = " ";
      } else {
        szSize.Format("%d KB", fileSize);
      }
      CString szTime = time.Format("%F %T");

      FileInfoStruct fileInfo = {};
      memcpy(fileInfo.m_szName, fileName.GetBuffer(), fileName.GetLength());
      memcpy(fileInfo.m_szDate, szTime.GetBuffer(), szTime.GetLength());
      memcpy(fileInfo.m_szType, szType.GetBuffer(), szType.GetLength());
      memcpy(fileInfo.m_szSize, szSize.GetBuffer(), szSize.GetLength());
      memcpy(fileInfo.m_szPath, filePath.GetBuffer(), filePath.GetLength());
      fileInfo.m_nSize = fileFind.GetLength();
      fileInfo.m_bIsDir = fileFind.IsDirectory();
      vctFileInfo.push_back(fileInfo);

    }
    //返回数据
    int nLen = vctFileInfo.size() * sizeof(FileInfoStruct);
    m_fileTcp->SendPacketFromServer<S2CFilePacket>(m_sock, nLen, nLen,
      vctFileInfo.size(), (char*)vctFileInfo.data());

    m_bIsRunning = false;
  }
  //获取盘符信息
  void GetDireveLetter(CString szPath) {
    DWORD dwLen = GetLogicalDriveStrings(0, NULL);//获取系统盘符字符串长度
    //构建字符数组
    char pszDriver[500];
    GetLogicalDriveStrings(dwLen, pszDriver);//获取系统盘符字符串
    m_fileTcp->SendPacketFromServer<S2CFilePacket>(m_sock, 500, 500, 0, pszDriver);
  }

  //获取目录和文件信息
  void GetDirInfo(CString szPath) {
    //启动线程获取系统目录
    if (!m_bIsRunning) {//线程没有运行时才启动，否则不
      m_szPath = szPath;
      m_bIsRunning = true;
      //启动线程接收CMD回显数据
      std::thread(&FileServer::GetDirThreadFunc, this, szPath).detach();
    }
  }

  //文件上传
  void FileUpload(std::shared_ptr<Packet> ptr) {
    C2SUploadPacket* pUploadPacket = (C2SUploadPacket*)ptr.get();
    size_t nTotalLen = pUploadPacket->m_nTotalLen;
    size_t nTotalSend = pUploadPacket->m_nTotalSendLen;
    size_t nPathLen = pUploadPacket->m_nPathLen;
    size_t nDataLen = pUploadPacket->m_nDataLen;
    if (nTotalLen == 0 && nTotalSend == 0
      && nPathLen == 0 && nDataLen == 0) { //传输结束
      if (m_pUploadFile) {
        fclose(m_pUploadFile);
      }
      m_pUploadFile = nullptr;
      printf("上传结束，总大小：%lld 字节  %d KB  %d MB\n", m_nRecvSize, m_nRecvSize / 1024, m_nRecvSize / 1024 / 1024);
      m_nRecvSize = 0;
      return;
    }
    if (nPathLen > 1000000) {
      printf("异常数据……\n");
      return;
    }
    CString szSavePath;
    memcpy(szSavePath.GetBufferSetLength(nPathLen), pUploadPacket->m_szSavePath, nPathLen);
    //写入文件
    if (!m_pUploadFile) {
      m_pUploadFile = fopen(szSavePath.GetBuffer(), "wb");
      if (!m_pUploadFile) {
        printf("打开文件失败");
        return;
      }
    }
    if (m_pUploadFile) {
      fwrite(pUploadPacket->m_pDataBuf, 1, nDataLen, m_pUploadFile);
      m_nRecvSize += nDataLen;
    }
    //fflush(fo.m_pFile);
  }
  //文件下载
  void DownloadFile(std::shared_ptr<Packet> ptr) {
    C2SDownloadPacket* pDownPacket = (C2SDownloadPacket*)ptr.get();
    int nPathLen = pDownPacket->m_nDataLen;
    CString szDownPath;
    memcpy(szDownPath.GetBufferSetLength(nPathLen), pDownPacket->m_nDataBuf, nPathLen);
    path = szDownPath;
    if (!m_pDownFile) {
      std::thread([&]()
      {
        if (!m_pDownFile) {

          m_pDownFile = fopen(path.GetBuffer(), "rb+");
          if (!m_pDownFile) {
            printf("打开下载文件失败\n");
            return;
          }
        }
        constexpr int BUF_SIZE = 0x10000;
        fpos_t nFileSize = 0; //获取文件大小
        fseek(m_pDownFile, 0, SEEK_END);
        fgetpos(m_pDownFile, &nFileSize);
        fseek(m_pDownFile, 0, SEEK_SET);
        int nTotalSend = 0;
        char buf[BUF_SIZE] = {};
        int nCnt = (nFileSize % BUF_SIZE == 0) ? nFileSize / BUF_SIZE : nFileSize / BUF_SIZE + 1;
        for (int i = 0; i < nCnt; ++i) {
          int nSendSize = (i == nCnt - 1) ? nFileSize - i * BUF_SIZE : BUF_SIZE;

          fread(buf, nSendSize, 1, m_pDownFile);
          m_fileTcp->SendPacketFromServer<S2CDownloadPacket>(m_sock,
            nSendSize, nFileSize, nTotalSend, nSendSize, buf);
          nTotalSend += nSendSize;
          m_nSendSize += nSendSize;
        }
        //发送结束包
        m_fileTcp->SendPacketFromServer<S2CDownloadPacket>(m_sock, 0);
        printf("发送完毕\n");
        fclose(m_pDownFile);
        m_pDownFile = nullptr;
      }
      ).detach();

    }

  }

  void C2SFile(std::shared_ptr<Packet> ptr) {
    C2SFilePacket* filePacket = (C2SFilePacket*)ptr.get();
    CString szPath;
    memcpy(szPath.GetBufferSetLength(filePacket->m_nDataLen), filePacket->m_nDataBuf, filePacket->m_nDataLen);
    if (szPath == "获取盘符") {
      GetDireveLetter(szPath);
    } else {
      GetDirInfo(szPath);
    }
  }

};


//被控端总服务，包含 CmdServer ScreenMouseKeyServer FileServer 三个服务
struct ControlServer {
  ITCP itcp;
  CmdServer cmdServer;
  FileServer fileServer;
  ScreenMouseKeyServer smkServer;

  ControlServer() {
    itcp = {};
    cmdServer = {};
    fileServer = {};
    smkServer = {};
  }

  //处理控制端的消息
  int HandleControlMsg(SOCKET sock) {
    smkServer.SetSocket(sock);
    fileServer.SetSocket(sock);
    cmdServer.SetSocket(sock);
    auto pPacket = itcp.RecvPacket(sock);
    switch ((*pPacket).m_packetType) {
      case PACKET_TYPE::C2S_DOWNLOAD: //接收控制端发送的下载文件命令
      {
        fileServer.DownloadFile(pPacket);
        break;
      }
      case PACKET_TYPE::C2S_UPLOAD: //接收控制端上传文件
      {
        fileServer.FileUpload((pPacket));
        break;
      }
      case PACKET_TYPE::C2S_FILE: //接收控制端发送的文件浏览
      {
        fileServer.C2SFile(pPacket);
        break;
      }
      case PACKET_TYPE::C2S_SCREEN: //接收控制端发来的获取屏幕命令
      {
        smkServer.SendScreen(pPacket);
        break;
      }
      case PACKET_TYPE::C2S_KEYBOARD://接收控制端发来的键盘命令
      {
        smkServer.KeyEvent(pPacket);
        break;
      }
      case PACKET_TYPE::C2S_MOUSE: //接收控制端发来的鼠标命令
      {
        smkServer.MouseEvent(pPacket);
        break;
      }
      case PACKET_TYPE::C2S_CMD: //接收控制端发来的远程执行CMD命令
      {
        cmdServer.C2SCmd(pPacket);
        break;
      }
      case PACKET_TYPE::NO_CONNECT: //断开连接
      {
        smkServer.m_bIsSendScreen = false; //停止发送屏幕
        printf("断开连接\n");
        return 0; //返回0断开连接
      }
    }
    return 1; //正常返回
  }

  bool StartServer(short nPort, const char* szIp = "0.0.0.0") {
    return itcp.StartServer(nPort, szIp);
  }

  SOCKET GetListenSocket() {
    return itcp.GetListenSocket();
  }
  void SetTcp() {
    cmdServer.SetTcp(std::make_shared<ITCP>(itcp));
    smkServer.SetTcp(std::make_shared<ITCP>(itcp));
    fileServer.SetTcp(std::make_shared<ITCP>(itcp));
  }
};


//单线程死循环
#if 0
void ServerThreadFunc(int port) {
  ControlServer ctrlServer;
  if (!ctrlServer.StartServer(port)) {
    printf("StartServer 错误");
    getchar();
    return;
  }
  printf("等待连接\n");
  ctrlServer.itcp.Accept();
  SOCKET curSocket = ctrlServer.itcp.GetClientSocket();
  while (true) {
    int res = ctrlServer.SocketHandle(curSocket);
    if (res <= 0)//如果socket句柄异常那么就删除这个句柄
    {
      closesocket(curSocket);
      return;
    }
  }
}
#endif // 0

//select 模型 1对多TCP连接
#if 1 //select

void ServerThreadFunc(int port) {
  ControlServer ctrlServer;
  if (!ctrlServer.StartServer(port)) {
    printf("StartServer 错误");
    getchar();
    return;
  }
  printf("等待连接\n");

  SOCKET listenSock = ctrlServer.GetListenSocket();
  std::map<SOCKET, sockaddr_in> mapClients;

  //配置select模型
  fd_set fdReadTotal = {};
  fd_set fdRead = {};// 由于每次检查后都会改变队列.这个作为 fdReadTotal 的副本去select
  TIMEVAL timeout = { 5, 0 };
  //将监听连接建立的socket先放进去
  FD_SET(listenSock, &fdReadTotal);
  while (true) {
    //1 配置副本
    fdRead = fdReadTotal;
    //2 select选择
    int nRet = select(0, &fdRead, NULL, NULL, &timeout);
    if (nRet == SOCKET_ERROR) { //表示出错了
      printf("连接失败\n");
      continue;
    } else if (nRet == 0) { //表示等待超时
      //printf("wait for select!");
    } else {
      if (FD_ISSET(listenSock, &fdRead)) { //说明有新的链接要建立
        sockaddr_in clientAddr = { 0 };
        clientAddr.sin_family = AF_INET;
        int nLength = sizeof(sockaddr_in);
        SOCKET sockClient = accept(listenSock, (sockaddr*)&clientAddr, &nLength);
        FD_SET(sockClient, &fdReadTotal);
        if (sockClient == INVALID_SOCKET) {
          printf("%s:%d 客户端连接错误\n",
            inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
          continue;
        }
        printf("连接成功\n");
        //连接成功再设置
        ctrlServer.SetTcp();
        mapClients[sockClient] = clientAddr;
      } else {
        //第0个是 listenSocket，客户端socket从1开始
        for (size_t i = 1; i < fdReadTotal.fd_count; i++) {
          if (FD_ISSET(fdReadTotal.fd_array[i], &fdRead)) {
            //表示可以收包了
            SOCKET curSocket = fdReadTotal.fd_array[i];
            int res = ctrlServer.HandleControlMsg(curSocket);
            if (res <= 0)//如果socket句柄异常那么就删除这个句柄
            {
              closesocket(curSocket);
              FD_CLR(curSocket, &fdReadTotal);
            }
          }
        }
      }
    }
  }

}

#endif // 0 select 模型 1对多TCP连接

int main() {

  //多Select连接
  std::thread(ServerThreadFunc, 10086).detach(); //处理屏幕、鼠标、键盘
  std::thread(ServerThreadFunc, 10087).detach(); //处理CMD
  std::thread(ServerThreadFunc, 10088).detach(); //处理文件浏览、传输


  while (true) {
    ;
  }



  //单TCP连接
#if 0  
  ITCP itcp;

  if (!itcp.StartServer(10086)) {
    printf("StartServer 错误");
    getchar();
    return 0;
  }
  printf("等待连接\n");

  CmdServer cmdServer;
  SOCKET listenSock = itcp.GetListenSocket();
  map<SOCKET, sockaddr_in> mapClients;

  while (true) {
    if (!itcp.Accept()) {
      continue;
    }
    printf("连接成功\n");
    //连接成功再设置
    cmdServer.SetTcp(std::make_shared<ITCP>(itcp));
    cmdServer.SetSocket(itcp.GetClientSocket());
    std::thread([&]()
    {
      while (true) {
        auto pPacket = itcp.RecvPacket();
        switch ((*pPacket).m_packetType) {
          //switch (PACKET_TYPE::C2S_SCREEN) {
          case PACKET_TYPE::C2S_SCREEN: //接收控制端发来的获取屏幕命令
          {
            //获取屏幕高宽
            int nWidth = GetSystemMetrics(SM_CXSCREEN);
            int nHight = GetSystemMetrics(SM_CYSCREEN);
            //创建屏幕DC，内存DC
            RAIIHDC hdcScreen = GetDC(NULL);
            RAIIHDC hdcMem = CreateCompatibleDC(hdcScreen.m_hdc);

            CImage img;
            img.Create(nWidth, nHight, 32);
            HDC hdcImg = img.GetDC();
            BitBlt(hdcImg, 0, 0, nWidth, nHight, hdcScreen.m_hdc, 0, 0, SRCCOPY);
            img.ReleaseDC();

            IStream* pStream = nullptr;
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);//可移动的缓冲区
            //BYTE* pMem = (BYTE*)GlobalLock(hMem);
            chASSERT(hMem);
            //创建一个流对象，该对象使用 HGLOBAL 内存句柄来存储流内容。
            CreateStreamOnHGlobal(hMem, true, &pStream);
            //以JPEG格式保存进 is 流中
            img.Save(pStream, ImageFormatJPEG);
            //得到缓冲区的起始地址
            BYTE* pbyBmp = (BYTE*)GlobalLock(hMem);
            chASSERT(pbyBmp);
            //锁住内存
            GlobalUnlock(hMem);
            //得到格式转换后图片的大小
            size_t nBitsCount = GlobalSize(hMem);

            //申请 nBitsCount 大小的缓冲区
            std::shared_ptr<char> pBitmapBit(new char[nBitsCount]);
            //将指定设备相关位图的位图位复制到缓冲区中
            memcpy(pBitmapBit.get(), pbyBmp, nBitsCount);
            printf("w=%d, h=%d, nBitsCount=%d\n", nWidth, nHight, nBitsCount);
            //发包
            itcp.SendPacket<S2CScreenPacket>(nBitsCount,
              nBitsCount, nWidth, nHight, pBitmapBit.get());

            img.Destroy();
            pStream->Release();
            GlobalUnlock(hMem);
            GlobalFree(hMem);
            break;
          }
          case PACKET_TYPE::C2S_KEYBOARD://接收控制端发来的获取键盘命令
          {
            break;
          }
          case PACKET_TYPE::C2S_MOUSE: //接收控制端发来的获取鼠标命令
          {
            C2SMousePacket* pMousePacket = (C2SMousePacket*)pPacket.get();
            POINT pt = pMousePacket->m_msg.pt;
            POINT pt2;
            pt2.x = LOWORD(pMousePacket->m_msg.lParam);
            pt2.y = HIWORD(pMousePacket->m_msg.lParam);
            int nMsgType = pMousePacket->m_msg.message;
            int nMouseEvent = GetMouseEvent(nMsgType);

            if (nMouseEvent == MOUSEEVENTF_MOVE) { //移动的
              SetCursorPos(pt2.x, pt2.y);
              SetCapture(NULL);
            } else if (nMouseEvent != MOUSEEVENTF_MOVE) { //点击的
#if 0
              printf("GetMouseEvent=%d   ", nMouseEvent);
              printf("msgType=%d pt.x=%d,  pt.y=%d\n", nMsgType, pt.x, pt.y);

#endif // 0

              SetCursorPos(pt.x, pt.y);
              mouse_event(nMouseEvent, pt.x, pt.y, 0, 0);
            }
            m_nPrevMouseEvent = nMouseEvent;
            break;
          }
          case PACKET_TYPE::C2S_CMD: //接收控制端发来的CMD命令
          {
            //解包
            C2SCmdPacket* cmdPacket = (C2SCmdPacket*)pPacket.get();
            //获取命令
            CString szCmd;
            memcpy(szCmd.GetBufferSetLength(cmdPacket->m_nDataLen), cmdPacket->m_nDataBuf, cmdPacket->m_nDataLen);
            //如果是第一次就创建管道和CMD进程
            if (!cmdServer.hRead) {
              cmdServer.CmdCreatePipeAndProcess();
            }

            //写入命令
            cmdServer.CmdWrite(szCmd);

            break;
          }
          case PACKET_TYPE::NO_CONNECT: //断开连接
          {
            printf("断开连接\n");
            return;
          }
        }
      }
    }
    ).detach();
  }


#endif // 0



  //单 select 模型 1对多
#if 0

  ControlServer ctrlServer;
  if (!ctrlServer.StartServer(10086)) {
    printf("StartServer 错误");
    getchar();
    return 0;
  }
  printf("等待连接\n");

  SOCKET listenSock = ctrlServer.GetListenSocket();
  map<SOCKET, sockaddr_in> mapClients;

  //配置select模型
  fd_set fdReadTotal = {};
  fd_set fdRead = {};// 由于每次检查后都会改变队列.这个作为 fdReadTotal 的副本去select
  TIMEVAL timeout = { 5, 0 };
  //将监听连接建立的socket先放进去
  FD_SET(listenSock, &fdReadTotal);
  while (true) {
    //1 配置副本
    fdRead = fdReadTotal;
    //2 select选择
    int nRet = select(0, &fdRead, NULL, NULL, &timeout);
    if (nRet == SOCKET_ERROR) { //表示出错了
      printf("连接失败\n");
      return -1;
    } else if (nRet == 0) { //表示等待超时
      //printf("wait for select!");
    } else {
      if (FD_ISSET(listenSock, &fdRead)) { //说明有新的链接要建立
        sockaddr_in clientAddr = { 0 };
        clientAddr.sin_family = AF_INET;
        int nLength = sizeof(sockaddr_in);
        SOCKET sockClient = accept(listenSock, (sockaddr*)&clientAddr, &nLength);
        FD_SET(sockClient, &fdReadTotal);
        if (sockClient == INVALID_SOCKET) {
          printf("%s:%d 客户端连接错误\n",
            inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
          continue;
        }
        printf("连接成功\n");
        //连接成功再设置
        ctrlServer.SetTcp();
        mapClients[sockClient] = clientAddr;
      } else {
        //第0个是 listenSocket，客户端socket从1开始
        for (size_t i = 1; i < fdReadTotal.fd_count; i++) {
          if (FD_ISSET(fdReadTotal.fd_array[i], &fdRead)) {
            //表示可以收包了
            SOCKET curSocket = fdReadTotal.fd_array[i];
            int res = ctrlServer.SocketHandle(curSocket);
            if (res <= 0)//如果socket句柄异常那么就删除这个句柄
            {
              closesocket(curSocket);
              FD_CLR(curSocket, &fdReadTotal);
            }
          }
        }
      }
    }
  }


#endif // 0


  return 0;
}

#endif // 0
