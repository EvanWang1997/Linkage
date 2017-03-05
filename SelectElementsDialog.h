#pragma once
#include "MyDialog.h"
#include "MyCheckListBox.h"
#include "Element.h"
#include "afxwin.h"
#include "afxcmn.h"

// CSelectElementsDialog dialog

class CSelectElementsDialog : public CMyDialog
{
	DECLARE_DYNAMIC(CSelectElementsDialog)

public:
	CSelectElementsDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSelectElementsDialog();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SELECTELEMENTS };
#endif

public:
	ElementList m_AllElements;
	bool m_bShowDebug;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CListCtrl m_ListControl2;
	void OnOK( void );

};
