#pragma once
#include "MyDialog.h"


// CLengthDistanceDialog dialog

class CLengthDistanceDialog : public CMyDialog
{
	DECLARE_DYNAMIC(CLengthDistanceDialog)

public:
	CLengthDistanceDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CLengthDistanceDialog();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_LENGTH };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString m_Distance;
};
