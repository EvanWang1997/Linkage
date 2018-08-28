#include "stdafx.h"
#include "geometry.h"
#include "link.h"
#include "connector.h"
#include "math.h"
#include "simulator.h"

static const char *ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

static CString GetAlphaIdentifier( int ID )
{
	if( ID < 52 )
		return CString( ALPHABET[ID] );

	CString TempString;
	TempString.Format( "%c%d", ALPHABET[ID%52], ID / 52 );
	return TempString;
}

void CConnector::Reset( void )
{
	m_Point.x = 0;
	m_Point.y = 0;
	m_OriginalPoint.x = 0;
	m_OriginalPoint.y = 0;
	m_TempPoint.x = 0;
	m_TempPoint.y = 0;
	m_RotationAngle = 0.0;
	m_TempRotationAngle = 0.0;
	m_Identifier = 0;
	m_bInput = false;
	m_InputStartOffset = 0.0;
	m_bAnchor = false;
	m_bTempFixed = false;
	m_bDrawingConnector = false;
	m_bAlwaysManual = false;
	m_RPM = 0;
	m_LimitAngle = 0;
	m_StartPoint = 0;
	m_PointCount = 0;
	m_DrawingPointCounter = 9999;
	m_pSlideLimits[0] = 0;
	m_pSlideLimits[1] = 0;
	m_DrawCircleRadius = 0;
	m_OriginalDrawCircleRadius = 0;
	m_SlideRadius = 0;
	m_OriginalSlideRadius = 0;
	m_bShowAsPoint = false;
	m_Color = RGB( 200, 200, 200 );
	UpdateControlKnobs();
}

double CConnector::GetSpeed( void ) { return Distance( m_PreviousPoint, m_Point ) * CSimulator::GetStepsPerMinute(); }

CConnector::CConnector()
{
	Reset();
}

CString CConnector::GetIdentifierString( bool bDebugging )
{
	if( bDebugging )
	{
		CString String;
		String.Format( "C%d", GetIdentifier() );
		return String;
	}

	if( m_Name.IsEmpty() )
		return GetAlphaIdentifier( GetIdentifier() );
	else
		return m_Name;
}

void CConnector::Select( bool bSelected )
{
	m_bSelected = bSelected;
}

bool CConnector::IsLinkSelected( void )
{
	POSITION Position = m_Links.GetHeadPosition();
	for( int Counter = 0; Position != NULL; ++Counter )
	{
		CLink* pLink = m_Links.GetNext( Position );
		if( pLink == 0 )
			continue;
		if( pLink->IsSelected() )
			return true;
	}
	return false;
}

bool CConnector::PointOnConnector( CFPoint Point, double TestDistance )
{
	double Distance = m_Point.DistanceToPoint( Point );

	return Distance <= TestDistance;
}

void CConnector::AddLink( class CLink* pLink )
{
	if( m_Links.Find( pLink ) == 0 )
		m_Links.AddTail( pLink );
}

CLink* CConnector::GetLink( int Index )
{
	POSITION Position = m_Links.GetHeadPosition();
	for( int Counter = 0; Position != NULL; ++Counter )
	{
		CLink* pLink = m_Links.GetNext( Position );
		if( Counter == Index )
			return pLink;
	}
	return 0;
}

int CConnector::GetSelectedLinkCount( void )
{
	int Count = 0;
	POSITION Position = m_Links.GetHeadPosition();
	for( int Counter = 0; Position != NULL; ++Counter )
	{
		CLink* pLink = m_Links.GetNext( Position );
		if( pLink == 0 || !pLink->IsSelected() )
			continue;
		++Count;
	}
	return Count;
}

void CConnector::UpdateControlKnobs( void )
{
	int Knobs = 0;
	CControlKnob *pControlKnob = GetControlKnobs( Knobs );
	if( m_DrawCircleRadius == 0.0 || pControlKnob == 0 )
		return;

	CFPoint Point = pControlKnob->GetPoint();
	CFPoint Center = GetPoint();
	if( Point == Center )
		Point += CFPoint( 10, 10 );
	CFLine Line( GetPoint(), Point );
	Line.SetLength( m_DrawCircleRadius );

	pControlKnob->SetPoint( Line.GetEnd() );
}

