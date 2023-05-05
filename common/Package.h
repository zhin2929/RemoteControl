#pragma once
#pragma warning(disable : 4200)
#pragma pack(push)
#pragma pack(1)
enum class PACKET_TYPE : UINT8 {
  NO_CONNECT,  //δ֪������
  C2S_SCREEN, //���ƶ��򱻿ض˷��ͣ�Ҫ�������Ļ����
  S2C_SCREEN, //���ض�����ƶ˷��ͣ���Ļ����
  C2S_MOUSE,  //���ƶ��򱻿ض˷��ͣ�Ҫ���������¼���λ�ã������
  S2C_MOUSE,  //���ض�����ƶ˷��ͣ����λ�ã�����¼���
  C2S_KEYBOARD,  //���ƶ��򱻿ض˷��ͣ�Ҫ����ռ����¼���������
  S2C_KEYBOARD,  //���ض�����ƶ˷��ͣ������¼���������
  C2S_CMD,  //���ƶ��򱻿ض˷��ͣ�CMD����
  S2C_CMD,  //���ض�����ƶ˷��ͣ�����CMDִ�н�������Խ����

  C2S_FILE, //���ƶ˷���Ҫ�����·��
  S2C_FILE, //���ض˷��ص�Ŀ¼����

  C2S_UPLOAD, //���ƶ˷����ϴ�����

  C2S_DOWNLOAD, //���ƶ˷�����������
  S2C_DOWNLOAD, //���ض���Ӧ��������
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

//���ƶ˷��͵�CMD��
struct C2SCmdPacket : public Packet {
  C2SCmdPacket() : m_nDataLen(0) {}
  C2SCmdPacket(size_t nDataLen, char* pBuf)
    : m_nDataLen(nDataLen) {
    //�������ĳ���
    m_nLen = sizeof(m_nDataLen) + m_nDataLen;
    m_packetType = PACKET_TYPE::C2S_CMD;
    memcpy(m_nDataBuf, pBuf, m_nDataLen);
  }
  size_t m_nDataLen; //CMD�����ַ����ĳ���
  char m_nDataBuf[0]; //CMD�����ַ���
};

//���ض˷��͵�ִ��CMD���԰�
struct S2CCmdPacket : public Packet {
  S2CCmdPacket() : m_nDataLen(0) {}
  S2CCmdPacket(size_t nDataLen, char* pBuf)
    : m_nDataLen(nDataLen) {
    //�������ĳ���
    m_nLen = sizeof(m_nDataLen) + m_nDataLen;
    m_packetType = PACKET_TYPE::S2C_CMD;
    memcpy(m_nDataBuf, pBuf, m_nDataLen);
  }
  size_t m_nDataLen; //CMD����ִ�л��Ե��ַ�������
  char m_nDataBuf[0]; //CMD�����ַ���
};

//���ض˷��͵���Ļ��
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

  size_t m_nWidth; //��Ļ��
  size_t m_nHight; //��Ļ��
  size_t m_nDataLen; //��Ļ���ݵĳ���
  char m_nDataBuf[0]; //��Ļ����
};

struct C2SScreenPacket : public Packet {
  C2SScreenPacket() : m_nDataLen(0), m_bIsRecvScreen(false) {};
  C2SScreenPacket(bool bIsRecvScreen, size_t nDelay)
    : m_nDataLen(0), m_nDelay(nDelay),
    m_bIsRecvScreen(bIsRecvScreen) {
    m_packetType = PACKET_TYPE::C2S_SCREEN;
    m_nLen = sizeof(m_nDataLen) + sizeof(m_nDelay)
      + sizeof(m_bIsRecvScreen) + m_nDataLen; //����m_nDataLen��Ϊ��ͳһ
  }

  size_t m_nDataLen; //�鿴��Ļ��Ϣ�ĳ���
  size_t m_nDelay; //�೤ʱ�䷢����Ļ��Ϣ
  bool m_bIsRecvScreen; //�Ƿ������Ļ
};

//���ƶ˷��͵�����
struct C2SMousePacket : public Packet {
  C2SMousePacket(MSG msg) :
    m_nDataLen(sizeof(MSG)), m_msg(msg) {
    m_nLen = sizeof(m_nDataLen) + m_nDataLen;
    m_packetType = PACKET_TYPE::C2S_MOUSE;
  }
  size_t m_nDataLen; //�����Ϣ�ĳ���
  MSG m_msg;
};

