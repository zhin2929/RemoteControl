// FileBrowseDlg.cpp: 实现文件
//

#include "pch.h"
#include "ControlClient.h"
#include "afxdialogex.h"
#include "FileBrowseDlg.h"
#include <memory>
#include <thread>


// FileBrowseDlg 对话框

IMPLEMENT_DYNAMIC(FileBrowseDlg, CDialogEx)

FileBrowseDlg::FileBrowseDlg(CWnd* pParent /*=nullptr*/)
  : CDialogEx(DLG_BROWSE, pParent)
  , m_szBrowsePath(_T("")) {

}

FileBrowseDlg::~FileBrowseDlg() {
}

void FileBrowseDlg::DoDataExchange(CDataExchange* pDX) {
  CDialogEx::DoDataExchange(pDX);
  DDX_Text(pDX, EDIT_PATH, m_szBrowsePath);
  DDX_Control(pDX, IDC_TREE1, m_dirTree);
  DDX_Control(pDX, IDC_LIST2, m_listCtrl);
  DDX_Control(pDX, PROGRESS_UPLOAD, m_proUpload);
  DDX_Control(pDX, PROGRESS_DOWN, m_proDownload);
}


BEGIN_MESSAGE_MAP(FileBrowseDlg, CDialogEx)
  ON_WM_LBUTTONDBLCLK()
  ON_NOTIFY(TVN_ITEMEXPANDED, IDC_TREE1, &FileBrowseDlg::OnItemexpandedTree)
  ON_NOTIFY(TVN_SELCHANGED, IDC_TREE1, &FileBrowseDlg::OnSelchangedTree)
  ON_NOTIFY(NM_DBLCLK, IDC_LIST2, &FileBrowseDlg::OnDblclkList)
  ON_BN_CLICKED(BTN_BROWSE, &FileBrowseDlg::OnBnClickedBrowse)
  ON_NOTIFY(LVN_GETDISPINFO, IDC_LIST2, &FileBrowseDlg::OnGetdispinfoList)
  ON_NOTIFY(NM_RCLICK, IDC_LIST2, &FileBrowseDlg::OnRclickList)
  ON_COMMAND(ID_UPLOAD_TO, &FileBrowseDlg::OnUploadTo)
  ON_COMMAND(ID_DOWNLOAD_FROM, &FileBrowseDlg::OnDownloadFrom)
  ON_COMMAND(MN_REFRESH, &FileBrowseDlg::OnRefresh)
END_MESSAGE_MAP()

//根据 Item 获取树控件路径
CString PathFromItem(CTreeCtrl const& tree, HTREEITEM hItem) {
  CString path{ tree.GetItemText(hItem) };
  while (hItem = tree.GetParentItem(hItem)) {
    if (tree.GetItemText(hItem).Compare("此电脑") == 0) {
      break;
    }
    path = tree.GetItemText(hItem) + L"\\" + path;
  }
  return path;
}
// FileBrowseDlg 消息处理程序


