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

	/*
	 * This is not quite right. If I pass in a 24x24 image and the scale is 2.0, the code will not
	 * resize the image even though it is needed. The code should resize the image in every case except when
	 * it is already the correct size, not just when it is not 16x16 or not 32x32 as done below.
	 *
	 * The fix is to change this code so that it resizes the images if they are anything except the expected
	 * final size. Small images would need to be 24x24 for a 150% display setting.
	 *
	 * Also, the scale value is unaffected by the user display scaling option. The MS code will go ahead and
	 * try to scale the small images up from 16x16 to 24x24 in the case of a display of 144 DPI display even if
	 * the users wants everything tiny. There is no way around this because the MS code elsewhere takes the 
	 * incorrectly enlargd images and shrinks them back to the 16x16 for output in that situation - something that
	 * I have not found a way to fix.
	 */

	const double dblScale = GetGlobalData()->GetRibbonImageScale();
	if (dblScale != 1.0)
	{
		if (sizeSmallImage != CSize((int)(16.0*dblScale), (int)(16.0*dblScale)))
		{
			m_SmallImages.SmoothResize(dblScale);
		}

		if (sizeLargeImage != CSize((int)(32.0*dblScale), (int)(32.0*dblScale)))
		{
			m_LargeImages.SmoothResize(dblScale);
		}
	}
}

