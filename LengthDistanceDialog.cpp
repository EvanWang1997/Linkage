// LengthDistanceDialog.cpp : implementation file
//

#include "stdafx.h"
#include "linkage.h"
#include "LengthDistanceDialog.h"
#include "afxdialogex.h"


// CLengthDistanceDialog dialog

IMPLEMENT_DYNAMIC(CLengthDistanceDialog, CMyDialog)

CLengthDistanceDialog::CLengthDistanceDialog(CWnd* pParent /*=NULL*/)
	: CMyDialog( pParent, IDD_LENGTH)
	, m_Distance( _T("") )
{

}

CLengthDistanceDialog::~CLengthDistanceDialog()
{
}

void CLengthDistanceDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_Distance);
}


BEGIN_MESSAGE_MAP(CLengthDistanceDialog, CMyDialog)
END_MESSAGE_MAP()


// CLengthDistanceDialog message handlers
