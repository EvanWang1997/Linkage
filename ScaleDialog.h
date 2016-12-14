#pragma once
#include "MyDialog.h"

// CScaleDialog dialog

class CScaleDialog : public CMyDialog
{
	DECLARE_DYNAMIC(CScaleDialog)

public:
	CScaleDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CScaleDialog();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SCALE };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString m_Scale;
};