void CConnector::UpdateControlKnob( CControlKnob *pKnob, CFPoint Point )
{
	int Knobs = 0;
	CControlKnob *pControlKnob = GetControlKnobs( Knobs );
	if( m_DrawCircleRadius == 0.0 || pControlKnob == 0 )
		return;

	Point = pControlKnob->SetPoint( Point );

	m_DrawCircleRadius = Distance( GetPoint(), Point );
	if( m_DrawCircleRadius <= 0.0 )
		m_DrawCircleRadius = 0.001;
	m_OriginalDrawCircleRadius = m_DrawCircleRadius;
}

int CConnector::GetControlKnobCount( void )
{
	return m_OriginalDrawCircleRadius != 0.0 ? 1 : 0;
}

void CConnector::SetDrawCircleRadius( double Radius )
{
	m_DrawCircleRadius = fabs( Radius );
	CFPoint Temp = GetPoint();
	if( m_DrawCircleRadius == 0 )
		Temp += 1000;
	CFLine Line( GetPoint(), Temp );

	int Knobs = 0;
	CControlKnob *pControlKnob = GetControlKnobs( Knobs );
	pControlKnob->SetParent( Radius == 0 ? 0 : this );
	Line.SetLength( m_DrawCircleRadius );

	pControlKnob->SetPoint( Line.GetEnd() );
	pControlKnob->SetShowOnParentSelect( true );
		
	m_OriginalDrawCircleRadius = fabs( Radius );
	UpdateControlKnobs();
}

void CConnector::RotateAround( CFPoint& Point, double Angle )
{
	SetTempFixed( true );

	m_TempPoint.RotateAround( Point, Angle );

/*	Angle *= 0.0174532925; // Convert to radians.

	double x = m_TempPoint.x - Point.x;
	double y = m_TempPoint.y - Point.y;

	double Cosine = cos( Angle );
	double Sine = sin( Angle );

	double x1 = x * Cosine - y * Sine;
	double y1 = x * Sine + y * Cosine;

	m_TempPoint.x = x1 + Point.x;
	m_TempPoint.y = y1 + Point.y;
	*/
}

void CConnector::IncrementRotationAngle( double Change )
{
	double Angle = GetRotationAngle();
	Angle += Change;
//	if( Angle < 0.0 )
//		Angle += 360.0;
//	else if( Angle >= 360.0 )
//		Angle -= 360.0;
	SetRotationAngle( Angle );
}

void CConnector::GetArea( CFRect &Rect )
{
	CFPoint Point = GetPoint();
	Rect.left = Point.x - m_DrawCircleRadius;
	Rect.top = Point.y + m_DrawCircleRadius;
	Rect.right = Point.x + m_DrawCircleRadius;
	Rect.bottom = Point.y - m_DrawCircleRadius;
}

void CConnector::GetAdjustArea( CFRect &Rect )
{
	CFPoint Point = GetPoint();
	Rect.left = Point.x - m_DrawCircleRadius;
	Rect.top = Point.y + m_DrawCircleRadius;
	Rect.right = Point.x + m_DrawCircleRadius;
	Rect.bottom = Point.y - m_DrawCircleRadius;
}

void CConnector::RemoveLink( CLink *pLink )
{
	POSITION Position = m_Links.GetHeadPosition();
	while( Position != 0 )
	{
		POSITION RemovePosition = Position;
		CLink *pCheckLink = m_Links.GetNext( Position );
		if( pCheckLink != 0 && ( pLink == 0 || pCheckLink == pLink ) )
		{
			m_Links.RemoveAt( RemovePosition );
			pCheckLink->SetActuator( false ); // If the link used this connector then there is no way it could still be an actuator since it would now have one connector.
		}
	}
}

