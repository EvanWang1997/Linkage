// SelectElementsDialog.cpp : implementation file
//

#include "stdafx.h"
#include "linkage.h"
#include "Element.h"
#include "linkageDoc.h"
#include "SelectElementsDialog.h"
#include "afxdialogex.h"
#include <algorithm>
#include <vector>

using namespace std;

// CSelectElementsDialog dialog

IMPLEMENT_DYNAMIC(CSelectElementsDialog, CMyDialog)

CSelectElementsDialog::CSelectElementsDialog(CWnd* pParent /*=NULL*/)
	: CMyDialog( pParent, IDD_SELECTELEMENTS)
{
	m_bShowDebug = false;
	m_pDocument = 0;
	m_bInitialized = false;
}

CSelectElementsDialog::~CSelectElementsDialog()
{
}

void CSelectElementsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST3, m_ListControl2);
}



BEGIN_MESSAGE_MAP(CSelectElementsDialog, CMyDialog)
	ON_NOTIFY( LVN_ITEMCHANGED, IDC_LIST3, OnItemchangedList2 )
END_MESSAGE_MAP()


// CSelectElementsDialog message handlers

static void AddListData( CListCtrl &List, int row, int col, const char *str )
{
    LVITEM lv;
    lv.iItem = row;
    lv.iSubItem = col;
    lv.pszText = (LPSTR) str;
    lv.mask = LVIF_TEXT;
    if(col == 0)
        List.InsertItem(&lv);
    else
        List.SetItem(&lv);  
}

bool IsNumber( CString &String )
{
    return String.SpanIncluding("0123456789").GetLength() == String.GetLength();
}

bool CompareElementsEx ( CElement *lhs, CElement *rhs) 
{
	if( lhs == 0 || rhs == 0 )
		return false;
	CString Left = lhs->GetIdentifierString( false );
	CString Right = rhs->GetIdentifierString( false );

	if( IsNumber( Left ) && IsNumber( Right ) )
	{
		if( Left.GetLength() < Right.GetLength() )
			return true;
		else if( Right.GetLength() < Left.GetLength() )
			return false;
	}

	return lhs->GetIdentifierString( false ) < rhs->GetIdentifierString( false ); 
}

BOOL CSelectElementsDialog::OnInitDialog()
{
	#if defined( LINKAGE_USE_DIRECT2D )
		CWindowDC DC( this );
		int PPI = DC.GetDeviceCaps( LOGPIXELSX );
		double DPIScale = (double)PPI / 96.0;
	#else
		double DPIScale = 1.0;
	#endif

	CMyDialog::OnInitDialog();

	m_ListControl2.SetExtendedStyle( LVS_EX_CHECKBOXES );

	m_ListControl2.InsertColumn(0, "");
	m_ListControl2.InsertColumn(1, "Element");
	m_ListControl2.InsertColumn(2, "Name");
	m_ListControl2.SetColumnWidth(0, (int)( 23 * DPIScale ) );
	m_ListControl2.SetColumnWidth(1, (int)( 80 * DPIScale ) );
	m_ListControl2.SetColumnWidth(2, 999999);

	vector<CElement*> Elements( 0 );
	POSITION Position = m_AllElements.GetHeadPosition();
	while( Position != 0 )
	{
		CElementItem *pElementItem = m_AllElements.GetNext( Position );
		if( pElementItem == 0 )
			continue;

		CElement *pElement = pElementItem->GetElement();
		if( pElement == 0 )
			continue;

		if( pElement->IsLink() && ((CLink*)pElement)->GetConnectorCount() <= 1 && !((CLink*)pElement)->IsGear() )
			continue; // No links that have just one connector.

		Elements.push_back( pElement );
	}

	sort( Elements.begin(), Elements.end(), CompareElementsEx );

	int Row = 0;
	for( std::vector<CElement*>::iterator it = Elements.begin(); it != Elements.end(); ++it )
	{
		CElement *pElement = *it;
		if( pElement == 0 )
			continue;
		AddListData( m_ListControl2, Row, 0, "" );
		AddListData( m_ListControl2, Row, 1, (const char*)pElement->GetTypeString() );
		CString Name = pElement->GetIdentifierString( m_bShowDebug );
		if( pElement->GetName().IsEmpty() && ( pElement->GetLayers() & CLinkageDoc::MECHANISMLAYERS ) == 0 )
			Name += "  (unlabeled)";

		AddListData( m_ListControl2, Row, 2, (const char*)Name );
		m_ListControl2.SetItemData( Row, (DWORD_PTR)pElement );

		if( pElement->IsSelected() )
			m_ListControl2.SetCheck( Row, BST_CHECKED );

		++Row;
	}

	m_bInitialized = true;

	return TRUE;
}

void CSelectElementsDialog::OnItemchangedList2(NMHDR* pNMHDR, LRESULT* pResult) 
{
	if( !m_bInitialized )
		return;

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if ((pNMListView->uChanged & LVIF_STATE) 
		&& (pNMListView->uNewState == 0x1000 || pNMListView->uNewState == 0x2000 ))
	{
		for( int Index = 0; Index < m_ListControl2.GetItemCount(); ++Index )
		{
			CElement *pElement = (CElement*)m_ListControl2.GetItemData( Index );
			if( pElement == 0 )
				continue;
			if( m_pDocument != 0 )
				m_pDocument->SelectElement( pElement );
				//pElement->Select( m_ListControl2.GetCheck( Index ) != FALSE );
		}

		if( m_pDocument != 0 )
			m_pDocument->UpdateAllViews( 0 );
	}
}

void CSelectElementsDialog::OnOK( void )
{
	for( int Index = 0; Index < m_ListControl2.GetItemCount(); ++Index )
	{
		CElement *pElement = (CElement*)m_ListControl2.GetItemData( Index );
		if( pElement == 0 )
			continue;
		pElement->Select( m_ListControl2.GetCheck( Index ) != FALSE );
	}

	CMyDialog::OnOK();
}
