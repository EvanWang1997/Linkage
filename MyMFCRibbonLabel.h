#pragma once
#include "afxribbonlabel.h"
class CMyMFCRibbonLabel : public CMFCRibbonLabel
{
public:
	CMyMFCRibbonLabel();
	~CMyMFCRibbonLabel();

	void SetTextAndResize(LPCTSTR lpszText);
};
