// LinkageDoc.cpp : implementation of the CLinkageDoc class
//

#include "stdafx.h"
#include "Linkage.h"
#include "afxcoll.h"
#include "geometry.h"
#include "link.h"
#include "connector.h"
#include "LinkageDoc.h"
#include "Linkageview.h"
#include "quickxml.h"
#include "examples_xml.h"
#include "bitarray.h"
#include "SampleGallery.h"
#include "DebugItem.h"
#include "colors.h"
#include <algorithm>
#include <vector>

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const char *CRLF = "\r\n";

static const int DEFAULT_RPM = 15;
static const int MAX_UNDO = 300;
static const int INTERMEDIATE_STEPS = 2;

/////////////////////////////////////////////////////////////////////////////
// CLinkageDoc

IMPLEMENT_DYNCREATE(CLinkageDoc, CDocument)

BEGIN_MESSAGE_MAP(CLinkageDoc, CDocument)
	//{{AFX_MSG_MAP(CLinkageDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	//ON_COMMAND_RANGE( ID_SAMPLE_SIMPLE, ID_SAMPLE_UNUSED19, OnSelectSample )

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLinkageDoc construction/destruction

CString CLinkageDoc::GetUnitsString( CLinkageDoc::_Units Units, bool bShortVersion )
{
	switch( Units )
	{
		case CLinkageDoc::INCH: return CString( "Inches" ); break;
		case CLinkageDoc::MM:
		default: return bShortVersion ? "MM" : "Millimeters"; break;
	}
}

CLinkageDoc::_Units CLinkageDoc::GetUnitsValue( const char *pUnits )
{
	CString Units = pUnits;
	Units.MakeUpper();
	if( Units == "MILLIMETERS" )
		return CLinkageDoc::MM;
	else if( Units == "INCHES" )
		return CLinkageDoc::INCH;
	else
		return CLinkageDoc::MM;
}

bool CLinkageDoc::IsEmpty( void )
{
	return m_Connectors.GetCount() == 0;
}

void CLinkageDoc::SetLinkConnector( CLink* pLink, CConnector* pConnector )
{
	if( pLink == 0 || pConnector == 0 )
		return;

	pLink->AddConnector( pConnector );
	pConnector->AddLink( pLink );
}

CLinkageDoc::CLinkageDoc()
{
	m_pCapturedConnector = 0;
	m_pCapturedConrolKnob = 0;

	m_bSelectionMakeAnchor = false;
	m_bSelectionConnectable = false;
	m_bSelectionCombinable = false;
	m_bSelectionJoinable = false;
	m_bSelectionSlideable = false;
	m_bSelectionSplittable = false;
	m_bSelectionTriangle = false;
	m_bSelectionAngleable = false;
	m_bSelectionRectangle = false;
	m_bSelectionLineable = false;
	m_bSelectionFastenable = false;
	m_bSelectionUnfastenable = false;
	m_bSelectionLockable = false;
	m_bSelectionMeshable = false;
	m_bSelectionPositionable = false;
	m_bSelectionLengthable = false;
	m_bSelectionRotatable = false;
	m_bSelectionScalable = false;
	m_bSelectionMeetable = false;

	m_bSelectionPoint = false;

	m_AlignConnectorCount = 0;
	m_HighestConnectorID = -1;
	m_HighestLinkID = -1;
	m_UnitScaling = 1;
	m_SelectedLayers = 0;
	m_bUseGrid = false;

	SetViewLayers( ALLLAYERS );
	SetEditLayers( ALLLAYERS );

	SetUnits( CLinkageDoc::MM );
	m_ScaleFactor = 1.0;

	m_BackgroundTransparency = 0.0;
	m_BackgroundImageData = "";
}

CLinkageDoc::~CLinkageDoc()
{
	DeleteContents();
}

BOOL CLinkageDoc::OnNewDocument()
{
	ClearDocument();

	// Insert an anchor and then unselect it.
	InsertAnchor( MECHANISMLAYER, 1, CFPoint( 0, 0 ), true, false );
	SelectElement();
	SetModifiedFlag( false );

	// The op added to the undo stack so remove it now.
	for( std::deque<CMemorySaveRecord*>::iterator it = m_UndoStack.begin(); it != m_UndoStack.end(); ++it )
		delete *it;
	m_UndoStack.clear();

	return TRUE;
}

void CLinkageDoc::ClearDocument( void )
{
	DeleteContents(); // the framework probably called this already, but in case this function gets used outside of the framework, call it now.

	if (!CDocument::OnNewDocument())
		return;

	m_SelectedConnectors.RemoveAll();
	m_SelectedLinks.RemoveAll();

	m_pCapturedConnector = 0;
	m_pCapturedConrolKnob = 0;
	m_bUseGrid = false;

	m_bSelectionMakeAnchor = false;
	m_bSelectionConnectable = false;
	m_bSelectionCombinable = false;
	m_bSelectionJoinable = false;
	m_bSelectionSlideable = false;
	m_bSelectionSplittable = false;
	m_bSelectionTriangle = false;
	m_bSelectionAngleable = false;
	m_bSelectionRectangle = false;
	m_bSelectionLineable = false;
	m_bSelectionFastenable = false;
	m_bSelectionUnfastenable = false;
	m_bSelectionLockable = false;
	m_bSelectionMeshable = false;
	m_bSelectionPositionable = false;
	m_bSelectionLengthable = false;
	m_bSelectionRotatable = false;
	m_bSelectionScalable = false;

	m_bSelectionPoint = false;

	m_AlignConnectorCount = 0;
	m_HighestConnectorID = -1;
	m_HighestLinkID = -1;

	CFrameWnd *pFrame = (CFrameWnd*)AfxGetMainWnd();
	CLinkageView *pView = pFrame == 0 ? 0 : (CLinkageView*)pFrame->GetActiveView();
	if( pView != 0 )
	{
		pView->SetOffset( CPoint( 0, 0 ) );
		pView->SetZoom( 1 );
	}

	m_BackgroundTransparency = 0.0;
	m_BackgroundImageData = "";
}

/////////////////////////////////////////////////////////////////////////////
// CLinkageDoc serialization

CConnector *CLinkageDoc::FindConnector( int ID )
{
	POSITION Position = m_Connectors.GetHeadPosition();
	while( Position != 0 )
	{
		CConnector *pConnector = m_Connectors.GetNext( Position );
		if( pConnector != 0 && pConnector->GetIdentifier() == ID )
			return pConnector;
	}
	return 0;
}

CLink *CLinkageDoc::FindLink( int ID )
{
	POSITION Position = m_Links.GetHeadPosition();
	while( Position != 0 )
	{
		CLink *pLink = m_Links.GetNext( Position );
		if( pLink != 0 && pLink->GetIdentifier() == ID )
			return pLink;
	}
	return 0;
}

static void AppendXMLAttribute( CString &String, const char *pName, const char *pValue, bool bIsSet = true )
{
	if( String.GetLength() < 2 || !bIsSet )
		return;

	if( String[String.GetLength()-1] == '>' )
		return;

	if( String[String.GetLength()-1] != ' ' )
		String += " ";

	String += pName;
	String += "=";
	String += "\"";
	String += pValue;
	String += "\"";
}

static void AppendXMLAttribute( CString &String, const char *pName, int Value, bool bIsSet = true )
{
	if( !bIsSet )
		return;
	CString TempString;
	TempString.Format( "%d", Value );
	AppendXMLAttribute( String, pName, (const char*)TempString, bIsSet );
}

static void AppendXMLAttribute( CString &String, const char *pName, unsigned int Value, bool bIsSet = true )
{
	if( !bIsSet )
		return;
	CString TempString;
	TempString.Format( "%u", Value );
	AppendXMLAttribute( String, pName, (const char*)TempString, bIsSet );
}

static void AppendXMLAttribute( CString &String, const char *pName, bool bValue, bool bIsSet = true )
{
	if( !bIsSet )
		return;
	AppendXMLAttribute( String, pName, bValue ? "true" : "false", bIsSet );
}

static void AppendXMLAttribute( CString &String, const char *pName, double Value, bool bIsSet = true )
{
	if( !bIsSet )
		return;
	CString TempString;
	TempString.Format( "%lf", Value );
	AppendXMLAttribute( String, pName, (const char*)TempString, bIsSet );
}

bool CLinkageDoc::ReadIn( CArchive& ar, bool bSelectAll, bool bObeyUnscaleOffset, bool bUseSavedSelection, bool bResetColors, bool bUseBackground )
{
	CWaitCursor Wait;
	int OffsetConnectorIdentifer = m_HighestConnectorID + 1;
	int OffsetLinkIdentifer = m_HighestLinkID + 1;
	unsigned long long Length = ar.GetFile()->GetLength();
	CString Data;
	ar.Read( Data.GetBuffer( (int)Length + 1 ), (unsigned int)Length );
	Data.ReleaseBuffer( (int)Length );

	QuickXMLNode XMLData;
	if( !XMLData.Parse( Data ) )
		return false;

	QuickXMLNode *pRootNode = XMLData.GetFirstChild();
	if( pRootNode == 0 || !pRootNode->IsElement() || pRootNode->GetText() != "linkage2" )
		return false;

	m_SelectedConnectors.RemoveAll();
	m_SelectedLinks.RemoveAll();

	CPoint Offset( 0, 0 );
	double Unscale = 1;
	double ScaleFactor = 1.0;
	CString Units = GetUnitsString( CLinkageDoc::MM );

	CString Value;

	QuickXMLNode *pSelectedNode = 0;

	/*
	 * Get program information first. Skip anything else.
	 */
	for( QuickXMLNode *pNode = pRootNode->GetFirstChild(); pNode != 0; pNode = pNode->GetNextSibling() )
	{
		if( !pNode->IsElement() )
			continue;

		if( pNode->GetText() == "program" )
		{
			Value = pNode->GetAttribute( "zoom" );
			Unscale = atof( Value );
			if( Unscale == 0 )
				Unscale = 1;
			Value = pNode->GetAttribute( "xoffset" );
			Offset.x = atoi( Value );
			Value = pNode->GetAttribute( "yoffset" );
			Offset.y = atoi( Value );
			Value = pNode->GetAttribute( "units" );
			if( !Value.IsEmpty() )
				Units = Value;
			Value = pNode->GetAttribute( "scalefactor" );
			ScaleFactor = atoi( Value );
			if( Value.IsEmpty() || ScaleFactor == 0.0 )
				ScaleFactor = 1.0;
			Value = pNode->GetAttribute( "backgroundtransparency" );
			m_BackgroundTransparency = atof( Value );
			m_bUseGrid = true;
			Value = pNode->GetAttribute( "xgrid" );
			m_UserGrid.x = atof( Value );
			if( Value.IsEmpty() )
				m_bUseGrid = false;
			Value = pNode->GetAttribute( "ygrid" );
			m_UserGrid.y = atof( Value );
			if( Value.IsEmpty() )
				m_bUseGrid = false;
		}

		if( pNode->GetText() == "selected" )
			pSelectedNode = pNode;
	}

	CFPoint UpperLeft( DBL_MAX, DBL_MAX );

	/*
	 * Get connector information. Some data is skipped here because all of
	 * the connectors are needed to handle sliding connector information.
	 */
	for( QuickXMLNode *pNode = pRootNode->GetFirstChild(); pNode != 0; pNode = pNode->GetNextSibling() )
	{
		if( !pNode->IsElement() )
			continue;

		if( pNode->GetText() == "connector" )
		{
			CConnector *pConnector = new CConnector;
			if( pConnector == 0 )
				break;
			CFPoint Point;
			CString Value;
			Value = pNode->GetAttribute( "anchor" );
			pConnector->SetAsAnchor( Value == "true" );
			Value = pNode->GetAttribute( "input" );
			pConnector->SetAsInput( Value == "true" );
			Value = pNode->GetAttribute( "draw" );
			pConnector->SetAsDrawing( Value == "true" );
			Value = pNode->GetAttribute( "rpm" );
			pConnector->SetRPM( Value.IsEmpty() ? DEFAULT_RPM : atof( Value ) );
			Value = pNode->GetAttribute( "x" );
			Point.x = atof( Value );
			Value = pNode->GetAttribute( "y" );
			Point.y = atof( Value );
			pConnector->SetPoint( Point );
			Value = pNode->GetAttribute( "drawcircle" );
			pConnector->SetDrawCircleRadius( atof( Value ) );
			Value = pNode->GetAttribute( "linesize" );
			pConnector->SetLineSize( atoi( Value ) );
			Value = pNode->GetAttribute( "rpm" );
			pConnector->SetRPM( atof( Value ) );
			Value = pNode->GetAttribute( "limitangle" );
			pConnector->SetLimitAngle( atof( Value ) );
			Value = pNode->GetAttribute( "startoffset" );
			pConnector->SetStartOffset( atof( Value ) );
			Value = pNode->GetAttribute( "slideradius" );
			pConnector->SetSlideRadius( atof( Value ) );
			Value = pNode->GetAttribute( "id" );
			pConnector->SetIdentifier( atoi( Value ) + OffsetConnectorIdentifer );
			Value = pNode->GetAttribute( "alwaysmanual" );
			pConnector->SetAlwaysManual( Value == "true" );
			Value = pNode->GetAttribute( "measurementelement" );
			bool bMeasurementElement = Value == "true";
			Value = pNode->GetAttribute( "measurementuseoffset" );
			pConnector->SetMeasurementElement( bMeasurementElement, Value == "true" );
			Value = pNode->GetAttribute( "name" );
			pConnector->SetName( Value );
			Value = pNode->GetAttribute( "locked" );
			pConnector->SetLocked( Value == "true" );
			Value = pNode->GetAttribute( "showaspoint" );
			pConnector->SetShowAsPoint( Value == "true" );
			Value = pNode->GetAttribute( "layer" );
			if( Value.IsEmpty() || atoi( Value ) == 0 )
				pConnector->SetLayers( MECHANISMLAYER );
			else
				pConnector->SetLayers( atoi( Value ) );
			Value = pNode->GetAttribute( "usercolor" );
			bool bUserColor = Value == "true";
			Value = pNode->GetAttribute( "color" );

			if( Value.IsEmpty() || ( bResetColors && !bUserColor ) )
			{
				if( ( pConnector->GetLayers() & DRAWINGLAYER ) != 0 )
					pConnector->SetColor( RGB( 200, 200, 200 ) );
				else
					pConnector->SetColor( Colors[pConnector->GetIdentifier() % COLORS_COUNT] );
			}
			else
				pConnector->SetColor( (COLORREF)atoi( Value ) );
			Value = pNode->GetAttribute( "selected" );
//			if( bUseSavedSelection && Value == "true" )
//				SelectElement( pConnector );
			if( bSelectAll )
				SelectElement( pConnector );
			m_Connectors.AddTail( pConnector );
		}
	}

	/*
	 * Read the link information.
	 */
	for( QuickXMLNode *pNode = pRootNode->GetFirstChild(); pNode != 0; pNode = pNode->GetNextSibling() )
	{
		if( !pNode->IsElement() )
			continue;

		if( pNode->GetText() == "Link" || pNode->GetText() == "element" )
		{
			CLink *pLink = new CLink;
			if( pLink == 0 )
				break;
			CString Value;
			Value = pNode->GetAttribute( "id" );
			pLink->SetIdentifier( atoi( Value ) + OffsetLinkIdentifer );
			Value = pNode->GetAttribute( "linesize" );
			pLink->SetLineSize( atoi( Value ) );
			Value = pNode->GetAttribute( "solid" );
			pLink->SetSolid( Value == "true" );

			Value = pNode->GetAttribute( "polyline" );
			if( Value == "true" )
				pLink->SetShapeType( CLink::HULL );
			else
			{
				Value = pNode->GetAttribute( "shapetype" );
				if( Value == "polygon" )
					pLink->SetShapeType( CLink::POLYGON );
				else if( Value == "polyline" )
					pLink->SetShapeType( CLink::POLYLINE );
				else
					pLink->SetShapeType( CLink::HULL );
			}

			Value = pNode->GetAttribute( "locked" );
			pLink->SetLocked( Value == "true" );
			Value = pNode->GetAttribute( "measurementelement" );
			bool bMeasurementElement = Value == "true";
			Value = pNode->GetAttribute( "measurementuseoffset" );
			pLink->SetMeasurementElement( bMeasurementElement, Value == "true" );
			Value = pNode->GetAttribute( "name" );
			pLink->SetName( Value );
			Value = pNode->GetAttribute( "selected" );
			if( bUseSavedSelection && Value == "true" )
				SelectElement( pLink );
			Value = pNode->GetAttribute( "layer" );
			if( Value.IsEmpty() || atoi( Value ) == 0 )
				pLink->SetLayers( MECHANISMLAYER );
			else
				pLink->SetLayers( atoi( Value ) );

			/*
			 * Loop through the child connector list and find the actual
			 * document connectors that have been created and connect things.
			 */
			for( QuickXMLNode *pConnectorNode = pNode->GetFirstChild(); pConnectorNode != 0; pConnectorNode = pConnectorNode->GetNextSibling() )
			{
				Value = pConnectorNode->GetAttribute( "id" );
				int Identifier = atoi( Value ) + OffsetConnectorIdentifer;
				CConnector *pConnector = FindConnector( Identifier );
				if( pConnector == 0 )
					continue;

				if( pConnectorNode->GetText() == "connector" )
				{
					pLink->AddConnector( pConnector );
					pConnector->AddLink( pLink );
				}
				else if( pConnectorNode->GetText() == "fastenconnector" )
					FastenThese( pConnector, pLink );
			}

			/*
			 * Read the actuator information after the connectors are all set
			 * to avoid any connector count or position problems.
			 */
			Value = pNode->GetAttribute( "actuator" );
			pLink->SetActuator( Value == "true" );
			Value = pNode->GetAttribute( "throw" );
			if( !Value.IsEmpty() )
				pLink->SetStroke( atof( Value ) );
			Value = pNode->GetAttribute( "cpm" );
			pLink->SetCPM( Value.IsEmpty() ? DEFAULT_RPM : atof( Value ) );
			Value = pNode->GetAttribute( "alwaysmanual" );
			pLink->SetAlwaysManual( Value == "true" );
			Value = pNode->GetAttribute( "startoffset" );
			pLink->SetStartOffset( atof( Value ) );
			Value = pNode->GetAttribute( "gear" );
			pLink->SetGear( Value == "true" );
			Value = pNode->GetAttribute( "showangles" );
			bool bShowAngles = Value == "true";
			pLink->SetMeasurementUseAngles( bShowAngles );
			Value = pNode->GetAttribute( "usercolor" );
			bool bUserColor = Value == "true";
			Value = pNode->GetAttribute( "color" );

			if( Value.IsEmpty() || ( bResetColors && !bUserColor ) )
			{
				if( ( pLink->GetLayers() & DRAWINGLAYER ) != 0 )
					pLink->SetColor( RGB( 200, 200, 200 ) );
				else
					pLink->SetColor( Colors[pLink->GetIdentifier() % COLORS_COUNT] );
			}
			else
				pLink->SetColor( (COLORREF)atoi( Value ) );
			if( bSelectAll )
				SelectElement( pLink );
			m_Links.AddTail( pLink );
		}
	}

	for( QuickXMLNode *pNode = pRootNode->GetFirstChild(); pNode != 0; pNode = pNode->GetNextSibling() )
	{
		if( !pNode->IsElement() )
			continue;

		if( pNode->GetText() == "Link" || pNode->GetText() == "element" )
		{
			CString Value;
			Value = pNode->GetAttribute( "id" );
			CLink *pLink = FindLink( atoi( Value ) + OffsetLinkIdentifer );
			if( pLink == 0 )
				break;

			Value = pNode->GetAttribute( "fastenlink" );
			if( !Value.IsEmpty() )
			{
				int FastenToId = atoi( Value ) + OffsetLinkIdentifer;
				CLink *pFastenToLink = FindLink( FastenToId );
				if( pFastenToLink != 0 )
					FastenThese( pLink, pFastenToLink );
			}
			Value = pNode->GetAttribute( "fastenconnector" );
			if( !Value.IsEmpty() )
			{
				int FastenToId = atoi( Value ) + OffsetConnectorIdentifer;
				CConnector *pFastenToConnector = FindConnector( FastenToId );
				if( pFastenToConnector != 0 )
					FastenThese( pLink, pFastenToConnector );
			}
		}
	}

	/*
	 * Find connectors to link fastening and handle it.
	 * Read the sliding connector information and connect the sliders to
	 * the limiting connectors.
	 */
	for( QuickXMLNode *pNode = pRootNode->GetFirstChild(); pNode != 0; pNode = pNode->GetNextSibling() )
	{
		if( !pNode->IsElement() )
			continue;

		if( pNode->GetText() == "connector" )
		{
			Value = pNode->GetAttribute( "id" );
			int TestIdentifier = atoi( Value ) + OffsetConnectorIdentifer;

			/*
			 * Find the connector for this entry that is now part of the
			 * document.
			 */
			CConnector *pConnector = FindConnector( TestIdentifier );
			if( pConnector == 0 )
				break;

			Value = pNode->GetAttribute( "fasten" );
			if( Value.IsEmpty() )
				Value = pNode->GetAttribute( "fastenlink" );
			if( !Value.IsEmpty() )
			{
				int FastenToId = atoi( Value ) + OffsetLinkIdentifer;
				CLink *pFastenToLink = FindLink( FastenToId );
				if( pFastenToLink != 0 )
					FastenThese( pConnector, pFastenToLink );
			}
			Value = pNode->GetAttribute( "fastenconnector" );
			if( !Value.IsEmpty() )
			{
				int FastenToId = atoi( Value ) + OffsetConnectorIdentifer;
				CConnector *pFastenToConnector = FindConnector( FastenToId );
				if( pFastenToConnector != 0 )
					FastenThese( pConnector, pFastenToConnector );
			}

			Value = pNode->GetAttribute( "slider" );
			if( Value != "true" )
				continue;

			/*
			 * Loop through the child connector list and find the actual
			 * document connectors that have been created and connect things.
			 */
			CConnector *pStart = 0;
			CConnector *pEnd = 0;
			for( QuickXMLNode *pLimiterNode = pNode->GetFirstChild(); pLimiterNode != 0; pLimiterNode = pLimiterNode->GetNextSibling() )
			{
				if( pLimiterNode->GetText() != "slidelimit" )
					continue;
				Value = pLimiterNode->GetAttribute( "id" );
				int LimitIdentifier = atoi( Value ) + OffsetConnectorIdentifer;

				CConnector *pLimitConnector = FindConnector( LimitIdentifier );
				if( pLimitConnector != 0 )
				{
					if( pStart == 0 )
						pStart = pLimitConnector;
					else if( pEnd == 0 )
						pEnd = pLimitConnector;
					else
						break;
				}
			}
			if( pEnd == 0 )
				pStart = 0;
			pConnector->SlideBetween( pStart, pEnd );
		}
	}

	for( QuickXMLNode *pNode = pRootNode->GetFirstChild(); pNode != 0; pNode = pNode->GetNextSibling() )
	{
		if( !pNode->IsElement() )
			continue;

		if( pNode->GetText() == "ratios" )
		{
			for( QuickXMLNode *pRatioNode = pNode->GetFirstChild(); pRatioNode != 0; pRatioNode = pRatioNode->GetNextSibling() )
			{
				if( pRatioNode->GetText() != "ratio" )
					continue;

				Value = pRatioNode->GetAttribute( "type" );
				CGearConnection::ConnectionType Type = ( Value == "gears" ? CGearConnection::GEARS : CGearConnection::CHAIN );
				Value = pRatioNode->GetAttribute( "sizeassize" );
				bool bSizeAsSize = Value == "true" && Type == CGearConnection::CHAIN;

				CLink *pLink1 = 0;
				CLink *pLink2 = 0;
				double Size1 = 0;
				double Size2 = 0;
				for( QuickXMLNode *pGearLinkNode = pRatioNode->GetFirstChild(); pGearLinkNode != 0; pGearLinkNode = pGearLinkNode->GetNextSibling() )
				{
					if( pGearLinkNode->GetText() != "link" )
						continue;
					Value = pGearLinkNode->GetAttribute( "id" );
					int GearLinkIdentifier = atoi( Value ) + OffsetLinkIdentifer;
					Value = pGearLinkNode->GetAttribute( "size" );
					double Size = atof( Value );

					CLink *pGearLink = FindLink( GearLinkIdentifier );
					if( pGearLink != 0 )
					{
						if( pLink1 == 0 )
						{
							pLink1 = pGearLink;
							Size1 = Size;
						}
						else
						{
							pLink2 = pGearLink;
							Size2 = Size;
						}
					}
				}

				if( bSizeAsSize )
				{
					// This cancels out unit scaling done inside of the SetGearRatio function so that
					// the value set is the document gear size, not the gear size for display.
					Size1 *= m_UnitScaling;
					Size2 *= m_UnitScaling;
				}

				if( pLink1 != 0 && pLink2 != 0 && Size1 > 0.0 && Size2 > 0.0 )
					SetGearRatio( pLink1, pLink2, Size1, Size2, bSizeAsSize, false, Type, false );
			}
		}
	}

	if( pSelectedNode != 0 && bUseSavedSelection && !bSelectAll )
	{
		// Make a new selected element list that lets the original list be processed back to front.
		std::list<QuickXMLNode*> SelectedElementList; 

		// Select connectors based on the <selected> elements so that the selection is in the same order as when originally selected.
		for( QuickXMLNode *pNode = pSelectedNode->GetFirstChild(); pNode != 0; pNode = pNode->GetNextSibling() )
		{
			if( !pNode->IsElement() )
				continue;

			SelectedElementList.push_front( pNode );
		}

		for( std::list<QuickXMLNode*>::iterator it = SelectedElementList.begin(); it != SelectedElementList.end(); ++it )
		{
			QuickXMLNode* pNode = *it;

			Value = pNode->GetAttribute( "id" );
			int TestIdentifier = atoi( Value ) + OffsetLinkIdentifer;
			if( TestIdentifier < 0 )
				continue;

			if( pNode->GetText() == "connector" )
			{
				POSITION Position = m_Connectors.GetHeadPosition();
				while( Position != 0 )
				{
					CConnector *pConnector = m_Connectors.GetNext( Position );
					if( pConnector == 0 )
						break;
					if( pConnector->GetIdentifier() == TestIdentifier )
						SelectElement( pConnector );
				}
			}
			else if( pNode->GetText() == "link" )
			{
				POSITION Position = m_Links.GetHeadPosition();
				while( Position != 0 )
				{
					CLink *pLink = m_Links.GetNext( Position );
					if( pLink == 0 )
						break;
					if( pLink->GetIdentifier() == TestIdentifier )
						SelectElement( pLink );
				}
			}
		}
	}

	// Detect paste operations or any operation that adds to an existing
	// document. Any addition to an existing document causes the ID's to
	// be adjusted to use the lowest ID's available. Only stand-alone
	// document read operations result in maintaining the original ID's

	bool bChangeIdentifiers = ( OffsetConnectorIdentifer > 0 || OffsetLinkIdentifer > 0 );
	int NewHighConnectorID = m_HighestConnectorID;
	int NewHighLinkID = m_HighestLinkID;

	POSITION Position = m_Connectors.GetHeadPosition();
	while( Position != 0 )
	{
		CConnector *pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 || pConnector->GetIdentifier() < OffsetConnectorIdentifer )
			continue;
		int UseID;
		if( bChangeIdentifiers )
		{
			UseID = m_IdentifiedConnectors.FindAndSetBit();
			pConnector->SetIdentifier( UseID );
		}
		else
		{
			UseID = pConnector->GetIdentifier();
			m_IdentifiedConnectors.SetBit( UseID );
		}
		if( UseID > NewHighConnectorID )
			NewHighConnectorID = UseID;
	}

	Position = m_Links.GetHeadPosition();
	while( Position != 0 )
	{
		CLink *pLink = m_Links.GetNext( Position );
		if( pLink == 0 || pLink->GetIdentifier() < OffsetLinkIdentifer )
			continue;
		int UseID;
		if( bChangeIdentifiers )
		{
			UseID = m_IdentifiedLinks.FindAndSetBit();
			pLink->SetIdentifier( UseID );
		}
		else
		{
			UseID = pLink->GetIdentifier();
			m_IdentifiedLinks.SetBit( UseID );
		}
		if( UseID > NewHighLinkID )
			NewHighLinkID = UseID;
	}

	m_HighestConnectorID = NewHighConnectorID;
	m_HighestLinkID = NewHighLinkID;

	NormalizeConnectorLinks();

	if( bObeyUnscaleOffset )
	{
		//AfxMessageBox( "ObeyUnscaleOffset" );
		CFrameWnd *pFrame = (CFrameWnd*)AfxGetMainWnd();
		CLinkageView *pView = pFrame == 0 ? 0 : (CLinkageView*)pFrame->GetActiveView();
		if( pView != 0 )
		{
			pView->SetOffset( Offset );
			pView->SetZoom( Unscale );
		}

		SetUnits( GetUnitsValue( Units ) );
		m_ScaleFactor = ScaleFactor;
	}

	if( bUseBackground )
	{
		for( QuickXMLNode *pNode = pRootNode->GetFirstChild(); pNode != 0; pNode = pNode->GetNextSibling() )
		{
			if( !pNode->IsElement() )
				continue;

			if( pNode->GetText() == "background" )
			{
				QuickXMLNode *pContentNode = pNode->GetFirstChild();
				if( pContentNode != 0 )
					m_BackgroundImageData = pContentNode->GetText();
				break;
			}
		}
	}

	SetSelectedModifiableCondition();

	return true;
}

