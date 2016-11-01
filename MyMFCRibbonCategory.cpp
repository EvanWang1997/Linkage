#include "stdafx.h"
#include "MyMFCRibbonCategory.h"


CMyMFCRibbonCategory::CMyMFCRibbonCategory()
{
}


CMyMFCRibbonCategory::~CMyMFCRibbonCategory()
{
}

void CMyMFCRibbonCategory::SetImageResources( CMyMFCRibbonCategory *pCopyCategory )
{
	pCopyCategory->m_SmallImages.CopyTo( m_SmallImages );
	pCopyCategory->m_LargeImages.CopyTo( m_LargeImages );
}

void CMyMFCRibbonCategory::SetImageResources( UINT uiSmallImagesResID,	UINT uiLargeImagesResID, CSize sizeSmallImage, CSize sizeLargeImage )
{
	// Load images:
	if (sizeSmallImage != CSize(0, 0))
	{
		m_SmallImages.SetImageSize(sizeSmallImage);
	}

	if (sizeLargeImage != CSize(0, 0))
	{
		m_LargeImages.SetImageSize(sizeLargeImage);
	}

	m_uiSmallImagesResID = uiSmallImagesResID;
	m_uiLargeImagesResID = uiLargeImagesResID;

	if (m_uiSmallImagesResID > 0)
	{
		if (!m_SmallImages.Load(m_uiSmallImagesResID))
		{
			m_uiSmallImagesResID = 0;
			ASSERT(FALSE);
		}
	}

	if (m_uiLargeImagesResID > 0)
	{
		if (!m_LargeImages.Load(m_uiLargeImagesResID))
		{
			m_uiLargeImagesResID = 0;
			ASSERT(FALSE);
		}
	}

	const double dblScale = GetGlobalData()->GetRibbonImageScale();
	if (dblScale != 1.0)
	{
		if (sizeSmallImage == CSize(16, 16))
		{
			m_SmallImages.SmoothResize(dblScale);
		}

		if (sizeLargeImage == CSize(32, 32))
		{
			m_LargeImages.SmoothResize(dblScale);
		}
	}
}