CFPoint * CConnector::GetMotionPath( int &StartPoint, int &PointCount, int &MaxPoint )
{
	StartPoint = m_StartPoint;
	PointCount = m_PointCount;
	MaxPoint = MAX_DRAWING_POINTS - 1;
	return m_DrawingPoints;
}

void CConnector::AddMotionPoint( void )
{
	if( !m_bDrawingConnector )
		return;

	++m_DrawingPointCounter;
	if( m_DrawingPointCounter < 3 )
		return;

	m_DrawingPointCounter = 0;

	CFPoint Point = GetPoint();

	if( m_PointCount < MAX_DRAWING_POINTS )
	{
		if( m_PointCount > 0 && m_DrawingPoints[m_PointCount] == Point )
			return;
		m_DrawingPoints[m_PointCount] = Point;
		++m_PointCount;
	}
	else
	{
		int PreviousPoint = m_StartPoint == 0 ? MAX_DRAWING_POINTS - 1 : m_StartPoint - 1;
		if( m_DrawingPoints[PreviousPoint] == Point )
			return;
		m_DrawingPoints[m_StartPoint] = Point;
		++m_StartPoint;
		if( m_StartPoint == MAX_DRAWING_POINTS )
			m_StartPoint = 0;
	}
}

void CConnector::Reset( bool bClearMotionPath )
{
	m_Point = m_OriginalPoint;
	m_PreviousPoint = m_OriginalPoint;
	m_RotationAngle = 0.0;
	m_TempRotationAngle = 0.0;
	SetPositionValid( true );
	if( bClearMotionPath )
	{
		m_StartPoint = 0;
		m_PointCount = 0;
		m_DrawingPointCounter = 9999;
	}
}

void CConnector::MakePermanent( void )
{
	m_PreviousPoint = m_Point;
	m_Point = m_TempPoint;
	m_RotationAngle = m_TempRotationAngle;
	m_OriginalDrawCircleRadius = m_DrawCircleRadius;
	m_OriginalSlideRadius = m_SlideRadius;

	// Maybe this is where fastened elements should move to reflect the point movement? Let's try it.
	CFPoint Offset;
	Offset.x = m_Point.x - GetOriginalPoint().x;
	Offset.y = m_Point.y - GetOriginalPoint().y;
	POSITION Position = m_FastenedElements.GetHeadPosition();
	while( Position != 0 )
	{
		CElementItem *pItem = m_FastenedElements.GetNext( Position );
		if( pItem == 0 )
			continue;

		CConnector *pMoveConnector = pItem->GetConnector();
		CLink *pMoveLink = pItem->GetLink();

		if( pMoveConnector == 0 && pMoveLink == 0 )
			continue; 

		POSITION Position2 = pMoveLink == 0 ? 0 : pMoveLink->GetConnectorList()->GetHeadPosition();
		if( pMoveConnector == 0 )
			pMoveConnector = pMoveLink->GetConnectorList()->GetNext( Position2 );

		while( true )
		{
			CFPoint Temp = pMoveConnector->GetOriginalPoint();

			Temp.x += Offset.x;
			Temp.y += Offset.y;
			pMoveConnector->MovePoint( Temp );
			pMoveConnector->SetTempFixed( true );
			pMoveConnector->MakePermanent();

			if( Position2 == 0 )
				break;

			pMoveConnector = pMoveLink->GetConnectorList()->GetNext( Position2 );
		}
	}
}