bool CLinkageDoc::WriteOut( CArchive& ar, bool bUseBackground, bool bSelectedOnly )
{
	CWaitCursor Wait;

	ar.WriteString( "<linkage2>" );
	ar.WriteString( CRLF );

	CString TempString;

	CFrameWnd *pFrame = (CFrameWnd*)AfxGetMainWnd();
	CLinkageView *pView = pFrame == 0 ? 0 : (CLinkageView*)pFrame->GetActiveView();
	if( pView != 0 )
	{
		CFPoint Point = pView->GetOffset();
		CString Units = GetUnitsString( m_Units );

		TempString = "\t<program";
		AppendXMLAttribute( TempString, "zoom", pView->GetZoom() );
		AppendXMLAttribute( TempString, "xoffset", (int)Point.x );
		AppendXMLAttribute( TempString, "yoffset", (int)Point.y );
		AppendXMLAttribute( TempString, "scalefactor", m_ScaleFactor );
		AppendXMLAttribute( TempString, "units", (const char*)Units );
		AppendXMLAttribute( TempString, "backgroundtransparency", m_BackgroundTransparency );

		if( m_bUseGrid )
		{
			AppendXMLAttribute( TempString, "xgrid", m_UserGrid.x, true );
			AppendXMLAttribute( TempString, "ygrid", m_UserGrid.y, true );
		}
		else
		{
			AppendXMLAttribute( TempString, "xgrid", "", true );
			AppendXMLAttribute( TempString, "ygrid", "", true );
		}

		TempString += "/>";

//		TempString.Format( "\t<program zoom=\"%lf\" xoffset=\"%d\" yoffset=\"%d\" scalefactor=\"%lf\" units=\"%s\" viewlayers=\"%u\" editlayers=\"%u\"/>",
//		                   pView->GetZoom(), (int)Point.x, (int)Point.y, m_ScaleFactor, (const char*)Units,
//						   m_ViewLayers, m_EditLayers );
		ar.WriteString( TempString );
		ar.WriteString( CRLF );
	}

	POSITION Position = m_Connectors.GetHeadPosition();
	while( Position != 0 )
	{
		CConnector* pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 )
			continue;
		if( bSelectedOnly &&!pConnector->IsSelected() && !pConnector->IsLinkSelected() )
			continue;
		CFPoint Point = pConnector->GetOriginalPoint();

		CConnector *pSlideStart;
		CConnector *pSlideEnd;
		bool bSlideLimits = pConnector->IsSlider() && pConnector->GetSlideLimits( pSlideStart, pSlideEnd ) && pSlideStart != 0 && pSlideEnd != 0;

//		CString FastenId;
//		FastenId.Format( "%d", pConnector->GetFastenedTo() == 0 ? 0 : pConnector->GetFastenedTo()->GetIdentifier() );

		TempString = "\t<connector";
		AppendXMLAttribute( TempString, "id", pConnector->GetIdentifier() );
		AppendXMLAttribute( TempString, "selected", pConnector->IsSelected() );
		AppendXMLAttribute( TempString, "name", (const char*)pConnector->GetName(), !pConnector->GetName().IsEmpty() );
		AppendXMLAttribute( TempString, "layer", pConnector->GetLayers() );
		AppendXMLAttribute( TempString, "x", Point.x );
		AppendXMLAttribute( TempString, "y", Point.y );
		AppendXMLAttribute( TempString, "anchor", pConnector->IsAnchor(), pConnector->IsAnchor() );
		AppendXMLAttribute( TempString, "locked", pConnector->IsLocked(), pConnector->IsLocked() && pConnector->IsAnchor() );
		AppendXMLAttribute( TempString, "input", pConnector->IsInput(), pConnector->IsInput() );
		AppendXMLAttribute( TempString, "draw", pConnector->IsDrawing(), pConnector->IsDrawing() );
		bool MeasurementUseOffset = false;
		AppendXMLAttribute( TempString, "measurementelement", pConnector->IsMeasurementElement( &MeasurementUseOffset ), pConnector->IsMeasurementElement() );
		AppendXMLAttribute( TempString, "measurementuseoffset", MeasurementUseOffset, pConnector->IsMeasurementElement() );
		AppendXMLAttribute( TempString, "rpm", pConnector->GetRPM(), pConnector->IsInput() );
		AppendXMLAttribute( TempString, "limitangle", pConnector->GetLimitAngle(), pConnector->GetLimitAngle() > 0 && pConnector->IsInput() );
		AppendXMLAttribute( TempString, "startoffset", pConnector->GetStartOffset(), pConnector->GetLimitAngle() > 0 && pConnector->IsInput() );
		AppendXMLAttribute( TempString, "slider", pConnector->IsSlider(), pConnector->IsSlider() );
		AppendXMLAttribute( TempString, "alwaysmanual", pConnector->IsAlwaysManual(), pConnector->IsAlwaysManual() );
		AppendXMLAttribute( TempString, "drawcircle", pConnector->GetDrawCircleRadius(), pConnector->GetDrawCircleRadius() > 0 );
		AppendXMLAttribute( TempString, "linesize", pConnector->GetLineSize(), pConnector->GetDrawCircleRadius() > 0 );
		bool bIncludeFasten = pConnector->GetFastenedToLink() != 0 && ( pConnector->GetFastenedToLink()->IsSelected() || !bSelectedOnly );
		AppendXMLAttribute( TempString, "fastenlink", pConnector->GetFastenedToLink() == 0 ? 0 : pConnector->GetFastenedToLink()->GetIdentifier(), bIncludeFasten );
		bool bIncludeFastenToCon = pConnector->GetFastenedToConnector() != 0 && ( pConnector->GetFastenedToConnector()->IsSelected() || !bSelectedOnly );
		AppendXMLAttribute( TempString, "fastenconnector", pConnector->GetFastenedToConnector() == 0 ? 0 : pConnector->GetFastenedToConnector()->GetIdentifier(), bIncludeFastenToCon );
		AppendXMLAttribute( TempString, "slideradius", pConnector->GetSlideRadius(), pConnector->GetSlideRadius() != 0 && pConnector->IsSlider() );
		AppendXMLAttribute( TempString, "color", (unsigned int)(COLORREF)pConnector->GetColor() );
		AppendXMLAttribute( TempString, "usercolor", pConnector->IsUserColor() );
		AppendXMLAttribute( TempString, "showaspoint", pConnector->IsShowAsPoint() && pConnector->GetLinkCount() <= 1, pConnector->IsShowAsPoint() && pConnector->GetLinkCount() <= 1 );

		TempString += bSlideLimits ? ">" : "/>";

//		TempString.Format( "\t<connector id=\"%d\" selected=\"%s\" name=\"%s\" layer=\"%d\" anchor=\"%s\" input=\"%s\" draw=\"%s\" "
//						   "measurementelement=\"%s\" rpm=\"%lf\" x=\"%lf\" y=\"%lf\" slider=\"%s\" alwaysmanual=\"%s\" drawcircle=\"%f\" fasten=\"%s\" "
//						   "slideradius=\"%f\" color=\"%d\">",
//						   pConnector->GetIdentifier(), pConnector->IsSelected() ? "true" : "false", (const char*)pConnector->GetName(), pConnector->GetLayers(),
//		                   pConnector->IsAnchor() ? "true" : "false", pConnector->IsInput() ? "true" : "false",  pConnector->IsDrawing() ? "true" : "false",
//						   pConnector->IsMeasurementElement() ? "true" : "false",
//		                   pConnector->GetRPM(), Point.x, Point.y, pConnector->IsSlider() ? "true" : "false", pConnector->IsAlwaysManual() ? "true" : "false",
//						   pConnector->GetDrawCircleRadius(), pConnector->GetFastenedTo() == 0 ? "" : (const char*)FastenId,
//		                   pConnector->GetSlideRadius(), (COLORREF)pConnector->GetColor() );
		ar.WriteString( TempString );
		ar.WriteString( CRLF );

		if( bSlideLimits )
		{
			TempString.Format( "\t\t<slidelimit id=\"%d\"/>", pSlideStart->GetIdentifier() );
			ar.WriteString( TempString );
			ar.WriteString( CRLF );
			TempString.Format( "\t\t<slidelimit id=\"%d\"/>", pSlideEnd->GetIdentifier() );
			ar.WriteString( TempString );
			ar.WriteString( CRLF );

			ar.WriteString( "\t</connector>" );
			ar.WriteString( CRLF );
		}
	}

	Position = m_Links.GetHeadPosition();
	while( Position != 0 )
	{
		CLink* pLink = m_Links.GetNext( Position );
		if( pLink == 0 )
			continue;
		if( bSelectedOnly && !pLink->IsSelected() && !pLink->IsAnySelected() )
			continue;

		/*
		 * We may end up with multiple Links sharing a single connector
		 * while having no other connectors. This is because of the way a
		 * connector can alone be selected. Instead of trying to detect this,
		 * the code that reads data will detect this after things have been
		 * read using the normalizing function. We can ignore the problem
		 * since it only makes the files a tony bit confusing if read by eye.
		 */

//		CString FastenId;
//		FastenId.Format( "%d", pLink->GetFastenedTo() == 0 ? 0 : pLink->GetFastenedTo()->GetIdentifier() );

		TempString = "\t<Link";
		AppendXMLAttribute( TempString, "id", pLink->GetIdentifier() );
		AppendXMLAttribute( TempString, "selected", pLink->IsSelected() );
		AppendXMLAttribute( TempString, "name", (const char*)pLink->GetName(), !pLink->GetName().IsEmpty() );
		AppendXMLAttribute( TempString, "layer", pLink->GetLayers() );
		AppendXMLAttribute( TempString, "linesize", pLink->GetLineSize() );
		AppendXMLAttribute( TempString, "solid", pLink->IsSolid(), pLink->IsSolid() );
		AppendXMLAttribute( TempString, "locked", pLink->IsLocked(), pLink->IsLocked() );
		AppendXMLAttribute( TempString, "actuator", pLink->IsActuator(), pLink->IsActuator() );
		AppendXMLAttribute( TempString, "startoffset", pLink->GetStartOffset(), pLink->IsActuator() );
		AppendXMLAttribute( TempString, "alwaysmanual", pLink->IsAlwaysManual(), pLink->IsAlwaysManual() );
		AppendXMLAttribute( TempString, "measurementelement", pLink->IsMeasurementElement(), pLink->IsMeasurementElement() );
		AppendXMLAttribute( TempString, "throw", pLink->GetStroke(), pLink->IsActuator() );
		AppendXMLAttribute( TempString, "cpm", pLink->GetCPM(), pLink->IsActuator() );
		if( pLink->GetShapeType() == CLink::POLYGON )
			AppendXMLAttribute( TempString, "shapetype", "polygon", true );
		else if( pLink->GetShapeType() == CLink::POLYLINE )
			AppendXMLAttribute( TempString, "shapetype", "polyline", true );
		else
			AppendXMLAttribute( TempString, "shapetype", "hull", true );
		AppendXMLAttribute( TempString, "fastenlink", pLink->GetFastenedToLink() == 0 ? 0 : pLink->GetFastenedToLink()->GetIdentifier(), pLink->GetFastenedToLink() != 0 );
		AppendXMLAttribute( TempString, "fastenconnector", pLink->GetFastenedToConnector() == 0 ? 0 : pLink->GetFastenedToConnector()->GetIdentifier(), pLink->GetFastenedToConnector() != 0 );
		AppendXMLAttribute( TempString, "gear", pLink->IsGear(), pLink->IsGear() );
		AppendXMLAttribute( TempString, "showangles", pLink->IsMeasurementAngles(), pLink->IsMeasurementAngles() && pLink->IsMeasurementElement() );

		AppendXMLAttribute( TempString, "color", (unsigned int)(COLORREF)pLink->GetColor() );
		AppendXMLAttribute( TempString, "usercolor", pLink->IsUserColor() );
		TempString += ">";

//		TempString.Format( "\t<Link id=\"%d\" selected=\"%s\" name=\"%s\" layer=\"%d\" linesize=\"%d\" solid=\"%s\" actuator=\"%s\" alwaysmanual=\"%s\" "
//						   "measurementelement=\"%s\" throw=\"%lf\" cpm=\"%lf\" fasten=\"%s\" gear=\"%s\" color=\"%d\">",
//		                   pLink->GetIdentifier(), pLink->IsSelected() ? "true" : "false", (const char*)pLink->GetName(), pLink->GetLayers(),
//						   pLink->GetLineSize(), pLink->IsSolid() ? "true" : "false",
//		                   pLink->IsActuator() ? "true" : "false", pLink->IsAlwaysManual() ? "true" : "false",
//						   pLink->IsMeasurementElement() ? "true" : "false",
//						   pLink->GetStroke(), pLink->GetCPM(), pLink->GetFastenedTo() == 0 ? "" : (const char*)FastenId,
//						   pLink->IsGear() ? "true" : "false", (COLORREF)pLink->GetColor() );
		ar.WriteString( TempString );
		ar.WriteString( CRLF );

		POSITION Position2 = pLink->GetConnectorList()->GetHeadPosition();
		while( Position2 != 0 )
		{
			CConnector* pConnector = pLink->GetConnectorList()->GetNext( Position2 );
			if( pConnector == 0 )
				continue;
			if( bSelectedOnly )
			{
				if( !pConnector->IsSelected() && !pLink->IsSelected() )
					continue;
			}
			TempString.Format( "\t\t<connector id=\"%d\"/>", pConnector->GetIdentifier() );
			ar.WriteString( TempString );
			ar.WriteString( CRLF );
		}

		ar.WriteString( "\t</Link>" );
		ar.WriteString( CRLF );
	}

	ar.WriteString( "\t<selected>" );
	ar.WriteString( CRLF );

	Position = m_SelectedConnectors.GetHeadPosition();
	while( Position != 0 )
	{
		CConnector* pConnector = m_SelectedConnectors.GetNext( Position );
		if( pConnector != 0 )
		{
			TempString.Format( "\t\t<connector id=\"%d\"/>", pConnector->GetIdentifier() );
			ar.WriteString( TempString );
			ar.WriteString( CRLF );
		}
	}
	Position = m_SelectedLinks.GetHeadPosition();
	while( Position != 0 )
	{
		CLink* pLink = m_SelectedLinks.GetNext( Position );
		if( pLink != 0 )
		{
			TempString.Format( "\t\t<link id=\"%d\"/>", pLink->GetIdentifier() );
			ar.WriteString( TempString );
			ar.WriteString( CRLF );
		}
	}
	ar.WriteString( "\t</selected>" );
	ar.WriteString( CRLF );

	Position = m_GearConnectionList.GetHeadPosition();
	if( Position != 0 )
	{
		ar.WriteString( "\t<ratios>" );
		ar.WriteString( CRLF );
		while( Position != 0 )
		{
			CGearConnection *pGearConnection = m_GearConnectionList.GetNext( Position );
			if( pGearConnection == 0 || pGearConnection->m_pGear1 == 0 || pGearConnection->m_pGear2 == 0 )
				continue;

			bool bGear1Selected = pGearConnection->m_pGear1->IsSelected() || pGearConnection->m_pGear1->IsAnySelected();
			bool bGear2Selected = pGearConnection->m_pGear2->IsSelected() || pGearConnection->m_pGear2->IsAnySelected();
			if( bSelectedOnly && ( !bGear1Selected || !bGear2Selected ) )
				continue;

			TempString.Format( "\t\t<ratio type=\"%s\" sizeassize=\"%s\">", pGearConnection->m_ConnectionType == pGearConnection->GEARS ? "gears" : "chain", pGearConnection->m_bUseSizeAsRadius ? "true" : "false" );
			ar.WriteString( TempString );
			ar.WriteString( CRLF );

			TempString.Format( "\t\t\t<link id=\"%d\" size=\"%lf\"/>", pGearConnection->m_pGear1->GetIdentifier(), pGearConnection->m_Gear1Size );
			ar.WriteString( TempString );
			ar.WriteString( CRLF );
			TempString.Format( "\t\t\t<link id=\"%d\" size=\"%lf\"/>", pGearConnection->m_pGear2->GetIdentifier(), pGearConnection->m_Gear2Size );
			ar.WriteString( TempString );
			ar.WriteString( CRLF );

			ar.WriteString( "\t\t</ratio>" );
			ar.WriteString( CRLF );
		}
		ar.WriteString( "\t</ratios>" );
		ar.WriteString( CRLF );
	}

	if( !bSelectedOnly && bUseBackground )
	{
		ar.WriteString( "\t<background>" );
		if( m_BackgroundImageData.length() > 0 )
			ar.WriteString( m_BackgroundImageData.c_str() );
		ar.WriteString( "</background>" );
		ar.WriteString( CRLF );
	}
	ar.WriteString( "</linkage2>" );
	ar.WriteString( CRLF );

	return true;
}

void CLinkageDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
		WriteOut( ar, true, false );
	else
	{
		UpdateAllViews( 0, 2 );
		ReadIn( ar, false, true, false, false, true );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CLinkageDoc diagnostics

#ifdef _DEBUG
void CLinkageDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CLinkageDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

bool CLinkageDoc::ClearSelection( void )
{
	bool bChanged = false;
	POSITION Position = m_Links.GetHeadPosition();
	while( Position != 0 )
	{
		CLink* pLink = m_Links.GetNext( Position );
		if( pLink == 0 )
			continue;
		if( pLink->IsSelected() )
			bChanged = true;
		pLink->Select( false );
	}
	Position = m_Connectors.GetHeadPosition();
	while( Position != 0 )
	{
		CConnector* pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 )
			continue;
		if( pConnector->IsSelected() )
			bChanged = true;
		pConnector->Select( false );
	}
	m_pCapturedConnector = 0;
	m_pCapturedConrolKnob = 0;
	m_SelectedConnectors.RemoveAll();
	m_SelectedLinks.RemoveAll();

	SetSelectedModifiableCondition();

	return bChanged;
}

bool CLinkageDoc::SelectLinkFromConnectors( void )
{
	if( GetSelectedConnectorCount() <= 1 || GetSelectedLinkCount( false ) > 0 )
		return false;

	CConnector *pConnector1 = GetSelectedConnector( 0 );
	CConnector *pConnector2 = GetSelectedConnector( 1 );

	if( pConnector1 == 0 || pConnector2 == 0 )
		return false;

	POSITION Position1 = pConnector1->GetLinkList()->GetHeadPosition();
	while( Position1 != 0 )
	{
		CLink *pLink1 = pConnector1->GetLinkList()->GetNext( Position1 );
		if( pLink1 == 0 )
			continue;

		POSITION Position2 = pConnector2->GetLinkList()->GetHeadPosition();
		while( Position2 != 0 )
		{
			CLink *pLink2 = pConnector2->GetLinkList()->GetNext( Position2 );
			if( pLink2 == 0 )
				continue;

			if( pLink1 == pLink2 )
			{
				ClearSelection();
				SelectElement( pLink1 );
				return true;
			}
		}
	}
	return false;
}

bool CLinkageDoc::SelectElement( void )
{
	ClearSelection();
	return false;
}

bool CLinkageDoc::SelectElement( CElement *pElement )
{
	if( pElement->IsSelected() )
		return false;

	if( pElement->IsLink() )
		SelectElement( (CLink*)pElement );
	else
		SelectElement( (CConnector*)pElement );

	return true;
}

bool CLinkageDoc::DeSelectElement( CElement *pElement )
{
	if( !pElement->IsSelected() )
		return false;

	if( pElement->IsLink() )
		DeSelectElement( (CLink*)pElement );
	else
		DeSelectElement( (CConnector*)pElement );

	return true;
}

bool CLinkageDoc::SelectElement( CLink *pLink )
{
	if( pLink->IsSelected() )
		return false;

	m_SelectedLinks.AddHead( pLink );

	pLink->Select( true );

	SetSelectedModifiableCondition();

	return true;
}

bool CLinkageDoc::DeSelectElement( CLink *pLink )
{
	if( !pLink->IsSelected() )
		return false;

	if( !m_SelectedLinks.Remove( pLink ) )
		return false;

	pLink->Select( false );

	SetSelectedModifiableCondition();

	return true;
}

bool CLinkageDoc::DeSelectElement( CConnector *pConnector )
{
	if( !pConnector->IsSelected() )
		return false;

	if( !m_SelectedConnectors.Remove( pConnector ) )
		return false;

	pConnector->Select( false );

	if( m_pCapturedConnector == pConnector )
		m_pCapturedConnector = 0;

	SetSelectedModifiableCondition();

	return true;
}

bool CLinkageDoc::SelectElement( CConnector *pConnector )
{
	if( pConnector->IsSelected() )
		return false;

	m_SelectedConnectors.AddHead( pConnector );
	pConnector->Select( true );
	m_pCapturedConnector = pConnector;
	SetSelectedModifiableCondition();

	return true;
}

bool CLinkageDoc::SelectElement( CFRect Rect, double SolidLinkExpansion, bool bMultiSelect, bool &bSelectionChanged )
{
	bSelectionChanged = false;
	if( !bMultiSelect )
		bSelectionChanged = ClearSelection();

	POSITION Position = m_Links.GetHeadPosition();
	while( Position != 0 )
	{
		CLink* pLink = m_Links.GetNext( Position );
		if( pLink == 0 || ( pLink->GetLayers() & m_UsableLayers ) == 0 )
			continue;
		CFRect Area;
		pLink->GetAdjustArea( m_GearConnectionList, Area );
		if( Area.IsInsideOf( Rect ) )
			bSelectionChanged |= SelectElement( pLink );
	}
	Position = m_Connectors.GetHeadPosition();
	while( Position != 0 )
	{
		CConnector* pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 || ( pConnector->GetLayers() & m_UsableLayers ) == 0 )
			continue;
		CFPoint Point = pConnector->GetPoint();
		if( Point.IsInsideOf( Rect ) )
			bSelectionChanged |= SelectElement( pConnector );
	}
	m_pCapturedConnector = 0;
	m_pCapturedConrolKnob = 0;

	SetSelectedModifiableCondition();

	return bSelectionChanged;
}

bool CLinkageDoc::FindElement( CFPoint Point, double GrabDistance, double SolidLinkExpansion, CLink *&pFoundLink, CConnector *&pFoundConnector )
{
	pFoundLink = 0;
	pFoundConnector = 0;

	POSITION Position = m_Connectors.GetHeadPosition();
	while( Position != 0 )
	{
		CConnector* pConnector = m_Connectors.GetNext( Position );
		if( pConnector != 0 && pConnector->PointOnConnector( Point, GrabDistance )
		    && ( pConnector->GetLayers() & m_UsableLayers ) != 0 )
		{
			pFoundConnector = pConnector;
			return true;
		}
	}
	Position = m_Links.GetHeadPosition();
	while( Position != NULL )
	{
		CLink* pLink = m_Links.GetNext( Position );
		if( pLink != 0 && pLink->PointOnLink( m_GearConnectionList, Point, GrabDistance, SolidLinkExpansion )
		    && ( pLink->GetLayers() & m_UsableLayers ) != 0 )
		{
			pFoundLink = pLink;
			return true;
		}
	}
	return false;
}

