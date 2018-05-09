#pragma once

#include "NullableColor.h"
#include "ControlKnob.h"

class CElementItem
{
	public:

	CElementItem() { m_ElementType = NONE; pElement = 0; }
	CElementItem( class CLink *pLink ) { m_ElementType = LINK; pElement = (class CElement*)pLink; }
	CElementItem( class CConnector *pConnector ) { m_ElementType = CONNECTOR; pElement = (class CElement*)pConnector; }

	class CLink *GetLink( void ) { return m_ElementType == LINK ? (CLink*)pElement : 0; }
	class CConnector *GetConnector( void ) { return m_ElementType == CONNECTOR ? (CConnector*)pElement : 0; }
	class CElement *GetElement( void ) { return pElement; }

	private:

	enum { CONNECTOR, LINK, NONE } m_ElementType;
	class CElement *pElement;
};

class ElementList : public CList< class CElementItem*, class CElementItem* >
{
	public:
	
	ElementList() {}
	~ElementList() {}
	
	bool Remove( class CElement *pElement )
	{
		POSITION Position = GetHeadPosition();
		while( Position != 0 )
		{
			POSITION RemovePosition = Position;
			CElementItem* pCheckItem = GetNext( Position );
			if( pCheckItem == 0 )
				continue;
			
			if( pCheckItem->GetElement() == pElement )
			{
				RemoveAt( RemovePosition );
				return true;
			}
		}
		return false;
	}
};

class CElement
{
	public:

	CElement(void);
	CElement( const CElement &ExistingElement );
	virtual ~CElement(void);

	void SetName( const char *pName ) { m_Name = pName; }
	CString &GetName( void ) { return m_Name; }

	bool IsSelected( void ) { return m_bSelected; }
	void Select( bool bSelected ) { m_bSelected = bSelected; }

	void SetIdentifier( int Value ) { m_Identifier = Value; }
	int GetIdentifier( void ) { return m_Identifier; }
	virtual CString GetIdentifierString( bool bDebugging ) = 0;
	virtual CString GetTypeString( void );

	void SetLayers( unsigned int Layers ) { m_Layers = Layers; }
	unsigned int GetLayers( void ) { return m_Layers; }
	bool IsOnLayers( int Layers ) { return ( m_Layers & Layers ) != 0; }
	void SetMeasurementElement( bool bSet, bool bUseOffset ) { m_bMeasurementElement = bSet; m_bMeasurementElementOffset = bUseOffset; }
	bool IsMeasurementElement( bool *pbUseOffset = 0 ) 
		{ if( pbUseOffset != 0 ) { *pbUseOffset = m_bMeasurementElementOffset; } return m_bMeasurementElement; }

	void SetPositionValid( bool bSet ) { m_bPositionValid = bSet; }
	bool IsPositionValid( void ) { return m_bPositionValid; }

	CNullableColor GetColor( void ) { return m_Color; }
	void SetColor( CNullableColor Color ) { m_Color = Color; }
	void SetUserColor( bool bSet ) { m_bUserColor = bSet; }
	bool IsUserColor( void ) { return m_bUserColor; }

	bool IsLocked( void ) { return m_bLocked; }
	void SetLocked( bool bLocked ) { m_bLocked = bLocked; }

	int GetLineSize( void ) { return m_LineSize; }
	void SetLineSize( int Size ) { m_LineSize = Size <= 0 ? 1 : Size; }

	CElementItem * GetFastenedTo( void ) { return m_FastenedTo.GetElement() == 0 ? 0 : &m_FastenedTo; }
	CLink * GetFastenedToLink( void ) { return m_FastenedTo.GetLink() == 0 ? 0 : m_FastenedTo.GetLink(); }
	CConnector * GetFastenedToConnector( void ) { return m_FastenedTo.GetConnector() == 0 ? 0 : m_FastenedTo.GetConnector(); }
	void UnfastenTo( void );
	void FastenTo( CLink *pFastenLink ) 
		{ m_FastenedTo = pFastenLink; }
	void FastenTo( CConnector *pFastenConnector ) 
		{ m_FastenedTo = pFastenConnector; }

	ElementList* GetFastenedElementList( void ) { return &m_FastenedElements; }
	void AddFastenConnector( class CConnector *pConnector );
	void AddFastenLink( class CLink *pLink );
	void RemoveFastenElement( class CElement *pElement );

	virtual bool IsLink( void ) = 0;
	virtual bool IsConnector( void ) = 0;

	virtual CControlKnob *GetControlKnob( void ) { return &m_ControlKnob; }
	virtual CFPoint GetLocation( void ) = 0;
	virtual void UpdateControlKnob( void ) = 0;
	virtual void UpdateControlKnob( CFPoint Point ) = 0;

	protected:

	int m_Identifier;
	bool m_bSelected;
	unsigned int m_Layers;
	CString m_Name;
	bool m_bMeasurementElement;
	bool m_bPositionValid; // True until there is some sort of binding problem
	CNullableColor m_Color;
	bool m_bUserColor;
	CElementItem m_FastenedTo;
	ElementList m_FastenedElements;
	CControlKnob m_ControlKnob;
	bool m_bLocked;
	int m_LineSize;
	bool m_bMeasurementElementOffset;
};

