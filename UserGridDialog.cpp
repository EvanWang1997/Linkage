// UserGridDialog.cpp : implementation file
//

#include "stdafx.h"
#include "linkage.h"
#include "UserGridDialog.h"
#include "afxdialogex.h"

void AFXAPI DDX_MyDoubleText( CDataExchange* pDX, int nIDC, double& value, int Precision );

// CUserGridDialog dialog

IMPLEMENT_DYNAMIC(CUserGridDialog, CMyDialog)

CUserGridDialog::CUserGridDialog(CWnd* pParent /*=NULL*/)
	: CMyDialog( pParent, IDD_USERGRID )
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
	CMyDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK1, m_bShowUserGrid);
	DDX_Radio(pDX, IDC_RADIO3, m_GridTypeSelection);
	DDX_Control(pDX, IDC_PROMPT1, m_HorizontalPrompt);
	DDX_Control(pDX, IDC_PROMPT2, m_VerticalPrompt);
	DDX_Control(pDX, IDC_EDIT7, m_HorizontalControl);
	DDX_Control(pDX, IDC_EDIT1, m_VerticalControl);
	DDX_MyDoubleText(pDX, IDC_EDIT7, m_HorizontalSpacing, 4);
	DDX_MyDoubleText(pDX, IDC_EDIT1, m_VerticalSpacing, 4);


	OnTypeClick();
}


BEGIN_MESSAGE_MAP(CUserGridDialog, CMyDialog)
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


