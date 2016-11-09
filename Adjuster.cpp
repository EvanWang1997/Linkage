#include "stdafx.h"
#include "connector.h"
#include "Adjuster.h"
#include "geometry.h"

class CAdjusterImplementation
{
	public:
	CAdjusterImplementation()
	{
		m_pElement = 0;
		pLimit1 = 0;
		pLimit2 = 0;
		m_bShowOnParentSelect = false;
		m_bSelected = false;
	}

	CFPoint GetPoint( void ) { return m_Point; }
	void SetPoint( CFPoint Point ) { m_Point = Point; }
	void SetParent( CElement *pElement ) { m_pElement = pElement; }
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
	CElement *m_pElement;
	bool m_bShowOnParentSelect;
	CConnector *pLimit1;
	CConnector *pLimit2;
	bool m_bSelected;
};


CAdjuster::CAdjuster()
{
	m_pImplementation = new CAdjusterImplementation;
}

CAdjuster::~CAdjuster()
{
	if( m_pImplementation != 0 )
		delete m_pImplementation;
	m_pImplementation = 0;
}

void CAdjuster::Select( bool bSelect )
{
	if( m_pImplementation == 0 )
		return;

	m_pImplementation->Select( bSelect );
}

bool CAdjuster::IsSelected( void )
{
	if( m_pImplementation == 0 )
		return false;

	return m_pImplementation->IsSelected();
}

bool CAdjuster::PointOnAdjuster( CFPoint Point, double TestDistance )
{
	if( m_pImplementation == 0 )
		return false;

	CFPoint OurPoint = m_pImplementation->GetPoint();
	double Distance = OurPoint.DistanceToPoint( Point );

	return Distance <= TestDistance;
}

CFPoint CAdjuster::GetPoint( void ) 
{
	if( m_pImplementation == 0 )
		return CFPoint();
	return m_pImplementation->GetPoint();
}

void CAdjuster::SetPoint( CFPoint Point )
{
	if( m_pImplementation == 0 )
		return;
	m_pImplementation->SetPoint( Point );
}

void CAdjuster::SetParent( CElement *pElement )
{
	if( m_pImplementation == 0 )
		return;
	m_pImplementation->SetParent( pElement );
}

CElement *CAdjuster::GetParent( void )
{
	if( m_pImplementation == 0 )
		return 0;
	return m_pImplementation->GetParent();
}

bool CAdjuster::IsShowOnParentSelect( void ) 
{
	if( m_pImplementation == 0 )
		return false;
	return m_pImplementation->IsShowOnParentSelect();
}

void CAdjuster::SetShowOnParentSelect( bool bValue )
{
	if( m_pImplementation == 0 )
		return;
	m_pImplementation->SetShowOnParentSelect( bValue );
}

/*
void CAdjuster::SnapToSlideLimits( CFPoint Point1, CFPoint Point2 )
{
	if( m_pImplementation == 0 )
		return;
	CFLine TempLine( Point1, Point2 );
	CFPoint Point = m_pImplementation->GetPoint();
	Point.SnapToLine( TempLine, true, true );
	m_pImplementation->SetPoint( Point );
}
*/

void CAdjuster::SetSlideLimits( class CConnector* Point1, class CConnector* Point2 )
{
	if( m_pImplementation == 0 )
		return;

	m_pImplementation->SetSlideLimits( Point1, Point2 );
}

bool CAdjuster::GetSlideLimits( CFPoint &Point1, CFPoint &Point2 )
{
	if( m_pImplementation == 0 )
		return false;

	return m_pImplementation->GetSlideLimits( Point1, Point2 );
}