bool CLinkageDoc::AutoJoinSelected( void )
{
	if( m_SelectedConnectors.GetCount() != 1 || m_SelectedLinks.GetCount() > 0 )
		return false;

	CConnector *pConnector = (CConnector*)m_SelectedConnectors.GetHead();
	if( pConnector == 0 )
		return false;

	CFPoint Point = pConnector->GetPoint();

	POSITION Position = m_Connectors.GetHeadPosition();
	while( Position != NULL )
	{
		CConnector* pCheckConnector = m_Connectors.GetNext( Position );
		if( pCheckConnector == 0 || pCheckConnector == pConnector
		    || ( pConnector->GetLayers() & pCheckConnector->GetLayers() ) == 0 )
			continue;

		//if( pCheckConnector->IsAlone() )
		//	continue;

		CFPoint CheckPoint = pCheckConnector->GetPoint();

		if( Point == CheckPoint )
			SelectElement( pCheckConnector );
	}

	m_bSelectionJoinable = true;
	JoinSelected( false );

	return true;
}

bool CLinkageDoc::SelectElement( CFPoint Point, double GrabDistance, double SolidLinkExpansion, bool bMultiSelect, int SelectionDepth, bool &bSelectionChanged )
{
	CConnector* pSelectingConnector = 0;
	CLink *pSelectingLink = 0;
	CControlKnob *pSelectingControlKnob = 0;

	bSelectionChanged = false;
	bool bClearExistingSelection = true;
	POSITION Position = 0;

	m_bSelectionPoint = false;

	// Check only the already selected items first before checking all items.
	// This makes it easier to drag a pasted item that is selected but not
	// checked first otherwise.

	if( SelectionDepth == 0 )
	{
		Position = m_Connectors.GetHeadPosition();
		while( Position != 0 )
		{
			CConnector* pConnector = m_Connectors.GetNext( Position );
			if( pConnector != 0 && pConnector->IsSelected() )
			{
				int Knobs = 0;
				CControlKnob *pControlKnob = pConnector->GetControlKnobs( Knobs );
				if( !bMultiSelect && Knobs > 0 && pControlKnob != 0 && pControlKnob->PointOnControlKnob( Point, GrabDistance )
					&& pControlKnob->IsShowOnParentSelect()
					&& ( pConnector->GetLayers() & m_UsableLayers ) != 0 )
				{
					pSelectingControlKnob = pControlKnob;
					bClearExistingSelection = false;
					break;
				}
			
				if( pConnector->PointOnConnector( Point, GrabDistance )
					&& ( pConnector->GetLayers() & m_UsableLayers ) != 0 )
				{
					pSelectingConnector = pConnector;
					break;
				}
			}
		}

		if( pSelectingConnector == 0 && pSelectingLink == 0 && pSelectingControlKnob == 0 )
		{
			Position = m_Links.GetHeadPosition();
			while( Position != NULL )
			{
				CLink* pLink = m_Links.GetNext( Position );
				if( pLink != 0 && pLink->IsSelected() )
				{
					int Knobs = 0;
					CControlKnob *pControlKnob = pLink->GetControlKnobs( Knobs );
					if( !bMultiSelect && Knobs > 0 && pControlKnob != 0 && pControlKnob->PointOnControlKnob( Point, GrabDistance )
						&& pControlKnob->IsShowOnParentSelect()
						&& ( pLink->GetLayers() & m_UsableLayers ) != 0 )
					{
						bClearExistingSelection = false;
						pSelectingControlKnob = pControlKnob;
						break;
					}
				
					if( pLink != 0 && pLink->PointOnLink( m_GearConnectionList, Point, GrabDistance, SolidLinkExpansion )
						&& ( pLink->GetLayers() & m_UsableLayers ) != 0 )
					{
						if( pLink->GetConnectorCount() == 1 && !pLink->IsGear() )
							pSelectingConnector = pLink->GetConnector( 0 );
						else
							pSelectingLink = pLink;
						break;
					}
				}
			}
		}
	}
	else
	{
		// Clear previous selections.
		ClearSelection();
	}

	/*
	 * Test for selecting an unselected connector, controller, or link.
	 */

	if( pSelectingConnector == 0 && pSelectingLink == 0 && pSelectingControlKnob == 0 )
	{
		Position = m_Links.GetHeadPosition();
		while( Position != NULL )
		{
			CLink* pLink = m_Links.GetNext( Position );
			if( pLink != NULL )
			{
				CControlKnob *pControlKnob = pLink->GetControlKnob();
				if( !bMultiSelect && pControlKnob != 0 && pControlKnob->PointOnControlKnob( Point, GrabDistance )
				    && !pControlKnob->IsShowOnParentSelect()
				    && ( pLink->GetLayers() & m_UsableLayers ) != 0 )
				{
					if( SelectionDepth == 0 )
					{
						pSelectingControlKnob = pControlKnob;
						break;
					}
					else
						--SelectionDepth;
				}
			}
		}
	}

	if( pSelectingConnector == 0 && pSelectingLink == 0 && pSelectingControlKnob == 0 )
	{
		Position = m_Connectors.GetHeadPosition();
		while( Position != 0 )
		{
			CConnector* pConnector = m_Connectors.GetNext( Position );
			if( pConnector != 0 )
			{
				int Knobs = 0;
				CControlKnob *pControlKnob = pConnector->GetControlKnobs( Knobs );
				if( !bMultiSelect && pControlKnob != 0 && pControlKnob->PointOnControlKnob( Point, GrabDistance )
				    && !pControlKnob->IsShowOnParentSelect()
				    && ( pConnector->GetLayers() & m_UsableLayers ) != 0 )
				{
					if( SelectionDepth == 0 )
					{
						pSelectingControlKnob = pControlKnob;
						break;
					}
					else
						--SelectionDepth;
				}
			}
		}
	}

	if( pSelectingConnector == 0 && pSelectingLink == 0 && pSelectingControlKnob == 0 )
	{
		Position = m_Connectors.GetHeadPosition();
		while( Position != 0 )
		{
			CConnector* pConnector = m_Connectors.GetNext( Position );
			if( pConnector != 0 && pConnector->PointOnConnector( Point, GrabDistance )
			    && ( pConnector->GetLayers() & m_UsableLayers ) != 0 )
			{
				if( SelectionDepth == 0 )
				{
					pSelectingConnector = pConnector;
					break;
				}
				else
					--SelectionDepth;
			}
		}
	}

	if( pSelectingConnector == 0 && pSelectingLink == 0 && pSelectingControlKnob == 0 )
	{
		Position = m_Links.GetHeadPosition();
		while( Position != NULL )
		{
			CLink* pLink = m_Links.GetNext( Position );
			if( pLink != NULL && pLink->PointOnLink( m_GearConnectionList, Point, GrabDistance, 0 )
			    && ( pLink->GetLayers() & m_UsableLayers ) != 0 )
			{
				if( SelectionDepth == 0 )
				{
					if( pLink->GetConnectorCount() == 1 && !pLink->IsGear() )
						pSelectingConnector = pLink->GetConnector( 0 );
					else
						pSelectingLink = pLink;
					break;
				}
				else
					--SelectionDepth;
			}
		}
	}

	if( pSelectingConnector == 0 && pSelectingLink == 0 && pSelectingControlKnob == 0 )
	{
		Position = m_Connectors.GetHeadPosition();
		while( Position != 0 )
		{
			CConnector* pConnector = m_Connectors.GetNext( Position );
			if( pConnector != 0 && ( pConnector->GetLayers() & m_UsableLayers ) != 0 && pConnector->GetDrawCircleRadius() > 0 )
			{
				double Distance = pConnector->GetPoint().DistanceToPoint( Point );
				if( fabs( Distance - pConnector->GetDrawCircleRadius() ) < GrabDistance )
				{
					if( SelectionDepth == 0 )
					{
						pSelectingConnector = pConnector;
						break;
					}
					else
						--SelectionDepth;
				}
			}
		}
	}

	if( pSelectingControlKnob != 0 )
	{			
		if( bClearExistingSelection )
			ClearSelection();
		bSelectionChanged = true;

		m_pCapturedConrolKnob = pSelectingControlKnob;
		m_pCapturedConrolKnob->Select( true );
		m_CaptureOffset.x = Point.x - pSelectingControlKnob->GetPoint().x;
		m_CaptureOffset.y = Point.y - pSelectingControlKnob->GetPoint().y;

		SetSelectedModifiableCondition();

		return true;
	}

	if( pSelectingConnector != 0 )
	{
		m_SelectionPoint = pSelectingConnector->GetOriginalPoint();
		m_bSelectionPoint = true;

		if( bMultiSelect )
		{
			if( pSelectingConnector->IsSelected() )
			{
				/*
				 * Remove this from the selected connector list.
				 */
				POSITION Position = m_SelectedConnectors.GetHeadPosition();
				while( Position != 0 )
				{
					CConnector *pCheckConnector = m_SelectedConnectors.GetAt( Position );
					if( pCheckConnector == pSelectingConnector )
					{
						m_SelectedConnectors.RemoveAt( Position );
						break;
					}
					m_SelectedConnectors.GetNext( Position );
				}
			}
			else
			{
				/*
				 * Add this to the selected connectors list at the beginning
				 * of the list.
				 */
				m_SelectedConnectors.AddHead( pSelectingConnector );
			}

			m_pCapturedConnector = 0;
			m_pCapturedConrolKnob = 0;
			pSelectingConnector->Select( !pSelectingConnector->IsSelected() );
			bSelectionChanged = true;

			SetSelectedModifiableCondition();

			return true;
		}
		else
		{
			if( !pSelectingConnector->IsSelected() )
			{
				if( bClearExistingSelection )
					ClearSelection();
				bSelectionChanged = SelectElement( pSelectingConnector );
			}
			m_pCapturedConnector = pSelectingConnector;
			m_CaptureOffset.x = Point.x - pSelectingConnector->GetPoint().x;
			m_CaptureOffset.y = Point.y - pSelectingConnector->GetPoint().y;
		}

		// Uncomment this in order to only snap to a connector previous position if it is the only thing selected.
		//if( m_SelectedConnectors.GetCount() + m_SelectedLinks.GetCount() > 1 )
		//	m_bSelectionPoint = false;

		SetSelectedModifiableCondition();

		return m_pCapturedConnector != 0;
	}

	if( pSelectingLink != 0 )
	{
		m_pCapturedConnector = 0;
		m_pCapturedConrolKnob = 0;
		if( bMultiSelect )
		{
			if( pSelectingLink->IsSelected() )
			{
				/*
				 * Remove this from the selected connector list.
				 */
				POSITION Position = m_SelectedLinks.GetHeadPosition();
				while( Position != 0 )
				{
					CLink *pCheckLink = m_SelectedLinks.GetAt( Position );
					if( pCheckLink == pSelectingLink )
					{
						m_SelectedLinks.RemoveAt( Position );
						break;
					}
					m_SelectedLinks.GetNext( Position );
				}
			}
			else
			{
				/*
				 * Add this to the selected connectors list at the beginning
				 * of the list.
				 */
				m_SelectedLinks.AddHead( pSelectingLink );
			}

			pSelectingLink->Select( !pSelectingLink->IsSelected() );
			bSelectionChanged = true;
			SetSelectedModifiableCondition();
			return true;
		}
		else
		{
			if( !pSelectingLink->IsSelected() )
			{
				ClearSelection();
				bSelectionChanged = SelectElement( pSelectingLink );
			}
			m_pCapturedConnector = pSelectingLink->GetConnectorList()->GetHead();
			m_CaptureOffset.x = Point.x - m_pCapturedConnector->GetPoint().x;
			m_CaptureOffset.y = Point.y - m_pCapturedConnector->GetPoint().y;
		}
		SetSelectedModifiableCondition();
		return m_pCapturedConnector != 0;
	}

	if( !bMultiSelect )
		ClearSelection();
	else
		SetSelectedModifiableCondition();

	return false;
}

bool CLinkageDoc::StretchSelected( double Percentage )
{
	CFPoint AveragePoint;
	int Points = 0;

	POSITION Position = m_Connectors.GetHeadPosition();
	while( Position != 0 )
	{
		CConnector* pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 )
			continue;
		if( pConnector->IsSelected() || pConnector->IsLinkSelected() || ( pConnector->GetLayers() & m_UsableLayers ) != 0 )
		{
			AveragePoint += pConnector->GetOriginalPoint();
			++Points;
		}
	}
	AveragePoint.x /= Points;
	AveragePoint.y /= Points;

	CFRect OriginalRect( AveragePoint.x - 100, AveragePoint.y - 100, AveragePoint.x + 100, AveragePoint.y + 100 );
	CFRect NewRect( AveragePoint.x - Percentage, AveragePoint.y - Percentage, AveragePoint.x + Percentage, AveragePoint.y + Percentage );

	bool bResult = StretchSelected( OriginalRect, NewRect, DIAGONAL );

	if( bResult )
		FinishChangeSelected();

	return bResult;
}

bool CLinkageDoc::StretchSelected( CFRect OriginalRect, CFRect NewRect, _Direction Direction )
{
	PushUndo();

	POSITION Position = m_Connectors.GetHeadPosition();
	while( Position != 0 )
	{
		CConnector* pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 )
			continue;

		if( !pConnector->IsSelected() && !pConnector->IsLinkSelected() )
			continue;

		if( pConnector->IsLocked() || ( pConnector->GetLayers() & m_UsableLayers ) == 0 )
			continue;

		CFPoint Scale( OriginalRect.Width() == 0 ? 1 : NewRect.Width() / OriginalRect.Width(), OriginalRect.Height() == 0 ? 1 : NewRect.Height() / OriginalRect.Height() );

		CFPoint OldPoint = pConnector->GetOriginalPoint();
		CFPoint NewPoint;
		NewPoint.x = NewRect.left + ( ( OldPoint.x - OriginalRect.left ) * Scale.x );
		NewPoint.y = NewRect.top + ( ( OldPoint.y - OriginalRect.top ) * Scale.y );

		if( Direction == DIAGONAL )
		{
			// These adjustments to the scale are based on there being different x and y scale factors. For
			// diagonal scaling, the scaling should be the same in both directions but this code is left
			// here for when this changes or for when scaling is needed to work when just one direction is scaled.
			double UseScale = max( Scale.x, Scale.y );
			if( Scale.x == 1.0 )
				UseScale = Scale.y;
			else if( Scale.y == 1.0 )
				UseScale = Scale.x;

			pConnector->SetIntermediateDrawCircleRadius( pConnector->GetOriginalDrawCircleRadius() * UseScale );
			pConnector->SetIntermediateSlideRadius( pConnector->GetOriginalSlideRadius() * UseScale );
		}

		bool bSkipThisOne = false;
		POSITION Position2 = pConnector->GetLinkList()->GetHeadPosition();
		while( Position2 != 0 )
		{
			CLink *pLink = pConnector->GetLinkList()->GetNext( Position2 );
			if( pLink == 0 )
				continue;

			if( pLink->IsLocked() )
			{
				bSkipThisOne = true;
				break;
			}
		}

		if( !bSkipThisOne )
			pConnector->SetIntermediatePoint( NewPoint );
	}

	FixupSliderLocations();

    SetModifiedFlag( true );
	return true;
}

bool CLinkageDoc::MeetSelected( void )
{
	if( !m_bSelectionMeetable )
		return false;

	CFPoint Points[4];
	Points[0] = GetSelectedConnector( 0 )->GetOriginalPoint();
	Points[1] = GetSelectedConnector( 1 )->GetOriginalPoint();
	Points[2] = GetSelectedConnector( 2 )->GetOriginalPoint();
	Points[3] = GetSelectedConnector( 3 )->GetOriginalPoint();

	CFCircle Circle1( Points[0], Distance( Points[0], Points[1] ) );
	CFCircle Circle2( Points[3], Distance( Points[3], Points[2] ) );

	CFPoint Intersect;
	CFPoint Intersect2;

	if( Circle1.CircleIntersection( Circle2, &Intersect, &Intersect2 ) )
		m_bSelectionMeetable = true;
	{
		CFPoint SuggestedPoint = ( Points[1] + Points[2] ) / 2;

		double d1 = Distance( SuggestedPoint, Intersect );
		double d2 = Distance( SuggestedPoint, Intersect2 );

		if( d2 < d1 )
			Intersect = Intersect2;

		GetSelectedConnector( 1 )->SetPoint( Intersect );
		GetSelectedConnector( 2 )->SetPoint( Intersect );
	}	

	NormalizeConnectorLinks();
	SetSelectedModifiableCondition();
	FixupSliderLocations();
    SetModifiedFlag( true );

	return true;
}

bool CLinkageDoc::MoveCapturedController( CFPoint Point )
{
	if( m_pCapturedConrolKnob == 0 )
		return false;

	CElement *pElement = m_pCapturedConrolKnob->GetParent();
	if( pElement == 0 )
		return false;

	pElement->UpdateControlKnob( m_pCapturedConrolKnob, Point );

	return true;
}

bool CLinkageDoc::CheckForSliderElementSnap( CConnector *pConnector, double SnapDistance, CFPoint &ReferencePoint, CFPoint &Adjustment, bool bSnapToSelectionPoint )
{	
	if( !pConnector->IsSlider() )
		return false;

	bool bSnapItem = false;
	CFPoint ConnectorPoint;
	if( !FixupSliderLocation( pConnector, ConnectorPoint ) )
		return false;

	/*
	 * A starting limit on how far a snap point can be from the connector point.
	 * if a snap point is within the horizontal snap distance on a curved slider
	 * path, it could still be a long way off vertically, and vice-versa. This
	 * attempts to limit that possible error later. It is adjusted to the 
	 * closest snap point is used if multiple qualify.
	 */ 
	double FoundDistance = SnapDistance * 1.41421356237;

	POSITION Position2 = bSnapToSelectionPoint ? 0 : m_Connectors.GetHeadPosition();
	while( bSnapToSelectionPoint || Position2 != 0 )
	{
		CFPoint CheckPoint;
		if( Position2 == 0 )
		{
			CheckPoint = m_SelectionPoint;
			Position2 = m_Connectors.GetHeadPosition();
			bSnapToSelectionPoint = false;
		}
		else
		{
			CConnector* pCheckConnector = m_Connectors.GetNext( Position2 );

			if( pCheckConnector == 0 || ( pConnector->GetLayers() & m_UsableLayers ) == 0 )
				continue;

			if( pCheckConnector->IsSelected() || pCheckConnector->IsLinkSelected() )
				continue;

			CheckPoint = pCheckConnector->GetOriginalPoint();
		}

		/*
		* This is a snap to the intersection of the slider track and the horizontal position
		* of the other connector, or to the intersection of the track and the vertical position
		* of the other connector.
		*/

		CFPoint Intersect[4];
		bool bIntersect[4] = { false, false, false, false };

		CFLine HorizontalLine( CheckPoint.x - 1, CheckPoint.y, CheckPoint.x + 1, CheckPoint.y );
		CFLine VerticalLine( CheckPoint.x, CheckPoint.y - 1, CheckPoint.x, CheckPoint.y + 1 );

		if( pConnector->GetSlideRadius() != 0 )
		{
			CFArc Arc;
			if( pConnector->GetSliderArc( Arc ) )
			{
				Intersects( HorizontalLine, Arc, Intersect[0], Intersect[1], bIntersect[0], bIntersect[1], true, true );
				bIntersect[0] = bIntersect[0] && Arc.PointOnArc( Intersect[0] );
				bIntersect[1] = bIntersect[1] && Arc.PointOnArc( Intersect[1] );
				Intersects( VerticalLine, Arc, Intersect[2], Intersect[3], bIntersect[2], bIntersect[3], true, true );
				bIntersect[2] = bIntersect[2] && Arc.PointOnArc( Intersect[2] );
				bIntersect[3] = bIntersect[3] && Arc.PointOnArc( Intersect[3] );
			}
		}
		else
		{
			CFPoint Point1;
			CFPoint Point2;
			if( pConnector->GetSlideLimits( Point1, Point2 ) )
			{
				CFLine Limit( Point1, Point2 );
				bIntersect[0] = Intersects( HorizontalLine, Limit, Intersect[0] );
				bIntersect[2] = Intersects( VerticalLine, Limit, Intersect[2] );
			}
		}

		double TestDistance = 0;
		double AbsDistance = 0;
		for( int Counter = 0; Counter < 4; ++Counter )
		{
			AbsDistance = Distance( ConnectorPoint, Intersect[Counter] );
			if( Counter < 2 )
				TestDistance = fabs( ConnectorPoint.y - CheckPoint.y );
			else
				TestDistance = fabs( ConnectorPoint.x - CheckPoint.x );
			if( bIntersect[Counter] && TestDistance < SnapDistance && AbsDistance < FoundDistance )
			{
				bSnapItem = true;
				Adjustment = Intersect[Counter] - ConnectorPoint;
				m_SnapLine[0].SetLine( CheckPoint, Intersect[Counter] );
				ReferencePoint = Intersect[Counter];
				FoundDistance = Distance( ConnectorPoint, Intersect[Counter] );
			}
		}
	}
	return bSnapItem;
}

bool CLinkageDoc::CheckForMotionPathSnap( CConnector *pConnector, double SnapDistance, CFPoint &ReferencePoint, CFPoint &Adjustment )
{
	return false;
}

bool CLinkageDoc::CheckForElementSnap( CConnector *pConnector, double SnapDistance, CFPoint &ReferencePoint, CFPoint &Adjustment, bool bSnapToSelectionPoint )
{
	bool bSnapItem = false;
	CFPoint ConnectorPoint = pConnector->GetPoint();

	POSITION Position2 = m_Connectors.GetHeadPosition();
	while( Position2 != 0 )
	{
		CConnector* pCheckConnector = m_Connectors.GetNext( Position2 );
		if( pCheckConnector == 0 || ( pConnector->GetLayers() & m_UsableLayers ) == 0 )
			continue;

		if( pCheckConnector->IsSelected() || pCheckConnector->IsLinkSelected() )
			continue;

		/* 
		 * Check for both x and y snapping before checking for one of the other.
		 */

		CFPoint CheckPoint = pCheckConnector->GetOriginalPoint();
		if( fabs( ConnectorPoint.x - CheckPoint.x ) < SnapDistance )
		{
			bSnapItem = true;
			Adjustment.x = CheckPoint.x - ConnectorPoint.x;
			m_SnapLine[0].SetLine( CheckPoint, CFPoint( ConnectorPoint.x + Adjustment.x, ConnectorPoint.y ) );
			ReferencePoint.x = ConnectorPoint.x + Adjustment.x;
		}
		if( fabs( ConnectorPoint.y - CheckPoint.y ) < SnapDistance )
		{
			bSnapItem = true;
			Adjustment.y = CheckPoint.y - ConnectorPoint.y;
			m_SnapLine[1].SetLine( CheckPoint, CFPoint( ConnectorPoint.x, ConnectorPoint.y + Adjustment.y ) );
			ReferencePoint.y = ConnectorPoint.y + Adjustment.y;
		}
	}

	if( bSnapToSelectionPoint )
	{
		if( fabs( ConnectorPoint.x - m_SelectionPoint.x ) < SnapDistance )
		{
			bSnapItem = true;
			Adjustment.x = m_SelectionPoint.x - ConnectorPoint.x;
			m_SnapLine[0].SetLine( m_SelectionPoint, CFPoint( ConnectorPoint.x + Adjustment.x, ConnectorPoint.y ) );
			ReferencePoint.x = ConnectorPoint.x + Adjustment.x;
		}
		if( fabs( ConnectorPoint.y - m_SelectionPoint.y ) < SnapDistance )
		{
			bSnapItem = true;
			Adjustment.y = m_SelectionPoint.y - ConnectorPoint.y;
			m_SnapLine[1].SetLine( m_SelectionPoint, CFPoint( ConnectorPoint.x, ConnectorPoint.y + Adjustment.y ) );
			ReferencePoint.y = ConnectorPoint.y + Adjustment.y;
		}
	}
	return bSnapItem;
}

bool CLinkageDoc::CheckForGridSnap( CConnector *pConnector, double SnapDistance, double xGrid, double yGrid, CFPoint &ReferencePoint, CFPoint &Adjustment )
{
	bool bSnapGrid = false;

	CFPoint ConnectorPoint = pConnector->GetPoint();

	double xMin = ( (int)( ConnectorPoint.x / xGrid ) ) * xGrid;
	double xMax = xMin + ( xGrid * ( ConnectorPoint.x >= 0 ? 1 : -1 ) );

	double yMin = ( (int)( ConnectorPoint.y / yGrid ) ) * yGrid;
	double yMax = yMin + ( yGrid * ( ConnectorPoint.y >= 0 ? 1 : -1 ) );

	if( fabs( ConnectorPoint.x - xMin ) < SnapDistance )
	{
		bSnapGrid = true;
		Adjustment.x = xMin - ConnectorPoint.x;
		m_SnapLine[0].SetLine( xMin, -99999, xMin, 99999 );
		ReferencePoint.x = ConnectorPoint.x + Adjustment.x;
	}
	else if( fabs( ConnectorPoint.x - xMax ) < SnapDistance )
	{
		bSnapGrid = true;
		Adjustment.x = xMax - ConnectorPoint.x;
		m_SnapLine[0].SetLine( xMax, -99999, xMax, 99999 );
		ReferencePoint.x = ConnectorPoint.x + Adjustment.x;
	}
	if( fabs( ConnectorPoint.y - yMin ) < SnapDistance )
	{
		bSnapGrid = true;
		Adjustment.y = yMin - ConnectorPoint.y;
		m_SnapLine[1].SetLine( -99999, yMin, 99999, yMin );
		ReferencePoint.y = ConnectorPoint.y + Adjustment.y;
	}
	else if( fabs( ConnectorPoint.y - yMax ) < SnapDistance )
	{
		bSnapGrid = true;
		Adjustment.y = yMax - ConnectorPoint.y;
		m_SnapLine[1].SetLine( -99999, yMax, 99999, yMax );
		ReferencePoint.y = ConnectorPoint.y + Adjustment.y;
	}

	return bSnapGrid;
}


