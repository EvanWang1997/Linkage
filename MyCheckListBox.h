#pragma once
#include "afxwin.h"
class CMyCheckListBox :
	public CCheckListBox
{
public:
	CMyCheckListBox();
	virtual ~CMyCheckListBox();

protected:

	virtual void PreSubclassWindow();
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual CRect OnGetCheckPosition(CRect rectItem, CRect rectCheckBox);

};

