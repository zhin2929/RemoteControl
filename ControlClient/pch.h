// pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。

#ifndef PCH_H
#define PCH_H

// 添加要在此处预编译的标头
#include "framework.h"
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
#endif //PCH_H
