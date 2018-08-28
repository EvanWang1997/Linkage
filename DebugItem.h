#pragma once

#include "geometry.h"

#define DEFAULT_COLOR RGB( 255, 0, 0 )

class CDebugItem
{
	public:
	CDebugItem( const CFPoint &Point, COLORREF Color = DEFAULT_COLOR ) 
	{ 
		m_Type = DEBUG_OBJECT_POINT;
		m_Point = Point;
		m_Color = Color;
	}
	CDebugItem( const CFLine &Line, COLORREF Color = DEFAULT_COLOR ) 
	{ 
		m_Type = DEBUG_OBJECT_LINE;
		m_Line = Line;
		m_Color = Color;
	}
	CDebugItem( const CFCircle &Circle, COLORREF Color = DEFAULT_COLOR ) 
	{ 
		m_Type = DEBUG_OBJECT_CIRCLE;
		m_Circle = Circle;
		m_Color = Color;
	}
	CDebugItem( const CFArc &Arc, COLORREF Color = DEFAULT_COLOR ) 
	{ 
		m_Type = DEBUG_OBJECT_ARC;
		m_Arc = Arc;
		m_Color = Color;
	}
	~CDebugItem() {}

	enum _Type { DEBUG_OBJECT_POINT, DEBUG_OBJECT_LINE, DEBUG_OBJECT_CIRCLE, DEBUG_OBJECT_ARC } m_Type;
	CFPoint m_Point;
	CFLine m_Line;
	CFCircle m_Circle;
	CFArc m_Arc;
	COLORREF m_Color;
};

class CDebugItemList : public CList< class CDebugItem*, class CDebugItem* >
{
	public:
	
	CDebugItemList() {}
	~CDebugItemList() {}

	CDebugItem *Pop( void ) { return GetHeadPosition() == 0 ? 0 : RemoveHead(); }
	void Push( CDebugItem *pItem ) { AddHead( pItem ); }

	void Clear( void )
	{
		while( true )
		{
			CDebugItem *pDebugItem = Pop();
			if( pDebugItem == 0 )
				break;
			delete pDebugItem;
		}
	}
};