CFPoint CLinkageDoc::CheckForSnap( ConnectorList &SelectedConnectors, double SnapDistance, bool bElementSnap, bool bGridSnap, double xGrid, double yGrid, CFPoint &ReferencePoint, bool bSnapToSelectionPoint )
{
	CFPoint Adjustment( 0, 0 );

	bool bSnapGrid = false;

	bSnapToSelectionPoint = bSnapToSelectionPoint & m_bSelectionPoint;

	if( bGridSnap )
	{
		POSITION Position = SelectedConnectors.GetHeadPosition();
		while( Position != 0 && !bSnapGrid )
		{
			CConnector* pConnector = SelectedConnectors.GetNext( Position );
			if( pConnector == 0 )
				continue;

			if( !pConnector->IsSelected() && !pConnector->IsLinkSelected() )
				continue;

			if( pConnector->IsSlider() )
				continue;

			bSnapGrid = CheckForGridSnap( pConnector, SnapDistance, xGrid, yGrid, ReferencePoint, Adjustment );
			if( bSnapGrid )
				break;
		}
	}

	/*
	 * Item snap overrides grid snap to make auto-join easier.
	 */

	bool bSnapItem = false;

	if( bElementSnap )
	{
		/*
		 * Check each selected connector to see if it is close to a non-selected
		 * connector. If so, move all of the selected connectors so that the close
		 * one is lined up on the snap connector.
		 */

		POSITION Position = SelectedConnectors.GetHeadPosition();
		while( Position != 0 )
		{
			CConnector* pConnector = SelectedConnectors.GetNext( Position );
			if( pConnector == 0 || ( pConnector->GetLayers() & m_UsableLayers ) == 0 )
				continue;

			if( !pConnector->IsSelected() && !pConnector->IsLinkSelected() )
				continue;

			if( pConnector->IsSlider() )
			{
				bSnapItem |= CheckForSliderElementSnap( pConnector, SnapDistance, ReferencePoint, Adjustment, bSnapToSelectionPoint );
				//if( bSnapItem )
				//	break;	
			}
			else
			{
				bSnapItem |= CheckForElementSnap( pConnector, SnapDistance, ReferencePoint, Adjustment, bSnapToSelectionPoint );
				//if( bBoth )
				//	break;
			}		
		}
	}

	if( bSnapItem || bSnapGrid )
		return Adjustment;
	else
		return CFPoint( 0, 0 );
}

int CLinkageDoc::BuildSelectedLockGroup( ConnectorList *pLockGroup )
{
	/*
	 * Build a list of all connectors that are selected or are attached to links that are locked and
	 * have a selected connector.
	 */

	CBitArray LockedConnectors;
	LockedConnectors.SetLength( 0 );

	POSITION Position = m_Connectors.GetHeadPosition();
	while( Position != 0 )
	{
		CConnector* pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 )
			continue;

		if( !pConnector->IsSelected() && !pConnector->IsLinkSelected() )
			continue;

		if( pConnector->IsLocked() )
			continue;

		LockedConnectors.SetBit( pConnector->GetIdentifier() );
	}

	for(;;)
	{
		int ChangeCount = 0;
		POSITION Position = m_Links.GetHeadPosition();
		while( Position != 0 )
		{
			CLink *pLink = m_Links.GetNext( Position );
			if( pLink == 0 || !pLink->IsLocked() )
				continue;

			bool bOneIsSelectedLocked = false;
			bool bConnectorIsLocked = false;
			POSITION Position2 = pLink->GetConnectorList()->GetHeadPosition();
			while( Position2 != 0 )
			{
				CConnector *pConnector = pLink->GetConnectorList()->GetNext( Position2 );
				if( pConnector == 0 )
					continue;
				if( LockedConnectors.GetBit( pConnector->GetIdentifier() ) )
					bOneIsSelectedLocked = true;
				if( pConnector->IsLocked() )
					bConnectorIsLocked = true;
			}

			/*if( bConnectorIsLocked && false )
			{
				POSITION Position2 = pLink->GetConnectorList()->GetHeadPosition();
				while( Position2 != 0 )
				{
					CConnector *pConnector = pLink->GetConnectorList()->GetNext( Position2 );
					if( pConnector == 0 || !LockedConnectors.GetBit( pConnector->GetIdentifier() ) )
						continue;

					LockedConnectors.ClearBit( pConnector->GetIdentifier() );
					++ChangeCount;
				}
			}
			else */

			if( bOneIsSelectedLocked )
			{
				Position2 = pLink->GetConnectorList()->GetHeadPosition();
				while( Position2 != 0 )
				{
					CConnector *pConnector = pLink->GetConnectorList()->GetNext( Position2 );
					if( pConnector == 0 || LockedConnectors.GetBit( pConnector->GetIdentifier() ) )
						continue;

					LockedConnectors.SetBit( pConnector->GetIdentifier() );
					++ChangeCount;
				}
			}
		}
		if( ChangeCount == 0 )
			break;
	}

	Position = m_Connectors.GetHeadPosition();
	while( Position != 0 )
	{
		CConnector *pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 )
			continue;
		if( LockedConnectors.GetBit( pConnector->GetIdentifier() ) )
			pLockGroup->AddTail( pConnector );
	}

	return (int)pLockGroup->GetCount();
}

bool CLinkageDoc::MoveSelected( CFPoint Point, bool bElementSnap, bool bGridSnap, double xGrid, double yGrid, double SnapDistance, CFPoint &ReferencePoint, bool bSnapToSelectionPoint )
{
	m_SnapLine[0].SetLine( 0, 0, 0, 0 );
	m_SnapLine[1].SetLine( 0, 0, 0, 0 );

	if( m_pCapturedConrolKnob != 0 )
		return MoveCapturedController( Point );

	if( m_pCapturedConnector == 0 )
		return false;

	CFPoint OriginalPoint = m_pCapturedConnector->GetOriginalPoint();
	CFPoint Offset;
	Offset.x = ( Point.x - m_CaptureOffset.x ) - OriginalPoint.x;
	Offset.y = ( Point.y - m_CaptureOffset.y ) - OriginalPoint.y;

	if( Offset.x == 0.0 && Offset.y == 0.0 )
		return false;

	/*
	 * There is one special case where a single moved connector on a single locked link that has only two connectors
	 * will rotate the locked link.
	 */

	if( m_SelectedConnectors.GetCount() == 1 && m_SelectedLinks.GetCount() == 0 )
	{
		CConnector *pConnector = m_SelectedConnectors.GetHead();
		if( pConnector != 0 && pConnector == m_pCapturedConnector && !pConnector->IsLocked() && ( pConnector->GetLayers() & m_UsableLayers ) != 0 )
		{
			int LockedLinkCount = 0;
			CLink *pLockedLink = 0;
			POSITION cPosition = pConnector->GetLinkList()->GetHeadPosition();
			while( cPosition != 0 )
			{
				CLink *pLink = pConnector->GetLinkList()->GetNext( cPosition );
				if( pLink == 0 || !pLink->IsLocked() || pLink->GetConnectorCount() != 2 )
					continue;
				++LockedLinkCount;
				pLockedLink = pLink;
			}

			if( LockedLinkCount == 1 )
			{
				if( pConnector->IsSlider() )
					return true; // Moved it to same location (since it never moved).

				// Rotate the locked link around it's other connector to get the
				// selected connector to be as close to the desired position as possible.
				CConnector *pCenter = pLockedLink->GetConnector( 0 );
				if( pCenter == pConnector )
					pCenter = pLockedLink->GetConnector( 1 );

				double Angle = GetAngle( pCenter->GetOriginalPoint(), OriginalPoint, CFPoint( Point.x - m_CaptureOffset.x, Point.y - m_CaptureOffset.y ) );
				pConnector->MovePoint( pConnector->GetOriginalPoint() );
				pConnector->RotateAround( pCenter->GetOriginalPoint(), Angle );
				pConnector->MakePermanent();

				return true;
			}
		}
	}

	ConnectorList MoveConnectors;
	int MoveCount = BuildSelectedLockGroup( &MoveConnectors );
	POSITION Position = MoveConnectors.GetHeadPosition();
	while( Position != 0 )
	{
		CConnector* pConnector = MoveConnectors.GetNext( Position );
		if( pConnector == 0 )
			continue;

		if( pConnector->IsLocked() && pConnector->IsAnchor() )
			return false; // Cannot move anything if one of the lock group is a locked anchor.
	}

	Position = MoveConnectors.GetHeadPosition();
	while( Position != 0 )
	{
		CConnector* pConnector = MoveConnectors.GetNext( Position );
		if( pConnector == 0 )
			continue;

		CFPoint ConnectorPoint = pConnector->GetOriginalPoint();
		ConnectorPoint += Offset;
		pConnector->SetIntermediatePoint( ConnectorPoint );
	}

	bool bSnapReference = false;

	if( bElementSnap || bGridSnap )
	{
		CFPoint SnapReference = ReferencePoint;
		CFPoint Adjustment = CheckForSnap( MoveConnectors, SnapDistance, bElementSnap, bGridSnap, xGrid, yGrid, SnapReference, bSnapToSelectionPoint );

		if( Adjustment.x != 0 || Adjustment.y != 0 )
		{
			Position = MoveConnectors.GetHeadPosition();
			while( Position != 0 )
			{
				CConnector* pConnector = MoveConnectors.GetNext( Position );
				if( pConnector == 0 )
					continue;

				CFPoint ConnectorPoint = pConnector->GetPoint();
				ConnectorPoint += Adjustment;
				pConnector->SetIntermediatePoint( ConnectorPoint );
			}
		}
	}

	FixupSliderLocations();

	if( !bSnapReference && m_pCapturedConnector != 0 && m_SelectedConnectors.GetCount() == 1 )
		ReferencePoint = m_pCapturedConnector->GetPoint();

    SetModifiedFlag( true );
	return true;
}

bool CLinkageDoc::MoveSelected( CFPoint Offset )
{
	PushUndo();

	ConnectorList MoveConnectors;
	int MoveCount = BuildSelectedLockGroup( &MoveConnectors );
	POSITION Position = MoveConnectors.GetHeadPosition();
	while( Position != 0 )
	{
		CConnector* pConnector = MoveConnectors.GetNext( Position );
		if( pConnector == 0 )
			continue;

		CFPoint ConnectorPoint = pConnector->GetOriginalPoint();
		ConnectorPoint += Offset;
		pConnector->SetPoint( ConnectorPoint );
	}

	FixupSliderLocations();

    SetModifiedFlag( true );
	return true;
}

bool CLinkageDoc::FixupSliderLocation( CConnector *pConnector )
{
	CFPoint NewPoint;
	if( FixupSliderLocation( pConnector, NewPoint ) )
	{
		pConnector->SetIntermediatePoint( NewPoint );
		return true;
	}

	return false;
}

bool CLinkageDoc::FixupSliderLocation( CConnector *pConnector, CFPoint &NewPoint )
{
	CConnector *pStart;
	CConnector *pEnd;
	pConnector->GetSlideLimits( pStart, pEnd );

	if( !pConnector->IsSlider() || pStart == 0 || pEnd == 0 )
		return false;

	if( pStart->IsSelected() || pEnd->IsSelected()
		|| pStart->IsLinkSelected() || pEnd->IsLinkSelected() )
	{
		// Find the closest place on the path for the slider.
		if( pConnector->GetSlideRadius() == 0 )
		{
			CFLine Line( pStart->GetPoint(), pEnd->GetPoint() );
			NewPoint = pConnector->GetPoint();
			NewPoint.SnapToLine( Line, true );
		}
		else
		{
			CFArc TheArc;
			if( !pConnector->GetSliderArc( TheArc ) )
				return false;

			NewPoint = pConnector->GetPoint();
			NewPoint.SnapToArc( TheArc );
		}
	}
	else //if( pConnector->IsSelected() || pConnector->IsLinkSelected() )
	{
		// the slider needs to be snapped onto the slider path because it is
		// the element that is being changed at this time.
		CFPoint ConnectorPoint = pConnector->GetPoint();
		if( pConnector->GetSlideRadius() == 0 )
		{
			CFLine TempLine( pStart->GetPoint(), pEnd->GetPoint() );
			ConnectorPoint.SnapToLine( TempLine, true );
			NewPoint = ConnectorPoint;
		}
		else
		{
			CFArc TheArc;
			if( !pConnector->GetSliderArc( TheArc ) )
				return false;
			ConnectorPoint.SnapToArc( TheArc );
			NewPoint = ConnectorPoint;
		}
	}

	return true;
}


bool CLinkageDoc::FixupSliderLocations( void )
{
	for( int Counter = 0; Counter < 2; ++Counter)
	{
		POSITION Position = m_Connectors.GetHeadPosition();
		while( Position != 0 )
		{
			CConnector* pConnector = m_Connectors.GetNext( Position );
			if( pConnector == 0 )
				continue;

			pConnector->UpdateControlKnobs();

			if( !pConnector->IsSlider() )
				continue;

			FixupSliderLocation( pConnector );
		}

		// Also fixup controllers for all links that have them.
		Position = m_Links.GetHeadPosition();
		while( Position != 0 )
		{
			CLink *pLink = m_Links.GetNext( Position );
			if( pLink == 0 )
				continue;

			pLink->UpdateControlKnobs();
		}
	}

	return true;
}

bool CLinkageDoc::FixupGearConnections( void )
{
	POSITION Position = m_GearConnectionList.GetHeadPosition();
	while( Position != 0 )
	{
		CGearConnection *pGC = m_GearConnectionList.GetNext( Position );
	}

	return true;
}

bool CLinkageDoc::RotateSelected( CFPoint CenterPoint, double Angle )
{
	if( Angle == 0 )
		return true;

	ConnectorList RotateConnectors;
	int RotateCount = BuildSelectedLockGroup( &RotateConnectors );
	POSITION Position = RotateConnectors.GetHeadPosition();
	while( Position != 0 )
	{
		CConnector* pConnector = RotateConnectors.GetNext( Position );
		if( pConnector == 0 || ( pConnector->GetLayers() & m_UsableLayers ) == 0 )
			continue;

		pConnector->MovePoint( pConnector->GetOriginalPoint() );
		pConnector->RotateAround( CenterPoint, Angle );
		pConnector->MakePermanent();
	}

	FixupSliderLocations();

    SetModifiedFlag( true );
	return true;
}

bool CLinkageDoc::StartChangeSelected( void )
{
	PushUndo();
	return true;
}

bool CLinkageDoc::FinishChangeSelected( void )
{
	POSITION Position = m_Connectors.GetHeadPosition();
	while( Position != 0 )
	{
		CConnector* pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 )
			continue;

		pConnector->SetPoint( pConnector->GetPoint() );
		pConnector->SetDrawCircleRadius( pConnector->GetDrawCircleRadius() );
		pConnector->SetSlideRadius( pConnector->GetSlideRadius() );
		pConnector->Reset( false );
	}

	if( m_pCapturedConrolKnob != 0 )
	{
		m_pCapturedConrolKnob->SetPoint( m_pCapturedConrolKnob->GetPoint() );
		//m_pCapturedConrolKnob->Reset( true );
		m_pCapturedConrolKnob->Select( false );
		m_pCapturedConrolKnob = 0;
	}

	m_SnapLine[0].SetLine( 0, 0, 0, 0 );
	m_SnapLine[1].SetLine( 0, 0, 0, 0 );

    SetModifiedFlag( true );
	return true;
}

void CLinkageDoc::Reset( bool bClearMotionPath, bool KeepCurrentPositions )
{
	if( KeepCurrentPositions )
		PushUndo();

	//ClearSelection();

	POSITION Position;

	Position = m_Connectors.GetHeadPosition();
	while( Position != NULL )
	{
		CConnector* pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 )
			continue;

		if( KeepCurrentPositions )
		{
			pConnector->SetPoint( pConnector->GetPoint() );
			if( pConnector->GetLimitAngle() > 0 )
				pConnector->SetStartOffset( fmod( fabs( pConnector->GetRotationAngle() ), pConnector->GetLimitAngle() * 2 ) );
			else
				pConnector->SetStartOffset( 0.0 );
		}
		pConnector->Reset( bClearMotionPath );
	}

	Position = m_Links.GetHeadPosition();
	while( Position != NULL )
	{
		CLink *pLink = m_Links.GetNext( Position );
		if( pLink == 0 )
			continue;

		if( KeepCurrentPositions )
		{
			if( pLink->IsActuator() && pLink->GetStroke() != 0 )
				pLink->SetStartOffset( fmod( fabs( pLink->GetTempActuatorExtension() ) + pLink->GetStartOffset(), pLink->GetStroke() * 2 ) ); // Don't use extend distance since that is adjusted for the stroke. Use the temp distance and add the offset to start with.
			else
				pLink->SetStartOffset( 0 );
		}
		pLink->Reset();
	}

	SetSelectedModifiableCondition();
}

bool CLinkageDoc::LockSelected( void )
{
	if( m_SelectedConnectors.GetCount() == 0 && m_SelectedLinks.GetCount() == 0 )
		return false;

	bool bNeedPush = true;

	POSITION Position = m_SelectedLinks.GetHeadPosition();
	while( Position != NULL )
	{
		CLink* pLink = m_SelectedLinks.GetNext( Position );
		if( pLink == 0 )
			continue;

		if( bNeedPush )
			PushUndo();
		bNeedPush = false;
		pLink->SetLocked( true );
	}
	Position = m_SelectedConnectors.GetHeadPosition();
	while( Position != NULL )
	{
		CConnector* pConnector = m_SelectedConnectors.GetNext( Position );
		if( pConnector == 0 || !pConnector->IsAnchor() )
			continue;

		if( bNeedPush )
			PushUndo();
		bNeedPush = false;
		pConnector->SetLocked( true );
	}

	return !bNeedPush;
}

bool CLinkageDoc::IsLinkLocked( CConnector *pConnector )
{
	if( pConnector == 0 )
		return false;

	POSITION Position = pConnector->GetLinkList()->GetHeadPosition();
	while( Position != 0 )
	{
		CLink *pLink = pConnector->GetLinkList()->GetNext( Position );
		if( pLink == 0 )
			continue;

		if( pLink->IsLocked() )
			return true;
	}
	return false;
}

bool CLinkageDoc::JoinSelected( bool bSaveUndoState )
{
	if( !m_bSelectionJoinable )
		return false;

	CFPoint AveragePoint;
	CFPoint LockedPoint;
	int Counter = 0;
	bool bInput = false;
	bool bAnchor = false;
	bool bIsSlider = false;
	CFPoint ForcePoint;
	int Sliders = 0;
	int CombinedState = 0;
	unsigned int CombinedLayers = 0;
	int LockedLinks = 0;
	CConnector *pLockedConnector = 0;
	int Connectors = (int)m_SelectedConnectors.GetCount();

	CConnector *pLimit1 = 0;
	CConnector *pLimit2 = 0;
	bool bSlidersShareLimits = true;

	POSITION Position = m_SelectedConnectors.GetHeadPosition();
	CConnector *pKeepConnector = m_SelectedConnectors.GetAt( Position );

	for( ; Position != NULL; )
	{
		CConnector* pConnector = m_SelectedConnectors.GetNext( Position );
		if( pConnector == 0 || !pConnector->IsSelected() )
			continue;

		POSITION Position2 = pConnector->GetLinkList()->GetHeadPosition();
		while( Position2 != 0 )
		{
			CLink *pLink = pConnector->GetLinkList()->GetNext( Position2 );
			if( pLink == 0 || !pLink->IsLocked() )
				continue;
			++LockedLinks;
			pLockedConnector = pConnector;
			LockedPoint = pLockedConnector->GetOriginalPoint();
		}

		bInput |= pConnector->IsInput();
		bAnchor |= pConnector->IsAnchor();
		bIsSlider |= pConnector->IsSlider();

		if( pConnector->IsSlider() )
		{
			++Sliders;
			pKeepConnector = pConnector;
			CConnector *pTemp1 = 0;
			CConnector *pTemp2 = 0;
			pConnector->GetSlideLimits( pTemp1, pTemp2 );
			if( ( pLimit1 != 0 && pTemp1 != pLimit1 ) || ( pLimit2 != 0 && pTemp2 != pLimit2 ) )
				bSlidersShareLimits = false;
			pLimit1 = pTemp1;
			pLimit2 = pTemp2;
		}
	}

	if( Sliders > 1 && !bSlidersShareLimits )
		return false;

	if( LockedLinks > 1 )
		return false;

	if( m_SelectedLinks.GetCount() > 0 && Sliders > 0 )
		return false;

	if( pKeepConnector->IsSlider() && LockedLinks > 0 && pLockedConnector != pKeepConnector )
		return false;

	if( bSaveUndoState )
		PushUndo();

	/*
	* Check for sliding connectors that have any of the joined conectors as their limits. Change the limits to use the new joined connector.
	* IMPORTANT: The joined connectors are all deleted except for the one being kept. The pointers are stil available but are not pointing to
	* any valid memory!
	*/
	Position = m_Connectors.GetHeadPosition();
	while( Position != NULL )
	{
		CConnector *pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 )
			continue;

		CConnector *pLimit1 = NULL;
		CConnector *pLimit2 = NULL;

		if( pConnector->GetSlideLimits( pLimit1, pLimit2 ) )
		{
			if( pLimit1->IsSelected() && pLimit2->IsSelected() )
			{
				// No sliding between the single resulting connectors when the join is finished!
				pConnector->SlideBetween();
				continue;
			}

			if( pConnector->IsSelected() && ( pLimit1->IsSelected() || pLimit2->IsSelected() ) )
			{
				// No slider if it's joined to it's limit connector.
				pConnector->SlideBetween();
				continue;
			}

			if( pConnector->IsSelected() && pConnector != pKeepConnector )
			{
				// This slider will be joined to another connector and that connector must also be a slider (or this one would be pKeepConnector)...
				// Change it to a non-slider so it's limit connectors no longer know about it.
				pConnector->SlideBetween();
				continue;
			}

			if( pLimit1->IsSelected() || pLimit2->IsSelected() )
			{
				bool bKeepChecking = true;
				CConnector *pSelectedLimit = pLimit1;
				if( pLimit2->IsSelected() )
					pSelectedLimit = pLimit2;

				POSITION Position2 = pConnector->GetLinkList()->GetHeadPosition();
				while( Position2 != 0 && bKeepChecking )
				{
					CLink *pLink = pConnector->GetLinkList()->GetNext( Position2 );
					if( pLink == 0 )
						continue;
					POSITION Position3 = pLink->GetConnectorList()->GetHeadPosition();
					while( Position3 != 0 && bKeepChecking )
					{
						CConnector *pTestConnector = pLink->GetConnectorList()->GetNext( Position3 );
						if( pTestConnector == 0 || !pTestConnector->IsSelected() )
							continue;
						// No slider if one of the limits is being joined to a connector that is on one of the links with the slider.
						pConnector->SlideBetween();

						bKeepChecking = false;
					}
				}

				if( !bKeepChecking )
					continue;
			}
		}
	}

	Position = m_SelectedConnectors.GetHeadPosition();
	for( Counter = 0; Position != NULL; ++Counter )
	{
		CConnector* pConnector = m_SelectedConnectors.GetNext( Position );
		if( pConnector == 0 || !pConnector->IsSelected() )
			continue;

		CombinedLayers |= pConnector->GetLayers();

		AveragePoint += pConnector->GetOriginalPoint();

		// After this is stuff to be done to the links that are being deleted.
		if( pConnector == pKeepConnector )
			continue;

		POSITION Position2 = pConnector->GetLinkList()->GetHeadPosition();
		while( Position2 != 0 )
		{
			CLink *pLink = pConnector->GetLinkList()->GetNext( Position2 );
			if( pLink == 0 )
				continue;

			pLink->RemoveConnector( pConnector );
			pLink->AddConnector( pKeepConnector );
			pKeepConnector->AddLink( pLink );
			pLink->SetActuator( pLink->IsActuator() );
		}

		m_Connectors.Remove( pConnector );
		m_IdentifiedConnectors.ClearBit( pConnector->GetIdentifier() );
		delete pConnector;
	}

	/*
	* Check for changes to the slide limits where the slide limit changes to the new joined connector.
	* This is done after links are changed because otherwise the sliding might be invalid due to limits being on two differnt links.
	*/
	Position = m_Connectors.GetHeadPosition();
	while( Position != NULL )
	{
		CConnector *pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 )
			continue;

		CConnector *pLimit1 = NULL;
		CConnector *pLimit2 = NULL;

		if( pConnector->GetSlideLimits( pLimit1, pLimit2 ) )
		{
			POSITION Position2 = m_SelectedConnectors.GetHeadPosition();

			while( Position2 != NULL )
			{
				CConnector* pTestConnector = m_SelectedConnectors.GetNext( Position2 );
				if( pTestConnector == 0 || !pTestConnector->IsSelected() || pTestConnector == pKeepConnector )
					continue;

				// If pTestConnector is selected and it's not being kept and it's also a limit, change the limit to the keep connector. 
				if( pLimit1 == pTestConnector )
					pLimit1 = pKeepConnector;
				else if( pLimit2 == pTestConnector )
					pLimit2 = pKeepConnector;
				else
					continue;

				pConnector->SlideBetween( pLimit1, pLimit2 );
			}
		}
	}

	AveragePoint.x /= Counter;
	AveragePoint.y /= Counter;

	if( pLockedConnector != 0 )
		AveragePoint = LockedPoint;

	if( !pKeepConnector->IsSlider() )
		pKeepConnector->SetPoint( AveragePoint );

	pKeepConnector->SetAsInput( bInput );
	pKeepConnector->SetAsAnchor( bAnchor && !pKeepConnector->IsSlider() );
	pKeepConnector->SetLayers( CombinedLayers );

	m_SelectedConnectors.RemoveAll();
	m_SelectedConnectors.AddHead( pKeepConnector );

	/*
	 * Now that all connectors are joined, join the new single connector to all of the selected links.
	 */
	if( m_SelectedLinks.GetCount() )
	{
		Position = m_SelectedLinks.GetHeadPosition();
		while( Position != 0 )
		{
			CLink *pLink = m_SelectedLinks.GetNext( Position );
			if( pLink == 0 )
				continue;
			pLink->AddConnector( pKeepConnector );
			pKeepConnector->AddLink( pLink );
		}
	}

	NormalizeConnectorLinks();

	SetSelectedModifiableCondition();

	FixupSliderLocations();

    SetModifiedFlag( true );

	return true;
}

