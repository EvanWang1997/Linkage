// ScaleDialog.cpp : implementation file
//

#include "stdafx.h"
#include "linkage.h"
#include "ScaleDialog.h"
#include "afxdialogex.h"


// CScaleDialog dialog

IMPLEMENT_DYNAMIC(CScaleDialog, CMyDialog)

CScaleDialog::CScaleDialog(CWnd* pParent /*=NULL*/)
	: CMyDialog( pParent, IDD_SCALE )
	, m_Scale(_T(""))
{

}

CScaleDialog::~CScaleDialog()
{
}

void CScaleDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_Scale);
}


BEGIN_MESSAGE_MAP(CScaleDialog, CMyDialog)
END_MESSAGE_MAP()


// CScaleDialog message handlers
