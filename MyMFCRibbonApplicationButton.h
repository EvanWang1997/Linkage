#pragma once
#include "afxribbonbar.h"
class CMyMFCRibbonApplicationButton :
	public CMFCRibbonApplicationButton
{
public:
	CMyMFCRibbonApplicationButton();
	~CMyMFCRibbonApplicationButton();

	void DrawImage( CDC* pDC, RibbonImageType /*type*/, CRect rectImage );
	//virtual void OnDraw(CDC* pDC);
};

