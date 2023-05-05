#pragma once

#include "CSocketInit.h"
#include "Package.h"
#pragma once
#include <windows.h>
#include <vector>
#include <map>
#include <mswsock.h>
#pragma comment(lib, "Mswsock.lib")



namespace zh {


  //用于打开文件后，自动关闭文件指针
  struct FileOpen {
    FILE* m_pFile;
    FileOpen() = default;
    FileOpen(const char* szUploadPath, const char* mode) {
      m_pFile = fopen(szUploadPath, mode);
    }
    void Close() {
      if (m_pFile) {
        fclose(m_pFile);
        m_pFile = nullptr;
      }
    }
    ~FileOpen() {
      Close();
    }
  };

  //用于打开 HDC 后自动释放资源
  struct RAIIHDC {
    HDC m_hdc;
    RAIIHDC(HDC hdc) : m_hdc(hdc) {}
    ~RAIIHDC() {
      // 析构关闭句柄
      if (m_hdc) {
        DeleteDC(m_hdc);
      }
    }
  };


  /*
    int AcceptNewClient(); //服务端/被控端使用
    void SetSendScreen(bool bIsSendScreen);
    void Send(char* buf, size_t nLen);
  */


  class ITCP {
  public:
    //服务端/被控端使用，绑定本地端口
    bool StartServer(short nPort, const char* szIp = "0.0.0.0"); 
    
    //客户端/控制端使用，连接到被控端
    bool ConnectServer(short nPort, const char* szIp = "127.0.0.1"); 
    
    //服务端/被控端使用，用于监听控制端的连接
    bool Accept(); 

    //发包函数，控制端使用
    template<typename TYPE, typename... ARGS>
    int SendPacket(size_t nLen, ARGS... args);
    
    //发包函数，被控端使用
    template<typename TYPE, typename... ARGS>
    int SendPacketFromServer(SOCKET sock, size_t nLen, ARGS... args);

    //收包函数，控制端包使用
    std::shared_ptr<Packet> RecvPacket();

    //收包函数，被控端包使用
    std::shared_ptr<Packet> RecvPacket(SOCKET sock);

    //返回客户端 socket
    SOCKET GetClientSocket() {
      return m_clientSock;
    }

    //返回监听 socket
    SOCKET GetListenSocket() {
      return m_listenSock;
    }
    //检测到控制端或被控端意外退出，则关闭连接
    void ExitConnect();
  private:
    //根据流来接收和发送数据，阻塞函数
    void Recv(char* buf, size_t nLen);
    void Recv(SOCKET sock, char* buf, size_t nLen);
   
  private:
    //服务端socket
    SOCKET m_listenSock = INVALID_SOCKET;
    
    //客户端socket
    SOCKET m_clientSock = INVALID_SOCKET;
    
    //连接目标套接字地址
    sockaddr_in m_destAddr = {}; 
    
    //目标端口
    int m_nDestPort{ -1 };
    //是否不停的发送屏幕画面
    bool m_bIsSendScreen{ false };
  };

  //控制端使用的发包函数
  /*
  TYPE：发包类型
  nLen：申请缓冲区所需大小，若无柔性数组，填0
  ARGS：类构造的所需参数，具体包类型的构造参数
  */
  template<typename TYPE, typename ...ARGS>
  inline int ITCP::SendPacket(size_t nLen, ARGS... args) {
    //申请缓冲区，nLen 是柔性数组的长度，若没有，则填 0
    std::shared_ptr<char> pBuf(new char[sizeof(TYPE) + nLen]);
    //将缓冲区转为 TYPE 类型
    new (pBuf.get()) TYPE(args...);
    //printf("nBitsCount=%d, type=%d\n", sizeof(TYPE) + nLen, pBuf->m_packetType);
    int nRet = send(m_clientSock, pBuf.get(), sizeof(TYPE) + nLen, 0);
    if (nRet == SOCKET_ERROR) {
      printf("SendPacket fail\n");
      return -1;
    }
    //返回发送字节数
    return nRet;
  }

  //被控端使用的发包函数
  template<typename TYPE, typename ...ARGS>
  inline int ITCP::SendPacketFromServer(SOCKET sock, size_t nLen, ARGS... args) {
    //申请缓冲区，nLen 是柔性数组的长度，若没有，则填 0
    std::shared_ptr<char> pBuf(new char[sizeof(TYPE) + nLen]);
    //将缓冲区转为 TYPE 类型
    auto p = new (pBuf.get()) TYPE(args...);
    //printf("nBitsCount=%d, type=%d\n", sizeof(TYPE) + nLen, p->m_packetType);
    int nRet = send(sock, pBuf.get(), sizeof(TYPE) + nLen, 0);
    if (nRet == SOCKET_ERROR) {
      printf("SendPacket fail\n");
      return -1;
    }
    //返回发送字节数
    return nRet;
  }



}