CLink* CLinkageDoc::ConnectSelected( void )
{
	PushUndo();

	// Make a new Link and add the selected connectors to it.
	// Do not move anything.

	if( GetSelectedConnectorCount() <= 1 )
		return 0;

	CLink *pLink = new CLink();
	if( pLink == 0 )
		return 0;

	int NewID = m_IdentifiedLinks.FindAndSetBit();
	if( NewID > m_HighestLinkID )
		m_HighestLinkID = NewID;
	pLink->SetIdentifier( NewID );

	unsigned int Layers = 0;

	POSITION Position = m_Connectors.GetHeadPosition();
	while( Position != NULL )
	{
		CConnector* pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 || !pConnector->IsSelected() )
			continue;
		// Check for an existing single link for this connector.
		CLink *pLoneLink = pConnector->GetLink( 0 );
		if( pLoneLink != 0 && pLoneLink->GetConnectorCount() == 1 && !pLoneLink->IsGear() )
		{
			pLoneLink->RemoveAllConnectors();
			pConnector->RemoveAllLinks();
			m_Links.Remove( pLoneLink );
			m_SelectedLinks.Remove( pLoneLink );
			m_IdentifiedLinks.ClearBit( pLoneLink->GetIdentifier() );
			delete pLoneLink;
		}

		Layers = pConnector->GetLayers();
		pLink->SetLayers( Layers );
		pConnector->AddLink( pLink );
		pLink->AddConnector( pConnector );
	}

	pLink->SetColor( ( Layers & DRAWINGLAYER ) != 0 ? RGB( 200, 200, 200 ) : Colors[pLink->GetIdentifier() % COLORS_COUNT] );

	m_Links.AddTail( pLink );

	SetSelectedModifiableCondition();

    SetModifiedFlag( true );

	return pLink;
}

void CLinkageDoc::RawAngleSelected( double Angle )
{
	if( GetSelectedConnectorCount() < 1 || GetSelectedConnectorCount() > 3 )
		return;

	CConnector *pConnectors[3];

	int SelectedConnector = GetSelectedConnectorCount() - 1;
	int Counter = 2;
	for( ; Counter >= 0 && SelectedConnector >= 0; --Counter, --SelectedConnector )
	{
		pConnectors[Counter] = (CConnector*)GetSelectedConnector( SelectedConnector );
		if( pConnectors[Counter] == 0 )
			return;
	}
	double OldAngle = 0;
	if( Counter < 0 )
		OldAngle = GetAngle( pConnectors[1]->GetPoint(), pConnectors[0]->GetPoint(), pConnectors[2]->GetPoint() );
	else
	{
		CFPoint FirstPoint( pConnectors[1]->GetPoint().x + 1000, pConnectors[1]->GetPoint().y );
		OldAngle = GetAngle( pConnectors[1]->GetPoint(), FirstPoint, pConnectors[2]->GetPoint() );
	}

	double ChangeAngle;
	//if( OldAngle <= 180 )
		ChangeAngle = Angle - OldAngle;
	//else
	//	ChangeAngle = OldAngle - ( 360 - Angle );

	CFPoint MovePoint = pConnectors[2]->GetPoint();

	MovePoint.RotateAround( pConnectors[1]->GetPoint(), ChangeAngle );
	pConnectors[2]->SetPoint( MovePoint );
}

CString CLinkageDoc::GetSelectedElementAngle( void )
{
	if( !IsSelectionAngleable() )
		return "";
	if( GetSelectedConnectorCount() == 3 )
		return GetSelectedElementCoordinates( 0 );
	else if( GetSelectedConnectorCount() == 2 )
	{
		// Compute line angle, not angle between to different lines.
		CConnector *pConnector0 = GetSelectedConnector( 0 );
		CConnector *pConnector1 = GetSelectedConnector( 1 );
		double Angle = GetAngle( pConnector0->GetOriginalPoint(), pConnector1->GetOriginalPoint() );
		if( Angle < 0 )
			Angle += 360.0; // Because the hint mark for the angle is never shown on the "negative" side of the angle, don't show the number as negative.
		CString Text;
		Text.Format( "%.3lf", Angle );
		return Text;
	}
	else
		return "";
}

CLinkageDoc::_CoordinateChange CLinkageDoc::MakeSelectedAtAngle( double Angle )
{
	if( !IsSelectionAngleable() )
		return _CoordinateChange::NONE;

	int SelectedConnectorCount = GetSelectedConnectorCount();
	CConnector *pLastConnector = GetSelectedConnector( SelectedConnectorCount - 1 );
	if( pLastConnector != 0 && pLastConnector->IsSlider() )
		return _CoordinateChange::NONE;

	if( IsLinkLocked( pLastConnector ) )
		return _CoordinateChange::NONE;

	PushUndo();

	RawAngleSelected( Angle );

    SetModifiedFlag( true );

	NormalizeConnectorLinks();

	SetSelectedModifiableCondition();

	return _CoordinateChange::ROTATION;
}

void CLinkageDoc::MakeRightAngleSelected( void )
{
	if( !IsSelectionTriangle() )
		return;

	PushUndo();

	RawAngleSelected( 90 );

    SetModifiedFlag( true );

	NormalizeConnectorLinks();

	SetSelectedModifiableCondition();
}

// Need a global variable for the distance sort function :(
static CFPoint DistanceCompareStart;

// Distance sort function.
static int DistanceCompare( const void *tp1, const void *tp2 )
{
}

void CLinkageDoc::AlignSelected( _Direction Direction )
{
	switch( Direction )
	{
		case INLINE:
		case INLINESPACED:
		{
			if( m_AlignConnectorCount <= 2 )
				return;

			PushUndo();

			CFLine Line;

			POSITION Position = m_SelectedConnectors.GetTailPosition();
			if( Position == 0 )
				return;

			CConnector *pConnector = m_SelectedConnectors.GetAt( Position );
			if( pConnector == 0 )
				return;

			Line.m_Start = pConnector->GetPoint();

			Position = m_SelectedConnectors.GetHeadPosition();
			if( Position == 0 )
				return;

			pConnector = m_SelectedConnectors.GetNext( Position );
			if( pConnector == 0 )
				return;

			Line.m_End = pConnector->GetPoint();

			int PointCount = (int)m_SelectedConnectors.GetCount();

			vector<ConnectorDistance> ConnectorReference( PointCount - 2 );

			int Counter = 0;
			while( Position != 0 && Position != m_SelectedConnectors.GetTailPosition() )
			{
				CConnector *pConnector = m_SelectedConnectors.GetNext( Position );
				CFPoint MovePoint = pConnector->GetPoint();

				CFLine TempLine;
				Line.PerpendicularLine( TempLine );
				TempLine -= TempLine[0];
				TempLine += MovePoint;
				if( Intersects( TempLine, Line, MovePoint ) )
					pConnector->SetPoint( MovePoint );

				ConnectorReference[Counter].m_pConnector = pConnector;
				ConnectorReference[Counter].m_Distance = Distance( Line.m_Start, MovePoint );

				++Counter;
			}

			if( Direction == INLINESPACED )
			{
				sort( ConnectorReference.begin(), ConnectorReference.end(), CompareConnectorDistance() );

				double Space = 0.0;
				double Distance = Line.GetLength();
				Space = Distance / ( PointCount - 1 );

				for( int Counter = 0; Counter < PointCount - 2; ++Counter )
				{
					Line.SetLength( Space * ( Counter + 1 ) );
					ConnectorReference[Counter].m_pConnector->SetPoint( Line.m_End );
				}
			}

			break;
		}

		case HORIZONTAL:
		case VERTICAL:
		{
			if( m_AlignConnectorCount == 0 )
				return;
			PushUndo();

			CConnector *pFirstConnector = 0;

			POSITION Position = m_SelectedConnectors.GetTailPosition();
			if( Position != 0 )
				pFirstConnector = m_SelectedConnectors.GetPrev( Position );
			while( Position != 0 )
			{
				CConnector *pConnector = m_SelectedConnectors.GetPrev( Position );
				CFPoint MovePoint = pConnector->GetPoint();
				if( Direction == HORIZONTAL )
					MovePoint.y = pFirstConnector->GetPoint().y;
				else
					MovePoint.x = pFirstConnector->GetPoint().x;
				pConnector->SetPoint( MovePoint );
			}
			break;
		}

		case FLIPHORIZONTAL:
		case FLIPVERTICAL:
		{
			int Selected = GetSelectedConnectorCount();
			if( GetSelectedLinkCount( true ) > 0 )
				Selected += 2;

			if( Selected < 2 )
				return;

			PushUndo();

			double LowestLocation = DBL_MAX;
			double HighestLocation = -DBL_MAX;

			POSITION Position = m_Connectors.GetHeadPosition();
			while( Position != 0 )
			{
				CConnector *pConnector = m_SelectedConnectors.GetNext( Position );
				if( pConnector == 0 )
					continue;

				if( pConnector->IsSelected() || pConnector->IsLinkSelected() )
				{
					CFPoint Point = pConnector->GetPoint();
					if( Direction == FLIPHORIZONTAL )
					{
						if( Point.x < LowestLocation )
							LowestLocation = Point.x;
						if( Point.x > HighestLocation )
							HighestLocation = Point.x;
					}
					else
					{
						if( Point.y < LowestLocation )
							LowestLocation = Point.y;
						if( Point.y > HighestLocation )
							HighestLocation = Point.y;
					}

					pConnector->SetRPM( -pConnector->GetRPM() );
				}
			}

			double Midpoint = ( LowestLocation + HighestLocation ) / 2;

			Position = m_Connectors.GetHeadPosition();
			while( Position != 0 )
			{
				CConnector *pConnector = m_SelectedConnectors.GetNext( Position );
				if( pConnector == 0 )
					continue;

				if( pConnector->IsSelected() || pConnector->IsLinkSelected() )
				{
					CFPoint Point = pConnector->GetPoint();
					double Distance = 0;
					if( Direction == FLIPHORIZONTAL )
					{
						Distance = Point.x - Midpoint;
						Point.x = Midpoint - Distance;
					}
					else
					{
						Distance = Point.y - Midpoint;
						Point.y = Midpoint - Distance;
					}
					pConnector->SetPoint( Point );
				}
			}
			break;
		}
	}

	FinishChangeSelected();

    SetModifiedFlag( true );

	NormalizeConnectorLinks();

	SetSelectedModifiableCondition();
}

void CLinkageDoc::MakeParallelogramSelected( bool bMakeRectangle )
{
	if( !IsSelectionRectangle() )
		return;

	PushUndo();

	if( bMakeRectangle )
	{
		// Create the right triangle first.
		RawAngleSelected( 90 );
	}

	// Move the last point into position.
	CConnector *pConnectors[4];

	for( int Counter = 0; Counter < 4; ++Counter )
	{
		pConnectors[Counter] = (CConnector*)GetSelectedConnector( Counter );
		if( pConnectors[Counter] == 0 )
			return;
	}

	double dx = 0;
	double dy = 0;

	dx = pConnectors[1]->GetPoint().x - pConnectors[0]->GetPoint().x;
	dy = pConnectors[1]->GetPoint().y - pConnectors[0]->GetPoint().y;

	CFPoint NewPoint( pConnectors[2]->GetPoint().x - dx, pConnectors[2]->GetPoint().y - dy );
	pConnectors[3]->SetPoint( NewPoint );

    SetModifiedFlag( true );

	FinishChangeSelected();

	NormalizeConnectorLinks();

	SetSelectedModifiableCondition();
}

void CLinkageDoc::MakeAnchorSelected( void )
{
	PushUndo();

	POSITION Position = m_Connectors.GetHeadPosition();
	while( Position != NULL )
	{
		POSITION DeletePosition = Position;
		CConnector* pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 || !pConnector->IsSelected() )
			continue;

		pConnector->SetAsAnchor( true );
	}

	FinishChangeSelected();

	SetSelectedModifiableCondition();
}

void CLinkageDoc::CombineSelected( void )
{
	PushUndo();

	int Count = 0;
	CLink *pKeepLink = 0;
	POSITION Position = m_Links.GetHeadPosition();
	while( Position != NULL )
	{
		POSITION DeletePosition = Position;
		CLink* pLink = m_Links.GetNext( Position );
		if( pLink == 0 || !pLink->IsSelected() )
			continue;

		++Count;
		if( Count == 1 )
		{
			pKeepLink = pLink;
			continue;
		}

		POSITION Position2 = pLink->GetConnectorList()->GetHeadPosition();
		while( Position2 != NULL )
		{
			CConnector* pConnector = pLink->GetConnectorList()->GetNext( Position2 );
			if( pConnector == 0 )
				return;
			pConnector->RemoveLink( pLink );
			pConnector->AddLink( pKeepLink );
			pKeepLink->AddConnector( pConnector );
		}
		m_Links.RemoveAt( DeletePosition );
		m_IdentifiedLinks.ClearBit( pLink->GetIdentifier() );
		RemoveGearRatio( 0, pLink );
		delete pLink;
	    SetModifiedFlag( true );
	}

	NormalizeConnectorLinks();

	FinishChangeSelected();

	SetSelectedModifiableCondition();
}

void CLinkageDoc::DeleteSelected( void )
{
	PushUndo();

	POSITION Position = m_Links.GetHeadPosition();
	while( Position != 0 )
	{
		POSITION DeletePosition = Position;
		CLink *pLink = m_Links.GetNext( Position );
		if( pLink == 0 || !pLink->IsSelected() || ( pLink->GetLayers() & m_UsableLayers ) == 0 )
			continue;

		DeleteLink( pLink );

		// Start over because the list may have changed.
		POSITION Position = m_Links.GetHeadPosition();
	}

	Position = m_Connectors.GetHeadPosition();
	while( Position != NULL )
	{
		POSITION DeletePosition = Position;
		CConnector* pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 || !pConnector->IsSelected()  || ( pConnector->GetLayers() & m_UsableLayers ) == 0)
			continue;

		DeleteConnector( pConnector );

		// Start over because the list may have changed.
		POSITION Position = m_Connectors.GetHeadPosition();
	}

	NormalizeConnectorLinks();

	SetSelectedModifiableCondition();
}

void CLinkageDoc::RemoveConnector( CConnector *pConnector )
{
/*
	POSITION Position = m_Links.GetHeadPosition();
	while( Position != 0 )
	{
		CLink *pLink = m_Links.GetNext( Position );
		if( pLink == 0 )
			continue;
		pLink->RemoveConnector( pConnector );
	    SetModifiedFlag( true );
	}

	Position = m_Connectors.GetHeadPosition();
	while( Position != NULL )
	{
		CConnector* pCheckConnector = m_Connectors.GetNext( Position );
		if( pCheckConnector == 0 || pCheckConnector == pConnector )
			continue;

		CConnector* pLimit1 = 0;
		CConnector* pLimit2 = 0;
		if( pCheckConnector->IsSlider() && pCheckConnector->GetSlideLimits( pLimit1, pLimit2 ) )
		{
			if( pConnector == pLimit1 || pConnector == pLimit2 )
				pCheckConnector->SlideBetween();
		}
	}

	RemoveGearRatio( pConnector, 0 );
	//SetSelectedModifiableCondition();
	*/
}

void CLinkageDoc::RemoveLink( CLink *pLink )
{
/*
	POSITION Position = m_Connectors.GetHeadPosition();
	while( Position != 0 )
	{
		CConnector *pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 )
			continue;
		pConnector->RemoveLink( pLink );
		Unfasten( pLink );
	    SetModifiedFlag( true );
		break;
	}
*/
}

bool CLinkageDoc::DeleteLink( CLink *pLink, CConnector *pDeletingConnector )
{
	ConnectorList *pList = pLink->GetConnectorList();
	POSITION Position = pList->GetHeadPosition();
	while( Position != 0 )
	{
		POSITION DeletePosition = Position;
		CConnector* pConnector = pList->GetNext( Position );
		if( pConnector == 0 || pConnector == pDeletingConnector )
			continue;

		if( pConnector->GetLinkCount() == 1 )
			DeleteConnector( pConnector, pLink );
		else
			pConnector->RemoveLink( pLink );
	}

	pLink->RemoveAllConnectors();
	m_Links.Remove( pLink );
	m_SelectedLinks.Remove( pLink );
	m_IdentifiedLinks.ClearBit( pLink->GetIdentifier() );
	RemoveGearRatio( 0, pLink );
	Unfasten( pLink );
	delete pLink;

	SetModifiedFlag( true );

	return true;
}

bool CLinkageDoc::DeleteConnector( CConnector *pConnector, CLink *pDeletingLink )
{
	CList<CLink*,CLink*> *pList = pConnector->GetLinkList();
	POSITION Position = pList->GetHeadPosition();
	while( Position != NULL )
	{
		POSITION DeletePosition = Position;
		CLink *pLink = pList->GetNext( Position );
		if( pLink == 0 || pLink == pDeletingLink )
			continue;

		if( pLink->GetConnectorCount() == 1 )
		{
			DeleteLink( pLink, pConnector );
			continue;
		}

		pLink->RemoveConnector( pConnector );
	}

	CConnector* pLimit1 = 0;
	CConnector* pLimit2 = 0;
	pConnector->RemoveAllLinks();
	pConnector->SlideBetween();

	Position = m_Connectors.GetHeadPosition();
	while( Position != NULL )
	{
		CConnector* pCheckConnector = m_Connectors.GetNext( Position );
		if( pCheckConnector == 0 || pCheckConnector == pConnector )
			continue;

		if( pCheckConnector->IsSlider() && pCheckConnector->GetSlideLimits( pLimit1, pLimit2 ) )
		{
			if( pConnector == pLimit1 || pConnector == pLimit2 )
				pCheckConnector->SlideBetween();
		}
	}

	RemoveGearRatio( pConnector, 0 );
	Unfasten( pConnector );
	m_Connectors.Remove( pConnector );
	m_SelectedConnectors.Remove( pConnector );
	delete pConnector;

	return true;
}

void CLinkageDoc::NormalizeConnectorLinks( void )
{
	POSITION Position = m_Links.GetHeadPosition();
	while( Position != 0 )
	{
		POSITION DeletePosition = Position;
		CLink *pLink = m_Links.GetNext( Position );
		if( pLink == 0 )
			continue;

		POSITION Position = pLink->GetConnectorList()->GetHeadPosition();
		while( Position != NULL )
		{
			CConnector* pCheckConnector = pLink->GetConnectorList()->GetNext( Position );
			if( pCheckConnector == 0 )
				continue;
			if( pCheckConnector->IsSelected() )
			{
				pLink->UpdateControlKnobs();
				break;
			}
		}

		CConnector *pConnector = 0;

		if( pLink->GetConnectorCount() == 1 )
			pConnector = pLink->GetConnectorList()->GetHead();
		else if( pLink->GetConnectorCount() > 1 )
			continue;

		// Detect when there is a link with a single connector and that single connector
		// is also on another link. This is a meaningless configuration unless the single-connector
		// link is a gear.

		if( pConnector == 0 || ( pConnector->GetLinkCount() > 1 && !pLink->IsGear() ) )
		{
			Unfasten( pLink );
			pLink->RemoveAllConnectors();
			if( pConnector != 0 )
				pConnector->RemoveLink( pLink );
			m_Links.RemoveAt( DeletePosition );
			m_SelectedLinks.Remove( pLink );
			m_IdentifiedLinks.ClearBit( pLink->GetIdentifier() );
			RemoveGearRatio( 0, pLink );
			delete pLink;
		    SetModifiedFlag( true );
		}
	}
}

void CLinkageDoc::KeepLayerSelections( unsigned int Layers )
{
	bool bChanged = false;
	// Make sure that nothing is selected unless it is on one of the specified layers.
	POSITION Position = m_Connectors.GetHeadPosition();
	while( Position != NULL )
	{
		CConnector* pConnector = m_Connectors.GetNext( Position );
		if( pConnector != 0 && ( pConnector->GetLayers() & Layers ) == 0 )
		{
			DeSelectElement( pConnector );
			bChanged = true;
		}
	}

	Position = m_Links.GetHeadPosition();
	while( Position != NULL )
	{
		CLink* pLink = m_Links.GetNext( Position );
		if( pLink != 0 && ( pLink->GetLayers() & Layers ) == 0 )
		{
			DeSelectElement( pLink );
			bChanged = true;
		}
	}
}

void CLinkageDoc::SetViewLayers( unsigned int Layers )
{
	m_ViewLayers = Layers;
	m_UsableLayers = m_EditLayers & m_ViewLayers;
	KeepLayerSelections( m_UsableLayers );
}

void CLinkageDoc::SetEditLayers( unsigned int Layers )
{
	m_EditLayers = Layers;
	m_UsableLayers = m_EditLayers & m_ViewLayers;
	KeepLayerSelections( m_UsableLayers );
}

int CLinkageDoc::GetSelectedConnectorCount( void )
{
	POSITION Position = m_Connectors.GetHeadPosition();
	int Count = 0;
	while( Position != NULL )
	{
		CConnector* pCheckConnector = m_Connectors.GetNext( Position );
		if( pCheckConnector != 0 && !pCheckConnector->IsSelected() )
			continue;
		++Count;
	}
	return Count;
}

int CLinkageDoc::GetSelectedLinkCount( bool bOnlyWithMultipleConnectors )
{
	int Count = 0;
	POSITION Position = m_SelectedLinks.GetHeadPosition();
	while( Position != NULL )
	{
		CLink* pLink = m_SelectedLinks.GetNext( Position );
		if( pLink == 0 )
			continue;

		if( pLink->IsSelected() )
		{
			if( !bOnlyWithMultipleConnectors || pLink->GetConnectorCount() > 1 )
				++Count;
		}
	}

	return Count;
}

bool CLinkageDoc::FindRoomFor( CFRect NeedRect, CFPoint &PlaceHere )
{
	PlaceHere.x -= NeedRect.Width() / 2;
	PlaceHere.y -= NeedRect.Height() / 2;
	for( int Counter = 0; Counter < 2000; ++Counter )
	{
		CFRect TestRect( PlaceHere.x, PlaceHere.y, PlaceHere.x+NeedRect.Width(), PlaceHere.y+NeedRect.Height() );

		bool bOverlapped = false;
		POSITION Position = m_Links.GetHeadPosition();
		while( Position != NULL )
		{
			CLink* pLink = m_Links.GetNext( Position );
			if( pLink == 0 || ( pLink->GetLayers() & m_UsableLayers ) == 0 )
				continue;
			CFRect Rect;
			pLink->GetArea( m_GearConnectionList, Rect );
			Rect.InflateRect( 8, 8 );

			if( TestRect.IsOverlapped( Rect ) )
			{
				bOverlapped = true;
				break;
			}
		}
		if( !bOverlapped )
		{
			PlaceHere.x = TestRect.left;
			PlaceHere.y = TestRect.top;
			return true;
		}

		PlaceHere.x += 3;
		PlaceHere.y += 1;
	}
	return false;
}

CLink * CLinkageDoc::InsertLink( unsigned int Layers, double ScaleFactor, CFPoint DesiredPoint, bool bForceToPoint, int ConnectorCount, _InsertType Type, bool bSolid )
{
	static bool bSelectInsert = true;

	if( ConnectorCount == 0 )
		return 0;

	if( ( Layers & m_EditLayers ) == 0 )
		return 0;

	static const int MAX_CONNECTOR_COUNT = 4;

	/*
	* right now, there is no clever way to generate the location for
	* the new connectors so 3 is the maximum.
	*/
	if( ConnectorCount > MAX_CONNECTOR_COUNT )
		return 0;

	PushUndo();

	ClearSelection();

	static CFPoint AddPoints[MAX_CONNECTOR_COUNT][MAX_CONNECTOR_COUNT] =
	{
		{ CFPoint( 0, 0 ), CFPoint( 0, 0 ), CFPoint( 0, 0 ), CFPoint( 0, 0 ) },
		{ CFPoint( 0, 0 ), CFPoint( 1.0, 1.0 ), CFPoint( 0, 0 ), CFPoint( 0, 0 ) },
		{ CFPoint( 0, 0 ), CFPoint( 0, 1.0 ), CFPoint( 1.0, 1.0 ), CFPoint( 0, 0 ) },
		{ CFPoint( 0, 0 ), CFPoint( 0, 1.0 ), CFPoint( 1.0, 1.0 ), CFPoint( 1.0, 0 ) }
	};

	CConnector **Connectors = new CConnector* [ConnectorCount];
	if( Connectors == 0 )
		return 0;
	int Index;
	for( Index = 0; Index < ConnectorCount; ++Index )
		Connectors[Index] = 0;

	CLink *pLink = new CLink;
	if( pLink == 0 )
	{
		delete [] Connectors;
		return 0;
	}

	bool bAnchor = Type == ANCHOR || Type == INPUT_ANCHOR;
	bool bRotating = Type == INPUT_ANCHOR;

	for( Index = 0; Index < ConnectorCount; ++Index )
	{
		CConnector *pConnector = new CConnector;
		if( pConnector == 0 )
		{
			for( int Counter = 0; Counter < ConnectorCount; ++Counter )
			{
				if( Connectors[Counter] != 0 )
					delete Connectors[Counter];
			}
			delete [] Connectors;
			RemoveGearRatio( 0, pLink );
			delete pLink;
			return 0;
		}
		Connectors[Index] = pConnector;
		pConnector->SetLayers( Layers );
		pConnector->SetAsAnchor( bAnchor );
		pConnector->SetAsInput( bRotating );
		pConnector->SetRPM( DEFAULT_RPM );
		pConnector->SetPoint( CFPoint( AddPoints[ConnectorCount-1][Index].x * ScaleFactor, AddPoints[ConnectorCount-1][Index].y * ScaleFactor ) );
		pConnector->AddLink( pLink );
		pConnector->SetDrawCircleRadius( Type == CIRCLE ? ScaleFactor : 0 );
		pLink->AddConnector( pConnector );

		bAnchor = false;
		bRotating = false;
	}

	CFRect Rect( 0, 0, 0, 0 );

	pLink->GetArea( m_GearConnectionList, Rect );
	CFPoint Offset( -Rect.left, -Rect.bottom );
	Rect.left += Offset.x;
	Rect.top += Offset.x;
	Rect.right += Offset.x;
	Rect.bottom += Offset.x;

	Rect.right += 16;
	Rect.bottom += 16;

	CFPoint Place = DesiredPoint;
	if( !bForceToPoint )
		FindRoomFor( Rect, Place );
//	if( !bForceToPoint && !FindRoomFor( Rect, Place ) )
//	{
//		for( int Counter = 0; Counter < ConnectorCount; ++Counter )
//		{
//			if( Connectors[Counter] != 0 )
//				delete Connectors[Counter];
//		}
//		delete [] Connectors;
//		RemoveGearRatio( 0, pLink );
//		delete pLink;
//		return;
//	}

	int NewID = m_IdentifiedLinks.FindAndSetBit();
	if( NewID > m_HighestLinkID )
		m_HighestLinkID = NewID;
	pLink->SetIdentifier( NewID );
	m_Links.AddTail( pLink );
	for( int Counter = 0; Counter < ConnectorCount; ++Counter )
	{
		CFPoint Point = Connectors[Counter]->GetOriginalPoint();
		Point += Place;
		Point += Offset;
		Connectors[Counter]->SetPoint( Point );
		int NewID = m_IdentifiedConnectors.FindAndSetBit();
		if( NewID > m_HighestConnectorID )
			m_HighestConnectorID = NewID;
		Connectors[Counter]->SetIdentifier( NewID );
		Connectors[Counter]->SetColor( ( Layers & DRAWINGLAYER ) != 0 ? RGB( 200, 200, 200 ) : Colors[Connectors[Counter]->GetIdentifier() % COLORS_COUNT] );
		m_Connectors.AddTail( Connectors[Counter] );
		if( bSelectInsert && ( m_UsableLayers & Layers ) != 0 )
			SelectElement( Connectors[Counter] );
	}

	pLink->SetLayers( Layers );
	pLink->SetActuator( ConnectorCount && Type == ACTUATOR );
	pLink->SetCPM( DEFAULT_RPM );
	pLink->SetSolid( bSolid );
	pLink->SetGear( Type == GEAR_LINK );
	pLink->SetColor( ( Layers & DRAWINGLAYER ) != 0 ? RGB( 200, 200, 200 ) : Colors[pLink->GetIdentifier() % COLORS_COUNT] );
	pLink->SetMeasurementElement( ( Type == MEASUREMENT || Type == ANGLE_MEASUREMENT ) && ( Layers & DRAWINGLAYER ) != 0, false );
	if( Type == ANGLE_MEASUREMENT )
		pLink->SetMeasurementUseAngles( true );
	if( pLink->IsMeasurementElement() )
		pLink->SetShapeType( CLink::POLYLINE );
	if( bSelectInsert && ( m_UsableLayers & Layers ) != 0 )
		SelectElement( pLink );

	SetSelectedModifiableCondition();

    SetModifiedFlag( true );

	FinishChangeSelected();

	delete [] Connectors;

	return pLink;
}