struct C2SKeyBoardPacket : public Packet {
  C2SKeyBoardPacket() = default;
  C2SKeyBoardPacket(MSG msg) :
    m_nDataLen(sizeof(MSG)), m_msg(msg) {
    m_nLen = sizeof(m_nDataLen) + m_nDataLen;
    m_packetType = PACKET_TYPE::C2S_KEYBOARD;
  }

  size_t m_nDataLen; //������Ϣ�ĳ���
  MSG m_msg;
};

//�ļ���Ϣ�ṹ
struct FileInfoStruct {
  char m_szName[128]; //����
  char m_szDate[32]; //����
  char m_szType[16]; //�ļ�����
  char m_szSize[32]; //�ļ���С�ַ�����ʽ��KB��λ
  ULONGLONG m_nSize; //�ļ���С���ֽڵ�λ
  char m_szPath[MAX_PATH]; //·��
  bool m_bIsDir; //�Ƿ�ΪĿ¼
};

//���ƶ��ļ���Ϣ��
struct C2SFilePacket : public Packet {
  C2SFilePacket() {}
  C2SFilePacket(size_t nDataLen, char* pBuf)
    : m_nDataLen(nDataLen) {
    //�������ĳ���
    m_nLen = sizeof(m_nDataLen) + m_nDataLen;
    m_packetType = PACKET_TYPE::C2S_FILE;
    memcpy(m_nDataBuf, pBuf, m_nDataLen);
  }
  size_t m_nDataLen; //·���ַ����ĳ���
  char m_nDataBuf[0]; //·���ַ�������
};

//���ض˷��͵��ļ���Ϣ��
struct S2CFilePacket : public Packet {
  S2CFilePacket() {}
  S2CFilePacket(size_t nDataLen, size_t nDataSize, char* pBuf)
    : m_nDataLen(nDataLen), m_nDataSize(nDataSize) {
    //�������ĳ���
    m_nLen = sizeof(m_nDataLen) + sizeof(m_nDataSize) + m_nDataLen;
    m_packetType = PACKET_TYPE::S2C_FILE;
    memcpy(m_nDataBuf, pBuf, m_nDataLen);
  }
  size_t m_nDataLen; //·���ַ����ĳ���
  size_t m_nDataSize; //FileInfoStruct �ĸ���
  char m_nDataBuf[0]; //·���ַ�������
};

//���ƶ˷��͵��ļ��ϴ���
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
    //��������
    m_nLen = sizeof(m_nTotalLen) + sizeof(m_nPathLen)
      + sizeof(m_nTotalSendLen) + sizeof(m_nDataLen)
      + m_nDataLen + MAX_PATH;
    m_packetType = PACKET_TYPE::C2S_UPLOAD;
    memcpy(m_pDataBuf, pBuf, m_nDataLen);
    memcpy(m_szSavePath, szPath, m_nPathLen);
  }
  size_t m_nTotalLen; //�ļ��ܴ�С
  size_t m_nTotalSendLen; //�ѷ����ܴ�С
  size_t m_nDataLen; //�˴η��͵����ݳ���
  size_t m_nPathLen; //�ļ�·���ַ�������
  char m_szSavePath[MAX_PATH]; //������ļ�·��
  char m_pDataBuf[0]; //�ļ�����
};

//���ƶ˷����ļ����ذ�
struct C2SDownloadPacket : public Packet {
  C2SDownloadPacket() = default;
  C2SDownloadPacket(size_t nDataLen, char* pBuf)
    : m_nDataLen(nDataLen) {
    m_nLen = sizeof(m_nDataLen) + m_nDataLen;
    m_packetType = PACKET_TYPE::C2S_DOWNLOAD;
    memcpy(m_nDataBuf, pBuf, m_nDataLen);
  }

  size_t m_nDataLen; //·���ַ����ĳ���
  char m_nDataBuf[0]; //·���ַ�������
};



//���ض˷����ļ����ذ�
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
    //��������
    memcpy(m_pDataBuf, pBuf, m_nDataLen);
  }
  size_t m_nTotalLen; //�ļ��ܴ�С
  size_t m_nTotalSendLen; //�ѷ����ܴ�С
  size_t m_nDataLen; //�˴η��͵����ݳ���
  char m_pDataBuf[0]; //�ļ�����
};

#pragma pack(pop)