CConnector::CConnector( const CConnector &ExistingConnector ) : CElement( ExistingConnector )
{
	Reset();
	m_Point = ExistingConnector.m_OriginalPoint;
	m_RotationAngle = ExistingConnector.m_RotationAngle;
	m_TempPoint = ExistingConnector.m_OriginalPoint;
	m_TempRotationAngle = ExistingConnector.m_TempRotationAngle;
	m_OriginalPoint = ExistingConnector.m_OriginalPoint;
	m_Identifier = ExistingConnector.m_Identifier;
	m_RPM = ExistingConnector.m_RPM;
	m_LimitAngle = ExistingConnector.m_LimitAngle;
	m_bInput = ExistingConnector.m_bInput;
	m_bAnchor = ExistingConnector.m_bAnchor;
	m_bSelected = false; // ExistingConnector.m_bSelected; // Can't copy the selection state since being selected is more than just having this flag set (like the document keeping track of all selected elements).
	m_bTempFixed = ExistingConnector.m_bTempFixed;
	m_bDrawingConnector = ExistingConnector.m_bDrawingConnector;
	m_Layers = ExistingConnector.m_Layers;
	m_PreviousPoint = ExistingConnector.m_PreviousPoint;
	m_RotationAngle = ExistingConnector.m_RotationAngle;
	m_OriginalDrawCircleRadius = ExistingConnector.m_OriginalDrawCircleRadius;
	m_OriginalSlideRadius = ExistingConnector.m_OriginalSlideRadius;
	m_InputStartOffset = ExistingConnector.m_InputStartOffset;
	m_bShowAsPoint = ExistingConnector.m_bShowAsPoint;

	m_StartPoint = 0;
	m_PointCount = 0;
	m_DrawingPointCounter = 0;
}

bool CConnector::SlideBetween( class CConnector *pConnector1, class CConnector *pConnector2 )
{
	if( pConnector2 == 0 )
		pConnector1 = 0;

	bool bSet = pConnector1 != 0;
	CLink *pLink = 0;

	if( bSet )
	{
		pLink = pConnector1->GetSharingLink( pConnector2 );
		// The two connectors must share a link.
		if( pLink == 0 && ( !pConnector1->IsAnchor() || !pConnector2->IsAnchor() ) )
			return false;
	}
	else if( m_pSlideLimits[0] != 0 && m_pSlideLimits[1] != 0 )
		pLink = m_pSlideLimits[0]->GetSharingLink( m_pSlideLimits[1] );

	if( pLink != 0 )
		pLink->SlideBetween( this, pConnector1, pConnector2 );

	m_pSlideLimits[0] = pConnector1;
	m_pSlideLimits[1] = pConnector2;

	return true;
}

bool ConnectorList::Iterate( ConnectorListOperation &Operation )
{
	POSITION Position = GetHeadPosition();
	bool bFirst = true;
	while( Position != 0 )
	{
		CConnector* pConnector = GetNext( Position );
		if( pConnector == 0 )
			continue;
		if( !Operation( pConnector, bFirst, pConnector == 0 ) )
			return false;
		bFirst = false;
	}
	return true;
}

CLink * CConnector::GetSharingLink( CConnector *pOtherConnector )
{
	POSITION Position = m_Links.GetHeadPosition();
	for( int Counter = 0; Position != NULL; ++Counter )
	{
		CLink* pLink = m_Links.GetNext( Position );
		if( pLink == 0 )
			continue;
		if( pOtherConnector->HasLink( pLink ) )
			return pLink;
	}

	return 0;
}

bool CConnector::IsSharingLink( CConnector *pOtherConnector )
{
	if( GetSharingLink( pOtherConnector ) != 0 )
		return true;

	return IsAnchor() && pOtherConnector->IsAnchor();
}

bool CConnector::HasLink( CLink *pLink )
{
	POSITION Position = m_Links.GetHeadPosition();
	while( Position != 0 )
	{
		POSITION RemovePosition = Position;
		CLink *pCheckLink = m_Links.GetNext( Position );
		if( pCheckLink != 0 && pLink == pCheckLink )
			return true;
	}
	return false;
}

bool CConnector::IsAlone( void )
{
	if( m_Links.GetCount() > 1 )
		return false;

	POSITION Position = m_Links.GetHeadPosition();
	if( Position != 0 )
	{
		CLink *pCheckLink = m_Links.GetNext( Position );
		if( pCheckLink->GetConnectorCount() == 1 )
			return true;
	}
	return false;
}

bool ConnectorList::Remove( class CConnector *pConnector )
{
	POSITION Position = GetHeadPosition();
	while( Position != 0 )
	{
		POSITION RemovePosition = Position;
		CConnector* pCheckConnector = GetNext( Position );
		if( pCheckConnector != 0 && ( pConnector == 0 || pCheckConnector == pConnector  ) )
		{
			RemoveAt( RemovePosition );
			return true;
		}
	}
	return false;
}