void CLinkageDoc::DeleteContents( bool bDeleteUndoInfo )
{
	POSITION Position = m_GearConnectionList.GetHeadPosition();
	while( Position != 0 )
	{
		CGearConnection *pGearConnection = m_GearConnectionList.GetNext( Position );
		if( pGearConnection != 0 )
			delete pGearConnection;
	}
	m_GearConnectionList.RemoveAll();

	Position = m_Connectors.GetHeadPosition();
	while( Position != NULL )
	{
		CConnector* pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 )
			continue;
		delete pConnector;
	}
	m_Connectors.RemoveAll();

	Position = m_Links.GetHeadPosition();
	while( Position != NULL )
	{
		CLink* pLink = m_Links.GetNext( Position );
		if( pLink == 0 )
			continue;
		delete pLink;
	}
	m_Links.RemoveAll();



	// This, and even the code to delete gear connections in one of the loops above, is redundant. Better safe than sorry.
	Position = m_GearConnectionList.GetHeadPosition();
	while( Position != 0 )
	{
		CGearConnection *pGearConnection = m_GearConnectionList.GetNext( Position );
		if( pGearConnection == 0 )
			continue;
		delete pGearConnection;
	}
	m_GearConnectionList.RemoveAll();

	if( bDeleteUndoInfo )
	{
		for( std::deque<CMemorySaveRecord*>::iterator it = m_UndoStack.begin(); it != m_UndoStack.end(); ++it )
			delete *it;
		m_UndoStack.clear();
		for( std::deque<CMemorySaveRecord*>::iterator it = m_RedoStack.begin(); it != m_RedoStack.end(); ++it )
			delete *it;
		m_RedoStack.clear();
	}

	m_pCapturedConnector = 0;
	m_pCapturedConrolKnob = 0;
	m_SelectedConnectors.RemoveAll();

	m_IdentifiedConnectors.SetLength( 0 );
	m_IdentifiedLinks.SetLength( 0 );
	m_HighestConnectorID = -1;
	m_HighestLinkID = -1;
}

void CLinkageDoc::DeleteContents()
{
	DeleteContents( true );
	CDocument::DeleteContents();
}

void CLinkageDoc::SelectAll( void )
{
	POSITION Position = m_Connectors.GetHeadPosition();
	while( Position != NULL )
	{
		CConnector* pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 || ( pConnector->GetLayers() & m_UsableLayers ) == 0 )
			continue;
		SelectElement( pConnector );
	}
	Position = m_Links.GetHeadPosition();
	while( Position != NULL )
	{
		CLink* pLink = m_Links.GetNext( Position );
		if( pLink == 0 || ( pLink->GetLayers() & m_UsableLayers ) == 0 )
			continue;
		SelectElement( pLink );
	}
}

void CLinkageDoc::SplitSelected( void )
{
	bool bNeedPush = true;

	POSITION Position = m_Connectors.GetHeadPosition();
	while( Position != NULL )
	{
		POSITION DeletePosition = Position;
		CConnector* pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 || !pConnector->IsSelected() )
			continue;

		CFPoint Point = pConnector->GetOriginalPoint();

		if( pConnector->IsSlider() )
		{
			if( bNeedPush )
				PushUndo();
			bNeedPush = false;

			// Remove the slid limits when splitting a slider. A second split might
			// be needed to actually split this into two connectors if possible.
			pConnector->SlideBetween();
			continue;
		}

		if( pConnector->GetLinkCount() <= 1 )
			continue;

		// Deal with all Links that are using this connector.
		// Remove this connector from the Link and then add
		// a new connector.

		int SplitLinkCount = 0;
		POSITION Position2 = pConnector->GetLinkList()->GetHeadPosition();
		while( Position2 != 0 )
		{
			if( bNeedPush )
				PushUndo();
			bNeedPush = false;

			CLink *pLink = pConnector->GetLinkList()->GetNext( Position2 );
			if( pLink == 0 )
				continue;

			++SplitLinkCount;
		}

		if( SplitLinkCount > 1 )
		{
			Position2 = pConnector->GetLinkList()->GetHeadPosition();
			while( Position2 != 0 )
			{
				CLink *pLink = pConnector->GetLinkList()->GetNext( Position2 );
				if( pLink == 0 )
					continue;

				CConnector *pNewConnector = new CConnector( *pConnector );
				if( pNewConnector == 0 )
					continue;

				if( bNeedPush )
					PushUndo();
				bNeedPush = false;

				pLink->RemoveConnector( pConnector );
				pLink->AddConnector( pNewConnector );
				pNewConnector->AddLink( pLink );
				int NewID = m_IdentifiedConnectors.FindAndSetBit();
				if( NewID > m_HighestConnectorID )
					m_HighestConnectorID = NewID;
				pNewConnector->SetIdentifier( NewID );
				m_Connectors.AddTail( pNewConnector );
				if( pConnector->IsSelected() )
					SelectElement( pNewConnector );

//				pNewConnector->SetLayers( pConnector->GetLayers() );
//				pNewConnector->SetAsAnchor( pConnector->IsAnchor() );
//				pNewConnector->SetAsInput( pConnector->IsInput() );
//				pNewConnector->SetRPM( pConnector->GetRPM() );
//				pNewConnector->SetPoint( Point );

				SetModifiedFlag( true );
			}

			RemoveGearRatio( pConnector, 0 );
			pConnector->RemoveAllLinks();
			DeleteConnector( pConnector );
		}
	}

	if( !bNeedPush )
	{
		NormalizeConnectorLinks();
		FixupSliderLocations();
		FinishChangeSelected();
	}
}

void CLinkageDoc::Copy( bool bCut )
{
    CMemFile mFile;
    CArchive ar( &mFile,CArchive::store );
    WriteOut( ar, false, true );
    ar.Write( "\0", 1 );
    ar.Close();

    int Size = (int)mFile.GetLength();
    BYTE* pData = mFile.Detach();

    UINT CF_Linkage = RegisterClipboardFormat( "RECTORSQUID_Linkage_CLIPBOARD_XML_FORMAT" );
    CWnd *pWnd = AfxGetMainWnd();
    ::OpenClipboard( pWnd == 0 ? 0 : pWnd->GetSafeHwnd() );
    ::EmptyClipboard();
    HANDLE hTextMemory = ::GlobalAlloc( NULL, Size );
    HANDLE hMemory = ::GlobalAlloc( NULL, Size );
    char *pTextMemory = (char*)::GlobalLock( hTextMemory );
    char *pMemory = (char*)::GlobalLock( hMemory );

    memcpy( pTextMemory, pData, Size );
    memcpy( pMemory, pData, Size );

    ::GlobalUnlock( hTextMemory );
    ::GlobalUnlock( hMemory );
    ::SetClipboardData( CF_TEXT, hTextMemory );
    ::SetClipboardData( CF_Linkage, hMemory );
    ::CloseClipboard();

    if( bCut )
		DeleteSelected();
}

void CLinkageDoc::Paste( void )
{
	CWnd *pWnd = AfxGetMainWnd();
	if( !::OpenClipboard( pWnd == 0 ? 0 : pWnd->GetSafeHwnd() ) )
		return;

	UINT CF_Linkage = RegisterClipboardFormat( "RECTORSQUID_Linkage_CLIPBOARD_XML_FORMAT" );
	UINT UseFormat = CF_Linkage;
    if( ::IsClipboardFormatAvailable( CF_Linkage ) == 0 )
	{
		if( ::IsClipboardFormatAvailable( CF_TEXT ) == 0 )
		{
			AfxMessageBox( "The pasted data is not recognized. It was ignored." );
			return;
		}
		UseFormat = CF_TEXT;
	}
    HANDLE hMemory = ::GetClipboardData( UseFormat );
    if( hMemory == 0 )
    {
		::CloseClipboard();
		return;
    }
    BYTE *pMemory = (BYTE*)GlobalLock( hMemory );
    if( pMemory == 0 )
    {
		GlobalUnlock( hMemory );
		::CloseClipboard();
		return;
    }

	// Test this if it's not the linkage format
	if( UseFormat != CF_Linkage )
	{
		// We only check for CF_TEXT right now so abort if it's not text.
		if( UseFormat != CF_TEXT )
		{
			AfxMessageBox( "The pasted data is not recognized. It was ignored." );
			return;
		}
		int Size = (int)strlen( (char*)pMemory );
		if( Size == 0 )
		{
			GlobalUnlock( hMemory );
			::CloseClipboard();
			return;
		}

		// Test the data to see if it might parse ok.
		QuickXMLNode XMLData;
		if( XMLData.Parse( (char*)pMemory ) )
		{
			QuickXMLNode *pRootNode = XMLData.GetFirstChild();
			if( pRootNode == 0 || !pRootNode->IsElement() || pRootNode->GetText() != "linkage2" )
			{
				AfxMessageBox( "The pasted data is not recognized. It was ignored." );
				return;
			}
		}
	}

	SelectElement();

    /*
     * There is  no way to query how much data is available on the clipboard.
     * The application (us) is responsible for figuring it out based on the
     * clipboard format. In our case, the data is a null terminated string
     * a strlen will give us the size. It is not a comfortable operation
     * since someone could write data in "our" format but leave off the
     * terminating null.
     */
    int Size = (int)strlen( (char*)pMemory );
    if( Size == 0 )
    {
		GlobalUnlock( hMemory );
		::CloseClipboard();
		return;
    }

    CMemFile mFile;
    mFile.Attach( pMemory, Size, 0 );
    CArchive ar( &mFile, CArchive::load );

	CFrameWnd *pFrame = (CFrameWnd*)AfxGetMainWnd();
	CLinkageView *pView = pFrame == 0 ? 0 : (CLinkageView*)pFrame->GetActiveView();
	if( pView != 0 )
		pView->HighlightSelected( true );

    PushUndo();

    ReadIn( ar, true, false, false, true, false );
    mFile.Detach();
    ::GlobalUnlock( hMemory );
    ::CloseClipboard();

    SetModifiedFlag( true );
}

void CLinkageDoc::SelectSample( int Index )
{
	CSampleGallery GalleryData;

	CString ExampleData;
	CString ExampleName;
	ExampleData = GalleryData.GetXMLData( Index );
	ExampleName.LoadString( GalleryData.GetStringID( Index ) );

	int EOL = ExampleName.Find( '\n' );
	if( EOL >= 0 )
		ExampleName = ExampleName.Mid( EOL + 1 );

	ExampleName += " Example";

	if (!SaveModified())
		return;

	ClearDocument();

	SetTitle( ExampleName );

    CMemFile mFile;
    mFile.Attach( (BYTE*)ExampleData.GetBuffer(), ExampleData.GetLength(), 0 );
    CArchive ar( &mFile, CArchive::load );

    ReadIn( ar, false, true, false, false, true );
    mFile.Detach();
    ExampleData.ReleaseBuffer();
    UpdateAllViews( 0 );
}

void CLinkageDoc::GetDocumentArea( CFRect &BoundingRect, bool bSelectedOnly )
{
	BoundingRect.SetRect( DBL_MAX, -DBL_MAX, -DBL_MAX, DBL_MAX );
	POSITION Position = m_Links.GetHeadPosition();
	CFRect Rect;
	while( Position != 0 )
	{
		CLink* pLink = m_Links.GetNext( Position );
		if( pLink == 0 )
			continue;
		if( bSelectedOnly && !pLink->IsSelected() )
			continue;
		pLink->GetArea( m_GearConnectionList, Rect );
		BoundingRect.left = min( BoundingRect.left, Rect.left );
		BoundingRect.right = max( BoundingRect.right, Rect.right );
		BoundingRect.top = max( BoundingRect.top, Rect.top );
		BoundingRect.bottom = min( BoundingRect.bottom, Rect.bottom );
	}

	Position = m_Connectors.GetHeadPosition();
	while( Position != 0 )
	{
		CConnector *pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 || !pConnector->IsSelected() )
			continue;
		pConnector->GetArea( Rect );
		BoundingRect.left = min( BoundingRect.left, Rect.left );
		BoundingRect.right = max( BoundingRect.right, Rect.right );
		BoundingRect.top = max( BoundingRect.top, Rect.top );
		BoundingRect.bottom = min( BoundingRect.bottom, Rect.bottom );
	}

	if( BoundingRect.left == DBL_MAX
		|| BoundingRect.top == -DBL_MAX
		|| BoundingRect.bottom == DBL_MAX
		|| BoundingRect.right == -DBL_MAX )
	BoundingRect.SetRect( 0, 0, 0, 0 );
}

void CLinkageDoc::GetDocumentAdjustArea( CFRect &BoundingRect, bool bSelectedOnly )
{
	BoundingRect.SetRect( DBL_MAX, -DBL_MAX, -DBL_MAX, DBL_MAX );
	POSITION Position = m_Links.GetHeadPosition();
	CFRect Rect;
	while( Position != 0 )
	{
		CLink* pLink = m_Links.GetNext( Position );
		if( pLink == 0 )
			continue;
		if( bSelectedOnly && !pLink->IsSelected() )
			continue;
		pLink->GetAdjustArea( m_GearConnectionList, Rect );
		BoundingRect.left = min( BoundingRect.left, Rect.left );
		BoundingRect.right = max( BoundingRect.right, Rect.right );
		BoundingRect.top = max( BoundingRect.top, Rect.top );
		BoundingRect.bottom = min( BoundingRect.bottom, Rect.bottom );
	}

	if( bSelectedOnly )
	{
		// If checking selected elements then check individual connectors.
		POSITION Position = m_Connectors.GetHeadPosition();
		while( Position != 0 )
		{
			CConnector *pConnector = m_Connectors.GetNext( Position );
			if( pConnector == 0 || !pConnector->IsSelected() )
				continue;
			pConnector->GetAdjustArea( Rect );
			BoundingRect.left = min( BoundingRect.left, Rect.left );
			BoundingRect.right = max( BoundingRect.right, Rect.right );
			BoundingRect.top = max( BoundingRect.top, Rect.top );
			BoundingRect.bottom = min( BoundingRect.bottom, Rect.bottom );
		}
	}

	if( BoundingRect.left == DBL_MAX
		|| BoundingRect.top == -DBL_MAX
		|| BoundingRect.bottom == DBL_MAX
		|| BoundingRect.right == -DBL_MAX )
	BoundingRect.SetRect( 0, 0, 0, 0 );
}

bool CLinkageDoc::IsAnySelected( void )
{
	return GetSelectedConnectorCount() + GetSelectedLinkCount( false ) > 0;
}

bool CLinkageDoc::IsSelectionAdjustable( void )
{
	int ConnectorCount = GetSelectedConnectorCount();
	int LinkCount = GetSelectedLinkCount( false );

	if( ConnectorCount > 1 || LinkCount > 1 )
		return true;

	if( LinkCount == 1 )
	{
		// If it has more than a single connector then it can be adjusted.
		POSITION Position = m_Links.GetHeadPosition();
		while( Position != NULL )
		{
			CLink* pLink = m_Links.GetNext( Position );
			if( pLink == 0 || !pLink->IsSelected() )
				continue;

			return pLink->GetConnectorCount() > 1;
		}
	}

	return false;
}



CMemorySaveRecord::CMemorySaveRecord( BYTE *pData )
{
	m_pData = pData;
}

void CMemorySaveRecord::ClearContent( BYTE **ppData )
{
	if( ppData == 0 )
		return;
	*ppData = m_pData;
	m_pData = 0;
}

CMemorySaveRecord::~CMemorySaveRecord()
{
	if( m_pData != 0 )
		delete [] m_pData;
	m_pData = 0;
}

void CLinkageDoc::PushUndoRedo( std::deque<class CMemorySaveRecord *> &TheStack )
{

	CMemFile mFile;
	CArchive ar( &mFile,CArchive::store );
	WriteOut( ar, false, false );
	ar.Write( "\0", 1 );
	ar.Close();

	int Size = (int)mFile.GetLength();
	BYTE* pData = mFile.Detach();

	CMemorySaveRecord *pNewRecord = new CMemorySaveRecord( pData );
	if( pNewRecord == 0 )
		return;

	if( TheStack.size() > MAX_UNDO )
	{
		CMemorySaveRecord *pRemoving = TheStack.back();
		TheStack.pop_back();
		if( pRemoving != 0 )
			delete pRemoving;
	}

	TheStack.push_front( pNewRecord );
}

void CLinkageDoc::PushUndo( void )
{
	PushUndoRedo( m_UndoStack );

	// Also clear the redo stack since it is no longer valid (the user has essentially created a new timeline and the old one is obsolete).
	for( std::deque<CMemorySaveRecord*>::iterator it = m_RedoStack.begin(); it != m_RedoStack.end(); ++it )
		delete *it;
	m_RedoStack.clear();

    SetModifiedFlag( true );
}

void CLinkageDoc::PushRedo( void )
{
	PushUndoRedo( m_RedoStack );

	SetModifiedFlag( true );
}

void CLinkageDoc::PopUndo( void )
{
	PushUndoRedo( m_RedoStack );
	PopUndoRedo( m_UndoStack );
}

void CLinkageDoc::PopRedo( void )
{
	PushUndoRedo( m_UndoStack );
	PopUndoRedo( m_RedoStack );
}

bool CLinkageDoc::PopUndoRedo( std::deque<CMemorySaveRecord*> &TheStack )
{
	if( TheStack.size() == 0 )
		return false;

	DeleteContents( false );

	CMemorySaveRecord *pUsing = TheStack.front();
	TheStack.pop_front();

	BYTE *pData = 0;
	pUsing->ClearContent( &pData );
	delete pUsing;

	CMemFile mFile;
	mFile.Attach( pData, (int)strlen( (char*)pData ), 0 );
	CArchive ar( &mFile, CArchive::load );

	ReadIn( ar, false, false, true, false, false );
	mFile.Detach();

	// Set the identifier bits in the new document.
	POSITION Position = m_Connectors.GetHeadPosition();
	while( Position != 0 )
	{
		CConnector *pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 )
			continue;
		int NewID = pConnector->GetIdentifier();
		if( NewID > m_HighestConnectorID )
			m_HighestConnectorID = NewID;
		m_IdentifiedConnectors.SetBit( NewID );
	}

	Position = m_Links.GetHeadPosition();
	while( Position != 0 )
	{
		CLink *pLink = m_Links.GetNext( Position );
		if( pLink == 0 )
			continue;
		int NewID = pLink->GetIdentifier();
		if( NewID > m_HighestLinkID )
			m_HighestLinkID = NewID;
		m_IdentifiedLinks.SetBit( NewID );
	}

	SetModifiedFlag( true );

	UpdateAllViews( 0, 1 );

	return true;
}

bool CLinkageDoc::ConnectSliderLimits( bool bTestOnly )
{
	if( !bTestOnly )
		PushUndo();

	if( GetSelectedConnectorCount() != 3 )
		return false;

	CConnector *pUseConnectors[3] = { 0, 0, 0 };

	int Index = 0;
	POSITION Position = m_Connectors.GetHeadPosition();
	while( Position != NULL )
	{
		CConnector* pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 || !pConnector->IsSelected() )
			continue;

		pUseConnectors[Index++] = pConnector;
	}

	int FirstLimit = -1;
	int SecondLimit = -1;
	int Slider = -1;

	// Make sure that two are on the same link and the other is not.
	int ShareCount = 0;
	if( pUseConnectors[0]->IsSharingLink( pUseConnectors[1] ) != 0 )
	{
		++ShareCount;
		FirstLimit = 0;
		SecondLimit = 1;
		Slider = 2;
	}
	if( pUseConnectors[0]->IsSharingLink( pUseConnectors[2] ) != 0 )
	{
		++ShareCount;
		FirstLimit = 0;
		SecondLimit = 2;
		Slider = 1;
	}
	if( pUseConnectors[1]->IsSharingLink( pUseConnectors[2] ) != 0 )
	{
		++ShareCount;
		FirstLimit = 1;
		SecondLimit = 2;
		Slider = 0;
	}

	if( ShareCount != 1 )
		return false;

	if( bTestOnly )
		return true;

	if( !pUseConnectors[Slider]->SlideBetween( pUseConnectors[FirstLimit], pUseConnectors[SecondLimit] ) )
		return false;

	// Test code... pUseConnectors[Slider]->SetSlideRadius( 400 );

	//pUseConnectors[Slider]->SetPoint( pUseConnectors[FirstLimit]->GetPoint().MidPoint( pUseConnectors[SecondLimit]->GetPoint(), .5 ) );
	FixupSliderLocation( pUseConnectors[Slider] );

	Position = pUseConnectors[Slider]->GetLinkList()->GetHeadPosition();
	while( Position != 0 )
	{
		CLink *pLink = pUseConnectors[Slider]->GetLinkList()->GetNext( Position );
		if( pLink == 0 )
			continue;
		pLink->UpdateControlKnobs();
	}

    SetModifiedFlag( true );

	FinishChangeSelected();

    return true;
}

