#pragma once

#include "Element.h"
#include "geometry.h"

class CControlKnob
{
public:
	CControlKnob();
	~CControlKnob();

	CFPoint GetPoint( void );
	CFPoint SetPoint( CFPoint Point );
	void SetParent( class CElement *pElement );
	CElement *GetParent( void );
	void SetSlideLimits( class CConnector* Point1, class CConnector* Point2 );
	bool GetSlideLimits( CFPoint &Point1, CFPoint &Point2 );
	bool IsShowOnParentSelect( void );
	void SetShowOnParentSelect( bool bValue );
	bool PointOnControlKnob( CFPoint Point, double TestDistance );
	void Select( bool bSelect );
	bool IsSelected( void );

private:
	class CControlKnobImplementation* m_pImplementation;
};

