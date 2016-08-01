#pragma once
#include "MyDialog.h"

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

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
