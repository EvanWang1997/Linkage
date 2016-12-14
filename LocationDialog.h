#pragma once
#include "afxwin.h"
#include "MyDialog.h"


// CLocationDialog dialog

class CLocationDialog : public CMyDialog
{
	DECLARE_DYNAMIC(CLocationDialog)

public:
	CLocationDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CLocationDialog();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_POSITION };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	double m_xPosition;
	double m_yPosition;
	CString m_PromptText;
};