//对话框初始化
BOOL FileBrowseDlg::OnInitDialog() {
  CDialogEx::OnInitDialog();



#if 0 //获取盘符
  DWORD dwLen = GetLogicalDriveStrings(0, NULL);//获取系统盘符字符串长度
  //构建字符数组
  std::unique_ptr<char> pszDriver(new char[dwLen]);
  GetLogicalDriveStrings(dwLen, pszDriver.get());//获取系统盘符字符串
  char* pDriver = pszDriver.get();
  HTREEITEM root = m_dirTree.InsertItem("此电脑");
  while (*pDriver != '\0') {
    HTREEITEM parent = m_dirTree.InsertItem(pDriver, root);
    CFileFind fileFind;
    CString path = pDriver;
    path += "\\*.*";
    bool bRet = fileFind.FindFile(path.GetBuffer());
    //SHFILEINFO shfi = {};
    //SHGetFileInfo(fileFind.GetFilePath(),
    //	FILE_ATTRIBUTE_NORMAL,
    //	&shfi, sizeof(shfi), SHGFI_ICON);
    while (bRet) {

      bRet = fileFind.FindNextFile();
      CString str = fileFind.GetFileName();
      if (fileFind.IsDirectory()) {

      }
      m_dirTree.InsertItem(str, parent);
    }
    pDriver += strlen(pDriver) + 1;//定位到下一个字符串
  }
#endif // 0 //获取盘符


  int nColIdx = 0;
  m_listCtrl.InsertColumn(nColIdx++, "名称");
  m_listCtrl.InsertColumn(nColIdx++, "修改日期");
  m_listCtrl.InsertColumn(nColIdx++, "类型");
  m_listCtrl.InsertColumn(nColIdx++, "大小");

  m_listCtrl.SetExtendedStyle(m_listCtrl.GetExtendedStyle()
    | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

  for (size_t i = 0; i < nColIdx; i++) {
    //LVSCW_AUTOSIZE_USEHEADER
    m_listCtrl.SetColumnWidth(i, 120);
  }
  m_listCtrl.SetColumnWidth(0, 200);
  //模态对话框走这里发送
  //获取C盘目录
  m_szBrowsePath = "C:\\";
  int nLen = m_szBrowsePath.GetLength();
  m_fileTcp.SendPacket<C2SFilePacket>(nLen, nLen, m_szBrowsePath.GetBuffer());

  //获取盘符
  CString tmp = "获取盘符";
  nLen = tmp.GetLength();
  m_fileTcp.SendPacket<C2SFilePacket>(nLen, nLen, tmp.GetBuffer());
  m_dirTree.InsertItem("此电脑");
  hInsertItem = m_dirTree.GetRootItem(); //一开始插入在根节点之下

  UpdateData(false);
  return TRUE;  // return TRUE unless you set the focus to a control
  // 异常: OCX 属性页应返回 FALSE
}

void FileBrowseDlg::SetTcp(zh::ITCP& tcp) {
  m_fileTcp = tcp;
}


//根据字符串查找 item
HTREEITEM FindItem(const CString& name, CTreeCtrl& tree, HTREEITEM hRoot) {
  CString text = tree.GetItemText(hRoot);
  if (text.Compare(name) == 0)
    return hRoot;

  HTREEITEM hSub = tree.GetChildItem(hRoot);
  while (hSub) {
    HTREEITEM hFound = FindItem(name, tree, hSub);
    if (hFound)
      return hFound;

    hSub = tree.GetNextSiblingItem(hSub);
  }

  return NULL;
}


//获取item
HTREEITEM FileBrowseDlg::GetItem(HTREEITEM parentItem, CString name) {
  HTREEITEM findItem = m_dirTree.GetNextItem(parentItem, TVGN_CHILD);
  while (findItem) {
    auto itemName = m_dirTree.GetItemText(findItem);
    if (itemName == name) {
      return findItem;
    }
    //检索下一个同级项，找到后，作为待插入的父节点
    findItem = m_dirTree.GetNextItem(findItem, TVGN_NEXT);
  }
  return nullptr;
}



//树控件展开
void FileBrowseDlg::OnItemexpandedTree(NMHDR* pNMHDR, LRESULT* pResult) {
  LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
  if (pNMTreeView->action & TVN_ITEMEXPANDED) {
    auto name = m_dirTree.GetItemText(pNMTreeView->itemNew.hItem);
    if (name == "此电脑") return;
    CString path = PathFromItem(m_dirTree, pNMTreeView->itemNew.hItem);
    int nLen = path.GetLength();
    m_fileTcp.SendPacket<C2SFilePacket>(nLen, nLen, path.GetBuffer());
    //现在插入点击的item之下
    hInsertItem = pNMTreeView->itemNew.hItem;
  }


#if 0 //本地浏览
  CString path = PathFromItem(m_dirTree, pNMTreeView->itemNew.hItem);
  path += "\\*.*";
  CFileFind fileFind;
  bool bRet = fileFind.FindFile(path.GetBuffer());

  while (bRet) {
    bRet = fileFind.FindNextFile();
    CString fileName = fileFind.GetFileName();
    if (fileFind.IsDirectory()) {
      HTREEITEM dirItem = GetItem(pNMTreeView->itemNew.hItem, fileName);
      CFileFind fileFind2;
      CString filePath = fileFind.GetFilePath();
      bool bRet2 = fileFind2.FindFile(filePath + "\\*.*");
      while (bRet2) {
        bRet2 = fileFind2.FindNextFile();
        CString fileName2 = fileFind2.GetFileName();
        m_dirTree.InsertItem(fileName2, dirItem);
      }
    }
  }
#endif // 0 本地浏览


  * pResult = 0;
}



//本地浏览：更新列表控件
void FileBrowseDlg::UpdateListCtrl(CString path) {
  path += "\\*.*";
  CFileFind fileFind;
  bool bRet = fileFind.FindFile(path.GetBuffer());
  m_listCtrl.DeleteAllItems();
  int nRow = 0;
  const int nNameCol = 0;
  const int nDateCol = 1;
  const int nTypeCol = 2;
  const int nSizeCol = 3;
  while (bRet) {
    bRet = fileFind.FindNextFile();
    CString fileName = fileFind.GetFileName();
    CTime time;
    fileFind.GetLastWriteTime(time);
    CString szType;
    CString szSize;
    ULONGLONG fileSize = fileFind.GetLength() / 1024; // KB
    if (fileSize < 1) fileSize = 1;
    if (fileFind.IsDirectory()) {
      szType = "文件夹";
      szSize = "";
    }
    else {
      szSize.Format("%d KB", fileSize);
    }

    m_listCtrl.InsertItem(nRow, fileName);
    CString szTime = time.Format("%F %T");
    m_listCtrl.SetItemText(nRow, nDateCol, szTime);
    m_listCtrl.SetItemText(nRow, nTypeCol, szType);
    m_listCtrl.SetItemText(nRow++, nSizeCol, szSize);

  }
  UpdateData(false);
}


void FileBrowseDlg::OnSelchangedTree(NMHDR* pNMHDR, LRESULT* pResult) {
  LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
  CString path = PathFromItem(m_dirTree, pNMTreeView->itemNew.hItem);
  if (path == "此电脑") {
    return;
  }
  hInsertItem = pNMTreeView->itemNew.hItem;
  m_szBrowsePath = path;
  int nLen = m_szBrowsePath.GetLength();
  m_fileTcp.SendPacket<C2SFilePacket>(nLen, nLen, m_szBrowsePath.GetBuffer());
  //UpdateListCtrl(path);
  UpdateData(false);

  *pResult = 0;
}

//双击列表控件
void FileBrowseDlg::OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult) {

  LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
  int nSelectItem = pNMItemActivate->iItem;
  int nSelectSubItem = pNMItemActivate->iSubItem;

  CString path = m_listCtrl.GetItemText(nSelectItem, 0);
  if (path == "..") {
    int index = m_szBrowsePath.ReverseFind('\\');
    m_szBrowsePath.Delete(index, m_szBrowsePath.GetLength() - index);
  }
  else if (path != ".") {
    m_szBrowsePath = m_listFileInfo[nSelectItem].m_szPath;
  }

  int nLen = m_szBrowsePath.GetLength();
  m_fileTcp.SendPacket<C2SFilePacket>(nLen, nLen, m_szBrowsePath.GetBuffer());
  UpdateData(false);
  *pResult = 0;
  return;

#if 0 //本地浏览

  CString path = m_listCtrl.GetItemText(nSelectItem, 0);
  if (path == ".") {
    return;
  }
  CString newPath = m_szPath + "\\" + text;
  DWORD attr = GetFileAttributes(newPath.GetBuffer());
  bool isFolder = (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
  if (isFolder) {
    //UpdateListCtrl(newPath);
    if (path == "..") {
      int index = m_szPath.ReverseFind('\\');
      m_szPath.Delete(index, m_szPath.GetLength() - index);
    }
    else {
      m_szPath = newPath;
    }
  }
  UpdateData(false);
  *pResult = 0;
#endif // 0 //本地浏览

}

//点击确定按钮
void FileBrowseDlg::OnBnClickedBrowse() {
  UpdateData(true);
  if (!m_szBrowsePath.IsEmpty()) {
    int nLen = m_szBrowsePath.GetLength();
    m_fileTcp.SendPacket<C2SFilePacket>(nLen, nLen, m_szBrowsePath.GetBuffer());
    //UpdateListCtrl(m_szPath);
  }
}

//设置列表控件的虚拟数据
void FileBrowseDlg::SetData(std::vector<FileInfoStruct>& vct) {
  m_listFileInfo = vct;
  m_listCtrl.SetItemCount(m_listFileInfo.size());
  m_listCtrl.Invalidate();
  if (hInsertItem != m_dirTree.GetRootItem()) {
    HTREEITEM item = m_dirTree.GetChildItem(hInsertItem);
    //如果已经有子项就不添加
    if (item && m_dirTree.GetItemText(item) != "tmp") {
      return;
    }
    for (auto& item : m_listFileInfo) {
      m_dirTree.InsertItem(item.m_szName, hInsertItem);
    }
  }
  //刷新树控件显示+号
  m_dirTree.Invalidate(false);

}

//设置树控件盘符
void FileBrowseDlg::SetTreeData(CString path) {
  auto parent = m_dirTree.InsertItem(path, hInsertItem);
  m_dirTree.InsertItem("tmp", parent);

}


//列表控件的虚拟列表设置
void FileBrowseDlg::OnGetdispinfoList(NMHDR* pNMHDR, LRESULT* pResult) {
  NMLVDISPINFO* pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
  LV_ITEM* pItem = &(pDispInfo)->item;

  int iItemIndx = pItem->iItem;
  if (iItemIndx >= m_listFileInfo.size()) {
    return;
  }
  if (pItem->mask & LVIF_TEXT) //字符串缓冲区有效
  {
    switch (pItem->iSubItem) { //列
      case 0: //填充数据项的名字
        lstrcpy(pItem->pszText, m_listFileInfo[iItemIndx].m_szName);
        break;
      case 1: //填充子项1
        lstrcpy(pItem->pszText, m_listFileInfo[iItemIndx].m_szDate);
        break;
      case 2: //填充子项2
        lstrcpy(pItem->pszText, m_listFileInfo[iItemIndx].m_szType);
        break;
      case 3: //填充子项3
        lstrcpy(pItem->pszText, m_listFileInfo[iItemIndx].m_szSize);
        break;
    }
  }

  *pResult = 0;
}


//列表控件右键点击
void FileBrowseDlg::OnRclickList(NMHDR* pNMHDR, LRESULT* pResult) {
  LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
  CMenu menu;
  menu.LoadMenu(MN_RCLICK);
  CPoint point;
  ::GetCursorPos(&point);
  ScreenToClient(&point);
  ClientToScreen(&point);
  menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN, point.x, point.y, this);
  m_nSelectItem = pNMItemActivate->iItem;
  *pResult = 0;
}

