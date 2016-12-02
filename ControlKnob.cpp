#include "stdafx.h"
#include "connector.h"
#include "ControlKnob.h"
#include "geometry.h"

class CControlKnobImplementation
{
	public:
	CControlKnobImplementation()
	{
		m_pElement = 0;
		pLimit1 = 0;
		pLimit2 = 0;
		m_bShowOnParentSelect = false;
		m_bSelected = false;
	}

	CFPoint GetPoint( void ) 
	{
		CFPoint Point = m_RelativePoint + ( m_pElement == 0 ? CPoint() : m_pElement->GetLocation() );
		return Point;
	}

	CFPoint SetPoint( CFPoint Point ) 
	{
		CFPoint Start;
		CFPoint End;
		if( GetSlideLimits( Start, End ) )
		{
			CFLine TempLine( Start, End );
			Point.SnapToLine( TempLine, true, true );
		}
		m_Point = Point; 
		m_RelativePoint = Point - ( m_pElement == 0 ? CPoint() : m_pElement->GetLocation() );
		return m_Point;
	}

	void SetParent( CElement *pElement )
	{
		m_pElement = pElement;
	}

	CElement *GetParent( void ) { return m_pElement; }
	bool IsShowOnParentSelect( void ) { return m_bShowOnParentSelect; }
	void SetShowOnParentSelect( bool bValue ) { m_bShowOnParentSelect = bValue; }
	void SetSlideLimits( CConnector* Point1, CConnector* Point2 ) { pLimit1 = Point1; pLimit2 = Point2; }
	bool GetSlideLimits( CFPoint &Point1, CFPoint &Point2 )
	{
		if( pLimit1 == 0 )
			return false;
		Point1 = pLimit1->GetPoint();
		if( pLimit2 == 0 )
			Point2.SetPoint( Point1.x + 100, Point1.y + 100 );
		else
			Point2 = pLimit2->GetPoint();
		return true;
	}

	void Select( bool bSelect ) { m_bSelected = bSelect; }
	bool IsSelected( void ) { return m_bSelected; }

	CFPoint m_Point;
	CFPoint m_RelativePoint;
	CElement *m_pElement;
	bool m_bShowOnParentSelect;
	CConnector *pLimit1;
	CConnector *pLimit2;
	bool m_bSelected;
};


CControlKnob::CControlKnob()
{
	m_pImplementation = new CControlKnobImplementation;
}

CControlKnob::~CControlKnob()
{
	if( m_pImplementation != 0 )
		delete m_pImplementation;
	m_pImplementation = 0;
}

void CControlKnob::Select( bool bSelect )
{
	if( m_pImplementation == 0 )
		return;

	m_pImplementation->Select( bSelect );
}

bool CControlKnob::IsSelected( void )
{
	if( m_pImplementation == 0 )
		return false;

	return m_pImplementation->IsSelected();
}

bool CControlKnob::PointOnControlKnob( CFPoint Point, double TestDistance )
{
	if( m_pImplementation == 0 )
		return false;

	CFPoint OurPoint = m_pImplementation->GetPoint();
	double Distance = OurPoint.DistanceToPoint( Point );

	return Distance <= TestDistance;
}

CFPoint CControlKnob::GetPoint( void ) 
{
	if( m_pImplementation == 0 )
		return CFPoint();
	return m_pImplementation->GetPoint();
}

CFPoint CControlKnob::SetPoint( CFPoint Point )
{
	if( m_pImplementation == 0 )
		return CFPoint();
	return m_pImplementation->SetPoint( Point );
}

void CControlKnob::SetParent( CElement *pElement )
{
	if( m_pImplementation == 0 )
		return;
	m_pImplementation->SetParent( pElement );
}

CElement *CControlKnob::GetParent( void )
{
	if( m_pImplementation == 0 )
		return 0;
	return m_pImplementation->GetParent();
}

bool CControlKnob::IsShowOnParentSelect( void ) 
{
	if( m_pImplementation == 0 )
		return false;
	return m_pImplementation->IsShowOnParentSelect();
}

void CControlKnob::SetShowOnParentSelect( bool bValue )
{
	if( m_pImplementation == 0 )
		return;
	m_pImplementation->SetShowOnParentSelect( bValue );
}

/*
void CControlKnob::SnapToSlideLimits( CFPoint Point1, CFPoint Point2 )
{
	if( m_pImplementation == 0 )
		return;
	CFLine TempLine( Point1, Point2 );
	CFPoint Point = m_pImplementation->GetPoint();
	Point.SnapToLine( TempLine, true, true );
	m_pImplementation->SetPoint( Point );
}
*/

void CControlKnob::SetSlideLimits( class CConnector* Point1, class CConnector* Point2 )
{
	if( m_pImplementation == 0 )
		return;

	m_pImplementation->SetSlideLimits( Point1, Point2 );
}

bool CControlKnob::GetSlideLimits( CFPoint &Point1, CFPoint &Point2 )
{
	if( m_pImplementation == 0 )
		return false;

	return m_pImplementation->GetSlideLimits( Point1, Point2 );
}



