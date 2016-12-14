// RotateDialog.cpp : implementation file
//

#include "stdafx.h"
#include "linkage.h"
#include "RotateDialog.h"
#include "afxdialogex.h"


// CRotateDialog dialog

IMPLEMENT_DYNAMIC(CRotateDialog, CMyDialog)

CRotateDialog::CRotateDialog(CWnd* pParent /*=NULL*/)
	: CMyDialog( pParent, IDD_ROTATE)
	, m_Angle(_T(""))
{

}

CRotateDialog::~CRotateDialog()
{
}

void CRotateDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_Angle);
}


BEGIN_MESSAGE_MAP(CRotateDialog, CMyDialog)
END_MESSAGE_MAP()


// CRotateDialog message handlers
