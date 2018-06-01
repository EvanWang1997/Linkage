#include "stdafx.h"
#include "geometry.h"
#include "link.h"
#include "connector.h"
#include "Renderer.h"
#include "DebugItem.h"

#define X	0
#define Y	1

extern CDebugItemList DebugItemList;

class CFFlaggablePoint
{
public:
	_inline CFFlaggablePoint( double xIn = 0, double yIn = 0, bool bFlagIn = false ) { x = xIn; y = yIn; bFlag = bFlagIn; };
	_inline CFFlaggablePoint( CFPoint &Point ) { x = Point.x; y = Point.y; bFlag = false; };
	//_inline void operator=( const CFPoint &Point )	{ x = Point.x; y = Point.y; }
	//_inline void operator=( const CFFlaggablePoint &Point )	{ x = Point.x; y = Point.y; bFlag = Point.bFlag; }

	double x;
	double y;
	bool bFlag;
};

CFFlaggablePoint* pSortPoints = 0; // Need a global for accessing in the sort function
int Graham( CFFlaggablePoint* pPoints, CFFlaggablePoint* pStack, int Count );
int Squash( CFFlaggablePoint* pPoints, int Count );
int Compare( const void *tp1, const void *tp2 );
void MoveLowest( CFFlaggablePoint* pPoints, int Count );
int AreaSign( CFFlaggablePoint a, CFFlaggablePoint b, CFFlaggablePoint c );
bool Left( CFFlaggablePoint a, CFFlaggablePoint b, CFFlaggablePoint c );

CFPoint* GetHull( ConnectorList* pConnectors, int &ReturnedPointCount, bool bUseOriginalPoints )
{
	POSITION Position;
	int Counter;

	int Count = (int)pConnectors->GetCount();
	if( Count == 0 )
		return 0;

	/*
	* With two or less points, there will be no points that are not part
	* of the hull. Return the points as-is in this case.
	*/

	if( Count <= 2 )
	{
		ReturnedPointCount = Count;
		CFPoint* ReturnArray = new CFPoint[2];
		Position = pConnectors->GetHeadPosition();
		for( Counter = 0; Position != NULL && Counter < Count; Counter++ )
		{
			CConnector* pConnector = pConnectors->GetNext( Position );
			if( bUseOriginalPoints )
			{
				ReturnArray[Counter].x = pConnector->GetOriginalPoint().x;
				ReturnArray[Counter].y = pConnector->GetOriginalPoint().y;
			}
			else
			{
				ReturnArray[Counter].x = pConnector->GetPoint().x;
				ReturnArray[Counter].y = pConnector->GetPoint().y;
			}
		}
		return ReturnArray;
	}

	CFFlaggablePoint* PointArray = new CFFlaggablePoint[Count];
	if( PointArray == NULL )
		return 0;

	Position = pConnectors->GetHeadPosition();
	for( Counter = 0; Position != NULL && Counter < Count; Counter++ )
	{
		PointArray[Counter].bFlag = false;
		CConnector* pConnector = pConnectors->GetNext( Position );
		if( bUseOriginalPoints )
		{
			PointArray[Counter].x = pConnector->GetOriginalPoint().x;
			PointArray[Counter].y = pConnector->GetOriginalPoint().y;
		}
		else
		{
			PointArray[Counter].x = pConnector->GetPoint().x;
			PointArray[Counter].y = pConnector->GetPoint().y;
		}
	}

	MoveLowest( PointArray, Count );

	pSortPoints = PointArray;

	qsort(
		&PointArray[1],		/* pointer to 1st elem */
		Count-1,			/* number of elems */
		sizeof( CFFlaggablePoint ),	/* size of each elem */
		Compare				/* -1,0,+1 compare function */
	);

	Count = Squash( PointArray, Count );

	CFPoint *OutputPoints = 0;

	if( Count == 1 )
	{
		CFPoint *OutputPoints = new CFPoint[1];
		if( OutputPoints == 0 )
		{
			delete [] PointArray;
			PointArray = 0;
			return 0;
		}
		OutputPoints[0].x = PointArray[0].x;
		OutputPoints[0].y = PointArray[0].y;
		ReturnedPointCount = 1;
	}
	else
	{
		CFFlaggablePoint* PointStack = new CFFlaggablePoint[Count];
		if( PointStack == 0 )
		{
			delete [] PointArray;
			return 0;
		}

		ReturnedPointCount = Graham( PointArray, PointStack, Count );
		OutputPoints = new CFPoint[ReturnedPointCount];
		if( PointStack == 0 )
		{
			delete [] PointStack;
			delete [] PointArray;
			return 0;
		}
		for( int Index = 0; Index < ReturnedPointCount; ++Index )
		{
			OutputPoints[Index].x = PointStack[Index].x;
			OutputPoints[Index].y = PointStack[Index].y;
		}
		delete [] PointStack;
		PointStack = 0;
	}

	delete [] PointArray;
	PointArray = 0;

	return OutputPoints;
}

