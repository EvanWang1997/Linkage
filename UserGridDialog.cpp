// UserGridDialog.cpp : implementation file
//

#include "stdafx.h"
#include "linkage.h"
#include "UserGridDialog.h"
#include "afxdialogex.h"


// CUserGridDialog dialog

IMPLEMENT_DYNAMIC(CUserGridDialog, CDialog)

CUserGridDialog::CUserGridDialog(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_USERGRID, pParent)
	, m_HorizontalSpacing(0)
	, m_VerticalSpacing(0)
	, m_bShowUserGrid(FALSE)
{

}

CUserGridDialog::~CUserGridDialog()
{
}

void CUserGridDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT7, m_HorizontalSpacing);
	DDX_Text(pDX, IDC_EDIT1, m_VerticalSpacing);
	DDX_Check(pDX, IDC_CHECK1, m_bShowUserGrid);
}


BEGIN_MESSAGE_MAP(CUserGridDialog, CDialog)
END_MESSAGE_MAP()


// CUserGridDialog message handlers
