#include "stdafx.h"
#include "MyMFCRibbonApplicationButton.h"

static const int nXMargin = 2;
static const int nYMargin = 2;

CMyMFCRibbonApplicationButton::CMyMFCRibbonApplicationButton()
{
}


CMyMFCRibbonApplicationButton::~CMyMFCRibbonApplicationButton()
{
}

void CMyMFCRibbonApplicationButton::DrawImage(CDC* pDC, RibbonImageType /*type*/, CRect rectImage)
{
	ASSERT_VALID(this);
	ASSERT_VALID(pDC);

	// Replace the image drawing code with code to display "File" instead. Windows 10 apps don't use the image style File menu like Windows 7 did.

	CFont* pOldFont = pDC->SelectObject(&(GetGlobalData()->fontRegular));
	ENSURE(pOldFont != NULL);
	COLORREF clrTextOld = pDC->SetTextColor( RGB( 255, 255, 255 ) );
	pDC->DrawText( "File", m_rect, DT_VCENTER | DT_CENTER | DT_SINGLELINE );

	pDC->SelectObject( pOldFont );
	pDC->SetTextColor( clrTextOld );
}


