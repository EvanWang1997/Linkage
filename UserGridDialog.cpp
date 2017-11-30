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
	, m_GridTypeSelection(0)
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
	DDX_Radio(pDX, IDC_RADIO3, m_GridTypeSelection);
	DDX_Control(pDX, IDC_PROMPT1, m_HorizontalPrompt);
	DDX_Control(pDX, IDC_PROMPT2, m_VerticalPrompt);
	DDX_Control(pDX, IDC_EDIT7, m_HorizontalControl);
	DDX_Control(pDX, IDC_EDIT1, m_VerticalControl);

	OnTypeClick();
}


BEGIN_MESSAGE_MAP(CUserGridDialog, CDialog)
	ON_BN_CLICKED(IDC_RADIO1, &CUserGridDialog::OnTypeClick)
	ON_BN_CLICKED(IDC_RADIO3, &CUserGridDialog::OnTypeClick)
END_MESSAGE_MAP()


// CUserGridDialog message handlers




void CUserGridDialog::OnTypeClick()
{
	CDataExchange dx( this, true );
	DDX_Radio( &dx, IDC_RADIO3, m_GridTypeSelection );
	m_HorizontalPrompt.EnableWindow( m_GridTypeSelection == 1 );
	m_VerticalPrompt.EnableWindow( m_GridTypeSelection == 1 );
	m_HorizontalControl.EnableWindow( m_GridTypeSelection == 1 );
	m_VerticalControl.EnableWindow( m_GridTypeSelection == 1 );
}