//右键上传到此处
void FileBrowseDlg::OnUploadTo() {
  UpdateData(true);
  if (m_nSelectItem < 0) { //如果选中的不是有效行，就使用当前目录
    AfxMessageBox("请选择一个目录");
    return;
  }
  bool bIsDir = m_listFileInfo[m_nSelectItem].m_bIsDir;
  if (!bIsDir) {
    AfxMessageBox("选择的不是文件夹");
    return;
  }
  CFileDialog dlg(true);
  if (dlg.DoModal()) {
    //获取上传文件的路径
    m_szUploadPath = dlg.GetPathName();
    m_szUploadName = dlg.GetFileName();

    if (m_szUploadPath.IsEmpty()) {
      return;
    }
    std::thread([&]()
      {
        //上传的目标文件
        CString destPath = m_listFileInfo[m_nSelectItem].m_szPath;
        destPath += "\\";
        destPath += m_szUploadName;
        
        FILE* pFile = fopen(m_szUploadPath.GetBuffer(), "rb+");
        fpos_t nFileSize; //获取文件大小
        fseek(pFile, 0, SEEK_END);
        fgetpos(pFile, &nFileSize);
        fseek(pFile, 0, SEEK_SET);
        int nTotalSend = 0;
        m_proUpload.SetRange32(0, nFileSize / FILE_BUF_SIZE);
        char buf[FILE_BUF_SIZE] = {};
        int nCnt = (nFileSize % FILE_BUF_SIZE == 0) ? nFileSize / FILE_BUF_SIZE : nFileSize / FILE_BUF_SIZE + 1;
        for (int i = 0; i < nCnt; ++i) {
          int nSendSize = (i == nCnt - 1) ? nFileSize - i * FILE_BUF_SIZE : FILE_BUF_SIZE;
          fread(buf, nSendSize, 1, pFile);
          m_fileTcp.SendPacket<C2SUploadPacket>(nSendSize, 
            nFileSize, nTotalSend, destPath.GetLength(), destPath.GetBuffer(),
             nSendSize, buf);
          nTotalSend += nSendSize;
          int pos = m_proUpload.GetPos();
          m_proUpload.SetPos(pos+1);
        }
        
        m_fileTcp.SendPacket<C2SUploadPacket>(0);
        fclose(pFile);
      }
    ).detach();
  }
}

