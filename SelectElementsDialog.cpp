// SelectElementsDialog.cpp : implementation file
//

#include "stdafx.h"
#include "linkage.h"
#include "SelectElementsDialog.h"
#include "afxdialogex.h"


// CSelectElementsDialog dialog

IMPLEMENT_DYNAMIC(CSelectElementsDialog, CMyDialog)

CSelectElementsDialog::CSelectElementsDialog(CWnd* pParent /*=NULL*/)
	: CMyDialog( pParent, IDD_SELECTELEMENTS)
{

}

CSelectElementsDialog::~CSelectElementsDialog()
{
}

void CSelectElementsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSelectElementsDialog, CMyDialog)
END_MESSAGE_MAP()


// CSelectElementsDialog message handlers