class CConnector * ConnectorList::Find( class CConnector *pConnector )
{
	POSITION Position = GetHeadPosition();
	while( Position != 0 )
	{
		POSITION RemovePosition = Position;
		CConnector* pCheckConnector = GetNext( Position );
		if( pCheckConnector != 0 && ( pConnector == 0 || pCheckConnector == pConnector  ) )
			return pConnector;
	}
	return 0;
}

bool CConnector::GetSlideLimits( CFPoint &Point1, CFPoint &Point2 )
{
	if(  m_pSlideLimits[0] != 0 )
		Point1 = m_pSlideLimits[0]->GetPoint();
	if(  m_pSlideLimits[1] != 0 )
		Point2 = m_pSlideLimits[1]->GetPoint();
	return IsSlider() && m_pSlideLimits[0] != 0;
}

CFPoint CConnector::GetTempPoint( void )
{
	return IsAnchor() ? m_OriginalPoint : m_TempPoint;
}

bool CConnector::GetSliderArc( CFArc &TheArc )
{
	return GetSliderArc( TheArc, false, false );
}

bool CConnector::GetOriginalSliderArc( CFArc &TheArc )
{
	return GetSliderArc( TheArc, true, false );
}

bool CConnector::GetTempSliderArc( CFArc &TheArc )
{
	return GetSliderArc( TheArc, false, true );
}

bool CConnector::GetSliderArc( CFArc &TheArc, bool bGetOriginal, bool bGetTemp )
{
	if( GetSlideRadius() == 0 )
		return false;

	ASSERT( !bGetOriginal || !bGetTemp );

	CConnector *pLimit1;
	CConnector *pLimit2;

	if( !GetSlideLimits( pLimit1, pLimit2 ) )
		return false;

	CFPoint Point1 = bGetOriginal ? pLimit1->GetOriginalPoint() : ( bGetTemp ? pLimit1->GetTempPoint() : pLimit1->GetPoint() );
	CFPoint Point2 = bGetOriginal ? pLimit2->GetOriginalPoint() : ( bGetTemp ? pLimit2->GetTempPoint() : pLimit2->GetPoint() );

	CFLine Line( Point1, Point2 );
	CFLine Perpendicular;
	double aLength = Line.GetLength() / 2;
	double Radius = GetSlideRadius();
	double bLength = sqrt( ( Radius * Radius ) - ( aLength * aLength ) );
	Line.SetLength( aLength );
	Line.PerpendicularLine( Perpendicular, Radius > 0 ? bLength : -bLength );

	if( Radius > 0 )
		TheArc.SetArc( Perpendicular.GetEnd(), Radius, Point1, Point2 );
	else
		TheArc.SetArc( Perpendicular.GetEnd(), Radius, Point2, Point1 );

	return true;
}

double CConnector::GetSlideRadius( void )
{
	if( m_SlideRadius == 0 )
		return 0;

	double sign = _copysign( 1, m_SlideRadius );

	CConnector *pLimit1;
	CConnector *pLimit2;
	if( !GetSlideLimits( pLimit1, pLimit2 ) )
		return 0;
	return sign * max( fabs( m_SlideRadius ), ( Distance( pLimit1->GetPoint(), pLimit2->GetPoint() ) / 2 ) + 0.0009 );
}

double CConnector::GetOriginalSlideRadius( void )
{
	if( m_OriginalSlideRadius == 0 )
		return 0;

	double sign = _copysign( 1, m_SlideRadius );

	CConnector *pLimit1;
	CConnector *pLimit2;
	if( !GetSlideLimits( pLimit1, pLimit2 ) )
		return 0;
	return sign * max( fabs( m_OriginalSlideRadius ), ( Distance( pLimit1->GetOriginalPoint(), pLimit2->GetOriginalPoint() ) / 2 ) + 0.0009 );
}
