#pragma once

class CMyMFCRibbonBar : public CMFCRibbonBar
{
	public:
	void SetPrintPreviewMode( BOOL bSet = 1 ) 
		{ CMFCRibbonBar::SetPrintPreviewMode( bSet ); }
	CMFCRibbonCategory *GetMainCategory( void ) 
		{ return m_pMainCategory; }
	CMFCRibbonCategory *GetPrintPreviewCategory( void ) 
		{ return m_pPrintPreviewCategory; }
	BOOL OnShowRibbonContextMenu(CWnd* pWnd, int x, int y, CMFCRibbonBaseElement* pHit)
		{ return TRUE; }

	virtual void RecalcLayout()
	{
		CMFCRibbonBar::RecalcLayout();

		CRect rect;
		GetClientRect(rect);

		static const int nXMargin = 2;
		static const int nYMargin = 2;

		const BOOL bHideAll = m_dwHideFlags & AFX_RIBBONBAR_HIDE_ALL;

		// Reposition main button:
		CSize sizeMainButton = m_sizeMainButton;
		double scale = GetGlobalData()->GetRibbonImageScale();
		if (scale > 1.)
		{
			sizeMainButton.cx = (int)(.5 + scale * sizeMainButton.cx);
			sizeMainButton.cy = (int)(.5 + scale * sizeMainButton.cy);
		}

		if (m_pMainButton != NULL)
		{
			ASSERT_VALID(m_pMainButton);

			if (bHideAll)
			{
				m_pMainButton->SetRect(CRect(0, 0, 0, 0));
			}
			else
			{
				int yOffset = nYMargin;

				if (GetGlobalData()->IsDwmCompositionEnabled())
				{
					yOffset += GetSystemMetrics(SM_CYSIZEFRAME) / 2;
				}

				m_pMainButton->SetRect(CRect(CPoint(rect.left, rect.top + yOffset), sizeMainButton));

				if (!IsWindows7Look() )
				{
					m_rectCaptionText.left = m_pMainButton->GetRect().right + nXMargin;
				}
				else
				{
					CRect rectMainBtn = rect;
					rectMainBtn.top = m_rectCaption.IsRectEmpty() ? rect.top : m_rectCaption.bottom;
					rectMainBtn.bottom = rectMainBtn.top + m_nTabsHeight;

					// The line below was the line with the bug. It was using m_sizeMainButton instead of the adjusted sizeMainButton structure.
					rectMainBtn.right = rectMainBtn.left + sizeMainButton.cx;

					m_pMainButton->SetRect(rectMainBtn);

					// The redraw is needed because some previous initial sizing also goes wrong and it's easier to
					// force a redraw now than to find the place in the code where the calculation is done wrong.
					// This is always done because there is some unknown code somewhere that is making the button
					// go to it's broken size when the client view window gets the focus. This might cause a flicker 
					// though.
					m_pMainButton->Redraw();

					if (IsQuickAccessToolbarOnTop())
					{
						m_rectCaptionText.left = m_rectCaption.left + ::GetSystemMetrics(SM_CXSMICON) + 4 * nXMargin;
					}
				}
			}
		}

	}

	protected:
};