static unsigned int NumberOfSetBits( unsigned int Value )
{
    Value = Value - ((Value >> 1) & 0x55555555);
    Value = (Value & 0x33333333) + ((Value >> 2) & 0x33333333);
    return (((Value + (Value >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

void CLinkageDoc::SetSelectedModifiableCondition( void )
{
	int SelectedDrawingElements = 0;
	int SelectedMechanismLinks = 0;
	int SelectedMechanismConnectors = 0;
	int SelectedGears = 0;
	int SelectedNonGears = 0;


	int SelectedRealLinks = 0;
	int SelectedLinks = 0;
	int SelectedConnectors = 0;
	int SelectedAnchors = 0;
	//int SelectedMechanismLinks = 0;
	//int SelectedDrawingElements = 0;
	int Actuators = 0;
	int Sliders = 0;
	//int SelectedGears = 0;
	int FastenedCount = 0;
	int GearFastenToCount = 0;
	int SelectedLinkConnectorCount = 0;

	CConnector *pFastenToConnector = 0;
	CLink *pSelectedGear = 0;
	CLink *pSelectedLink = 0;

	CConnector *pLimit1 = 0;
	CConnector *pLimit2 = 0;
	bool bSlidersShareLimits = true;

	m_SelectedLayers = 0;

	POSITION Position = m_Connectors.GetHeadPosition();
	while( Position != 0 )
	{
		CConnector *pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 )
			continue;
		if( pConnector->IsSelected() )
		{
			m_SelectedLayers |= pConnector->GetLayers();
			++SelectedConnectors;
			if( pConnector->IsSlider() )
			{
				++Sliders;
				CConnector *pTemp1 = 0;
				CConnector *pTemp2 = 0;
				pConnector->GetSlideLimits( pTemp1, pTemp2 );
				if( ( pLimit1 != 0 && pTemp1 != pLimit1 ) || ( pLimit2 != 0 && pTemp2 != pLimit2 ) )
					bSlidersShareLimits = false;
				pLimit1 = pTemp1;
				pLimit2 = pTemp2;
			}
			if( pConnector->IsAnchor() )
				++SelectedAnchors;
			if( pConnector->GetFastenedTo() != 0 )
				++FastenedCount;

			if( pConnector->GetLayers() & DRAWINGLAYER )
				++SelectedDrawingElements;
			else
				++SelectedMechanismConnectors;

			++SelectedNonGears;

			// This is tentative because I dont know if people will expect ALL fastened elements to become unfastened
			// from this element, or if they only expect to use the unfasten action when a fastened thing is selected.
			if( pConnector->GetFastenedElementList()->GetCount() > 0 )
				++FastenedCount;

			if( pConnector->IsAnchor() && !pConnector->IsInput() )
			{
				++GearFastenToCount;
				pFastenToConnector = pConnector;
			}
		}
	}

	CLink *pFastenToLink = 0;

	Position = m_Links.GetHeadPosition();
	while( Position != 0 )
	{
		CLink *pLink = m_Links.GetNext( Position );
		if( pLink == 0 )
			continue;
		if( pLink->IsSelected( false ) )
		{
			SelectedLinkConnectorCount = pLink->GetConnectorCount();

			if( pLink->IsGear() )
			{
				++SelectedGears;
				pSelectedGear = pLink;
			}

			if( SelectedLinkConnectorCount <= 1 )
				continue;

			++SelectedLinks;

			if( SelectedLinkConnectorCount > 1 || pLink->IsGear() ) // Lone connectors don't count as real links!
				++SelectedRealLinks;
			if( pLink->IsActuator() )
				++Actuators;
			if( !pLink->IsGear() )
			{
				++SelectedNonGears;
				pSelectedLink = pLink;
			}
				m_SelectedLayers |= pLink->GetLayers();
			if( pLink->GetLayers() & MECHANISMLAYERS )
			{
				++SelectedMechanismLinks;
				if( !pLink->IsGear() && SelectedLinkConnectorCount > 1 )
					pFastenToLink = pLink;
			}
			if( pLink->GetLayers() & DRAWINGLAYER )
				++SelectedDrawingElements;
			if( pLink->GetFastenedTo() != 0 )
				++FastenedCount;
			// This is tentative because I dont know if people will expect ALL fastened elements to become unfastened
			// from this element, or if they only expect to use the unfasten action when a fastened thing is selected.
			if( pLink->GetFastenedElementList()->GetCount() > 0 )
				++FastenedCount;
			if( !pLink->IsGear() && SelectedLinkConnectorCount > 1 )
				++GearFastenToCount;

			if( pLink->GetLayers() & DRAWINGLAYER )
				++SelectedDrawingElements;
		}
	}

	CConnector* pConnector1 = GetSelectedConnector( 0 );

	if( NumberOfSetBits( m_SelectedLayers ) > 1 )
	{
		m_bSelectionCombinable = false;
		m_bSelectionConnectable = false;
		m_bSelectionJoinable = false;
		m_bSelectionSlideable = false;
		m_bSelectionLockable = false;
		m_bSelectionMakeAnchor = false;
	}
	else
	{
		CConnector* pConnector2 = GetSelectedConnector( 1 );
		m_bSelectionCombinable = SelectedRealLinks >= 1 && ( SelectedRealLinks + SelectedConnectors > 1 ) && Actuators == 0 && SelectedGears == 0;
		m_bSelectionJoinable = ( SelectedConnectors > 1 && ( Sliders <= 1 || bSlidersShareLimits ) ) || ( SelectedConnectors == 1 && SelectedRealLinks >= 1 );
		m_bSelectionSlideable = ConnectSliderLimits( true );
		m_bSelectionLockable = SelectedAnchors > 0 || SelectedRealLinks > 0;
		m_bSelectionMakeAnchor = SelectedConnectors > 0 && SelectedRealLinks == 0;

		m_bSelectionConnectable = SelectedRealLinks == 0 && SelectedConnectors >= 2;
		// ADD A TEST FOR WEIRD THINGS LIKE...
		// If one of the connectors is a slider and one of the others is one if it's limits, that's no good.
		// if there is a link that uses esxactly all of these connectors then that is no good.
		// BUT FOR NOW, JUST HOPE FOR THE BEST.
	}

	m_bSelectionSplittable = SelectedRealLinks == 0 && SelectedConnectors == 1;
	if( m_bSelectionSplittable )
	{
		if (pConnector1->GetLinkCount() <= 1 && !pConnector1->IsSlider() )
			m_bSelectionSplittable = false;
	}

	m_bSelectionTriangle = SelectedRealLinks == 0 && SelectedConnectors == 3;
	m_bSelectionRectangle = SelectedRealLinks == 0 && SelectedConnectors == 4;
	m_bSelectionLineable = SelectedRealLinks == 0 && SelectedConnectors >= 3;
	m_AlignConnectorCount = SelectedRealLinks > 0 ? 0 : SelectedConnectors;
	m_bSelectionUnfastenable = FastenedCount > 0;

	m_bSelectionPositionable = SelectedConnectors == 1 && SelectedRealLinks == 0;
	m_bSelectionLengthable = ( SelectedConnectors == 2 && SelectedRealLinks == 0 ) || ( SelectedConnectors == 0 && SelectedRealLinks == 1 && SelectedLinkConnectorCount == 2 );
	m_bSelectionAngleable = m_bSelectionTriangle || m_bSelectionLengthable;
	m_bSelectionRotatable = SelectedRealLinks > 0 || SelectedConnectors> 1;
	m_bSelectionScalable = SelectedRealLinks > 0 || SelectedConnectors> 1;

	m_bSelectionMeetable = false;
	if( SelectedRealLinks == 0 && SelectedConnectors == 4 && GetSelectedConnectorCount() == 4 )
	{
		CFPoint Points[4];
		Points[0] = GetSelectedConnector( 0 )->GetOriginalPoint();
		Points[1] = GetSelectedConnector( 1 )->GetOriginalPoint();
		Points[2] = GetSelectedConnector( 2 )->GetOriginalPoint();
		Points[3] = GetSelectedConnector( 3 )->GetOriginalPoint();

		CFCircle Circle1( Points[0], Distance( Points[0], Points[1] ) );
		CFCircle Circle2( Points[3], Distance( Points[3], Points[2] ) );

		CFPoint Intersect;
		CFPoint Intersect2;

		if( Circle1.CircleIntersection( Circle2, &Intersect, &Intersect2 ) )
			m_bSelectionMeetable = true;
	}

	m_bSelectionFastenable = false;
	if( SelectedDrawingElements > 0 && ( SelectedMechanismLinks == 1 || SelectedMechanismConnectors == 1 ) )
		m_bSelectionFastenable = true;
	else if( SelectedDrawingElements > 0 && SelectedMechanismLinks == 1 )
		m_bSelectionFastenable = true;
	else if( SelectedGears == 1 && SelectedNonGears == 1 )
	{
		if( SelectedConnectors > 0 ) 
		{
			// Gear to connector fasten
			if( pFastenToConnector != 0 )
				m_bSelectionFastenable = true;
		}
		else
		{
			// Gear to non-gear link fasten
			if( pSelectedGear->GetConnector( 0 )->HasLink( pSelectedLink ) )
				m_bSelectionFastenable = true;
		}
	}
	else if( false ) //SelectedGears == 1 && ( SelectedMechanismLinks - SelectedGears ) == 1 && GearFastenToCount == 1 )
	{
		m_bSelectionFastenable = true;
		Position = m_Links.GetHeadPosition();
		while( Position != 0 && ( pFastenToLink != 0 || pFastenToConnector != 0 ) )
		{
			CLink *pLink = m_Links.GetNext( Position );
			if( pLink == 0 )
				continue;
			if( pLink->IsSelected() && pLink->IsGear() )
			{
				if( pFastenToLink != 0 )
				{
					CConnector *pCommon = CLink::GetCommonConnector( pFastenToLink, pLink );
					if( pCommon == 0 )
					{
						m_bSelectionFastenable = false;
						break;
					}
				}
				if( pFastenToConnector != 0 )
				{
					if( !pLink->GetGearConnector()->IsAnchor() )
					{
						m_bSelectionFastenable = false;
						break;
					}
				}
			}
		}
	}
	else if( false ) // SelectedGears == 1 && SelectedMechanismLinks == 1 && SelectedConnectors == 0 )
	         //&& SelectedConnectors == 1 && GetSelectedLink( 0, false ) != 0 
			 //&& pFastenToConnector == GetSelectedLink( 0, false )->GetConnector( 0 ) )
	{
		CConnector *pConnector = GetSelectedLink( 0, false )->GetConnector( 0 );
		if( pConnector->IsAnchor() && !pConnector->IsInput() )
			m_bSelectionFastenable = true;
	}

	m_bSelectionMeshable = CheckMeshableGears();
}

CConnector* CLinkageDoc::GetSelectedConnector( int Index )
{
	POSITION Position = m_SelectedConnectors.GetTailPosition();
	while( Position != 0 )
	{
		CConnector *pConnector = m_SelectedConnectors.GetPrev( Position );
		if( ( pConnector->GetLayers() & m_UsableLayers ) == 0 )
			continue;
		if( Index == 0 )
			return pConnector;
		--Index;
	}
	return 0;
}

void CLinkageDoc::GetNextAutoSelectable( _SelectDirection SelectDirection, POSITION &Position, bool &bIsConnector )
{
	if( SelectDirection == PREVIOUS )
	{
		if( bIsConnector )
		{
			m_Connectors.GetPrev( Position );
			if( Position == 0 )
			{
				Position = m_Links.GetTailPosition();
				bIsConnector = false;
				if( Position == 0 )
				{
					Position = m_Connectors.GetTailPosition();
					bIsConnector = true;
				}
			}
		}
		else
		{
			m_Links.GetPrev( Position );
			if( Position == 0 )
			{
				Position = m_Connectors.GetTailPosition();
				bIsConnector = true;
				if( Position == 0 )
				{
					Position = m_Links.GetTailPosition();
					bIsConnector = false;
				}
			}
		}
	}
	else
	{
		if( bIsConnector )
		{
			m_Connectors.GetNext( Position );
			if( Position == 0 )
			{
				Position = m_Links.GetHeadPosition();
				bIsConnector = false;
				if( Position == 0 )
				{
					Position = m_Connectors.GetHeadPosition();
					bIsConnector = true;
				}
			}
		}
		else
		{
			m_Links.GetNext( Position );
			if( Position == 0 )
			{
				Position = m_Connectors.GetHeadPosition();
				bIsConnector = true;
				if( Position == 0 )
				{
					Position = m_Links.GetHeadPosition();
					bIsConnector = false;
				}
			}
		}
	}
}

bool CLinkageDoc::SelectNext( _SelectDirection SelectDirection )
{
	POSITION Position = 0;
	POSITION LastSelection = 0;
	bool bLastIsConnector = 0;

	/*
	 * Find the last selected item, be it a connector or a link.
	 * This will be the item that is used for picking the next or
	 * previous selected item.
	 */

	Position = m_Connectors.GetHeadPosition();
	while( Position != 0 )
	{
		CConnector *pConnector = m_Connectors.GetAt( Position );
		if( pConnector != 0 && pConnector->IsSelected() )
		{
			LastSelection = Position;
			bLastIsConnector = true;
		}
		m_Connectors.GetNext( Position );
	}

	Position = m_Links.GetHeadPosition();
	while( Position != 0 )
	{
		CLink *pLink = m_Links.GetAt( Position );
		if( pLink != 0 && pLink->IsSelected( false ) )
		{
			LastSelection = Position;
			bLastIsConnector = false;
		}
		m_Links.GetNext( Position );
	}

	Position = LastSelection;

	if( Position == 0 )
	{
		Position = m_Connectors.GetHeadPosition();
		if( Position == 0 )
			return false;
		bLastIsConnector = true;
	}
	else
		GetNextAutoSelectable( SelectDirection, Position, bLastIsConnector );

	/*
	 * Skip links that have a single connector. This will get selected
	 * when the connector is selected.
	 */

	/*
		* Unless there are links and zero connectors, this loop is guaranteed
		* to terminate as soon as a connector is picked. It may also
		* terminate if a link with more than one connector is picked.
		*/
	while( Position != 0 && !bLastIsConnector )
	{
		CLink *pLink = m_Links.GetAt( Position );
		if( pLink == 0 )
			break;
		if( pLink->GetConnectorCount() > 1 || pLink->IsGear() )
			break;

		// Find another element since this link has a single connector.
		GetNextAutoSelectable( SelectDirection, Position, bLastIsConnector );
	}

	ClearSelection();

	if( bLastIsConnector )
	{
		CConnector *pConnector = m_Connectors.GetAt( Position );
		if( pConnector == 0 )
			return false;
		SelectElement( pConnector );
	}
	else
	{
		CLink *pLink = m_Links.GetAt( Position );
		if( pLink == 0 )
			return false;
		SelectElement( pLink );
	}

	SetSelectedModifiableCondition();
	return true;
}

#if 0

void CLinkageDoc::InsertXml( CFPoint DesiredPoint, bool bForceToPoint, const char *pXml )
{
    CMemFile mFile;
	unsigned int Size = strlen( pXml );
    mFile.Attach( (BYTE*)pXml, Size, 0 );
    CArchive ar( &mFile, CArchive::load );

    PushUndo();

    ReadIn( ar, true, false, false, false );
    mFile.Detach();

    SetModifiedFlag( true );
}

#endif

void CLinkageDoc::SetUnits( CLinkageDoc::_Units Units )
{
	double OneInch = GetBaseUnitsPerInch();

	m_Units = Units;
	switch( Units )
	{
		case INCH: m_UnitScaling = 1 / OneInch; break;
		case MM:
		default: m_UnitScaling = 25.4 / OneInch; break;
	}
}

CLinkageDoc::_Units CLinkageDoc::GetUnits( void )
{
	return m_Units;
}

bool CLinkageDoc::ChangeLinkLength( CLink *pLink, double Value, bool bPercentage )
{
	CConnector *pConnector = pLink->GetConnector( 0 );
	CConnector *pConnector2 = pLink->GetConnector( 1 );
	if( pConnector == 0 || pConnector2 == 0 )
		return false;
	if( IsLinkLocked( pConnector ) && IsLinkLocked( pConnector2 ) )
		return false;
	if( pConnector->IsLocked() && pConnector2->IsLocked() )
		return false;
	CFLine Line( pConnector->GetPoint(), pConnector2->GetPoint() );
	double LineLength = Line.GetLength();
	double NewLineLength = LineLength;
	if( bPercentage )
		NewLineLength *= Value / 100;
	else
		NewLineLength = Value;
	double OneEndAdd = ( NewLineLength - LineLength ) / 2;
	PushUndo();
	if( IsLinkLocked( pConnector ) || pConnector->IsLocked() )
	{
		CFLine Line( pConnector->GetPoint(), pConnector2->GetPoint() );
		Line.SetLength( NewLineLength );
		PushUndo();
		pConnector2->SetPoint( Line.GetEnd() );
	}
	else if( IsLinkLocked( pConnector2 ) || pConnector2->IsLocked() )
	{
		CFLine Line( pConnector2->GetPoint(), pConnector->GetPoint() );
		Line.SetLength( NewLineLength );
		PushUndo();
		pConnector->SetPoint( Line.GetEnd() );
	}
	else
	{
		Line.SetLength( LineLength + OneEndAdd );
		Line.ReverseDirection();
		Line.SetLength( NewLineLength );
		pConnector->SetPoint( Line.GetEnd() );
		pConnector2->SetPoint( Line.GetStart() );
	}
	return true;
}

CString CLinkageDoc::GetSelectedElementCoordinates( CString *pHintText )
{
	CString Text;
	if( pHintText != 0 )
		*pHintText = "";

	if( IsSelectionMeshableGears() )
	{
		CLink *pLink1 = GetSelectedLink( 0, false );
		CLink *pLink2 = GetSelectedLink( 1, false );
		double Size1 = 0;
		double Size2 = 0;

		CGearConnection *pConnection = GetGearRatio( pLink1, pLink2, &Size1, &Size2 );

		if( pConnection == 0 || Size1 == 0 || Size2 == 0 )
			return "";
		else
		{
			CString Temp;
			if( floor( Size1 ) == Size1 )
				Temp.Format( "%d", (int)Size1 );
			else
			{
				Temp.Format( "%.3lf", Size1 );
				Temp.TrimRight( "0" );
			}
			Text = Temp;
			Text += ":";
			if( floor( Size2 ) == Size2 )
				Temp.Format( "%d", (int)Size2 );
			else
			{
				Temp.Format( "%.3lf", Size2 );
				Temp.TrimRight( "0" );
			}
			Text += Temp;
		}
		if( pHintText != 0 )
			*pHintText = "Ratio";
		return Text;
	}

	double DocumentScale = GetUnitScale();

	CConnector *pConnector0 = 0;
	CConnector *pConnector1 = 0;
	CConnector *pConnector2 = 0;

	CLink *pSelectedLink = 0;

	bool bEnableEdit = true;

	int SelectedConnectors = GetSelectedConnectorCount();
	int SelectedLinks = GetSelectedLinkCount( false );
	bool bLinkSelected = false;

	pConnector0 = GetSelectedConnector( 0 );
	if( pConnector0 == 0 )
	{
		if( GetSelectedLinkCount( false ) == 1 )
		{
			CLink *pSelectedLink = GetSelectedLink( 0, false );
			if( pSelectedLink == 0 )
				return "";

			if( pSelectedLink->GetConnectorCount() == 2 )
			{
				pConnector0 = pSelectedLink->GetConnector( 0 );
				pConnector1 = pSelectedLink->GetConnector( 1 );

				SelectedLinks = 0; // Just using the connectors from the link and not the link.
				bLinkSelected = true; // For test hint info.

				SelectedConnectors = 2;
			}
		}
	}
	else
	{
		pConnector1 = GetSelectedConnector( 1 );
		pConnector2 = GetSelectedConnector( 2 );
	}

	if( SelectedConnectors > 0 && SelectedLinks == 0 )
	{
		CFPoint Point0 = pConnector0->GetOriginalPoint();
		Point0.x *= DocumentScale;
		Point0.y *= DocumentScale;

		switch( SelectedConnectors )
		{
			case 0:
				if( pHintText != 0 )
					*pHintText = "";
				return "";

			case 1:
			{
				if( GetSelectedLinkCount( false ) > 0 )
					break;

				Text.Format( "%.3lf,%.3lf", Point0.x, Point0.y );
				if( pHintText != 0 )
					*pHintText = "X,Y Coordinates";
				return Text;
			}

			case 2:
			{
				if( pConnector1 == 0 )
					return "";

				if( pConnector0->IsAlone() )
					--SelectedLinks;
				if( pConnector0->IsAlone() )
					--SelectedLinks;
				if( SelectedLinks > 0 )
					return "";
				CFPoint Point1 = pConnector1->GetOriginalPoint();
				Point1.x *= DocumentScale;
				Point1.y *= DocumentScale;

				double distance = Distance( Point0, Point1 );
				Text.Format( "%.3lf", distance );
				if( pHintText != 0 )
				{
					if( bLinkSelected )
						*pHintText = " Length";
					else
						*pHintText = " Distance";
				}
				return Text;
			}

			case 3:
			{
				if( pConnector1 == 0 || pConnector2 == 0 )
					return "";
				double Angle = GetAngle( pConnector1->GetOriginalPoint(), pConnector0->GetOriginalPoint(), pConnector2->GetOriginalPoint() );
				if( Angle < 0 )
					Angle += 360.0; // Because the hint mark for the angle is never shown on the "negative" side of the angle, don't show the number as negative.
				Text.Format( "%.3lf", Angle );
				if( pHintText != 0 )
					*pHintText = "�Angle";

				return Text;
			}
		}
	}

	if( SelectedConnectors + SelectedLinks > 0 )
	{
		// More than 3, show "scale" info
		Text = "100%";
		if( pHintText != 0 )
			*pHintText = "Scale";
	}
			
	return Text;
}

CLinkageDoc::_CoordinateChange CLinkageDoc::SetSelectedElementCoordinates( CFPoint *pRotationCenterPoint, const char *pCoordinateString )
{
	CString Text = pCoordinateString;

	Text.Replace( " ", "" );

	double xValue = 0.0;
	double yValue = 0.0;
	char Dummy1;
	char Modifier;
	int CoordinateCount = 0;
	double Value = 0.0;

	int SelectedConnectorCount = GetSelectedConnectorCount();
	int SelectedLinkCount = GetSelectedLinkCount( true );
	CConnector *pConnector = (CConnector*)GetSelectedConnector( 0 );

	CoordinateCount = sscanf_s( (const char*)Text, "%lf %c%c", &Value, &Modifier, 1, &Dummy1, 1 );

	if( CoordinateCount == 2 && Modifier == '%' )
	{
		if( SelectedConnectorCount == 2 && SelectedLinkCount == 0 )
		{
			// Distance
			CConnector *pConnector2 = (CConnector*)GetSelectedConnector( 1 );
			if( pConnector == 0 || pConnector2 == 0 )
				return _CoordinateChange::NONE;
			if( IsLinkLocked( pConnector ) )
				return _CoordinateChange::NONE;
			CFLine Line( pConnector->GetPoint(), pConnector2->GetPoint() );
			Line.SetLength( Line.GetLength() * Value / 100 );
			PushUndo();
			pConnector2->SetPoint( Line.GetEnd() );
		}
		else if( SelectedConnectorCount == 0 && SelectedLinkCount == 1 )
		{
			CLink *pLink = GetSelectedLink( 0, false );
			if( pLink != 0 && pLink->GetConnectorCount() == 2 && !pLink->IsLocked() )
			{
				if( ChangeLinkLength( pLink, Value, true ) )
					return _CoordinateChange::DISTANCE;
			}
		}
		else
		{
			// Scale
			StretchSelected( Value );
			return _CoordinateChange::SCALE;
		}
		return _CoordinateChange::OTHER;
	}
	else if( CoordinateCount == 2 && ( Modifier == 'd' || Modifier == 'D' || Modifier == '*' ) )
	{
		CFRect BoundingRect;
		GetDocumentArea( BoundingRect, true );
		PushUndo();
		RotateSelected( pRotationCenterPoint == 0 ? BoundingRect.GetCenter() : *pRotationCenterPoint, Value );
		FinishChangeSelected();
		SetModifiedFlag( true );
		return _CoordinateChange::ROTATION;
	}

	CoordinateCount = sscanf_s( (const char*)Text, "%lf , %lf%c", &xValue, &yValue, &Dummy1, 1 );

	if( CoordinateCount < 1 || CoordinateCount > 2 )
		return _CoordinateChange::NONE;

	double Angle = xValue;

	double DocumentScale = GetUnitScale();
	xValue /= DocumentScale;
	yValue /= DocumentScale;

	if( SelectedLinkCount > 0 )
	{
		if( SelectedLinkCount == 1 && CoordinateCount == 1 )
		{
			CLink *pLink = GetSelectedLink( 0, false );
			if( pLink == 0 || pLink->GetConnectorCount() != 2 || pLink->IsLocked() || GetSelectedConnectorCount() > 0 )
				return _CoordinateChange::NONE;

			if( ChangeLinkLength( pLink, xValue, false ) )
				return _CoordinateChange::DISTANCE;
			else
				return _CoordinateChange::NONE;
		}
		return _CoordinateChange::NONE;
	}

	int ExpectedCoordinateCount = SelectedConnectorCount == 1 ? 2 : 1;

	if( CoordinateCount != ExpectedCoordinateCount )
		return _CoordinateChange::NONE;

	if( CoordinateCount == 2 )
	{
		if( IsLinkLocked( pConnector ) || pConnector->IsLocked() )
			return _CoordinateChange::NONE;
		PushUndo();
		pConnector->SetPoint( CFPoint( xValue, yValue ) );
		return _CoordinateChange::OTHER;
	}
	else if( SelectedConnectorCount == 2 )
	{
		// Distance
		CConnector *pConnector2 = (CConnector*)GetSelectedConnector( 1 );
		if( pConnector2 == 0 )
			return _CoordinateChange::NONE;
		if( IsLinkLocked( pConnector2 ) )
			return _CoordinateChange::NONE;
		CFLine Line( pConnector->GetPoint(), pConnector2->GetPoint() );
		Line.SetLength( xValue );
		PushUndo();
		pConnector2->SetPoint( Line.GetEnd() );
		return _CoordinateChange::DISTANCE;
	}
	else if( SelectedConnectorCount == 3 )
	{
		// Angle
		CConnector *pConnector2 = GetSelectedConnector( 2 );
		if( pConnector2 != 0 && pConnector2->IsSlider() )
			return _CoordinateChange::NONE;
		if( IsLinkLocked( pConnector2 ) )
			return _CoordinateChange::NONE;

		MakeSelectedAtAngle( Angle );
		return _CoordinateChange::OTHER;
	}

	return _CoordinateChange::NONE;
}

bool CLinkageDoc::CanSimulate( CString &ErrorIfFalse )
{
	int Inputs = 0;
	int Actuators = 0;
	int GroundedActuators = 0;
	int Anchors = 0;
	POSITION Position = m_Links.GetHeadPosition();
	while( Position != 0 )
	{
		CLink *pLink = m_Links.GetNext( Position );
		if( pLink == 0 )
			continue;
		if( pLink->IsActuator() )
		{
			++Inputs;
			++Actuators;
			GroundedActuators += pLink->GetAnchorCount();
		}
	}
	Position = m_Connectors.GetHeadPosition();
	while( Position != 0 )
	{
		CConnector *pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 )
			continue;
		if( pConnector->IsInput() )
			++Inputs;
		if( pConnector->IsAnchor() )
			++Anchors; 
	}
	if( Inputs == 0 )
	{
		ErrorIfFalse = "The mechanism has no inputs. A rotating input connector or a linear actuator is required to run the mechanism.";
		return false;
	}
	if( Anchors == 0 )
	{
		ErrorIfFalse = "The mechanism has anchors. At least one anchor is required to connect the mechanism to the \"ground\"";
		return false;
	}
	//if( Actuators >= Inputs && GroundedActuators == 0 )
	//{
	//	ErrorIfFalse = "None of the linear actuators are anchored. The mechanism cannot be simulated.";
	//	return false;
	//}
	return true;

}


CLink *CLinkageDoc::GetSelectedLink( int Index, bool bOnlyWithMultipleConnectors )
{
	int Counter = 0;
	POSITION Position = m_SelectedLinks.GetTailPosition();
	while( Position != 0 )
	{
		CLink *pLink = m_SelectedLinks.GetPrev( Position );
		if( pLink == 0 || ( !pLink->IsSelected( false ) && !pLink->IsSelected( true ) ) )
			continue;
		if( bOnlyWithMultipleConnectors && pLink->GetConnectorCount() == 1 )
			continue;
		if( Counter == Index )
			return pLink;
		++Counter;
	}
	return 0;
}

bool CLinkageDoc::AddConnectorToSelected( double Radius )
{
	if( !CanAddConnector() )
		return false;

	CLink* pLink = GetSelectedLink( 0, false );
	if( pLink == 0 )
	{
		CConnector *pConnector = GetSelectedConnector( 0 );
		if( pConnector == 0 )
			return false;

		if( pConnector->IsAlone() )
		{
			pLink = pConnector->GetLink( 0 );
			if( pLink == 0 )
				return false;
		}
	}

	CFPoint Point;
	//pLink->GetAveragePoint( m_GearConnectionList, Point );
	CFRect Rect;
	pLink->GetAdjustArea( m_GearConnectionList, Rect );
	Point.x = ( Rect.left + Rect.right ) / 2;
	Point.y = ( Rect.top + Rect.bottom ) / 2;

	/*
	 * Find a place for the new connector that does not overlap any existing connectors.
	 */
	double Diameter = Radius * 2;
	vector<double> ConflictConnectors;
	POSITION Position = pLink->GetConnectorList()->GetHeadPosition();
	while( Position != 0 )
	{
		CConnector *pConnector = pLink->GetConnectorList()->GetNext( Position );
		if( pConnector == 0 )
			continue;
		CFPoint TestPoint = pConnector->GetPoint();
		if( TestPoint.y < Point.y - Diameter || TestPoint.y > Point.y + Diameter )
			continue;
		if( TestPoint.x < Point.x - Diameter )
			continue;
		ConflictConnectors.push_back( TestPoint.x );
	}

	sort( ConflictConnectors.begin(), ConflictConnectors.end() );
	for( vector<double>::iterator it = ConflictConnectors.begin(); it != ConflictConnectors.end(); ++it )
	{
		if( Point.x > *it - Radius && Point.x < *it + Radius )
			Point.x = *it + Diameter;
	}

	CConnector *pConnector = new CConnector;
	if( pConnector == 0 )
		return false;

	pConnector->SetLayers( pLink->GetLayers() );
	pConnector->SetAsAnchor( false );
	pConnector->SetAsInput( false );
	pConnector->SetRPM( DEFAULT_RPM );
	pConnector->SetPoint( Point );
	pConnector->AddLink( pLink );
	pLink->AddConnector( pConnector );

	int NewID = m_IdentifiedConnectors.FindAndSetBit();
	if( NewID > m_HighestConnectorID )
		m_HighestConnectorID = NewID;
	pConnector->SetIdentifier( NewID );
	m_Connectors.AddTail( pConnector );

	return true;
}

bool CLinkageDoc::CanAddConnector( void )
{
	if (GetSelectedLinkCount(false) == 0)
	{
		if( GetSelectedConnector(0) == 0 )
			return false;
		return GetSelectedConnectorCount() == 1 && GetSelectedConnector(0)->IsAlone() /*&& (GetSelectedConnector(0)->GetLayers() & MECHANISMLAYERS) != 0*/;
	}
	else if( GetSelectedLinkCount( false ) == 1 )
	{
		CLink *pLink = GetSelectedLink( 0, false );
		if( pLink == 0 || pLink->IsActuator() || pLink->IsGear() )
			return false;
		return /* ( GetSelectedLink( 0, false )->GetLayers() & MECHANISMLAYERS ) != 0 && */ GetSelectedConnectorCount() == 0 || ( GetSelectedConnectorCount() == 1 && GetSelectedConnector( 0 )->IsAlone() );
	}
	else
		return false;
}

bool CLinkageDoc::UnfastenSelected( void )
{
	PushUndo();

	POSITION Position = m_Links.GetHeadPosition();
	while( Position != 0 )
	{
		CLink *pLink = m_Links.GetNext( Position );
		if( pLink == 0 || !pLink->IsSelected() )
			continue;

		Unfasten( pLink );
	}

	Position = m_Connectors.GetHeadPosition();
	while( Position != NULL )
	{
		CConnector* pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 || ( pConnector->GetLayers() & MECHANISMLAYERS ) != 0 || !pConnector->IsSelected() )
			continue;

		Unfasten( pConnector );

		//CLink *pFastenedTo = pConnector->GetFastenedTo() == 0 ? 0 : pConnector->GetFastenedTo()->GetLink();
		//if( pFastenedTo != 0 )
		//	pFastenedTo->RemoveFastenElement( pConnector );

		//pConnector->UnfastenTo();
	}

	SetSelectedModifiableCondition();

	return true;
}

bool CLinkageDoc::Unfasten( CElement *pElement )
{
	//POSITION Position = pElement->GetFastenedElementList()->GetHeadPosition();
	//while( Position != 0 )
	//{
	//	CElementItem *pItem = pElement->GetFastenedElementList()->GetNext( Position );
	//	if( pItem == 0 || pItem->GetElement() == 0 )
	//		continue;
	//	pItem->GetElement()->RemoveFastenElement( pElement );
	//	if( pItem->GetElement()->GetFastenedTo() != 0 && pElement == pItem->GetElement()->GetFastenedTo()->GetElement() )
	//		pItem->GetElement()->UnfastenTo();
	//}

	pElement->RemoveFastenElement( 0 ); // All of them.
	pElement->UnfastenTo();

	return true;
}

bool CLinkageDoc::FastenSelected( void )
{
	if( !m_bSelectionFastenable )
		return false;

	bool bFastenDrawingElements = false;
	int DrawingFastenToCount = 0;
	CLink *pFastenToLink = 0;
	CConnector *pFastenToConnector = 0;
	CLink *pGearFastenToLink = 0;
	int GearFastenToCount = 0;

	POSITION Position = m_Links.GetHeadPosition();
	while( Position != 0 )
	{
		CLink *pLink = m_Links.GetNext( Position );
		if( pLink == 0 )
			continue;

		if( pLink->IsSelected( false ) )
		{
			if( pLink->GetConnectorCount() <= 1 && !pLink->IsGear() )
				continue;

			if( ( pLink->GetLayers() & DRAWINGLAYER ) != 0 )
				bFastenDrawingElements = true;
			else
			{
				++DrawingFastenToCount;
				pFastenToLink = pLink;

				if( !pLink->IsGear() && pLink->GetConnectorCount() > 1 )
				{
					pGearFastenToLink = pLink;
					++GearFastenToCount;
				}
			}
		}
	}

	Position = m_Connectors.GetHeadPosition();
	while( Position != NULL )
	{
		CConnector* pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 || !pConnector->IsSelected() )
			continue;

		if( ( pConnector->GetLayers() & DRAWINGLAYER ) != 0 )
			bFastenDrawingElements = true;
		else
		{
			++DrawingFastenToCount;
			//DrawingFastenToCount += 2; // A single connector is too many to fasten to so just increment this by 2.

			pFastenToConnector = pConnector;
			++GearFastenToCount;
		}
	}

	if( bFastenDrawingElements )
	{
		if( DrawingFastenToCount > 1 )
			return false;

		PushUndo();

		Position = m_Links.GetHeadPosition();
		while( Position != 0 )
		{
			CLink *pLink = m_Links.GetNext( Position );
			if( pLink == 0 || !pLink->IsSelected() || ( pLink->GetLayers() & DRAWINGLAYER ) == 0 )
				continue;

			if( pFastenToConnector == 0 )
				FastenThese( pLink, pFastenToLink );
			else
				FastenThese( pLink, pFastenToConnector );
		}

		Position = m_Connectors.GetHeadPosition();
		while( Position != NULL )
		{
			CConnector* pConnector = m_Connectors.GetNext( Position );
			if( pConnector == 0 || !pConnector->IsSelected() || ( pConnector->GetLayers() &DRAWINGLAYER ) == 0 )
				continue;

			if( pFastenToConnector == 0 )
				FastenThese( pConnector, pFastenToLink );
			else
				FastenThese( pConnector, pFastenToConnector );
		}
	}
	else
	{
		if( GearFastenToCount > 1 )
			return false;

		if( DrawingFastenToCount == 1 && pFastenToLink != 0 && pFastenToLink->IsGear() && pFastenToLink->GetConnector( 0 )->IsAnchor() && !pFastenToLink->GetConnector( 0 )->IsInput() )
			pFastenToConnector = pFastenToLink->GetConnector( 0 );

		PushUndo();

		Position = m_Links.GetHeadPosition();
		while( Position != 0 )
		{
			CLink *pLink = m_Links.GetNext( Position );
			if( pLink == 0 || !pLink->IsSelected() || ( pLink->GetLayers() & DRAWINGLAYER ) != 0 || !pLink->IsGear() )
				continue;

			if( pFastenToConnector != 0 )
				FastenThese( pLink, pFastenToConnector );
			else if( pGearFastenToLink != 0 )
				FastenThese( pLink, pGearFastenToLink );
		}
	}

	SetSelectedModifiableCondition();

	return true;
}
bool CLinkageDoc::ConnectGears( CLink *pLink1, CLink *pLink2, double Size1, double Size2 )
{
	if( pLink1 == 0 || !pLink1->IsGear() || pLink2 == 0 || !pLink2->IsGear() || pLink1 == pLink2 )
		return false;

	POSITION Position = m_GearConnectionList.GetHeadPosition();
	while( Position != 0 )
	{
		POSITION CurrentPosition = Position;
		CGearConnection *pConnection = m_GearConnectionList.GetNext( Position );
		if( pConnection == 0 )
			continue;

		if( ( pConnection->m_pGear1 == pLink1 && pConnection->m_pGear2 == pLink2 )
		    || ( pConnection->m_pGear2 == pLink1 && pConnection->m_pGear1 == pLink2 ) )
		{
			delete pConnection;
			m_GearConnectionList.RemoveAt( CurrentPosition );
			break;
		}
	}

	CGearConnection *pConnection = new CGearConnection;
	if( pConnection == 0 )
		return false;

	pConnection->m_pGear1 = pLink1;
	pConnection->m_pGear2 = pLink2;
	pConnection->m_Gear1Size = Size1;
	pConnection->m_Gear2Size = Size2;

	return true;
}

bool CLinkageDoc::DisconnectGear( CLink *pLink )
{
	if( pLink == 0 || !pLink->IsGear() )
		return false;

	POSITION Position = m_GearConnectionList.GetHeadPosition();
	while( Position != 0 )
	{
		POSITION CurrentPosition = Position;
		CGearConnection *pConnection = m_GearConnectionList.GetNext( Position );
		if( pConnection == 0 )
			continue;

		if( pConnection->m_pGear1 == pLink || pConnection->m_pGear2 == pLink )
		{
			delete pConnection;
			m_GearConnectionList.RemoveAt( CurrentPosition );
			break;
		}
	}

	return true;
}

bool CLinkageDoc::CheckMeshableGears( void )
{
	if( GetSelectedLinkCount( false ) != 2 )
		return false;

	CLink *pGear1 = GetSelectedLink( 0, false );
	CLink *pGear2 = GetSelectedLink( 1, false );

	return CheckMeshableGears( pGear1, pGear2 );
}

bool CLinkageDoc::CheckMeshableGears( CLink *pGear1, CLink *pGear2 )
{
	if( pGear1 == 0 || pGear2 == 0 )
		return false;

	if( !pGear1->IsGear() || !pGear2->IsGear() )
		return false;

	CConnector *pConnector1 = pGear1->GetConnector( 0 );
	CConnector *pConnector2 = pGear2->GetConnector( 0 );

	if( pConnector1->IsSharingLink( pConnector2 ) )
		return true;

	if( pConnector1->IsAnchor() && pConnector2->IsAnchor() )
		return true;

	return false;

#if 0

	POSITION Position = m_Connectors.GetHeadPosition();
	while( Position != 0 )
	{
		CConnector *pConnector = m_Connectors.GetNext( Position );
		if( pConnector != 0 && pConnector->IsSelected() )
		{
			bool bIsGear = false;
			CLink *pLink = 0;
			POSITION Position2 = pConnector->GetLinkList()->GetHeadPosition();
			while( Position2 != 0 )
			{
				pLink = pConnector->GetLinkList()->GetNext( Position2 );
				if( pLink == 0 )
					continue;

				if( pConnector == pLink->GetGearConnector() )
				{
					bIsGear = true;
					break;
				}
			}
			if( !bIsGear )
				return false;

			if( pConnector1 == 0 )
				pConnector1 = pConnector;
			else
				pConnector2 = pConnector;
		}
	}

	if( pConnector1 != 0 && pConnector2 != 0 )
		return true;

	/*
	 * There is one special case: If one of the gear connectors is an anchor and the other gear
	 * connector connects to an anchor at that same location, it is safe to mesh the gears.
	 */

	 CConnector *pAnchorConnector = 0;
	 CConnector *pOtherConnector = 0;
	 if( pConnector1->IsAnchor() )
	 {
		pAnchorConnector = pConnector1;
		pOtherConnector = pConnector2;
	 }
	 else if( pConnector2->IsAnchor() )
	 {
		pAnchorConnector = pConnector2;
		pOtherConnector = pConnector1;
	 }
	 else
		return false;

	Position = pOtherConnector->GetLinkList()->GetHeadPosition();
	while( Position != 0 )
	{
		CLink *pLink = pOtherConnector->GetLinkList()->GetNext( Position );
		if( pLink == 0 )
			continue;

		POSITION Position2 = pLink->GetConnectorList()->GetHeadPosition();
		while( Position2 != 0 )
		{
			CConnector *pCheckConnector = pLink->GetConnectorList()->GetNext( Position2 );
			if( pCheckConnector == 0 )
				continue;

			if( pCheckConnector->IsAnchor() && pAnchorConnector->GetPoint() == pCheckConnector->GetPoint() )
				return true;
		}
	}

	return false;
	#endif
}

CGearConnection * CLinkageDoc::GetGearRatio( CLink *pGear1, CLink *pGear2, double *pSize1, double *pSize2 )
{
	if( pGear1 == 0 || pGear2 == 0 || !pGear1->IsGear() || !pGear2->IsGear() )
		return 0;

	POSITION Position = m_GearConnectionList.GetHeadPosition();
	while( Position != 0 )
	{
		CGearConnection *pGearConnection = m_GearConnectionList.GetNext( Position );
		if( pGearConnection == 0 || pGearConnection->m_pGear1 == 0 || pGearConnection->m_pGear2 == 0 )
			continue;

		// Scale the size values if using a chain and gears whose size is set.
		double UseDocumentScale = pGearConnection->m_bUseSizeAsRadius ? GetUnitScale() : 1;

		if( ( pGearConnection->m_pGear1 == pGear1 && pGearConnection->m_pGear2 == pGear2 )
		    || ( pGearConnection->m_pGear1 == pGear2 && pGearConnection->m_pGear2 == pGear1 ) )
		{
			if( pGearConnection->m_pGear1 == pGear1 )
			{
				if( pSize1 != 0 )
					*pSize1 = pGearConnection->m_Gear1Size * UseDocumentScale;
				if( pSize2 != 0 )
					*pSize2 = pGearConnection->m_Gear2Size * UseDocumentScale;
			}
			else
			{
				if( pSize1 != 0 )
					*pSize1 = pGearConnection->m_Gear2Size * UseDocumentScale;
				if( pSize2 != 0 )
					*pSize2 = pGearConnection->m_Gear1Size * UseDocumentScale;
			}
			return pGearConnection;
		}
	}

	return 0;
}

bool CLinkageDoc::SetGearRatio( CLink *pGear1, CLink *pGear2, double Size1, double Size2, bool bUseSizeAsSize, bool bSizeIsDiameter, CGearConnection::ConnectionType ConnectionType )
{
	return SetGearRatio( pGear1, pGear2, Size1, Size2, bUseSizeAsSize, bSizeIsDiameter, ConnectionType, true );
}

bool CLinkageDoc::SetGearRatio( CLink *pGear1, CLink *pGear2, double Size1, double Size2, bool bUseSizeAsSize, bool bSizeIsDiameter, CGearConnection::ConnectionType ConnectionType, bool bFromUI )
{
	if( pGear1 == 0 || pGear2 == 0 || !pGear1->IsGear() || !pGear2->IsGear() )
		return false;

	if( bSizeIsDiameter )
	{
		Size1 /= 2.0;
		Size2 /= 2.0;
	}

	CGearConnection * pGearConnection = GetGearRatio( pGear1, pGear2 );

	bool bNew = false;
	if( pGearConnection == 0 )
	{
		pGearConnection = new CGearConnection;
		bNew = true;
	}

	if( pGearConnection == 0 )
		return false;

	if( bFromUI )
		PushUndo();

	if( bUseSizeAsSize )
	{
		double DocumentScale = GetUnitScale();
		Size1 /= DocumentScale;
		Size2 /= DocumentScale;
	}

	if( bNew || pGearConnection->m_pGear1 == pGear1 )
	{
		pGearConnection->m_pGear1 = pGear1;
		pGearConnection->m_pGear2 = pGear2;
		pGearConnection->m_Gear1Size = Size1;
		pGearConnection->m_Gear2Size = Size2;
	}
	else
	{
		pGearConnection->m_pGear1 = pGear2;
		pGearConnection->m_pGear2 = pGear1;
		pGearConnection->m_Gear1Size = Size2;
		pGearConnection->m_Gear2Size = Size1;
	}
	pGearConnection->m_ConnectionType = ConnectionType;
	pGearConnection->m_bUseSizeAsRadius = bUseSizeAsSize;

	if( bNew )
		m_GearConnectionList.AddTail( pGearConnection );

	return true;
}

void CLinkageDoc::RemoveGearRatio( CConnector *pGearConnector, CLink *pGearLink )
{
	POSITION Position = m_GearConnectionList.GetHeadPosition();
	while( Position != 0 )
	{
		POSITION CurrentPosition = Position;
		CGearConnection *pTestGearConnection = m_GearConnectionList.GetNext( Position );
		if( pTestGearConnection == 0 || pTestGearConnection->m_pGear1 == 0 || pTestGearConnection->m_pGear2 == 0 )
			continue;

		if( pTestGearConnection->m_pGear1 == pGearLink || pTestGearConnection->m_pGear2 == pGearLink
		    || pTestGearConnection->m_pGear1->GetGearConnector() == pGearConnector || pTestGearConnection->m_pGear2->GetGearConnector() == pGearConnector )
		{
			m_GearConnectionList.RemoveAt( CurrentPosition );
			delete pTestGearConnection;
		}
	}
}

void CLinkageDoc::FillElementList( ElementList &Elements )
{
	POSITION Position = m_Links.GetHeadPosition();
	while( Position != 0 )
	{
		CLink *pLink = m_Links.GetNext( Position );
		if( pLink == 0 )
			continue;
		Elements.AddTail( new CElementItem( pLink ) );
	}

	Position = m_Connectors.GetHeadPosition();
	while( Position != NULL )
	{
		CConnector* pConnector = m_Connectors.GetNext( Position );
		if( pConnector == 0 )
			continue;
		Elements.AddTail( new CElementItem( pConnector ) );
	}
}

void CLinkageDoc::FastenThese( CConnector *pFastenThis, CLink *pFastenToThis )
{
	Unfasten( pFastenThis );
	pFastenThis->FastenTo( pFastenToThis );
	pFastenToThis->AddFastenConnector( pFastenThis );
}

void CLinkageDoc::FastenThese( CLink *pFastenThis, CLink *pFastenToThis )
{
	if( pFastenThis == pFastenToThis )
		return;
	Unfasten( pFastenThis );
	pFastenThis->FastenTo( pFastenToThis );
	pFastenToThis->AddFastenLink( pFastenThis );
}

void CLinkageDoc::FastenThese( CLink *pFastenThis, CConnector *pFastenToThis )
{
	Unfasten( pFastenThis );
	pFastenThis->FastenTo( pFastenToThis );
	pFastenToThis->AddFastenLink( pFastenThis );
}

void CLinkageDoc::FastenThese( CConnector *pFastenThis, CConnector *pFastenToThis )
{
	Unfasten( pFastenThis );
	pFastenThis->FastenTo( pFastenToThis );
	pFastenToThis->AddFastenConnector( pFastenThis );
}

void CLinkageDoc::OnDocumentEvent(DocumentEvent deEvent)
{
	CDocument::OnDocumentEvent( deEvent );

	// NONE of this is necessary since the documents show up in the recent document list in Outlook if the system setting is enabled to show recent opened items.
	// But how do I get the file into that list when the setting is disabled? Word does it with doc files so the system setting is not the only thing affecting the list in Outlook.
	//if( deEvent == onAfterSaveDocument )
	//{
		// Save file name to system MRU.
		//SHAddToRecentDocs( SHARD::SHARD_PATHA, GetPathName() );
	//}
}

#if 0

CLinkageDoc *CLinkageDoc::GetPartsDocument( bool bRecompute )
{
	/*
	 * The parts document is a copy of the document but with some HUGE changes.
	 * Each part if rotated and aligned so that the bottom left corner of the part is at
	 * 0,0 in the document space. There are no connections between links; each link has a
	 * unique set of connectors. Right now, drawing elements are not part of the parts list,
	 * although I may change that in the future to allow some drawing elements to be used as parts.
	 * nothing is fastened in this parts docment.
	 *
	 * HOW DO I HANDLE GEAR SIZES?!?!
	 */

	if( !bRecompute && m_pPartsDoc != 0 )
		return m_pPartsDoc;

	if( m_pPartsDoc != 0 )
		delete m_pPartsDoc;

	m_pPartsDoc = new CLinkageDoc;
	m_pPartsDoc->CreatePartsFromDocument( this );
	return m_pPartsDoc;
}

void CLinkageDoc::MovePartsLinkToOrigin( CFPoint Origin, CLink *pPartsLink )
{
	int HullCount = 0;
	CFPoint *pPoints = pPartsLink->GetHull( HullCount );
	int BestStartPoint = -1;
	int BestEndPoint = -1;

	if( pPartsLink->GetConnectorCount() == 2 )
	{
		BestStartPoint = 0;
		BestEndPoint = 1;
	}
	else
	{
		double LargestDistance = 0.0;
		for( int Counter = 0; Counter < HullCount; ++Counter )
		{
			double TestDistance = 0;
			if( Counter < HullCount - 1 )
				TestDistance = Distance( pPoints[Counter], pPoints[Counter+1] );
			else
				TestDistance = Distance( pPoints[Counter], pPoints[0] );
			if( TestDistance > LargestDistance )
			{
				LargestDistance = TestDistance;
				BestStartPoint = Counter;
				// The hull is a clockwise collection of points so the end point of
				// the measurement is a better visual choice for the start connector.
				if( Counter < HullCount - 1 )
					BestEndPoint = Counter + 1;
				else
					BestEndPoint = 0;
			}
		}
	}
	if( BestStartPoint < 0 )
		return;

	CConnector *pStartConnector = 0;

	POSITION Position = pPartsLink->GetConnectorList()->GetHeadPosition();
	while( Position != 0 )
	{
		CConnector *pConnector = pPartsLink->GetConnectorList()->GetNext( Position );
		if( pConnector == 0 )
			continue;

		if( pConnector->GetPoint() == pPoints[BestStartPoint] )
		{
			pStartConnector = pConnector;
			break;
		}
	}

	CFLine OrientationLine( pPoints[BestStartPoint], pPoints[BestEndPoint] );
	double Angle = GetAngle( pPoints[BestStartPoint], pPoints[BestEndPoint] );

	CFPoint AdditionalOffset;
	Position = pPartsLink->GetConnectorList()->GetHeadPosition();
	while( Position != 0 )
	{
		CConnector *pConnector = pPartsLink->GetConnectorList()->GetNext( Position );
		if( pConnector == 0 || pConnector == pStartConnector )
			continue;

		CFPoint Point = pConnector->GetPoint();
		Point.RotateAround( pPoints[BestStartPoint], -Angle );
		pConnector->SetPoint( Point );
		if( Point.x - pPoints[BestStartPoint].x < AdditionalOffset.x )
			AdditionalOffset.x = Point.x - pPoints[BestStartPoint].x;
		if( Point.y - pPoints[BestStartPoint].y > AdditionalOffset.y )
			AdditionalOffset.y = Point.y - pPoints[BestStartPoint].y;
	}

	Position = pPartsLink->GetConnectorList()->GetHeadPosition();
	while( Position != 0 )
	{
		CConnector *pConnector = pPartsLink->GetConnectorList()->GetNext( Position );
		if( pConnector == 0 || pConnector == pStartConnector )
			continue;

		CFPoint Point = pConnector->GetPoint();
		Point -= pPoints[BestStartPoint];
		Point += Origin;
		Point -= AdditionalOffset;
		pConnector->SetPoint( Point );
	}
	Origin -= AdditionalOffset;
	pStartConnector->SetPoint( Origin );
}

void CLinkageDoc::CreatePartsFromDocument( CLinkageDoc *pOriginalDoc )
{
	CFRect Area;
	pOriginalDoc->GetDocumentArea( Area );

	POSITION Position = pOriginalDoc->m_Links.GetHeadPosition();
	while( Position != 0 )
	{
		CLink *pLink = pOriginalDoc->m_Links.GetNext( Position );
		if( pLink == 0 || ( pLink->GetLayers() & DRAWINGLAYER ) != 0 )
			continue;

		if( pLink->GetConnectorCount() <= 1 && !pLink->IsGear() )
			continue;

		CLink *pPartsLink = new CLink( *pLink );
		// MUST REMOVE THE COPIED CONNECTORS HERE BECAUSE THEY ARE JUST POINTER COPIES!
		pPartsLink->RemoveAllConnectors();

		POSITION Position2 = pLink->GetConnectorList()->GetHeadPosition();
		while( Position2 != 0 )
		{
			CConnector *pConnector = pLink->GetConnectorList()->GetNext( Position2 );
			if( pConnector == 0 )
				continue;

			CConnector *pPartsConnector = new CConnector( *pConnector );
			pPartsConnector->SetColor( pPartsLink->GetColor() );
			pPartsConnector->SetAsAnchor( false );
			pPartsConnector->SetAsInput( false );
			pPartsConnector->SetAsDrawing( false );
			pPartsLink->AddConnector( pPartsConnector );
			m_Connectors.AddTail( pPartsConnector );
		}

		MovePartsLinkToOrigin( Area.TopLeft(), pPartsLink );
		m_Links.AddTail( pPartsLink );
	}

	// Make a ground link. There must be at least one anchor for any mechanism.
	CLink *pGroundLink = new CLink;
	pGroundLink->SetIdentifier( 0x7FFFFFFF );
	pGroundLink->SetName( "Ground" );

	Position = pOriginalDoc->GetConnectorList()->GetHeadPosition();
	while( Position != 0 )
	{
		CConnector *pConnector = pOriginalDoc->GetConnectorList()->GetNext( Position );
		if( pConnector == 0 || !pConnector->IsAnchor() )
			continue;

		pGroundLink->SetLayers( pConnector->GetLayers() );

		CConnector *pPartsConnector = new CConnector( *pConnector );
		pPartsConnector->SetColor( CNullableColor( RGB( 0, 0, 0 ) ) ); // pPartsLink->GetColor() );
		pPartsConnector->SetAsAnchor( false );
		pPartsConnector->SetAsInput( false );
		pPartsConnector->SetAsDrawing( false );
		pGroundLink->AddConnector( pPartsConnector );
		m_Connectors.AddTail( pPartsConnector );
	}
	MovePartsLinkToOrigin( Area.TopLeft(), pGroundLink );
	m_Links.AddTail( pGroundLink );
}
#endif

