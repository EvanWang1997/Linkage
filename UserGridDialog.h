#pragma once
#include "afxwin.h"
#include "MyDialog.h"

// CUserGridDialog dialog

class CUserGridDialog : public CMyDialog
{
	DECLARE_DYNAMIC(CUserGridDialog)

public:
	CUserGridDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CUserGridDialog();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_USERGRID };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	double m_HorizontalSpacing;
	double m_VerticalSpacing;
	BOOL m_bShowUserGrid;
	int m_GridTypeSelection;
	afx_msg void OnTypeClick();
	CStatic m_HorizontalPrompt;
	CStatic m_VerticalPrompt;
	CEdit m_HorizontalControl;
	CEdit m_VerticalControl;
};
