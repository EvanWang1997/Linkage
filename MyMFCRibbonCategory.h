#pragma once
class CMyMFCRibbonCategory : CMFCRibbonCategory
{
public:
	CMyMFCRibbonCategory();
	~CMyMFCRibbonCategory();

	void SetImageResources( UINT uiSmallImagesResID,	UINT uiLargeImagesResID, CSize sizeSmallImage = CSize(16, 16), CSize sizeLargeImage = CSize(32, 32) );
	void SetImageResources( CMyMFCRibbonCategory *pCopyCategory );
};

