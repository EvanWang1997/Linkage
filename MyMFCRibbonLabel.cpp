#include "stdafx.h"
#include "MyMFCRibbonLabel.h"


CMyMFCRibbonLabel::CMyMFCRibbonLabel()
{
}


CMyMFCRibbonLabel::~CMyMFCRibbonLabel()
{
}

void CMyMFCRibbonLabel::SetTextAndResize(LPCTSTR lpszText)
{
	CWindowDC dc( 0 );
	CWnd *pWnd = this->GetParentWnd();
	// the call to SetWindowPos is necessary for some odd unknown reason.
	pWnd->SetWindowPos( 0, 0, 0, 100, 5, SWP_NOMOVE | SWP_NOZORDER );
	CString Text = "      ";
	Text += lpszText;
	CMFCRibbonLabel::SetText( Text );
	m_strText = Text;
	CMFCRibbonLabel::OnCalcTextSize( &dc );
	// The size calculation will force tbhe redraw dso the Redraw() function is not needed here.
	CMFCRibbonCategory *pCat = GetParentCategory();
	pCat->RecalcLayout( &dc );
}
