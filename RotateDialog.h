#pragma once

#include "MyDialog.h"


// CRotateDialog dialog

class CRotateDialog : public CMyDialog
{
	DECLARE_DYNAMIC(CRotateDialog)

public:
	CRotateDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CRotateDialog();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ROTATE };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString m_Angle;
};
