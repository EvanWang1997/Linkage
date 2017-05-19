#include "stdafx.h"
#include "Element.h"
#include "linkageDoc.h"

CElement::CElement(void)
{
	m_bSelected = false;
	m_Layers = 0;
	m_bMeasurementElement = false;
	m_bPositionValid = true;
	m_FastenedTo = CElementItem();
	m_bLocked = false;
}

CElement::~CElement(void)
{
}

CElement::CElement( const CElement &ExistingElement )
{
	m_Identifier = -1; // Should never copy and ID but some value needs to be set here.
	m_bSelected = ExistingElement.m_bSelected;
	m_Layers = ExistingElement.m_Layers;
	m_Name = ExistingElement.m_Name;
	m_bMeasurementElement = ExistingElement.m_bMeasurementElement;
	m_bPositionValid = ExistingElement.m_bPositionValid;
	m_Color = ExistingElement.m_Color;
	m_FastenedTo = CElementItem();
	m_bLocked = ExistingElement.m_bLocked;
	// The fastening is not copied because that seems just wrong - someother element can't be fastened to more than one element at a time.
}

void CElement::AddFastenConnector( class CConnector *pConnector )
{
	m_FastenedElements.AddTail( new CElementItem( pConnector ) );
}

void CElement::AddFastenLink( class CLink *pLink )
{
	m_FastenedElements.AddTail( new CElementItem( pLink ) );
}

void CElement::RemoveFastenElement( class CElement *pElement )
{
	POSITION Position = m_FastenedElements.GetHeadPosition();
	while( Position != 0 )
	{
		POSITION DeletePosition = Position;
		CElementItem *pItem = m_FastenedElements.GetNext( Position );
		if( pItem == 0  )
			continue;

		if( pItem->GetElement() == pElement || pElement == 0 )
		{
			m_FastenedElements.RemoveAt( DeletePosition );

//			pItem->GetElement()->RemoveFastenElement( this );
			if( pItem->GetElement()->GetFastenedTo() != 0 && this == pItem->GetElement()->GetFastenedTo()->GetElement() )
				pItem->GetElement()->UnfastenTo();

			break;
		}
	}
}

void CElement::UnfastenTo( void )
{
	CElementItem *pItem = GetFastenedTo();
	if( pItem != 0 && pItem->GetElement() != 0 )
		pItem->GetElement()->RemoveFastenElement( this );
	m_FastenedTo = CElementItem();
}

CString CElement::GetTypeString( void )
{
	bool bDrawingLayer = GetLayers() & CLinkageDoc::DRAWINGLAYER;
	if( IsMeasurementElement() )
		return "Measurement";

	if( IsConnector() )
	{
		if( bDrawingLayer )
		{
			if( ((CConnector*)this)->GetDrawCircleRadius() > 0 )
				return "Circle";
			else
				return "Point";
		}
		else if( ((CConnector*)this)->IsAnchor() )
			return "Anchor";
		else
			return "Connector";
	}
	else
	{
		if( bDrawingLayer )
			return "Polyline";
		else if( ((CLink*)this)->IsActuator() )
			return "Actuator";
		else if( ((CLink*)this)->IsGear() )
			return "Gear";
		else
			return "Link";
	}
}


