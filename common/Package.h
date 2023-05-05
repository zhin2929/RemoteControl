#pragma once
#pragma warning(disable : 4200)
#pragma pack(push)
#pragma pack(1)
enum class PACKET_TYPE : UINT8 {
  NO_CONNECT,  //未知包类型
  C2S_SCREEN, //控制端向被控端发送：要求接收屏幕数据
  S2C_SCREEN, //被控端向控制端发送：屏幕数据
  C2S_MOUSE,  //控制端向被控端发送：要求接收鼠标事件（位置，点击）
  S2C_MOUSE,  //被控端向控制端发送：鼠标位置，点击事件等
  C2S_KEYBOARD,  //控制端向被控端发送：要求接收键盘事件（按键）
  S2C_KEYBOARD,  //被控端向控制端发送：键盘事件（按键）
  C2S_CMD,  //控制端向被控端发送：CMD命令
  S2C_CMD,  //被控端向控制端发送：返回CMD执行结果（回显结果）

  C2S_FILE, //控制端发送要浏览的路径
  S2C_FILE, //被控端返回的目录数据

  C2S_UPLOAD, //控制端发送上传命令

  C2S_DOWNLOAD, //控制端发送下载下载
  S2C_DOWNLOAD, //被控端响应下载命令
};

struct PackCheckHash {
  size_t operator()(char* buf, int nSize) {
    size_t h1 = 0;
    for (int i = 0; i < nSize; ++i) {
      h1 = h1 ^ ((std::hash<char>{}(buf[i])) << 1);
    }
    return h1;
  }
};

struct Packet {
  Packet() : m_nLen(0),
    m_packetType(PACKET_TYPE::NO_CONNECT) {
  };
  Packet(size_t nLen, PACKET_TYPE packetType)
    : m_nLen(nLen), m_packetType(packetType) {
  }

  size_t m_nLen;
  PACKET_TYPE m_packetType;
};

//控制端发送的CMD包
struct C2SCmdPacket : public Packet {
  C2SCmdPacket() : m_nDataLen(0) {}
  C2SCmdPacket(size_t nDataLen, char* pBuf)
    : m_nDataLen(nDataLen) {
    //整个包的长度
    m_nLen = sizeof(m_nDataLen) + m_nDataLen;
    m_packetType = PACKET_TYPE::C2S_CMD;
    memcpy(m_nDataBuf, pBuf, m_nDataLen);
  }
  size_t m_nDataLen; //CMD命令字符串的长度
  char m_nDataBuf[0]; //CMD命令字符串
};

//被控端发送的执行CMD回显包
struct S2CCmdPacket : public Packet {
  S2CCmdPacket() : m_nDataLen(0) {}
  S2CCmdPacket(size_t nDataLen, char* pBuf)
    : m_nDataLen(nDataLen) {
    //整个包的长度
    m_nLen = sizeof(m_nDataLen) + m_nDataLen;
    m_packetType = PACKET_TYPE::S2C_CMD;
    memcpy(m_nDataBuf, pBuf, m_nDataLen);
  }
  size_t m_nDataLen; //CMD命令执行回显的字符串长度
  char m_nDataBuf[0]; //CMD回显字符串
};

//被控端发送的屏幕包
struct S2CScreenPacket : public Packet {
  S2CScreenPacket(size_t nDataLen, size_t nWidth,
    size_t nHight, char* pBuf)
    : m_nWidth(nWidth),
    m_nHight(nHight),
    m_nDataLen(nDataLen) {
    m_packetType = PACKET_TYPE::S2C_SCREEN;
    m_nLen = sizeof(m_nWidth) + sizeof(m_nHight)
      + sizeof(m_nDataLen) + m_nDataLen;
    memcpy(m_nDataBuf, pBuf, m_nDataLen);
  }

  size_t m_nWidth; //屏幕宽
  size_t m_nHight; //屏幕高
  size_t m_nDataLen; //屏幕数据的长度
  char m_nDataBuf[0]; //屏幕数据
};

struct C2SScreenPacket : public Packet {
  C2SScreenPacket() : m_nDataLen(0), m_bIsRecvScreen(false) {};
  C2SScreenPacket(bool bIsRecvScreen, size_t nDelay)
    : m_nDataLen(0), m_nDelay(nDelay),
    m_bIsRecvScreen(bIsRecvScreen) {
    m_packetType = PACKET_TYPE::C2S_SCREEN;
    m_nLen = sizeof(m_nDataLen) + sizeof(m_nDelay)
      + sizeof(m_bIsRecvScreen) + m_nDataLen; //加上m_nDataLen是为了统一
  }

  size_t m_nDataLen; //查看屏幕消息的长度
  size_t m_nDelay; //多长时间发送屏幕消息
  bool m_bIsRecvScreen; //是否接收屏幕
};

//控制端发送的鼠标包
struct C2SMousePacket : public Packet {
  C2SMousePacket(MSG msg) :
    m_nDataLen(sizeof(MSG)), m_msg(msg) {
    m_nLen = sizeof(m_nDataLen) + m_nDataLen;
    m_packetType = PACKET_TYPE::C2S_MOUSE;
  }
  size_t m_nDataLen; //鼠标消息的长度
  MSG m_msg;
};

struct C2SKeyBoardPacket : public Packet {
  C2SKeyBoardPacket() = default;
  C2SKeyBoardPacket(MSG msg) :
    m_nDataLen(sizeof(MSG)), m_msg(msg) {
    m_nLen = sizeof(m_nDataLen) + m_nDataLen;
    m_packetType = PACKET_TYPE::C2S_KEYBOARD;
  }

