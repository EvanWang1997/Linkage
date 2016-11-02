#include "stdafx.h"
#include "MyCheckListBox.h"


CMyCheckListBox::CMyCheckListBox()
{
}


CMyCheckListBox::~CMyCheckListBox()
{
}

void CMyCheckListBox::PreSubclassWindow()
{
	CCheckListBox::PreSubclassWindow();

	int Height = GetItemHeight( 0 );
	SetItemHeight( 0, Height + 15 );
}

void CMyCheckListBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	lpDrawItemStruct->rcItem.left += 2;
	CCheckListBox::DrawItem( lpDrawItemStruct );
}

CRect CMyCheckListBox::OnGetCheckPosition(CRect rectItem, CRect rectCheckBox)
{
	CRect Result = CCheckListBox::OnGetCheckPosition( rectItem, rectCheckBox );
	Result.left += 0;
	Result.right += 0;

	return Result;
}


