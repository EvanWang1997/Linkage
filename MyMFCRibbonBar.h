#pragma once

class CMyMFCRibbonBar : public CMFCRibbonBar
{
	public:
	void SetPrintPreviewMode( BOOL bSet = 1 ) { CMFCRibbonBar::SetPrintPreviewMode( bSet ); }
	CMFCRibbonCategory *GetMainCategory( void ) { return m_pMainCategory; }

	protected:
};