  size_t m_nDataLen; //键盘消息的长度
  MSG m_msg;
};

//文件信息结构
struct FileInfoStruct {
  char m_szName[128]; //名称
  char m_szDate[32]; //日期
  char m_szType[16]; //文件类型
  char m_szSize[32]; //文件大小字符串形式，KB单位
  ULONGLONG m_nSize; //文件大小，字节单位
  char m_szPath[MAX_PATH]; //路径
  bool m_bIsDir; //是否为目录
};

//控制端文件信息包
struct C2SFilePacket : public Packet {
  C2SFilePacket() {}
  C2SFilePacket(size_t nDataLen, char* pBuf)
    : m_nDataLen(nDataLen) {
    //整个包的长度
    m_nLen = sizeof(m_nDataLen) + m_nDataLen;
    m_packetType = PACKET_TYPE::C2S_FILE;
    memcpy(m_nDataBuf, pBuf, m_nDataLen);
  }
  size_t m_nDataLen; //路径字符串的长度
  char m_nDataBuf[0]; //路径字符串数据
};

//被控端发送的文件信息包
struct S2CFilePacket : public Packet {
  S2CFilePacket() {}
  S2CFilePacket(size_t nDataLen, size_t nDataSize, char* pBuf)
    : m_nDataLen(nDataLen), m_nDataSize(nDataSize) {
    //整个包的长度
    m_nLen = sizeof(m_nDataLen) + sizeof(m_nDataSize) + m_nDataLen;
    m_packetType = PACKET_TYPE::S2C_FILE;
    memcpy(m_nDataBuf, pBuf, m_nDataLen);
  }
  size_t m_nDataLen; //路径字符串的长度
  size_t m_nDataSize; //FileInfoStruct 的个数
  char m_nDataBuf[0]; //路径字符串数据
};

//控制端发送的文件上传包
struct C2SUploadPacket : public Packet {
  C2SUploadPacket() {
    m_nTotalLen = 0;
    m_nTotalSendLen = 0;
    m_nDataLen = 0;
    m_nPathLen = 0;
    m_nLen = sizeof(m_nTotalLen) + sizeof(m_nPathLen)
      + sizeof(m_nTotalSendLen) + sizeof(m_nDataLen)
      + m_nDataLen + MAX_PATH;
    m_packetType = PACKET_TYPE::C2S_UPLOAD;
  }
  C2SUploadPacket(size_t nTotalLen, size_t nSendLen,
    size_t nPathLen, char* szPath,
    size_t nDataLen, char* pBuf)
    : m_nTotalLen(nTotalLen),
    m_nTotalSendLen(nSendLen),
    m_nPathLen(nPathLen),
    m_nDataLen(nDataLen)
  {
    //复制数据
    m_nLen = sizeof(m_nTotalLen) + sizeof(m_nPathLen)
      + sizeof(m_nTotalSendLen) + sizeof(m_nDataLen)
      + m_nDataLen + MAX_PATH;
    m_packetType = PACKET_TYPE::C2S_UPLOAD;
    memcpy(m_pDataBuf, pBuf, m_nDataLen);
    memcpy(m_szSavePath, szPath, m_nPathLen);
  }
  size_t m_nTotalLen; //文件总大小
  size_t m_nTotalSendLen; //已发送总大小
  size_t m_nDataLen; //此次发送的数据长度
  size_t m_nPathLen; //文件路径字符串长度
  char m_szSavePath[MAX_PATH]; //保存的文件路径
  char m_pDataBuf[0]; //文件数据
};

//控制端发送文件下载包
struct C2SDownloadPacket : public Packet {
  C2SDownloadPacket() = default;
  C2SDownloadPacket(size_t nDataLen, char* pBuf)
    : m_nDataLen(nDataLen) {
    m_nLen = sizeof(m_nDataLen) + m_nDataLen;
    m_packetType = PACKET_TYPE::C2S_DOWNLOAD;
    memcpy(m_nDataBuf, pBuf, m_nDataLen);
  }

  size_t m_nDataLen; //路径字符串的长度
  char m_nDataBuf[0]; //路径字符串数据
};



//被控端发送文件下载包
struct S2CDownloadPacket : public Packet {
  S2CDownloadPacket() {
    m_nTotalLen = 0;
    m_nTotalSendLen = 0;
    m_nDataLen = 0;
    m_nLen = sizeof(m_nTotalLen) + sizeof(m_nTotalSendLen)
      + sizeof(m_nDataLen) + m_nDataLen;
    m_packetType = PACKET_TYPE::S2C_DOWNLOAD;
  }

  S2CDownloadPacket(size_t nTotalLen, size_t nSendLen,
    size_t nDataLen, char* pBuf)
    : m_nTotalLen(nTotalLen),
    m_nTotalSendLen(nSendLen),
    m_nDataLen(nDataLen)
  {
    m_nLen = sizeof(nTotalLen) + sizeof(m_nTotalSendLen)
      + sizeof(m_nDataLen) + m_nDataLen;
    m_packetType = PACKET_TYPE::S2C_DOWNLOAD;
    //复制数据
    memcpy(m_pDataBuf, pBuf, m_nDataLen);
  }
  size_t m_nTotalLen; //文件总大小
  size_t m_nTotalSendLen; //已发送总大小
  size_t m_nDataLen; //此次发送的数据长度
  char m_pDataBuf[0]; //文件数据
};

#pragma pack(pop)