//右键下载此文件
void FileBrowseDlg::OnDownloadFrom() {
  bool bIsDir = m_listFileInfo[m_nSelectItem].m_bIsDir;
  if (bIsDir) {
    AfxMessageBox("选择的不是文件");
    return;
  }
  
  CFileDialog dlg(true);
  if (dlg.DoModal()) {
    //保存文件的路径和名称
    m_szDownPath = dlg.GetPathName();
    m_szDownName = dlg.GetFileName();
    if (m_szDownPath.IsEmpty()) {
      AfxMessageBox("未选择保存路径");
      return;
    }
    //发送下载文件命令
    CString szDownloadFile = m_listFileInfo[m_nSelectItem].m_szPath;
    int nLen = szDownloadFile.GetLength();
    m_fileTcp.SendPacket<C2SDownloadPacket>(nLen, nLen, 
      szDownloadFile.GetBuffer());
    ULONGLONG nFileSize = m_listFileInfo[m_nSelectItem].m_nSize;
    m_proDownload.SetRange32(0, nFileSize / FILE_BUF_SIZE);

  }
}

//右键刷新
void FileBrowseDlg::OnRefresh() {
  UpdateData(true);
  if (!m_szBrowsePath.IsEmpty()) {
    int nLen = m_szBrowsePath.GetLength();
    m_fileTcp.SendPacket<C2SFilePacket>(nLen, nLen, m_szBrowsePath.GetBuffer());
  }
}
