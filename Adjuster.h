#pragma once

#include "Element.h"
#include "geometry.h"

class CAdjuster
{
public:
	CAdjuster();
	~CAdjuster();

	CFPoint GetPoint( void );
	void SetPoint( CFPoint Point );
	void SetParent( class CElement *pElement );
	CElement *GetParent( void );
	void SetSlideLimits( class CConnector* Point1, class CConnector* Point2 );
	bool GetSlideLimits( CFPoint &Point1, CFPoint &Point2 );
	bool IsShowOnParentSelect( void );
	void SetShowOnParentSelect( bool bValue );
	bool PointOnAdjuster( CFPoint Point, double TestDistance );
	void Select( bool bSelect );
	bool IsSelected( void );

private:
	class CAdjusterImplementation* m_pImplementation;
};

