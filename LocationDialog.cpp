// LocationDialog.cpp : implementation file
//

#include "stdafx.h"
#include "linkage.h"
#include "LocationDialog.h"
#include "afxdialogex.h"


// CLocationDialog dialog

IMPLEMENT_DYNAMIC(CLocationDialog, CMyDialog)

CLocationDialog::CLocationDialog(CWnd* pParent /*=NULL*/)
	: CMyDialog( pParent, IDD_POSITION)
	, m_xPosition(0)
	, m_yPosition(0)
{

}

CLocationDialog::~CLocationDialog()
{
}

void CLocationDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_MyDoubleText(pDX, IDC_EDIT2, m_xPosition, 4);
	DDX_MyDoubleText(pDX, IDC_EDIT4, m_yPosition, 4);
}


BEGIN_MESSAGE_MAP(CLocationDialog, CMyDialog)
END_MESSAGE_MAP()


// CLocationDialog message handlers
