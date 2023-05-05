#pragma once
#include "afxdialogex.h"
#include <vector>
#include "ITCP.h"
// FileBrowseDlg 对话框

constexpr int FILE_BUF_SIZE = 0x10000; //上传下载一次读取的缓冲区数量
class FileBrowseDlg : public CDialogEx
{
	DECLARE_DYNAMIC(FileBrowseDlg)

public:
	FileBrowseDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~FileBrowseDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = DLG_BROWSE };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CString m_szBrowsePath;
	CString m_szDownPath;
	CString m_szDownName;
	CString m_szUploadPath;
	CString m_szUploadName;
	zh::ITCP m_fileTcp;
	HTREEITEM hInsertItem; //树控件插入该Item之下，一开始是root
	CListCtrl m_listCtrl;
	CTreeCtrl m_dirTree;
	using FileInfoList = std::vector<FileInfoStruct>;
	FileInfoList m_listFileInfo;
	int m_nSelectItem;
public:
	void SetTcp(zh::ITCP& tcp);
	HTREEITEM GetItem(HTREEITEM parentItem, CString name);
	void UpdateListCtrl(CString path);
	void SetData(std::vector<FileInfoStruct>& vct);
	void SetTreeData(CString path);

	virtual BOOL OnInitDialog();
	afx_msg void OnDblclkTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemexpandedTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangedTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedBrowse();
	afx_msg void OnGetdispinfoList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRclickList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnUploadTo();
  afx_msg void OnDownloadFrom();
	CProgressCtrl m_proUpload;
	CProgressCtrl m_proDownload;
	afx_msg void OnRefresh();
};
