#include "stdafx.h"
#include "helper.h"

int GetxAty( int y, int x1, int y1, int x2, int y2, bool* pbValid )
	{
	if( pbValid != NULL )
		*pbValid = true;
	if( y1 == y2 )
		{
		if( y == y1 )
			return x1;
		else
			{
			if( pbValid != NULL )
				*pbValid = false;
			return 0;
			}
		}
	return x2 + (int)( (double)( x1 - x2 ) * ( (double)( y - y2 ) / (double)( y1 - y2 ) ) );
	}

int GetyAtx( int x, int x1, int y1, int x2, int y2, bool* pbValid )
	{
	if( pbValid != NULL )
		*pbValid = true;
	if( x1 == x2 )
		{
		if( x == x1 )
			return y1;
		else
			{
			if( pbValid != NULL )
				*pbValid = false;
			return 0;
			}
		}
	return y2 + (int)( (double)( y1 - y2 ) * ( (double)( x - x2 ) / (double)( x1 - x2 ) ) );
	}

double ConvertToSeconds( const char *pHMSString )
{
	double Value1 = 0;
	double Value2 = 0;
	double Value3 = 0;
	char Dummy = 0;

	int Count = sscanf_s( pHMSString, "%lf:%lf:%lf%c", &Value1, &Value2, &Value3, &Dummy, 1 );

	switch( Count )
	{
	case 1: return Value1;
	case 2: return ( Value1 * 60.0 ) + Value2;
	case 3: return ( Value1 * 3600.0 ) + ( Value2 * 60.0 ) + Value3;
	default: return 0.0;
	}
	return 0.0;
}


