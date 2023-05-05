#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN


#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

class CSocketInit {
  CSocketInit(const CSocketInit&) = delete;
  CSocketInit& operator=(const CSocketInit&) = delete;

  static CSocketInit& GetInstance() {
    return m_instance;
  }

  CSocketInit() {
    WSADATA wsaData;
    int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (nRet != 0) {
      std::cout << ("WSAStartup failed\n");
    }
  }

  ~CSocketInit() {
    WSACleanup();
  }
private:
  static CSocketInit m_instance;
};