/*---------------------------------------------------------------------
FindLowest finds the rightmost lowest point and swaps with 0-th.
The lowest point has the min y-coord, and amongst those, the
max x-coord: so it is rightmost among the lowest.
---------------------------------------------------------------------*/
void MoveLowest( CFFlaggablePoint* pPoints, int Count )
{
	int Counter;
	int Lowest = 0;
	CFFlaggablePoint Temp;

	for( Counter = 1; Counter < Count; Counter++ )
	{
		if( (pPoints[Counter].y < pPoints[Lowest].y) ||
			( (pPoints[Counter].y == pPoints[Lowest].y) && (pPoints[Counter].x < pPoints[Lowest].x) ) )
			Lowest = Counter;
	}
	if( Lowest == 0 )
		return;

	Temp = pPoints[0];
	pPoints[0] = pPoints[Lowest];
	pPoints[Lowest] = Temp;
}

/*---------------------------------------------------------------------
Compare: returns -1,0,+1 if p1 < p2, =, or > respectively;
here "<" means smaller angle.  Follows the conventions of qsort.
---------------------------------------------------------------------*/
int Compare( const void* tpi, const void* tpj )
{
	int a;			  /* area */
	double x, y;		  /* projections of ri & rj in 1st quadrant */
	CFFlaggablePoint* pi;
	CFFlaggablePoint* pj;
	pi = (CFFlaggablePoint*)tpi;
	pj = (CFFlaggablePoint*)tpj;

	a = AreaSign( pSortPoints[0], *pi, *pj );

	if( a > 0 )
		return -1;
	else if( a < 0 )
		return 1;
	else
	{ 
		/*
		 * pi and pj are Collinear with P[0].
		 * Flag the closer one to be deleted later.
		 * Order it so the further point is the lower angle.
		 */

		x = fabs( pi->x - pSortPoints[0].x ) - fabs( pj->x - pSortPoints[0].x );
		y = fabs( pi->y - pSortPoints[0].y ) - fabs( pj->y - pSortPoints[0].y );

		if( (x < 0) || (y < 0) )
		{
			pi->bFlag = true;
			return -1;
		}
		else if( (x > 0) || (y > 0) )
		{
			pj->bFlag = true;
			return 1;
		}
		else
		{ /* points are coincident */
			pj->bFlag = true;
			return 0;
		}
	}
}

int Graham( CFFlaggablePoint* pPoints, CFFlaggablePoint* pStack, int Count )
{
	int Counter;
	int Top;

	// Initialize stack.
	// Note that top is the index of the top point, not the next location after the top point
	Top = -1;
	pStack[Top+1] = pPoints[0];
	Top++;
	pStack[Top+1] = pPoints[1];
	Top++;

	// Bottom two Links will never be removed.
	Counter = 2;

	while( Counter < Count )
	{
		if( Left( pStack[Top-1], pStack[Top], pPoints[Counter] ) )
		{
			pStack[Top+1] = pPoints[Counter];
			Top++;
			Counter++;
		}
		else
			Top--;
	}

	return Top + 1;
}

/*---------------------------------------------------------------------
Squash removes all Links from P marked delete.
---------------------------------------------------------------------*/
int Squash( CFFlaggablePoint* pPoints, int Count )
{
	int Counter = 0;
	int j = 0;
	for( j = 0, Counter = 0; Counter < Count; )
	{
		if( !pPoints[Counter].bFlag )
		{
			pPoints[j].x = pPoints[Counter].x;
			pPoints[j].y = pPoints[Counter].y;
			j++;
		}
		Counter++;
	}
	return j;
}

/*---------------------------------------------------------------------
Returns twice the signed area of the triangle determined by a,b,c.
The area is positive if a,b,c are oriented ccw, negative if cw,
and zero if the points are collinear.
---------------------------------------------------------------------*/
double Area2( CFFlaggablePoint a, CFFlaggablePoint b, CFFlaggablePoint c )
{
	return (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
}

/*---------------------------------------------------------------------
Returns true iff c is strictly to the left of the directed
line through a to b.
---------------------------------------------------------------------*/
bool Left( CFFlaggablePoint a, CFFlaggablePoint b, CFFlaggablePoint c )
{
	return  Area2( a, b, c ) >= 0;
}

int AreaSign( CFFlaggablePoint a, CFFlaggablePoint b, CFFlaggablePoint c )
{
	double area2;

	area2 = ( b.x - a.x ) * ( c.y - a.y ) - ( c.x - a.x ) * ( b.y - a.y );

	/* The area should be an integer. */
	if ( area2 > 0.000000001 ) 
		return  1;
	else if ( area2 < -0.000000001 ) 
		return -1;
	else					 
		return  0;
}

