#include "ITCP.h"

//服务端/被控端启动服务，等待控制端连接
bool zh::ITCP::StartServer(short nPort, const char* szIp) {
  m_listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  //设置成非阻塞模式
  //u_long ul = true;
  //int ret = ioctlsocket(m_listenSock, FIONBIO, &ul);


  if (m_listenSock == INVALID_SOCKET) { return false; };
  //绑定本地套接字地址
  sockaddr_in localAddr = {};
  localAddr.sin_family = AF_INET;
  localAddr.sin_port = htons(nPort);
  inet_pton(AF_INET, szIp, &localAddr.sin_addr);
  int nResult = bind(m_listenSock, (sockaddr*)&localAddr, sizeof(localAddr));
  if (nResult == SOCKET_ERROR) {
    printf("bind nResult = %d\n", nResult);
    return false;
  }
  //转换为服务器监听socket，等待客户端连接
  nResult = listen(m_listenSock, SOMAXCONN);
  if (nResult == SOCKET_ERROR) {
    printf("listen nResult = %d\n", nResult);
    return false;
  }
  return true;
}

//客户端/控制端连接被控端
bool zh::ITCP::ConnectServer(short nPort, const char* szIp) {
  
  m_nDestPort = nPort;
  m_clientSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  //设置成非阻塞模式
  //int flag = 1;
  //int result = setsockopt(m_clientSock,
  //  IPPROTO_TCP,
  //  TCP_NODELAY,
  //  (char*)&flag,
  //  sizeof(int));

  if (m_clientSock == INVALID_SOCKET) { return false; };
  m_destAddr.sin_family = AF_INET;
  m_destAddr.sin_port = htons(nPort);
  inet_pton(AF_INET, szIp, &m_destAddr.sin_addr.s_addr);
  //连接
  int nRet = connect(m_clientSock,
    (sockaddr*)&m_destAddr, sizeof(m_destAddr));
  if (nRet == SOCKET_ERROR) {
    return false;
  }
  return true;
}

//被控端等待连接
bool zh::ITCP::Accept() {
  sockaddr_in clientAddr = {};
  int nAddrLen = sizeof(clientAddr);
  m_clientSock = accept(m_listenSock,
    (sockaddr*)&clientAddr, &nAddrLen);
  printf("sock= %d, %lld\n", (m_clientSock != INVALID_SOCKET), m_clientSock);
  int flag = 1;
  //int result = setsockopt(m_clientSock,
  //  IPPROTO_TCP,
  //  TCP_NODELAY,
  //  (char*)&flag,
  //  sizeof(int));
  return (m_clientSock != INVALID_SOCKET);
}



#if 0
int zh::ITCP::AcceptNewClient() {
  sockaddr_in clientAddr = {};
  int nAddrLen = sizeof(clientAddr);
  SOCKET clientSock = accept(m_listenSock,
    (sockaddr*)&clientAddr, &nAddrLen);
  printf("sock= %d, %d\n", (m_nClientId), clientSock);
  mapClients[m_nClientId] = clientSock;
  return m_nClientId++;
}
#endif // 0


//void zh::ITCP::SetSendScreen(bool bIsSendScreen) {
//  m_bIsSendScreen = bIsSendScreen;
//}

void zh::ITCP::ExitConnect() {
  closesocket(m_listenSock);
  closesocket(m_clientSock);
  m_clientSock = INVALID_SOCKET;
  m_listenSock = INVALID_SOCKET;

}

void zh::ITCP::Recv(char* buf, size_t nLen) {
  size_t nRecvLen = 0;
  while (nRecvLen < nLen) {
    int nRet = recv(m_clientSock, buf, nLen - nRecvLen, MSG_WAITALL);
    if (nRet > 0) {
      nRecvLen += nRet;
    }
    else if (nRet == SOCKET_ERROR) {
      // 出错，断开连接
      closesocket(m_clientSock);
      break;
    }
  }
}

void zh::ITCP::Recv(SOCKET sock, char* buf, size_t nLen) {
  size_t nRecvLen = 0;
  while (nRecvLen < nLen) {
    int nRet = recv(sock, buf, nLen - nRecvLen, MSG_WAITALL);
    if (nRet > 0) {
      nRecvLen += nRet;
    }
    else if (nRet == SOCKET_ERROR) {
      // 出错，断开连接
      closesocket(m_clientSock);
      break;
    }
  }
}

std::shared_ptr<Packet> zh::ITCP::RecvPacket() {
  //先收包头，包头里面有包的长度
  Packet hdr{};
  size_t nHeaderLen = sizeof(hdr);
  Recv((char*)&hdr, nHeaderLen);
  //再收数据
  char* pPacket = new char[hdr.m_nLen + nHeaderLen];

  memcpy(pPacket, &hdr, nHeaderLen); //先复制包头
  //如果数据长度大于0，再接收数据
  if (hdr.m_nLen > 0) {
    Recv(pPacket + nHeaderLen, hdr.m_nLen);
  }

  return std::shared_ptr<Packet>((Packet*)pPacket);
}

std::shared_ptr<Packet> zh::ITCP::RecvPacket(SOCKET sock) {

  //先收包头，包头里面有包的长度
  Packet hdr{};
  size_t nHeaderLen = sizeof(hdr);
  Recv(sock, (char*)&hdr, nHeaderLen);
  //再收数据
  char* pPacket = new char[hdr.m_nLen + nHeaderLen];

  memcpy(pPacket, &hdr, nHeaderLen); //先复制包头
  //如果数据长度大于0，再接收数据
  if (hdr.m_nLen > 0) {
    Recv(sock, pPacket + nHeaderLen, hdr.m_nLen);
  }

  return std::shared_ptr<Packet>((Packet*)pPacket);
}
