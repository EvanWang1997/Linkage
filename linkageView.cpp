// LinkageView.cpp : implementation of the CLinkageView class
//

#include "stdafx.h"
#include "Linkage.h"
#include "mainfrm.h"
#include "geometry.h"
#include "link.h"
#include "connector.h"
#include "LinkageDoc.h"
#include "LinkageView.h"
#include "mmsystem.h"
#include "ConnectorPropertiesDialog.h"
#include "Renderer.h"
#include "graham.h"
#include "imageselectdialog.h"
#include "float.h"
#include "linkpropertiesdialog.h"
#include "testdialog.h"
#include "DebugItem.h"
#include "RecordDialog.h"
#include "AngleDialog.h"
#include <stdio.h>
#include <string.h>
#include "gdiplus.h"
#include "SampleGallery.h"
#include "PointDialog.h"
#include "LinePropertiesDialog.h"
#include "DXFFile.h"
#include "GearRatioDialog.h"
#include "ExportImageSettingsDialog.h"
#include "SelectElementsDialog.h"
#include "LocationDialog.h"
#include "LengthDistanceDialog.h"
#include "RotateDialog.h"
#include "ScaleDialog.h"
#include "Base64.h"
#include "UserGridDialog.h"
#include "helper.h"
#include "MyMFCRibbonLabel.h"
#include <fstream>

#include <algorithm>
#include <vector>

using namespace std;

using namespace Gdiplus;

CDebugItemList DebugItemList;

static const int TimerMinimumResolution = 1; // Milliseconds.

static const double ZOOM_AMOUNT = 1.3;

static const double LINK_INSERT_PIXELS = 40;

static const int GRAB_DISTANCE = 6;

static const int FRAMES_PER_SECONDS = 30;

static const double PROFILE_FIXEDPOINT = 1000000;

static const int ANIMATIONWIDTH = 1280;
static const int ANIMATIONHEIGHT = 720;

static const int OFFSET_INCREMENT = 25;
static const int RADIUS_MEASUREMENT_OFFSET = 9;

static COLORREF COLOR_ADJUSTMENTKNOBS = RGB( 0, 0, 0 );
static COLORREF COLOR_BLACK = RGB( 0, 0, 0 );
static COLORREF COLOR_NONADJUSTMENTKNOBS = RGB( 170, 170, 170 );
static COLORREF COLOR_ALIGNMENTHINT = RGB( 170, 220, 170 );
static COLORREF COLOR_SNAPHINT = RGB( 152, 207, 239 );
static COLORREF COLOR_SUPERHIGHLIGHT = RGB( 255, 255, 192 );
static COLORREF COLOR_SUPERHIGHLIGHT2 = RGB( 255, 255, 128 );
static COLORREF COLOR_TEXT = RGB( 24, 24, 24 );
static COLORREF COLOR_CHAIN = RGB( 50, 50, 50 );
static COLORREF COLOR_STRETCHDOTS = RGB( 220, 220, 220 );
static COLORREF COLOR_MOTIONPATH = RGB( 60, 60, 60 );
static COLORREF COLOR_DRAWINGLIGHT = RGB( 200, 200, 200 );
static COLORREF COLOR_DRAWINGDARK = RGB( 70, 70, 70 );
static COLORREF COLOR_GRID = RGB( 200, 240, 240 );
static COLORREF COLOR_GROUND = RGB( 100, 100, 100 );
static COLORREF COLOR_MINORSELECTION = RGB( 190, 190, 190 );
static COLORREF COLOR_LASTSELECTION = RGB( 196, 96, 96 );
static COLORREF COLOR_MAJORSELECTION = RGB( 255, 200, 200 );
static COLORREF COLOR_ANICROP = RGB( 196, 196, 255 );

static const int CONNECTORRADIUS = 5;
static const int CONNECTORTRIANGLE = 6;
static const int ANCHORBASE = 4;

static const char* SETTINGS = "Settings";

#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const int VIRTUAL_PIXEL_SNAP_DISTANCE = 6;

static void swap( int& a, int& b )
{
	int temp = a;
	a = b;
	b = temp;
}

wchar_t* ToUnicode( const char *pString )
{
	size_t origsize = strlen( pString ) + 1;
    size_t convertedChars = 0;
    wchar_t *wcstring = new wchar_t[origsize];
    mbstowcs_s( &convertedChars, wcstring, origsize, pString, _TRUNCATE );
	return wcstring;
}

static int GetLongestHullDimensionLine( CLink *pLink, int &BestEndPoint, CFPoint *pHullPoints, int HullPointCount, CConnector **pStartConnector );
static int GetLongestLinearDimensionLine( CLink *pLink, int &BestEndPoint, CFPoint *pPoints, int PointCount, CConnector **pStartConnector );

class CTempLink : public CLink
{
	public:
	CTempLink() : CLink(), OriginalLink( *this ) {}
	CTempLink( const CLink &ExistingLink ) : CLink( ExistingLink ), OriginalLink( ExistingLink ) {}

	const CLink &OriginalLink; // Can't set it from a reference

	virtual bool GetGearRadii( const GearConnectionList &ConnectionList, std::list<double> &RadiusList ) const
	{
		if( &OriginalLink == this )
			return false;
		return OriginalLink.GetGearRadii( ConnectionList, RadiusList );
	}
};

/////////////////////////////////////////////////////////////////////////////
// CLinkageView

IMPLEMENT_DYNCREATE(CLinkageView, CView)

BEGIN_MESSAGE_MAP(CLinkageView, CView)
	//{{AFX_MSG_MAP(CLinkageView)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	//ON_MESSAGE(WM_GESTURE, OnGesture)
	ON_COMMAND(ID_MECHANISM_RESET, OnMechanismReset)
	ON_COMMAND(ID_MECHANISM_SIMULATE, &CLinkageView::OnMechanismSimulate)
	ON_COMMAND(ID_CONTROL_WINDOW, &CLinkageView::OnControlWindow)
	ON_UPDATE_COMMAND_UI(ID_MECHANISM_SIMULATE, &CLinkageView::OnUpdateButtonRun)
	ON_COMMAND(ID_SIMULATION_SIMULATE, &CLinkageView::OnMechanismQuicksim)
	ON_UPDATE_COMMAND_UI(ID_SIMULATION_SIMULATE, &CLinkageView::OnUpdateButtonRun)

	ON_COMMAND(ID_SIMULATE_INTERACTIVE, &CLinkageView::OnSimulateInteractive)
	ON_UPDATE_COMMAND_UI(ID_SIMULATE_INTERACTIVE, &CLinkageView::OnUpdateButtonRun)

	ON_COMMAND(ID_SIMULATION_PAUSE, &CLinkageView::OnSimulatePause)
	ON_UPDATE_COMMAND_UI(ID_SIMULATION_PAUSE, &CLinkageView::OnUpdateSimulatePause)
	ON_COMMAND(ID_SIMULATION_ONECYCLEX, &CLinkageView::OnSimulateOneCycleX)
	ON_UPDATE_COMMAND_UI(ID_SIMULATION_ONECYCLEX, &CLinkageView::OnUpdateSimulateOneCycle)
	ON_COMMAND(ID_SIMULATION_ONECYCLE, &CLinkageView::OnSimulateOneCycle)
	ON_UPDATE_COMMAND_UI(ID_SIMULATION_ONECYCLE, &CLinkageView::OnUpdateSimulateOneCycle)
	ON_COMMAND(ID_SIMULATE_FORWARD, &CLinkageView::OnSimulateForward)
	ON_UPDATE_COMMAND_UI(ID_SIMULATE_FORWARD, &CLinkageView::OnUpdateSimulateForwardBackward)
	ON_COMMAND(ID_SIMULATE_BACKWARD, &CLinkageView::OnSimulateBackward)
	ON_UPDATE_COMMAND_UI(ID_SIMULATE_BACKWARD, &CLinkageView::OnUpdateSimulateForwardBackward)

	ON_COMMAND(ID_ESCAPE, &CLinkageView::OnEscape)
	ON_UPDATE_COMMAND_UI(ID_ESCAPE, &CLinkageView::OnUpdateEscape)

	ON_COMMAND(ID_SIMULATE_FORWARD_BIG, &CLinkageView::OnSimulateForwardBig)
	ON_UPDATE_COMMAND_UI(ID_SIMULATE_FORWARD_BIG, &CLinkageView::OnUpdateSimulateForwardBackward)
	ON_COMMAND(ID_SIMULATE_BACKWARD_BIG, &CLinkageView::OnSimulateBackwardBig)
	ON_UPDATE_COMMAND_UI(ID_SIMULATE_BACKWARD_BIG, &CLinkageView::OnUpdateSimulateForwardBackward)
	ON_COMMAND(ID_SIMULATE_MANUAL, &CLinkageView::OnSimulateManual)
	ON_UPDATE_COMMAND_UI(ID_SIMULATE_MANUAL, &CLinkageView::OnUpdateButtonRun)
	ON_WM_TIMER()
	ON_UPDATE_COMMAND_UI(ID_MECHANISM_RESET, &CLinkageView::OnUpdateMechanismReset)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COMBINE, &CLinkageView::OnUpdateEditCombine)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CONNECT, &CLinkageView::OnUpdateEditConnect)
	ON_COMMAND(ID_EDIT_COMBINE, &CLinkageView::OnEditCombine)
	ON_COMMAND(ID_EDIT_CONNECT, &CLinkageView::OnEditConnect)
	ON_COMMAND(ID_EDIT_FASTEN, &CLinkageView::OnEditFasten)
	ON_UPDATE_COMMAND_UI(ID_EDIT_FASTEN, &CLinkageView::OnUpdateEditFasten)
	ON_COMMAND(ID_EDIT_UNFASTEN, &CLinkageView::OnEditUnfasten)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNFASTEN, &CLinkageView::OnUpdateEditUnfasten)
	ON_COMMAND(ID_EDIT_SELECTLINK, &CLinkageView::OnEditSelectLink)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SELECTLINK, &CLinkageView::OnUpdateEditSelectLink)
	ON_COMMAND(ID_EDIT_SET_RATIO, &CLinkageView::OnEditSetRatio)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SET_RATIO, &CLinkageView::OnUpdateEditSetRatio)
	ON_COMMAND(ID_DIMENSION_SETLOCATION, &CLinkageView::OnEditSetLocation)
	ON_UPDATE_COMMAND_UI(ID_DIMENSION_SETLOCATION, &CLinkageView::OnUpdateEditSetLocation)
	ON_COMMAND(ID_DIMENSION_SETLENGTH, &CLinkageView::OnEditSetLength)
	ON_UPDATE_COMMAND_UI(ID_DIMENSION_SETLENGTH, &CLinkageView::OnUpdateEditSetLength)
	ON_COMMAND(ID_DIMENSION_ANGLE, &CLinkageView::OnAlignSetAngle)
	ON_UPDATE_COMMAND_UI(ID_DIMENSION_ANGLE, &CLinkageView::OnUpdateAlignSetAngle)
	ON_COMMAND(ID_DIMENSION_ROTATE, &CLinkageView::OnEditRotate)
	ON_UPDATE_COMMAND_UI(ID_DIMENSION_ROTATE, &CLinkageView::OnUpdateEditRotate)
	ON_COMMAND(ID_DIMENSION_SCALE, &CLinkageView::OnEditScale)
	ON_UPDATE_COMMAND_UI(ID_DIMENSION_SCALE, &CLinkageView::OnUpdateEditScale)
	
	ON_COMMAND(ID_EDIT_MAKEANCHOR, &CLinkageView::OnEditmakeAnchor)
	ON_UPDATE_COMMAND_UI(ID_EDIT_MAKEANCHOR, &CLinkageView::OnUpdateEditmakeAnchor)

	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETESELECTED, &CLinkageView::OnUpdateEditSelected)
	ON_COMMAND(ID_EDIT_DELETESELECTED, &CLinkageView::OnEditDeleteselected)
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_UPDATE_COMMAND_UI(ID_SIMULATION_RUN, &CLinkageView::OnUpdateButtonRun)
	ON_COMMAND(ID_SIMULATION_RUN, &CLinkageView::OnButtonRun)
	ON_UPDATE_COMMAND_UI(ID_SIMULATION_STOP, &CLinkageView::OnUpdateButtonStop)
	ON_COMMAND(ID_SIMULATION_STOP, &CLinkageView::OnButtonStop)
	ON_UPDATE_COMMAND_UI(ID_SIMULATION_PIN, &CLinkageView::OnUpdateButtonPin)
	ON_COMMAND(ID_SIMULATION_PIN, &CLinkageView::OnButtonPin)

	ON_UPDATE_COMMAND_UI(ID_ALIGN_ALIGNBUTTON, &CLinkageView::OnUpdateAlignButton)
	ON_UPDATE_COMMAND_UI(ID_ALIGN_SNAPBUTTON, &CLinkageView::OnUpdateNotSimulating)
	ON_UPDATE_COMMAND_UI(ID_ALIGN_DETAILSBUTTON, &CLinkageView::OnUpdateNotSimulating)
	ON_UPDATE_COMMAND_UI(ID_DIMENSION_SET, &CLinkageView::OnUpdateNotSimulating)
	ON_UPDATE_COMMAND_UI(ID_EDIT_INSERTBUTTON, &CLinkageView::OnUpdateEdit)

	ON_COMMAND( ID_FILE_PRINTFULL, OnPrintFull )
	ON_UPDATE_COMMAND_UI( ID_FILE_PRINTFULL, OnUpdatePrintFull )

	ON_COMMAND( ID_FILE_PRINT_SETUP, OnFilePrintSetup )
	ON_COMMAND(ID_INSERT_CIRCLE, (AFX_PMSG)&CLinkageView::OnInsertCircle)
	ON_COMMAND(ID_INSERT_CONNECTOR, (AFX_PMSG)&CLinkageView::OnInsertConnector)
	ON_COMMAND(ID_INSERT_POINT,(AFX_PMSG)&CLinkageView::OnInsertPoint)
	ON_COMMAND(ID_INSERT_LINE,(AFX_PMSG)&CLinkageView::OnInsertLine)
	ON_COMMAND(ID_INSERT_MEASUREMENT,(AFX_PMSG)&CLinkageView::OnInsertMeasurement)
	ON_COMMAND(ID_INSERT_GEAR,(AFX_PMSG)&CLinkageView::OnInsertGear)
	ON_COMMAND(ID_INSERT_LINK2, (AFX_PMSG)&CLinkageView::OnInsertDouble)
	ON_COMMAND(ID_INSERT_LINK3, (AFX_PMSG)&CLinkageView::OnInsertTriple)
	ON_COMMAND(ID_INSERT_LINK4, (AFX_PMSG)&CLinkageView::OnInsertQuad)
	ON_COMMAND(ID_INSERT_ANCHOR, (AFX_PMSG)&CLinkageView::OnInsertAnchor)
	ON_COMMAND(ID_INSERT_ANCHORLINK, (AFX_PMSG)&CLinkageView::OnInsertAnchorLink)
	ON_COMMAND(ID_INSERT_INPUT, (AFX_PMSG)&CLinkageView::OnInsertRotatinganchor)
	ON_UPDATE_COMMAND_UI(ID_INSERT_POINT, &CLinkageView::OnUpdateEdit)
	ON_UPDATE_COMMAND_UI(ID_INSERT_LINE, &CLinkageView::OnUpdateEdit)
	ON_UPDATE_COMMAND_UI(ID_INSERT_CIRCLE, &CLinkageView::OnUpdateEdit)
	ON_UPDATE_COMMAND_UI(ID_INSERT_CONNECTOR, &CLinkageView::OnUpdateEdit)
	ON_UPDATE_COMMAND_UI(ID_INSERT_LINK2, &CLinkageView::OnUpdateEdit)
	ON_UPDATE_COMMAND_UI(ID_INSERT_LINK3, &CLinkageView::OnUpdateEdit)
	ON_UPDATE_COMMAND_UI(ID_INSERT_LINK4, &CLinkageView::OnUpdateEdit)
	ON_UPDATE_COMMAND_UI(ID_INSERT_ACTUATOR, &CLinkageView::OnUpdateEdit)
	ON_UPDATE_COMMAND_UI(ID_INSERT_ANCHOR, &CLinkageView::OnUpdateEdit)
	ON_UPDATE_COMMAND_UI(ID_INSERT_ANCHORLINK, &CLinkageView::OnUpdateEdit)
	ON_UPDATE_COMMAND_UI(ID_INSERT_INPUT, &CLinkageView::OnUpdateEdit)
	ON_COMMAND(ID_INSERT_LINK, (AFX_PMSG)&CLinkageView::OnInsertLink)
	ON_UPDATE_COMMAND_UI(ID_PROPERTIES_PROPERTIES, &CLinkageView::OnUpdateEditProperties)
	ON_COMMAND(ID_PROPERTIES_PROPERTIES, &CLinkageView::OnEditProperties)
	ON_WM_LBUTTONDBLCLK()
	ON_COMMAND(ID_EDIT_JOIN, &CLinkageView::OnEditJoin)
	ON_UPDATE_COMMAND_UI(ID_EDIT_JOIN, &CLinkageView::OnUpdateEditJoin)
	ON_COMMAND(ID_EDIT_LOCK, &CLinkageView::OnEditLock)
	ON_UPDATE_COMMAND_UI(ID_EDIT_LOCK, &CLinkageView::OnUpdateEditLock)
	ON_COMMAND(ID_EDIT_SELECT_ALL, &CLinkageView::OnEditSelectall)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SELECT_ALL, &CLinkageView::OnUpdateSelectall)
	ON_COMMAND(ID_EDIT_SELECT_ELEMENTS, &CLinkageView::OnEditSelectElements)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SELECT_ELEMENTS, &CLinkageView::OnUpdateSelectElements)

	ON_COMMAND(ID_EDIT_SPLIT, &CLinkageView::OnEditSplit)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SPLIT, &CLinkageView::OnUpdateEditSplit)

	ON_COMMAND(ID_EDIT_ADDCONNECTOR, &CLinkageView::OnAddConnector)
	ON_UPDATE_COMMAND_UI(ID_EDIT_ADDCONNECTOR, &CLinkageView::OnUpdateAddConnector)

	ON_COMMAND(ID_ALIGN_SETANGLE, &CLinkageView::OnAlignSetAngle)
	ON_UPDATE_COMMAND_UI(ID_ALIGN_SETANGLE, &CLinkageView::OnUpdateAlignSetAngle)
	ON_COMMAND(ID_ALIGN_RIGHTANGLE, &CLinkageView::OnAlignRightAngle)
	ON_UPDATE_COMMAND_UI(ID_ALIGN_RIGHTANGLE, &CLinkageView::OnUpdateAlignRightAngle)
	ON_COMMAND(ID_ALIGN_RECTANGLE, &CLinkageView::OnAlignRectangle)
	ON_UPDATE_COMMAND_UI(ID_ALIGN_RECTANGLE, &CLinkageView::OnUpdateAlignRectangle)
	ON_COMMAND(ID_ALIGN_PARALLELOGRAM, &CLinkageView::OnAlignParallelogram)
	ON_UPDATE_COMMAND_UI(ID_ALIGN_PARALLELOGRAM, &CLinkageView::OnUpdateAlignParallelogram)

	ON_COMMAND(ID_ALIGN_HORIZONTAL, &CLinkageView::OnAlignHorizontal)
	ON_UPDATE_COMMAND_UI(ID_ALIGN_HORIZONTAL, &CLinkageView::OnUpdateAlignHorizontal)
	ON_COMMAND(ID_ALIGN_VERTICAL, &CLinkageView::OnAlignVertical)
	ON_UPDATE_COMMAND_UI(ID_ALIGN_VERTICAL, &CLinkageView::OnUpdateAlignVertical)
	ON_COMMAND(ID_ALIGN_LINEUP, &CLinkageView::OnAlignLineup)
	ON_UPDATE_COMMAND_UI(ID_ALIGN_LINEUP, &CLinkageView::OnUpdateAlignLineup)
	ON_COMMAND(ID_ALIGN_EVENSPACE, &CLinkageView::OnAlignEvenSpace)
	ON_UPDATE_COMMAND_UI(ID_ALIGN_EVENSPACE, &CLinkageView::OnUpdateAlignEvenSpace)

	ON_COMMAND(ID_ALIGN_FLIPH, &CLinkageView::OnFlipHorizontal)
	ON_UPDATE_COMMAND_UI(ID_ALIGN_FLIPH, &CLinkageView::OnUpdateFlipHorizontal)
	ON_COMMAND(ID_ALIGN_FLIPV, &CLinkageView::OnFlipVertical)
	ON_UPDATE_COMMAND_UI(ID_ALIGN_FLIPV, &CLinkageView::OnUpdateFlipVertical)
	ON_COMMAND(ID_ALIGN_MEET, &CLinkageView::OnRotateToMeet)
	ON_UPDATE_COMMAND_UI(ID_ALIGN_MEET, &CLinkageView::OnUpdateRotateToMeet)



	
	ON_COMMAND(ID_VIEW_LABELS, &CLinkageView::OnViewLabels)
	ON_UPDATE_COMMAND_UI(ID_VIEW_LABELS, &CLinkageView::OnUpdateViewLabels)
	ON_COMMAND(ID_VIEW_ANGLES, &CLinkageView::OnViewAngles)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ANGLES, &CLinkageView::OnUpdateViewAngles)
	ON_COMMAND(ID_EDIT_SNAP, &CLinkageView::OnEditSnap)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SNAP, &CLinkageView::OnUpdateEditSnap)
	ON_COMMAND(ID_EDIT_GRIDSNAP, &CLinkageView::OnEditGridSnap)
	ON_UPDATE_COMMAND_UI(ID_EDIT_GRIDSNAP, &CLinkageView::OnUpdateEditGridSnap)
	ON_COMMAND(ID_EDIT_MOMENTUM, &CLinkageView::OnMoreMomentum)
	ON_UPDATE_COMMAND_UI(ID_EDIT_MOMENTUM, &CLinkageView::OnUpdateMoreMomentum)

	ON_COMMAND(ID_EDIT_AUTOJOIN, &CLinkageView::OnEditAutoJoin)
	ON_UPDATE_COMMAND_UI(ID_EDIT_AUTOJOIN, &CLinkageView::OnUpdateEditAutoJoin)

	ON_COMMAND(ID_VIEW_DATA, &CLinkageView::OnViewData)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DATA, &CLinkageView::OnUpdateViewData)
	ON_COMMAND(ID_VIEW_DIMENSIONS, &CLinkageView::OnViewDimensions)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DIMENSIONS, &CLinkageView::OnUpdateViewDimensions)
	ON_COMMAND(ID_VIEW_GROUNDDIMENSIONS, &CLinkageView::OnViewGroundDimensions)
	ON_UPDATE_COMMAND_UI(ID_VIEW_GROUNDDIMENSIONS, &CLinkageView::OnUpdateViewGroundDimensions)
	ON_COMMAND(ID_VIEW_DRAWINGLAYERDIMENSIONS, &CLinkageView::OnViewDrawingLayerDimensions)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DRAWINGLAYERDIMENSIONS, &CLinkageView::OnUpdateViewDrawingLayerDimensions)

	ON_COMMAND(ID_VIEW_SOLIDLINKS, &CLinkageView::OnViewSolidLinks)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SOLIDLINKS, &CLinkageView::OnUpdateViewSolidLinks)

	ON_COMMAND(ID_VIEW_DRAWING, &CLinkageView::OnViewDrawing)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DRAWING, &CLinkageView::OnUpdateViewDrawing)
	ON_COMMAND(ID_VIEW_MECHANISM, &CLinkageView::OnViewMechanism)
	ON_UPDATE_COMMAND_UI(ID_VIEW_MECHANISM, &CLinkageView::OnUpdateViewMechanism)

	ON_COMMAND(ID_EDIT_DRAWING, &CLinkageView::OnEditDrawing)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DRAWING, &CLinkageView::OnUpdateEditDrawing)
	ON_COMMAND(ID_EDIT_MECHANISM, &CLinkageView::OnEditMechanism)
	ON_UPDATE_COMMAND_UI(ID_EDIT_MECHANISM, &CLinkageView::OnUpdateEditMechanism)

	ON_COMMAND(ID_VIEW_DEBUG, &CLinkageView::OnViewDebug)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DEBUG, &CLinkageView::OnUpdateViewDebug)
	ON_COMMAND(ID_VIEW_ANICROP, &CLinkageView::OnViewAnicrop)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ANICROP, &CLinkageView::OnUpdateViewAnicrop)
	ON_COMMAND(ID_VIEW_LARGEFONT, &CLinkageView::OnViewLargeFont)
	ON_UPDATE_COMMAND_UI(ID_VIEW_LARGEFONT, &CLinkageView::OnUpdateViewLargeFont)

	ON_COMMAND(ID_VIEW_SHOWGRID, &CLinkageView::OnViewShowGrid)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOWGRID, &CLinkageView::OnUpdateViewShowGrid)
	ON_COMMAND(ID_VIEW_EDITGRID, &CLinkageView::OnEditGrid)
	ON_UPDATE_COMMAND_UI(ID_VIEW_EDITGRID, &CLinkageView::OnUpdateEditGrid)
	ON_COMMAND(ID_VIEW_PARTS, &CLinkageView::OnViewParts)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PARTS, &CLinkageView::OnUpdateViewParts)

	ON_COMMAND(ID_EDIT_CUT, &CLinkageView::OnEditCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, &CLinkageView::OnUpdateEditSelected)
	ON_COMMAND(ID_EDIT_COPY, &CLinkageView::OnEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, &CLinkageView::OnUpdateEditSelected)
	ON_COMMAND(ID_EDIT_PASTE, &CLinkageView::OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, &CLinkageView::OnUpdateEditPaste)

	ON_COMMAND(ID_VIEW_BOLD, &CLinkageView::OnViewBold)
	ON_UPDATE_COMMAND_UI(ID_VIEW_BOLD, &CLinkageView::OnUpdateViewBold)

	ON_COMMAND( ID_VIEW_UNITS, &CLinkageView::OnViewUnits )
	ON_COMMAND( ID_VIEW_COORDINATES, &CLinkageView::OnViewCoordinates )

	ON_UPDATE_COMMAND_UI(ID_VIEW_UNITS, &CLinkageView::OnUpdateViewUnits)
	ON_UPDATE_COMMAND_UI(ID_VIEW_COORDINATES, &CLinkageView::OnUpdateViewCoordinates)

	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_MOUSEWHEEL()
	ON_COMMAND(ID_VIEW_ZOOMIN, &CLinkageView::OnViewZoomin)
	ON_COMMAND(ID_VIEW_ZOOMOUT, &CLinkageView::OnViewZoomout)
	ON_COMMAND(ID_VIEW_ZOOMFIT, &CLinkageView::OnMenuZoomfit)
	//ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMIN, &CLinkageView::OnUpdateNotSimulating)
	//ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMOUT, &CLinkageView::OnUpdateNotSimulating)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMFIT, &CLinkageView::OnUpdateNotSimulating)
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_SAVEVIDEO, &OnFileSaveVideo)
	ON_COMMAND(ID_FILE_SAVEIMAGE, &OnFileSaveImage)
	ON_COMMAND(ID_FILE_SAVEDXF, &OnFileSaveDXF)
	ON_COMMAND(ID_FILE_SAVEMOTION, &OnFileSaveMotion)
	ON_COMMAND(ID_BACKGROUND_OPEN, &OnFileOpenBackground)
	ON_COMMAND(ID_BACKGROUND_TRANSPARENCY, &OnBackgroundTransparency)
	ON_UPDATE_COMMAND_UI(ID_BACKGROUND_TRANSPARENCY, &OnUpdateBackgroundTransparency)
	ON_COMMAND(ID_BACKGROUND_DELETE, &OnDeleteBackground)
	ON_UPDATE_COMMAND_UI(ID_BACKGROUND_DELETE, &OnUpdateDeleteBackground)
	
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVEMOTION, &CLinkageView::OnUpdateFileSaveMotion)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CLinkageView::OnFilePrintPreview)
	ON_COMMAND(ID_SELECT_NEXT, OnSelectNext)
	ON_COMMAND(ID_SELECT_PREVIOUS, OnSelectPrevious)

	ON_COMMAND(ID_NUDGE_LEFT, OnNudgeLeft)
	ON_COMMAND(ID_NUDGE_RIGHT, OnNudgeRight)
	ON_COMMAND(ID_NUDGE_UP, OnNudgeUp)
	ON_COMMAND(ID_NUDGE_DOWN, OnNudgeDown)
	ON_COMMAND(ID_NUDGE_BIGLEFT, OnNudgeBigLeft)
	ON_COMMAND(ID_NUDGE_BIGRIGHT, OnNudgeBigRight)
	ON_COMMAND(ID_NUDGE_BIGUP, OnNudgeBigUp)
	ON_COMMAND(ID_NUDGE_BIGDOWN, OnNudgeBigDown)

	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, &CLinkageView::OnUpdateEditUndo)
	ON_COMMAND(ID_EDIT_UNDO, &CLinkageView::OnEditUndo)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_COMMAND(ID_EDIT_SLIDE, &CLinkageView::OnEditSlide)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SLIDE, &CLinkageView::OnUpdateEditSlide)
	ON_UPDATE_COMMAND_UI(ID_INSERT_SLIDER, &CLinkageView::OnUpdateEdit)
	//ON_UPDATE_COMMAND_UI( ID_RIBBON_SAMPLE_GALLERY, &CLinkageView::OnUpdateEdit)
	ON_COMMAND(ID_INSERT_ACTUATOR, (AFX_PMSG)&CLinkageView::OnInsertActuator)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_COMMAND(ID_POPUP_GALLERY, &CLinkageView::OnPopupGallery)
	ON_WM_KEYUP()
	ON_WM_KEYDOWN()
	ON_WM_CHAR()
	ON_COMMAND_RANGE( ID_SAMPLE_SIMPLE, ID_SAMPLE_UNUSED25, OnSelectSample )
	//}}AFX_MSG_MAP
		ON_COMMAND(ID_FILE_SAVE, &CLinkageView::OnFileSave)
		ON_WM_SETFOCUS()
		ON_WM_KILLFOCUS()
		END_MESSAGE_MAP()

static const int SMALL_FONT_SIZE = 9;
static const int MEDIUM_FONT_SIZE = 13;

static COLORREF DesaturateColor( COLORREF Color, double ByAmount )
{
	double L = 0.3 * GetRValue( Color ) + 0.6 * GetGValue( Color ) + 0.1 * GetBValue( Color );
	return RGB( GetRValue( Color ) + ByAmount * (L - GetRValue( Color )),
		GetGValue( Color ) + ByAmount * (L - GetGValue( Color )),
		GetBValue( Color ) + ByAmount * (L - GetBValue( Color )) );
}

static COLORREF LightenColor( COLORREF Color, double byAmount )
{
	double Red = GetRValue( Color ) / 255.0;
	double Green = GetGValue( Color ) / 255.0;
	double Blue = GetBValue( Color ) / 255.0;
	if( Red < 1.0 )
		Red += ( 1.0 - Red ) * byAmount;
	if( Green < 1.0 )
		Green += ( 1.0 - Green ) * byAmount;
	if( Blue < 1.0 )
		Blue += ( 1.0 - Blue ) * byAmount;

	return RGB( (int)( Red * 255 ), (int)( Green * 255 ), (int)( Blue * 255 ) );
}

/////////////////////////////////////////////////////////////////////////////
// CLinkageView construction/destruction

CLinkageView::CLinkageView()
{
	m_MouseAction = ACTION_NONE;
	m_bSimulating = false;
	m_bAllowEdit = true;
	m_SimulationControl = AUTO;
	m_bShowLabels = true;
	m_bShowAngles = true;
	m_bSnapOn = true;
	m_bGridSnap = false;
	m_bAutoJoin = false;
	m_bShowDimensions = false;
	m_bShowGroundDimensions = true;
	m_bShowDrawingLayerDimensions = false;
	m_bNewLinksSolid = false;
	m_xAutoGrid = 20.0;
	m_yAutoGrid = 20.0;
	m_xUserGrid = 0;
	m_yUserGrid = 0;
	m_bShowData = false;
	m_bShowDebug = false;
	m_bShowBold = false;
	m_bShowAnicrop = false;
	m_bShowLargeFont = false;
	m_bShowGrid = false;
	m_GridType = 0;
	m_bUseMoreMomentum = false;
	m_ScreenZoom = 1;
	m_ScrollPosition.SetPoint( 0, 0 );
	m_ScreenScrollPosition.SetPoint( 0, 0 );
	m_DragStartPoint.SetPoint( 0, 0 );
	m_StretchingControl = AC_TOP_LEFT;
	m_VisibleAdjustment = ADJUSTMENT_NONE;
	m_bChangeAdjusters = false;
	m_bMouseMovedEnough = false;
	m_bSuperHighlight = false;
	m_YUpDirection = -1;
	m_SimulationSteps = 0;
	m_PauseStep = -1;
	m_ForceCPM = 0;
	m_hTimer = 0;
	m_hTimerQueue = 0;
	m_pCachedRenderBitmap = 0;
	m_CachedBitmapWidth = 0;
	m_CachedBitmapHeight = 0;
	m_PrintOrientationMode = DMORIENT_PORTRAIT;
	m_PrintWidthInPages = 1;
	m_VisiblePageNumber = 0;
	m_SelectionDepth = 0;

	m_bRequestAbort = false;
	m_bProcessingVideoThread = false;
	m_pVideoBitmapQueue = 0;
	m_hVideoFrameEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	m_VideoStartFrames = 0;
	m_VideoRecordFrames = 0;
	m_VideoEndFrames = 0;
	m_bUseVideoCounters = false;
	m_bShowSelection = true;
	m_pBackgroundBitmap = 0;

	m_SelectedEditLayers = CLinkageDoc::ALLLAYERS;
	m_SelectedViewLayers = CLinkageDoc::ALLLAYERS;

	m_pAvi = 0;
	m_bRecordingVideo = false;
	m_RecordQuality = 0;

	m_pPopupGallery = new CPopupGallery( ID_POPUP_GALLERY, IDB_INSERT_POPUP_GALLERY, 48 );
	if( m_pPopupGallery != 0 )
	{
		m_pPopupGallery->SetRows( 3 );
		m_pPopupGallery->SetTooltip( 0, ID_INSERT_CONNECTOR );
		m_pPopupGallery->SetTooltip( 1, ID_INSERT_ANCHOR );
		m_pPopupGallery->SetTooltip( 2, ID_INSERT_LINK2 );
		m_pPopupGallery->SetTooltip( 3, ID_INSERT_ANCHORLINK );
		m_pPopupGallery->SetTooltip( 4, ID_INSERT_INPUT );
		m_pPopupGallery->SetTooltip( 5, ID_INSERT_ACTUATOR );
		m_pPopupGallery->SetTooltip( 6, ID_INSERT_LINK3 );
		m_pPopupGallery->SetTooltip( 7, ID_INSERT_LINK4 );
		m_pPopupGallery->SetTooltip( 8, ID_INSERT_GEAR );
		//m_pPopupGallery->SetTooltip( 9, nothing );
		m_pPopupGallery->SetTooltip( 10, ID_INSERT_POINT );
		m_pPopupGallery->SetTooltip( 11, ID_INSERT_LINE );
		m_pPopupGallery->SetTooltip( 12, ID_INSERT_MEASUREMENT );
		m_pPopupGallery->SetTooltip( 13, ID_INSERT_CIRCLE );
	}

	m_SmallFont.CreateFont( -SMALL_FONT_SIZE, 0, 0, 0, FW_LIGHT, 0, 0, 0,
							DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
							ANTIALIASED_QUALITY | CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_SWISS,
							"arial" );

	m_MediumFont.CreateFont( -MEDIUM_FONT_SIZE, 0, 0, 0, FW_NORMAL, 0, 0, 0,
							DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
							ANTIALIASED_QUALITY | CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_SWISS,
							"arial" );

	CWinApp *pApp = AfxGetApp();
	if( pApp != 0 )
	{
		m_bShowLabels = pApp->GetProfileInt( SETTINGS, "Showlabels", 1 ) != 0;
		m_bShowAngles = pApp->GetProfileInt( SETTINGS, "Showangles", 1 ) != 0;
		m_bShowDebug = pApp->GetProfileInt( SETTINGS, "Showdebug", 0 ) != 0;
		m_bShowBold = pApp->GetProfileInt( SETTINGS, "Showdata", 0 ) != 0;
		m_bShowData = pApp->GetProfileInt( SETTINGS, "ShowBold", 0 ) != 0;
		m_bSnapOn = pApp->GetProfileInt( SETTINGS, "Snapon", 1 ) != 0;
		m_bGridSnap = pApp->GetProfileInt( SETTINGS, "Gridsnap", 1 ) != 0;
		m_bAutoJoin = pApp->GetProfileIntA( SETTINGS, "AutoJoin", 1 ) != 0;
		m_bNewLinksSolid = pApp->GetProfileInt( SETTINGS, "Newlinkssolid", 0 ) != 0;
		m_bShowAnicrop = pApp->GetProfileInt( SETTINGS, "Showanicrop", 0 ) != 0;
		m_bShowLargeFont = pApp->GetProfileInt( SETTINGS, "Showlargefont", 0 ) != 0;
		m_bPrintFullSize = pApp->GetProfileInt( SETTINGS, "PrintFullSize", 0 ) != 0;
		m_bShowGrid = pApp->GetProfileInt( SETTINGS, "ShowGrid", 0 ) != 0;
		m_GridType = pApp->GetProfileInt( SETTINGS, "GridType", 0 );
		m_xUserGrid = pApp->GetProfileInt( SETTINGS, "XGrid", (int)( m_xUserGrid * PROFILE_FIXEDPOINT ) ) / PROFILE_FIXEDPOINT;
		m_yUserGrid = pApp->GetProfileInt( SETTINGS, "YGrid", (int)( m_yUserGrid * PROFILE_FIXEDPOINT ) ) / PROFILE_FIXEDPOINT;
		m_GridType = pApp->GetProfileInt( SETTINGS, "GridType", 0 );
		m_bShowParts = pApp->GetProfileInt( SETTINGS, "ShowParts", 0 ) != 0;
		m_bUseMoreMomentum = pApp->GetProfileInt( SETTINGS, "MoreMomentum", 0 ) != 0;
		m_bAllowEdit = !m_bShowParts;
		//m_bShowDimensions = pApp->GetProfileInt( SETTINGS, "Showdimensions", 0 ) != 0;
		//m_bShowGroundDimensions = pApp->GetProfileInt( SETTINGS, "Showgrounddimensions", 1 ) != 0;
		//m_bShowDrawingLayerDimensions = pApp->GetProfileInt( SETTINGS, "Showdrawinglayerdimensions", 1 ) != 0;
		GetSetViewSpecificDimensionVisbility( false );
		m_SelectedEditLayers = (unsigned int)pApp->GetProfileInt( SETTINGS, "EditLayers", 0xFFFFFFFF );
		m_SelectedViewLayers = (unsigned int)pApp->GetProfileInt( SETTINGS, "ViewLayers", 0xFFFFFFFF );

		m_Rotate0 = pApp->LoadIcon( IDI_ICON5 );
		m_Rotate1 = pApp->LoadIcon( IDI_ICON1 );
		m_Rotate2 = pApp->LoadIcon( IDI_ICON2 );
		m_Rotate3 = pApp->LoadIcon( IDI_ICON3 );
		m_Rotate4 = pApp->LoadIcon( IDI_ICON4 );
	}

	SetupFont();

	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup( &m_gdiplusToken, &gdiplusStartupInput, 0 );

	timeBeginPeriod( TimerMinimumResolution );
	m_TimerID = 0;
}

void CLinkageView::GetSetViewSpecificDimensionVisbility( bool bSave )
{
	CWinApp *pApp = AfxGetApp();
	if( pApp == 0 )
		return;

	CString ShowDimensionsName = m_bShowParts ? "PartsShowdimensions" : "Showdimensions";
	CString ShowGroundDimensionsName = m_bShowParts ? "PartsShowgrounddimensions" : "Showgrounddimensions";
	CString ShowDrawingLayerDimensionsName = m_bShowParts ? "PartsShowdrawinglayerdimensions" : "Showdrawinglayerdimensions";

	// Not sure if I want to have separate dimension settings for the parts list while not having other settings be separate - it might be confusing to users.
	// Just use the one set of settings for now - and think about this for a while.
	ShowDimensionsName = "Showdimensions";
	ShowGroundDimensionsName = "Showgrounddimensions";
	ShowDrawingLayerDimensionsName = "Showdrawinglayerdimensions";

	if( bSave )
	{
		pApp->WriteProfileInt( SETTINGS, ShowDimensionsName, m_bShowDimensions ? 1 : 0  );
		pApp->WriteProfileInt( SETTINGS, ShowGroundDimensionsName, m_bShowGroundDimensions ? 1 : 0  );
		pApp->WriteProfileInt( SETTINGS, ShowDrawingLayerDimensionsName, m_bShowDrawingLayerDimensions ? 1 : 0  );
	}
	else
	{
		m_bShowDimensions = pApp->GetProfileInt( SETTINGS, ShowDimensionsName, 0 ) != 0;
		m_bShowGroundDimensions = pApp->GetProfileInt( SETTINGS, ShowGroundDimensionsName, 1 ) != 0;
		m_bShowDrawingLayerDimensions = pApp->GetProfileInt( SETTINGS, ShowDrawingLayerDimensionsName, 0 ) != 0;
	}
}

void CLinkageView::SetupFont( void )
{
	if( m_bShowLargeFont )
	{
		m_pUsingFont = &m_MediumFont;
		m_UsingFontHeight = MEDIUM_FONT_SIZE;
	}
	else
	{
		m_pUsingFont = &m_SmallFont;
		m_UsingFontHeight = SMALL_FONT_SIZE;
	}
}

void CLinkageView::SaveSettings( void )
{
	CWinApp *pApp = AfxGetApp();
	if( pApp == 0 )
		return;

	pApp->WriteProfileInt( SETTINGS, "Showlabels", m_bShowLabels ? 1 : 0  );
	pApp->WriteProfileInt( SETTINGS, "Showangles", m_bShowAngles ? 1 : 0  );
	pApp->WriteProfileInt( SETTINGS, "Showdebug", m_bShowDebug ? 1 : 0  );
	pApp->WriteProfileInt( SETTINGS, "ShowBold", m_bShowBold ? 1 : 0  );
	pApp->WriteProfileInt( SETTINGS, "Showdata", m_bShowData ? 1 : 0  );
	pApp->WriteProfileInt( SETTINGS, "Snapon", m_bSnapOn ? 1 : 0  );
	pApp->WriteProfileInt( SETTINGS, "Gridsnap", m_bGridSnap ? 1 : 0  );
	pApp->WriteProfileInt( SETTINGS, "AutoJoin", m_bAutoJoin ? 1 : 0 );
	pApp->WriteProfileInt( SETTINGS, "Newlinkssolid", m_bNewLinksSolid ? 1 : 0  );
	pApp->WriteProfileInt( SETTINGS, "Showanicrop", m_bShowAnicrop ? 1 : 0  );
	pApp->WriteProfileInt( SETTINGS, "Showlargefont", m_bShowLargeFont ? 1 : 0  );
	pApp->WriteProfileInt( SETTINGS, "ShowGrid", m_bShowGrid ? 1 : 0  );
	pApp->WriteProfileInt( SETTINGS, "GridType", m_GridType );
	pApp->WriteProfileInt( SETTINGS, "XGrid", (int)( m_xUserGrid * PROFILE_FIXEDPOINT ) );
	pApp->WriteProfileInt( SETTINGS, "YGrid", (int)( m_yUserGrid * PROFILE_FIXEDPOINT ) );
	pApp->WriteProfileInt( SETTINGS, "MoreMomentum", m_bUseMoreMomentum ? 1 : 0 );
	pApp->WriteProfileInt( SETTINGS, "ShowParts", m_bShowParts ? 1 : 0  );
	pApp->WriteProfileInt( SETTINGS, "PrintFullSize", m_bPrintFullSize ? 1 : 0 );
	GetSetViewSpecificDimensionVisbility( true );
	pApp->WriteProfileInt( SETTINGS, "EditLayers", (int)m_SelectedEditLayers  );
	pApp->WriteProfileInt( SETTINGS, "ViewLayers", (int)m_SelectedViewLayers );
}

CLinkageView::~CLinkageView()
{
	if( m_pPopupGallery != 0 )
		delete m_pPopupGallery;
	m_pPopupGallery = 0;

	SaveSettings();

	m_bSimulating = false;
	if( m_TimerID != 0 )
		timeKillEvent( m_TimerID );
	m_TimerID = 0;

	m_bRequestAbort = true;
	SetEvent( m_hVideoFrameEvent );
	DWORD Ticks = GetTickCount();
	static const int VIDEO_KILL_TIMEOUT = 2000;
	while( m_bProcessingVideoThread && GetTickCount() < Ticks + VIDEO_KILL_TIMEOUT )
		Sleep( 100 );

	CloseHandle( m_hVideoFrameEvent );

	if( m_pAvi != 0 )
		delete m_pAvi;

   GdiplusShutdown( m_gdiplusToken );
}

BOOL CLinkageView::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_HSCROLL | WS_VSCROLL;
	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CLinkageView diagnostics

#ifdef _DEBUG
void CLinkageView::AssertValid() const
{
	CView::AssertValid();
}

void CLinkageView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CLinkageDoc* CLinkageView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CLinkageDoc)));
	return (CLinkageDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CLinkageView drawing

void CLinkageView::GetAdjustmentControlRect( AdjustmentControl Control, CRect &Rect )
{
	CFRect FRect;
	GetAdjustmentControlRect( Control, FRect );
	Rect.SetRect( (int)FRect.left, (int)FRect.top, (int)FRect.right, (int)FRect.bottom );
}

void CLinkageView::GetAdjustmentControlRect( AdjustmentControl Control, CFRect &Rect )
{
	CFRect PixelRect = Scale( m_SelectionContainerRect );
	PixelRect.InflateRect( 4, 4 );

	static const int ArrowBoxSize = 8;
	static const int HalfCenteringBoxSize = 5;
	static const int GrabBoxSize = 6;
	static const int HalfGrabBox = GrabBoxSize / 2;

	if( m_VisibleAdjustment == ADJUSTMENT_ROTATE )
	{
		switch( Control )
		{
			case AC_ROTATE:
			{
				CFPoint PixelPoint = Scale( m_SelectionRotatePoint );
				Rect.SetRect( PixelPoint.x - HalfCenteringBoxSize, PixelPoint.y - HalfCenteringBoxSize, PixelPoint.x + HalfCenteringBoxSize, PixelPoint.y + HalfCenteringBoxSize );
				break;
			}
			case AC_TOP_LEFT:
				Rect.SetRect( PixelRect.left - ArrowBoxSize, PixelRect.top - ArrowBoxSize, PixelRect.left, PixelRect.top );
				break;
			case AC_TOP_RIGHT:
				Rect.SetRect( PixelRect.right, PixelRect.top - ArrowBoxSize, PixelRect.right + ArrowBoxSize, PixelRect.top );
				break;
			case AC_BOTTOM_RIGHT:
				Rect.SetRect( PixelRect.right, PixelRect.bottom, PixelRect.right + ArrowBoxSize, PixelRect.bottom + ArrowBoxSize );
				break;
			case AC_BOTTOM_LEFT:
				Rect.SetRect( PixelRect.left - ArrowBoxSize, PixelRect.bottom, PixelRect.left, PixelRect.bottom + ArrowBoxSize );
				break;
			default: return;
		}
	}
	else if( m_VisibleAdjustment == ADJUSTMENT_STRETCH )
	{
		switch( Control )
		{
			case AC_TOP_LEFT:
				Rect.SetRect( PixelRect.left - GrabBoxSize, PixelRect.top - GrabBoxSize, PixelRect.left, PixelRect.top );
				break;
			case AC_TOP_RIGHT:
				Rect.SetRect( PixelRect.right, PixelRect.top - GrabBoxSize, PixelRect.right + GrabBoxSize, PixelRect.top );
				break;
			case AC_BOTTOM_RIGHT:
				Rect.SetRect( PixelRect.right, PixelRect.bottom, PixelRect.right + GrabBoxSize, PixelRect.bottom + GrabBoxSize );
				break;
			case AC_BOTTOM_LEFT:
				Rect.SetRect( PixelRect.left - GrabBoxSize, PixelRect.bottom, PixelRect.left, PixelRect.bottom + GrabBoxSize );
				break;
			case AC_TOP:
				Rect.SetRect( ( ( PixelRect.left + PixelRect.right ) / 2 ) - HalfGrabBox,
							  PixelRect.top - GrabBoxSize,
							  ( ( PixelRect.left + PixelRect.right ) / 2 ) + HalfGrabBox,
							  PixelRect.top );
				break;
			case AC_RIGHT:
				Rect.SetRect( PixelRect.right,
							  ( ( PixelRect.top + PixelRect.bottom ) / 2 ) - HalfGrabBox,
							  PixelRect.right + GrabBoxSize,
							  ( ( PixelRect.top + PixelRect.bottom ) / 2 ) + HalfGrabBox );
				break;
			case AC_BOTTOM:
				Rect.SetRect( ( ( PixelRect.left + PixelRect.right ) / 2 ) - HalfGrabBox,
							  PixelRect.bottom,
							  ( ( PixelRect.left + PixelRect.right ) / 2 ) + HalfGrabBox,
							  PixelRect.bottom + GrabBoxSize );
				break;
			case AC_LEFT:
				Rect.SetRect( PixelRect.left - GrabBoxSize,
							  ( ( PixelRect.top + PixelRect.bottom ) / 2 ) - HalfGrabBox,
							  PixelRect.left,
							  ( ( PixelRect.top + PixelRect.bottom ) / 2 ) + HalfGrabBox );
				break;
			default: return;
		}
	}
}

void CLinkageView::DrawRotationControl( CRenderer *pRenderer, CFRect &Rect, enum CLinkageView::_AdjustmentControl AdjustmentControl )
{
	#if defined( LINKAGE_USE_DIRECT2D )
		if( AdjustmentControl == AC_ROTATE )
		{
			double Radius = Rect.Width() - 2;
			//double Radius = 6*m_DPIScale;
			//double SmallRadius = 4*m_DPIScale;
			double SmallRadius = Radius * 2 / 3;
			CFArc Arc( Rect.GetCenter(), Radius, Rect.TopLeft(), Rect.TopLeft() );
			pRenderer->Arc( Arc );
			pRenderer->Circle( CFCircle( Arc.x, Arc.y, 1.5 ) );
			pRenderer->DrawLine( CFPoint( Arc.x-Radius-1, Arc.y ), CFPoint( Arc.x-SmallRadius, Arc.y ) );
			pRenderer->DrawLine( CFPoint( Arc.x+Radius+1, Arc.y ), CFPoint( Arc.x+SmallRadius, Arc.y ) );
			pRenderer->DrawLine( CFPoint( Arc.x, Arc.y-Radius-1 ), CFPoint( Arc.x, Arc.y-SmallRadius ) );
			pRenderer->DrawLine( CFPoint( Arc.x, Arc.y+Radius+1 ), CFPoint( Arc.x, Arc.y+SmallRadius ) );
		}
		else
		{
			double ArrowLength = 5;
			double Radius = Rect.Width() - ( ArrowLength / 2 );
			Rect.InflateRect( 1, 1 );
			CFArc Arc( Rect.GetCenter(), 6, Rect.TopLeft(), Rect.TopLeft() );

			pRenderer->SetArcDirection( AD_COUNTERCLOCKWISE );
			static const double MoveRectForArc = Scale( 2.0 );
			switch( AdjustmentControl )
			{
				case AC_TOP_LEFT:
					Rect += CFPoint( -MoveRectForArc, -MoveRectForArc );
					Arc.SetArc( Rect.BottomRight(), Radius, Rect.TopLeft(), Rect.TopLeft() );
					Arc.m_Start.SetPoint( Arc.x, Arc.y-Radius );
					Arc.m_End.SetPoint( Arc.x-Radius, Arc.y );
					pRenderer->Arc( Arc );
					pRenderer->DrawArrow( CFPoint( Arc.m_Start.x-ArrowLength, Arc.m_Start.y ), CFPoint( Arc.m_Start.x+ArrowLength, Arc.m_Start.y ), ArrowLength, ArrowLength );
					pRenderer->DrawArrow( CFPoint( Arc.m_End.x, Arc.m_End.y-ArrowLength ), CFPoint( Arc.m_End.x, Arc.m_End.y+ArrowLength ), ArrowLength, ArrowLength );
					break;
				case AC_TOP_RIGHT:
					Rect += CFPoint( MoveRectForArc, -MoveRectForArc );
					Arc.SetArc( Rect.BottomLeft(), Radius, Rect.TopLeft(), Rect.TopLeft() );
					Arc.m_Start.SetPoint( Arc.x+Radius, Arc.y );
					Arc.m_End.SetPoint( Arc.x, Arc.y-Radius );
					pRenderer->Arc( Arc );
					pRenderer->DrawArrow( CFPoint( Arc.m_Start.x, Arc.m_Start.y-ArrowLength ), CFPoint( Arc.m_Start.x, Arc.m_Start.y+ArrowLength ), ArrowLength, ArrowLength );
					pRenderer->DrawArrow( CFPoint( Arc.m_End.x+ArrowLength, Arc.m_End.y ), CFPoint( Arc.m_End.x-ArrowLength, Arc.m_End.y ), ArrowLength, ArrowLength );
					break;
				case AC_BOTTOM_LEFT:
					Rect += CFPoint( -MoveRectForArc, MoveRectForArc );
					Arc.SetArc( Rect.TopRight(), Radius, Rect.TopLeft(), Rect.TopLeft() );
					Arc.m_Start.SetPoint( Arc.x-Radius, Arc.y );
					Arc.m_End.SetPoint( Arc.x, Arc.y+Radius );
					pRenderer->Arc( Arc );
					pRenderer->DrawArrow( CFPoint( Arc.m_Start.x, Arc.m_Start.y+ArrowLength ), CFPoint( Arc.m_Start.x, Arc.m_Start.y-ArrowLength ), ArrowLength, ArrowLength );
					pRenderer->DrawArrow( CFPoint( Arc.m_End.x-ArrowLength, Arc.m_End.y ), CFPoint( Arc.m_End.x+ArrowLength, Arc.m_End.y ), ArrowLength, ArrowLength );
					break;
				case AC_BOTTOM_RIGHT:
					Rect += CFPoint( MoveRectForArc, MoveRectForArc );
					Arc.SetArc( Rect.TopLeft(), Radius, Rect.TopLeft(), Rect.TopLeft() );
					Arc.m_Start.SetPoint( Arc.x, Arc.y+Radius );
					Arc.m_End.SetPoint( Arc.x+Radius, Arc.y );
					pRenderer->DrawArrow( CFPoint( Arc.m_Start.x+ArrowLength, Arc.m_Start.y ), CFPoint( Arc.m_Start.x-ArrowLength, Arc.m_Start.y ), ArrowLength, ArrowLength );
					pRenderer->DrawArrow( CFPoint( Arc.m_End.x, Arc.m_End.y+ArrowLength ), CFPoint( Arc.m_End.x, Arc.m_End.y-ArrowLength ), ArrowLength, ArrowLength );
					pRenderer->Arc( Arc );
					break;
			}
		}
	#else
		CDC *pDC = pRenderer->GetDC();
		switch( AdjustmentControl )
		{
			case AC_ROTATE:
				::DrawIconEx( pDC->GetSafeHdc(), (int)Rect.left, (int)Rect.top, m_Rotate0, 32, 32, 0, 0, DI_NORMAL );
				break;
			case AC_TOP_LEFT:
				::DrawIconEx(pDC->GetSafeHdc(), (int)Rect.left, (int)Rect.top, m_Rotate1, 32, 32, 0, 0, DI_NORMAL);
				break;
			case AC_TOP_RIGHT:
				::DrawIconEx(pDC->GetSafeHdc(), (int)Rect.left, (int)Rect.top, m_Rotate2, 32, 32, 0, 0, DI_NORMAL);
				break;
			case AC_BOTTOM_LEFT:
				::DrawIconEx(pDC->GetSafeHdc(), (int)Rect.left, (int)Rect.top, m_Rotate4, 32, 32, 0, 0, DI_NORMAL);
				break;
			case AC_BOTTOM_RIGHT:
				::DrawIconEx(pDC->GetSafeHdc(), (int)Rect.left, (int)Rect.top, m_Rotate3, 32, 32, 0, 0, DI_NORMAL);
				break;
		}
	#endif
}

void CLinkageView::DrawAdjustmentControls( CRenderer *pRenderer )
{
	if( !m_bAllowEdit )
		return;

	CPen Pen( PS_SOLID, 2, COLOR_BLACK );
	CPen *pOldPen = pRenderer->SelectObject( &Pen );
	CBrush Brush( m_bAllowEdit ? COLOR_ADJUSTMENTKNOBS : COLOR_NONADJUSTMENTKNOBS );
	CBrush *pOldBrush = pRenderer->SelectObject( &Brush );

	if( m_VisibleAdjustment == ADJUSTMENT_ROTATE )
	{
		if( m_MouseAction != ACTION_NONE && m_MouseAction != ACTION_RECENTER /*&& m_MouseAction != ACTION_ROTATE*/ )
		{
			pRenderer->SelectObject( pOldPen );
			pRenderer->SelectObject( pOldBrush );
			return;
		}

		CFRect Rect;
		GetAdjustmentControlRect( AC_ROTATE, Rect );
		DrawRotationControl( pRenderer, Rect, AC_ROTATE );
		if( m_MouseAction != ACTION_NONE )
		{
			pRenderer->SelectObject( pOldPen );
			pRenderer->SelectObject( pOldBrush );
			return;
		}

		GetAdjustmentControlRect( AC_TOP_LEFT, Rect );
		DrawRotationControl( pRenderer, Rect, AC_TOP_LEFT );
		GetAdjustmentControlRect( AC_TOP_RIGHT, Rect );
		DrawRotationControl( pRenderer, Rect, AC_TOP_RIGHT );
		GetAdjustmentControlRect( AC_BOTTOM_RIGHT, Rect );
		DrawRotationControl( pRenderer, Rect, AC_BOTTOM_RIGHT );
		GetAdjustmentControlRect( AC_BOTTOM_LEFT, Rect );
		DrawRotationControl( pRenderer, Rect, AC_BOTTOM_LEFT );
	}
	else if( m_VisibleAdjustment == ADJUSTMENT_STRETCH )
	{
		if( m_MouseAction != ACTION_NONE )
		{
			pRenderer->SelectObject( pOldPen );
			pRenderer->SelectObject( pOldBrush );
			return;
		}

		bool bVertical = fabs( m_SelectionContainerRect.Height() ) > 0;
		bool bHorizontal = fabs( m_SelectionContainerRect.Width() ) > 0;
		bool bBoth = bHorizontal && bVertical;

		CFRect Rect;

		GetAdjustmentControlRect( AC_TOP_LEFT, Rect );
		Rect.right++; Rect.bottom++;
		if( bBoth ) pRenderer->FillRect( &Rect, &Brush );
		GetAdjustmentControlRect( AC_TOP_RIGHT, Rect );
		Rect.right++; Rect.bottom++;
		if( bBoth ) pRenderer->FillRect( &Rect, &Brush );
		GetAdjustmentControlRect( AC_BOTTOM_RIGHT, Rect );
		Rect.right++; Rect.bottom++;
		if( bBoth ) pRenderer->FillRect( &Rect, &Brush );
		GetAdjustmentControlRect( AC_BOTTOM_LEFT, Rect );
		Rect.right++; Rect.bottom++;
		if( bBoth ) pRenderer->FillRect( &Rect, &Brush );
		GetAdjustmentControlRect( AC_TOP, Rect );
		Rect.right++; Rect.bottom++;
		if( bVertical ) pRenderer->FillRect( &Rect, &Brush );
		GetAdjustmentControlRect( AC_RIGHT, Rect );
		Rect.right++; Rect.bottom++;
		if( bHorizontal ) pRenderer->FillRect( &Rect, &Brush );
		GetAdjustmentControlRect( AC_BOTTOM, Rect );
		Rect.right++; Rect.bottom++;
		if( bVertical ) pRenderer->FillRect( &Rect, &Brush );
		GetAdjustmentControlRect( AC_LEFT, Rect );
		Rect.right++; Rect.bottom++;
		if( bHorizontal ) pRenderer->FillRect( &Rect, &Brush );
	}
	pRenderer->SelectObject( pOldBrush );
	pRenderer->SelectObject( pOldPen );
}

int CLinkageView::GetPrintPageCount( CDC *pDC, bool bPrintActualSize, CFRect DocumentArea, int &UseOrientation, int &WidthInPages )
{
	if( !bPrintActualSize )
		return 1;

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	static const double MyLogicalPixels = CLinkageDoc::GetBaseUnitsPerInch();

	int PixelsPerInch = pDC->GetDeviceCaps( LOGPIXELSX );
	double ScaleFactor = MyLogicalPixels / PixelsPerInch;

	CFRect Area = GetDocumentArea( m_bShowDimensions );
	Area.Normalize();
	CFRect PixelRect = Area;

	int cx = pDC->GetDeviceCaps(HORZRES);
	int cy = pDC->GetDeviceCaps(VERTRES);

	if( cx > cy )
		swap( cx, cy );
	// The size info is now for portrait mode

	//CRect DrawingRect( 0, 0, (int)( cx * ScaleFactor ), (int)( cy * ScaleFactor ) );
	int ySpace = pDC->GetTextExtent( "N", 1 ).cy;
	//DrawingRect.bottom -= cy;

	PixelRect.bottom -= ySpace * ScaleFactor;

	int WidthInPagesP = (int)( ceil( PixelRect.Width() / ( ScaleFactor * cx ) ) );
	int HeightInPagesP = (int)( ceil( PixelRect.Height() / ( ScaleFactor * cy ) ) );
	int WidthInPagesL = (int)( ceil( PixelRect.Width() / ( ScaleFactor * cy ) ) );
	int HeightInPagesL = (int)( ceil( PixelRect.Height() / ( ScaleFactor * cx ) ) );

	if( WidthInPagesL + HeightInPagesL < WidthInPagesP + HeightInPagesP )
	{
		UseOrientation = DMORIENT_LANDSCAPE;
		WidthInPages = WidthInPagesL;
		return WidthInPagesL * HeightInPagesL;
	}
	else
	{
		UseOrientation = DMORIENT_PORTRAIT;
		WidthInPages = WidthInPagesP;
		return WidthInPagesP * HeightInPagesP;
	}
}

CRect CLinkageView::PrepareRenderer( CRenderer &Renderer, CRect *pDrawRect, CBitmap *pBitmap, CDC *pDC, double ScalingValue, bool bScaleToFit, double MarginScale, double UnscaledUnitSize, bool bForScreen, bool bAntiAlias, bool bActualSize, int PageNumber )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	bool bEmpty = pDoc->IsEmpty();
	pDoc = 0; // Don't use this anymore after this.

	m_YUpDirection = ( Renderer.GetYOrientation() < 0 ) ? -1 : 1;
	m_BaseUnscaledUnit = UnscaledUnitSize;
	m_ConnectorRadius = CONNECTORRADIUS * UnscaledUnitSize;
	m_ConnectorTriangle = CONNECTORTRIANGLE * UnscaledUnitSize;
	m_ArrowBase = ANCHORBASE * UnscaledUnitSize;

	if( pDrawRect != 0 )
		m_DrawingRect = *pDrawRect;

	m_bShowSelection = bForScreen && !m_bSimulating && m_bAllowEdit;

	// Reset zoom, scroll, and DIP adjustment values before calcuating them.
	// Some calculations leave them as-is.
	m_Zoom = 1.0;
	m_ScrollPosition.SetPoint( 0, 0 );
	m_DPIScale = 1.0;

    if( Renderer.IsPrinting() )
    {
		ASSERT( pDC != 0 );

		/*
		* The 8.0 scale allows the drawing functions to draw to a much larger area and the window/viewport
		* scaling combined with the device context scaling makes it the correct size for the page. This has
		* the advantage of allowing a higher resolution to be drawn to than the document 96 DPI while still
		* making fonts, lines, arrowheads, all look correct on the page. This is forced here for printing.
		*/
		ScalingValue = 8.0;

		m_bShowSelection = false;

		m_Zoom = 1;
		m_ScrollPosition.SetPoint( 0, 0 );
		Renderer.Attach( pDC->GetSafeHdc() );
		Renderer.SetAttribDC( pDC->m_hAttribDC );

		int cx = pDC->GetDeviceCaps(HORZRES);
		int cy = pDC->GetDeviceCaps(VERTRES);

		/*
		 * There is a bug in the CRenderer code that make pen handling fail
		 * at unexpected times. The CRenderer code would try to adjust the
		 * pen width for a specified pen to account for the scaling but
		 * the arrow head drawing code makes this fail intermittently.
		 * Because of all of that, the only way to get proper printing
		 * is to use the window and viewport feature and not use the scaling
		 * feature of CRenderer.
		 */

		static const double MyLogicalPixels = CLinkageDoc::GetBaseUnitsPerInch();

		int PixelsPerInch = pDC->GetDeviceCaps( LOGPIXELSX );
		double ScaleFactor = MyLogicalPixels / PixelsPerInch;

		// if( pDrawRect == 0 )
		m_DrawingRect.SetRect( 0, 0, (int)( cx * ScaleFactor ), (int)( cy * ScaleFactor ) );

		Renderer.SetOffset( 0, 0 );
		Renderer.SetScale( ScalingValue );
		Renderer.SetSize( cx, cy );
		Renderer.BeginDraw();
		Renderer.Clear();

		// Room for page number.
		int ySpace = (int)Renderer.GetTextExtent( "N", 1 ).y;
		m_DrawingRect.bottom -= ySpace;

		// need to get the difference between dimensions and no dimensions
		// for doing the shrink-to-fit scaling, just like the fit function 
		// for the screen.

		CFRect Area = GetDocumentArea( m_bShowDimensions );
		Area.Normalize();
		CFRect PixelRect = Area;

		pDC->SetMapMode( MM_ISOTROPIC );
		pDC->SetWindowExt( (int)( m_DrawingRect.Width() * ScalingValue ), (int)( m_DrawingRect.Height() * ScalingValue ) );
		pDC->SetViewportExt( cx, cy );

		pDC->IntersectClipRect( CRect( 0, 0, (int)( m_DrawingRect.Width() * ScalingValue ), (int)( m_DrawingRect.Height() * ScalingValue ) ) );

		/*
		 * Shrink the drawing to fit on the page if it is too big.
		 */
		//double ExtraScaling = 1.0;
		// Get a usable area that has an extra 1/2 margin around it.
		//double xUsable = m_DrawingRect.Width();// - MyLogicalPixels;
		//double yUsable = m_DrawingRect.Height();// - MyLogicalPixels;
		//if( !bActualSize && ( PixelRect.Width() > xUsable || PixelRect.Height() > yUsable ) )
		//	ExtraScaling = min( yUsable / PixelRect.Height(), xUsable / PixelRect.Width() );

		//if( ExtraScaling < 1.0 )
		//	m_Zoom = ExtraScaling;
		//PixelRect = Scale( PixelRect );

		if( bActualSize )
		{
			int HorizontalPage = PageNumber % m_PrintWidthInPages;
			int VerticalPage = PageNumber / m_PrintWidthInPages;

			m_ScrollPosition.x = PixelRect.left + ( m_DrawingRect.Width() * HorizontalPage );
			m_ScrollPosition.y = PixelRect.top + ( m_DrawingRect.Height() * VerticalPage );
		}
		else
		{
			ZoomToFit( m_DrawingRect, false, 0, 1.0, false );

			//m_ScrollPosition.x = (int)( PixelRect.left + ( PixelRect.Width() / 2 ) - ( m_DrawingRect.Width() / 2 ) );
			//m_ScrollPosition.y = (int)( PixelRect.top + ( PixelRect.Height() / 2 ) - ( m_DrawingRect.Height() / 2 ) );

			//m_ScrollPosition.x = PixelRect.left;
			//m_ScrollPosition.y = PixelRect.top;
		}
	}
	else
	{
		if( bForScreen )
		{
			m_Zoom = m_ScreenZoom;
			m_ScrollPosition = m_ScreenScrollPosition;
			m_DPIScale = m_ScreenDPIScale;
		}

		if( pDrawRect == 0 )
			GetClientRect( &m_DrawingRect );

		if( pBitmap != 0 && pDC != 0 )
		{
			Renderer.CreateCompatibleDC( pDC, &m_DrawingRect );

			int NeedBitmapWidth = (int)( m_DrawingRect.Width() * ScalingValue );
			int NeedBitmapHeight = (int)( m_DrawingRect.Height() * ScalingValue );
			if( m_pCachedRenderBitmap == 0 || m_CachedBitmapWidth != NeedBitmapWidth || m_CachedBitmapHeight != NeedBitmapHeight )
			{
				if( m_pCachedRenderBitmap != 0 )
					delete m_pCachedRenderBitmap;

				m_pCachedRenderBitmap = new CBitmap;
				m_pCachedRenderBitmap->CreateCompatibleBitmap( pDC, (int)( m_DrawingRect.Width() * ScalingValue ), (int)( m_DrawingRect.Height() * ScalingValue ) );
				m_CachedBitmapWidth = NeedBitmapWidth;
				m_CachedBitmapHeight = NeedBitmapHeight;
			}

			//pBitmap->CreateCompatibleBitmap( pDC, (int)( m_DrawingRect.Width() * ScalingValue ), (int)( m_DrawingRect.Height() * ScalingValue ) );
			Renderer.SelectObject( m_pCachedRenderBitmap );
			//Renderer.PatBlt( 0, 0, (int)( m_DrawingRect.Width() * ScalingValue ), (int)( m_DrawingRect.Height() * ScalingValue ), WHITENESS );
		}

		Renderer.SetOffset( 0, 0 );
		Renderer.SetScale( ScalingValue );
		Renderer.SetSize( m_DrawingRect.Width(), m_DrawingRect.Height() );
		Renderer.BeginDraw();
		Renderer.Clear();

		if( bScaleToFit )
		{
			CFRect Temp = m_DrawingRect;
			Temp *= 1.0 / m_DPIScale;
			ZoomToFit( Temp, true, MarginScale, 1.0, bForScreen );
		}
	}

	return m_DrawingRect;
}

class CConnectorTextData : public ConnectorListOperation
{
	public:
	CString &m_Text;
	bool m_bShowDebug;
	CConnectorTextData( CString &Text, bool bShowDebug ) : m_Text( Text ), m_bShowDebug( bShowDebug )
	{
	}

	bool operator()( class CConnector *pConnector, bool bFirst, bool bLast )
	{
		CString TempString;
		if( m_bShowDebug )
			TempString.Format( "      Connector %s [%d]: %.3lf, %.3lf\n",
							   (const char*)pConnector->GetIdentifierString( m_bShowDebug ), pConnector->GetIdentifier(),
							   pConnector->GetPoint().x, pConnector->GetPoint().y );
		else
			TempString.Format( "      Connector %s: %.3lf, %.3lf\n",
							   (const char*)pConnector->GetIdentifierString( m_bShowDebug ),
							   pConnector->GetPoint().x, pConnector->GetPoint().y );
		m_Text += TempString;
		return true;
	}
};

class CConnectorDistanceData : public ConnectorListOperation
{
	public:
	CString &m_Text;
	CConnector *m_pLastConnector;
	CConnectorDistanceData( CString &Text, CConnector *pLastConnector ) : m_Text( Text ) { m_pLastConnector = pLastConnector; }
	bool operator()( class CConnector *pConnector, bool bFirst, bool bLast )
	{
		if( m_pLastConnector != 0 )
		{
			double Dist = Distance( m_pLastConnector->GetPoint(), pConnector->GetPoint() );
			CString TempString;
			TempString.Format( "      Distance %s%s %.3lf\n",
							   (const char*)m_pLastConnector->GetIdentifierString( false ), (const char*)pConnector->GetIdentifierString( false ), Dist );
			m_Text += TempString;
		}
		m_pLastConnector = pConnector;
		return true;
	}
};

void CLinkageView::DrawData( CRenderer *pRenderer )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	/*
	 * This data information is never scaled or offset.
	 */
	CFont *pOriginalFont = pRenderer->SelectObject( m_pUsingFont, UnscaledUnits( m_UsingFontHeight ) );

	pRenderer->SetTextAlign( TA_TOP | TA_LEFT );

	static const int x = 10;
	static const int x2 = 30;
	double y = 10;
	double dy = pRenderer->GetTextExtent( "X", 1 ).y - 1;

	CString Text;

	LinkList* pLinkList = pDoc->GetLinkList();
	POSITION Position = pLinkList->GetHeadPosition();
	while( Position != NULL )
	{
		CLink* pLink = pLinkList->GetNext( Position );
		if( pLink == NULL )
			continue;

		CString TempString;
		if( m_bShowDebug )
			TempString.Format( "Link %s [%d]\n", (const char*)pLink->GetIdentifierString( m_bShowDebug ), pLink->GetIdentifier() );
		else
			TempString.Format( "Link %s\n", (const char*)pLink->GetIdentifierString( m_bShowDebug ) );
		Text += TempString;

		CConnectorTextData FormatText( Text, m_bShowDebug );
		pLink->GetConnectorList()->Iterate( FormatText );

		CConnectorDistanceData FormatDistance( Text, pLink->GetConnectorList()->GetCount() <= 2 ? 0 : pLink->GetConnectorList()->GetTail() );
		pLink->GetConnectorList()->Iterate( FormatDistance );
	}

	char *pStart = Text.GetBuffer();
	for(;;)
	{
		char *pEnd = strrchr( pStart, '\n' );
		if( pEnd != 0 )
			*pEnd = '\0';
		pRenderer->TextOut( x, y, pStart );
		y += dy;
		if( pEnd == 0 )
			break;
		pStart = pEnd + 1;
	}
	Text.ReleaseBuffer( 0 );

	y += dy;

	/*
	 * Objects selected before we changed things need to be selected back
	 * in the device context before we reset the scale to non-zero.
	 * If we did this after the scale is set, the object would get scaled
	 * more than once.
	 */
	pRenderer->SelectObject( pOriginalFont, UnscaledUnits( m_UsingFontHeight ) );
}

void CLinkageView::DrawAlignmentHintLines( CRenderer *pRenderer )
{
	if( m_MouseAction != ACTION_NONE )
		return;

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if( pDoc->GetAlignConnectorCount() < 2 )
		return;

	int PointCount = pDoc->GetAlignConnectorCount();

	COLORREF Color = COLOR_ALIGNMENTHINT;
	CPen DotPen( PS_DOT, 1, Color );
	CPen * pOldPen = pRenderer->SelectObject( &DotPen );

	CFPoint Points[4];

	CConnector *pConnector = (CConnector*)pDoc->GetSelectedConnector( 0 );
	if( pConnector == 0 )
		return;
	Points[0] = pConnector->GetPoint();
	pRenderer->MoveTo( Scale( Points[0] ) );

	// Draw only a single line if there are too many points for a rectangle/parallelogram.
	if( PointCount > 4 )
	{
		for( int Counter = 1; Counter < PointCount; ++Counter )
		{
			pConnector = (CConnector*)pDoc->GetSelectedConnector( Counter );
			if( pConnector == 0 )
				return;
			Points[1] = pConnector->GetPoint();
		}

		pRenderer->LineTo( Scale( Points[1] ) );

		return;
	}

	for( int Counter = 1; Counter < PointCount; ++Counter )
	{
		pConnector = (CConnector*)pDoc->GetSelectedConnector( Counter );
		if( pConnector == 0 )
			return;
		Points[Counter] = pConnector->GetPoint();
		pRenderer->LineTo( Scale( Points[Counter] ) );
	}

	// Finish drawing the polygon when it is a rectangle.
	// Also draw the rotating hint arrow.
	if( PointCount == 4 )
	{
		pRenderer->LineTo( Scale( Points[0] ) );

		CFCircle Circle1( Points[0], Distance( Points[0], Points[1] ) );
		CFCircle Circle2( Points[3], Distance( Points[3], Points[2] ) );

		CFPoint Intersect;
		CFPoint Intersect2;

		if( Circle1.CircleIntersection( Circle2, &Intersect, &Intersect2 ) )
		{
			CFPoint SuggestedPoint = ( Points[1] + Points[2] ) / 2;

			double d1 = Distance( SuggestedPoint, Intersect );
			double d2 = Distance( SuggestedPoint, Intersect2 );

			if( d2 < d1 )
				Intersect = Intersect2;

			CFArc HintArc( Intersect, Unscale( m_ConnectorRadius ), Intersect, Intersect );

			pRenderer->Arc( Scale( HintArc ) );

			CFLine ArrowLine1( Points[1], Intersect );
			CFLine ArrowLine2( Points[2], Intersect );
			ArrowLine1.SetLength( ArrowLine1.GetLength() - Unscale( m_ConnectorRadius ) );
			ArrowLine2.SetLength( ArrowLine2.GetLength() - Unscale( m_ConnectorRadius ) );
			ArrowLine1 = Scale( ArrowLine1 );
			ArrowLine2 = Scale( ArrowLine2 );
			pRenderer->DrawArrow( ArrowLine1.GetStart(), ArrowLine1.GetEnd(), UnscaledUnits( 5 ), UnscaledUnits( 9 ) );
			pRenderer->DrawArrow( ArrowLine2.GetStart(), ArrowLine2.GetEnd(), UnscaledUnits( 5 ), UnscaledUnits( 9 ) );

			//CFLine ArrowLine1( Points[1], Points[2] );
			//CFLine ArrowLine2( Points[2], Points[1] );
			//ArrowLine1.SetLength( ArrowLine1.GetLength() / 2 );
			//ArrowLine2.SetLength( ArrowLine2.GetLength() / 2 );

			//ArrowLine1 = Scale( ArrowLine1 );
			//ArrowLine2 = Scale( ArrowLine2 );

			//pRenderer->DrawArrow( ArrowLine1.GetStart(), ArrowLine1.GetEnd(), UnscaledUnits( 5 ), UnscaledUnits( 9 ) );
			//pRenderer->DrawArrow( ArrowLine2.GetStart(), ArrowLine2.GetEnd(), UnscaledUnits( 5 ), UnscaledUnits( 9 ) );

		}
	}


	if( PointCount == 3 ) // && fabs( Angle ) >= 0.5 && fabs( Angle ) < 359.5 )
	{
		double Angle = GetAngle( Points[1], Points[0], Points[2] );
		if( Angle < 0 )
			Angle += 360;

		CFPoint TempPoint;
		TempPoint = Scale( Points[0] );
		Points[0].SetPoint( CPoint( (int)TempPoint.x, (int)TempPoint.y ) );
		TempPoint = Scale( Points[1] );
		Points[1].SetPoint( CPoint( (int)TempPoint.x, (int)TempPoint.y ) );
		TempPoint = Scale( Points[2] );
		Points[2].SetPoint( CPoint( (int)TempPoint.x, (int)TempPoint.y ) );

		double ArcRadius = 20;
		CFLine Line( Points[1], Points[0] );
		Line.SetLength( ArcRadius );
		Points[0] = Line[1];

		Line.SetLine( Points[1], Points[2] );
		Line.SetLength( ArcRadius );
		Points[2] = Line[1];

		CPen Pen( PS_SOLID, 1, Color );

		pRenderer->SelectObject( &Pen );

		pRenderer->Arc( Points[1].x, Points[1].y, ArcRadius, Points[0].x, Points[0].y, Points[2].x, Points[2].y );
	}

	pRenderer->SelectObject( pOldPen );
}

void CLinkageView::DrawSnapLines( CRenderer *pRenderer )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CFLine SnapLine1;
	CFLine SnapLine2;
	pDoc->GetSnapLines( SnapLine1, SnapLine2 );

	if( SnapLine1.GetStart() == SnapLine1.GetEnd()
		&& SnapLine2.GetStart() == SnapLine2.GetEnd() )
		return;

	COLORREF Color = COLOR_SNAPHINT;

	CPen DotPen( PS_DOT, 1, Color );

	CPen * pOldPen = pRenderer->SelectObject( &DotPen );

	CRect ClientRect;
	GetClientRect( &ClientRect );
	//CFRect Viewport = pRenderer->ScaleRect( ClientRect );
	CFRect Viewport = Unscale( ClientRect );

	if( SnapLine1.m_Start.x == -99999 )
		SnapLine1.m_Start.x = Viewport.left;
	if( SnapLine1.m_Start.y == -99999 )
		SnapLine1.m_Start.y = Viewport.top;
	if( SnapLine1.m_End.x == -99999 )
		SnapLine1.m_End.x = Viewport.left;
	if( SnapLine1.m_End.y == -99999 )
		SnapLine1.m_End.y = Viewport.top;

	if( SnapLine1.GetStart() != SnapLine1.GetEnd() )
	{
		pRenderer->DrawLine( Scale( SnapLine1 ) );
		//pRenderer->MoveTo( Scale( SnapLine1.GetStart() ) );
		//pRenderer->LineTo( Scale( SnapLine1.GetEnd() ) );
		CFArc HintArc( SnapLine1.GetStart(), Unscale( m_ConnectorRadius ), SnapLine1.GetStart(), SnapLine1.GetStart() );
		pRenderer->Arc( Scale( HintArc ) );
	}
	if( SnapLine2.GetStart() != SnapLine2.GetEnd() )
	{
		pRenderer->DrawLine( Scale( SnapLine2 ) );



		//pRenderer->MoveTo( Scale( SnapLine2.GetStart() ) );
		//pRenderer->LineTo( Scale( SnapLine2.GetEnd() ) );
		CFArc HintArc( SnapLine2.GetStart(), Unscale( m_ConnectorRadius ), SnapLine2.GetStart(), SnapLine2.GetStart() );
		pRenderer->Arc( Scale( HintArc ) );
	}

	pRenderer->SelectObject( pOldPen );
}

CFRect CLinkageView::GetDocumentAdjustArea( bool bSelectedOnly )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CFRect SelectRect;
	pDoc->GetDocumentAdjustArea( SelectRect, bSelectedOnly );

	// Dimensions are not included because they are not part of ahything that can be selected.
	return SelectRect;
}

CFRect CLinkageView::GetDocumentArea( bool bWithDimensions, bool bSelectedOnly )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CFRect Rect;
	pDoc->GetDocumentArea( Rect, bSelectedOnly );

	if( !bSelectedOnly && !m_bShowParts && m_pBackgroundBitmap != 0 )
	{
		CFRect BackgroundRect( -(m_pBackgroundBitmap->width/2), -(m_pBackgroundBitmap->height/2), -(m_pBackgroundBitmap->width/2) + m_pBackgroundBitmap->width, -(m_pBackgroundBitmap->height/2) + m_pBackgroundBitmap->height );
		//BackgroundRect = Scale( BackgroundRect );
		// rect is already in document coordinates and can be used as-is.
		CFArea AreaRect = Rect;
		AreaRect += BackgroundRect;
		Rect = AreaRect;
	}

//	if( !bWithDimensions )
//		return Rect;

	CFArea DocumentArea = Rect;

	DocumentArea.Normalize();

	CRenderer NullRenderer( CRenderer::NULL_RENDERER );
	if( m_bShowParts )
	{
		bool bTemp = m_bShowDimensions;
//		m_bShowDimensions = bWithDimensions;
		DocumentArea = DrawPartsList( &NullRenderer );
//		m_bShowDimensions = bTemp;
	}
	else
	{
		//double SolidLinkCornerRadius = Unscale( CONNECTORRADIUS + 3 );
		//DocumentArea.InflateRect( SolidLinkCornerRadius, SolidLinkCornerRadius );

		if( !bSelectedOnly && bWithDimensions )
		{
			LinkList* pLinkList = pDoc->GetLinkList();
			POSITION Position = pLinkList->GetHeadPosition();
			while( Position != NULL )
			{
				CLink* pLink = pLinkList->GetNext( Position );
				if( pLink != 0 && !pLink->IsMeasurementElement() )
				{
					pLink->ComputeHull();
					DocumentArea += DrawDimensions( &NullRenderer, pDoc->GetGearConnections(), m_SelectedViewLayers, pLink, true, true );
				}
			}

			ConnectorList* pConnectors = pDoc->GetConnectorList();
			Position = pConnectors->GetHeadPosition();
			while( Position != NULL )
			{
				CConnector* pConnector = pConnectors->GetNext( Position );
				if( pConnector != 0 )
					DocumentArea += DrawDimensions( &NullRenderer, m_SelectedViewLayers, pConnector, true, true );
			}

			DocumentArea += DrawGroundDimensions( &NullRenderer, pDoc, m_SelectedViewLayers, 0, true, true );
		}
	}
	//DocumentArea.InflateRect( CONNECTORRADIUS, CONNECTORRADIUS );
	return DocumentArea.GetRect();
}

CFArea CLinkageView::DoDraw( CRenderer* pRenderer )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CFArea Area;

	if( m_bShowDebug && false )
	{
		// As drawn
		CFRect Derfus = GetDocumentArea( true ) ;
		CPen aPen( PS_SOLID, 1, RGB( 90, 255, 90 ) );
		CPen *pOldPen = pRenderer->SelectObject( &aPen );
		pRenderer->DrawRect( Scale( Derfus ) );
		pRenderer->SelectObject( pOldPen );

		// Original document
		pDoc->GetDocumentArea( Derfus );
		CPen xxxPen( PS_SOLID, 1, RGB( 0, 180, 0 ) );
		pOldPen = pRenderer->SelectObject( &xxxPen );
		pRenderer->DrawRect( Scale( Derfus ) );
		pRenderer->SelectObject( pOldPen );
	}

	if( m_bShowParts )
		Area = DrawPartsList( pRenderer );
	else
		Area = DrawMechanism( pRenderer );

	return Area;
}

double roundUp( double number, double fixedBase )
{
    if (fixedBase != 0 && number != 0)
	{
        double sign = number > 0 ? 1 : -1;
        number *= sign;
        number /= fixedBase;
        int fixedPoint = (int) ceil(number);
        number = fixedPoint * fixedBase;
        number *= sign;
    }
    return number;
}

double roundDown( double number, double fixedBase )
{
    if (fixedBase != 0 && number != 0)
	{
        double sign = number > 0 ? 1 : -1;
        number *= sign;
        number /= fixedBase;
        int fixedPoint = (int) floor(number);
        number = fixedPoint * fixedBase;
        number *= sign;
    }
    return number;
}

int roundToNext(int numToRound, int multiple) 
{
	ASSERT(multiple);
	return ((numToRound + multiple - 1) / multiple) * multiple;
}

void CLinkageView::DrawGrid( CRenderer* pRenderer, int Type )
{
	if( pRenderer->IsPrinting() )
		return;

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	double xGapDistance = 0;
	double yGapDistance = 0;
	if( Type == 0 )
	{
		xGapDistance = CalculateDefaultGrid();
		yGapDistance = xGapDistance;
	}
	else
	{
		xGapDistance = m_xUserGrid * pDoc->GetUnitScale();
		yGapDistance = m_yUserGrid * pDoc->GetUnitScale();
	}

	if( xGapDistance <= 0 || yGapDistance <= 0 )
		return;

	xGapDistance /= pDoc->GetUnitScale();
	yGapDistance /= pDoc->GetUnitScale();

	double Test = Scale( min( xGapDistance, yGapDistance ) );
	if( Test < VIRTUAL_PIXEL_SNAP_DISTANCE * 1.5 )
	{
		//SetStatusText( "Grid is too small to display" );
		return;
	}

	CFPoint DrawStartPoint = Unscale( CFPoint( m_DrawingRect.left, m_DrawingRect.top ) );
	CFPoint DrawEndPoint = Unscale( CFPoint( m_DrawingRect.right, m_DrawingRect.bottom ) );
	CFPoint GridStartPoint = DrawStartPoint;

	GridStartPoint.x = roundDown( GridStartPoint.x, xGapDistance );
	GridStartPoint.y = roundDown( GridStartPoint.y, yGapDistance );

	COLORREF Color = COLOR_GRID;
	CPen Pen( PS_SOLID, 1, Color );
	CPen *pOldPen = pRenderer->SelectObject( &Pen );

	for( double Step = GridStartPoint.x; Step < DrawEndPoint.x; Step += xGapDistance )
	{
		pRenderer->MoveTo( Scale( CFPoint( Step, DrawStartPoint.y ) ) );
		pRenderer->LineTo( Scale( CFPoint( Step, DrawEndPoint.y ) ) );
	}

	// Y always goes from high values up to low values down.
	for( double Step = GridStartPoint.y; Step > DrawEndPoint.y; Step -= yGapDistance )
	{
		pRenderer->MoveTo( Scale( CFPoint( DrawStartPoint.x, Step ) ) );
		pRenderer->LineTo( Scale( CFPoint( DrawEndPoint.x, Step ) ) );
	}

	pRenderer->SelectObject( pOldPen );
}

void CLinkageView::ClearDebugItems( void )
{
	while( true )
	{
		CDebugItem *pDebugItem = DebugItemList.Pop();
		if( pDebugItem == 0 )
			break;
	}
}

void CLinkageView::DrawDebugItems( CRenderer *pRenderer )
{
	pRenderer->SetTextAlign( TA_LEFT | TA_TOP );
	pRenderer->SetTextColor( COLOR_TEXT );
	CString Label;
	CFPoint LabelPoint;
	int Count = 0;
	POSITION Position = DebugItemList.GetHeadPosition();
	CPen *pOldPen = 0;
	double LabelYAdjust = 0;
	while( Position != 0 )
	{
		CDebugItem *pDebugItem = DebugItemList.GetNext( Position );
		if( pDebugItem == 0 )
			break;

		CPen Pen( PS_SOLID, 1, pDebugItem->m_Color );
		CPen *pOldPen = pRenderer->SelectObject( &Pen );

		Label.Format( "DEBUG %d", Count );

		if( pDebugItem->m_Type == pDebugItem->DEBUG_OBJECT_POINT )
		{
			CFPoint Point = pDebugItem->m_Point;
			LabelPoint.SetPoint( Point.x + ( m_ConnectorRadius * 2 ), Point.y - ( m_ConnectorRadius * 2 ) );
			Point = Scale( Point );
			pRenderer->Arc( Point.x - m_ConnectorRadius * 2, Point.y - m_ConnectorRadius * 2, Point.x + m_ConnectorRadius * 2, Point.y + m_ConnectorRadius * 2,
				            Point.x, Point.y - m_ConnectorRadius * 2, Point.x, Point.y - m_ConnectorRadius * 2 );
			CFLine Cross1( Point.x - m_ConnectorRadius * 2, Point.y - m_ConnectorRadius * 2, Point.x + m_ConnectorRadius * 2, Point.y + m_ConnectorRadius * 2 );
			CFLine Cross2( Point.x - m_ConnectorRadius * 2, Point.y + m_ConnectorRadius * 2, Point.x + m_ConnectorRadius * 2, Point.y - m_ConnectorRadius * 2 );
			pRenderer->DrawLine( Cross1 );
			pRenderer->DrawLine( Cross2 );

		}
		else if( pDebugItem->m_Type == pDebugItem->DEBUG_OBJECT_LINE )
		{
			CFLine Line = pDebugItem->m_Line;
			pRenderer->DrawLine( Scale( Line ) );
			LabelPoint.SetPoint( Line.m_Start.x + ( m_ConnectorRadius * 2 ), Line.m_Start.y - ( m_ConnectorRadius * 2 ) );
		}
		else if( pDebugItem->m_Type == pDebugItem->DEBUG_OBJECT_CIRCLE )
		{
			CFCircle Circle = pDebugItem->m_Circle;
			LabelPoint.SetPoint( Circle.x + ( m_ConnectorRadius * 2 ), Circle.y - ( m_ConnectorRadius * 2 ) );
			Circle = Scale( Circle );
			CBrush *pSaveBrush = (CBrush*)pRenderer->SelectStockObject( NULL_BRUSH );
			pRenderer->Circle( Circle );
			pRenderer->SelectObject( pSaveBrush );
		}
		else if( pDebugItem->m_Type == pDebugItem->DEBUG_OBJECT_ARC )
		{
			CFArc Arc = pDebugItem->m_Arc;
			LabelPoint.SetPoint( Arc.x + ( m_ConnectorRadius * 2 ), Arc.y - ( m_ConnectorRadius * 2 ) );
			Arc = Scale( Arc );
			CBrush *pSaveBrush = (CBrush*)pRenderer->SelectStockObject( NULL_BRUSH );
			pRenderer->Arc( Arc );
			pRenderer->SelectObject( pSaveBrush );
		}
		LabelPoint = Scale( LabelPoint );
		LabelPoint.y -= LabelYAdjust;
		LabelYAdjust += 4;
		pRenderer->SetTextColor( pDebugItem->m_Color );
		pRenderer->SetTextAlign( TA_TOP | TA_LEFT );
		pRenderer->TextOut(LabelPoint.x, LabelPoint.y, Label );
		++Count;
	}

	if( pOldPen != 0 )
		pRenderer->SelectObject( pOldPen );
}

CFArea CLinkageView::DrawMechanism( CRenderer* pRenderer )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if( m_bShowGrid && pRenderer->BackgroundsAllowed() )
		DrawGrid( pRenderer, m_GridType );

	if( m_bShowSelection && m_bSuperHighlight && pDoc->IsAnySelected() )
	{
		CFRect SelectRect = GetDocumentArea( false, true );

		CFRect RealRect = Scale( SelectRect );
		RealRect.InflateRect( 3, 3 );

		bool bMoreHighlight = RealRect.Width() < 30 || RealRect.Height() < 30;
		RealRect.InflateRect( 2, 2 );
		double Inflate = max( 10 - RealRect.Width(), 10 - RealRect.Height() );
		if( Inflate > 0 )
			RealRect.InflateRect( Inflate, Inflate );

		CBrush Brush( bMoreHighlight ? COLOR_SUPERHIGHLIGHT2 : COLOR_SUPERHIGHLIGHT );
		pRenderer->FillRect( &RealRect, &Brush );
	}

	pRenderer->SelectObject( m_pUsingFont, UnscaledUnits( m_UsingFontHeight ) );
	pRenderer->SetTextColor( COLOR_TEXT );

	POSITION Position = 0;

	LinkList* pLinkList = pDoc->GetLinkList();
	Position = pLinkList->GetHeadPosition();
	while( Position != NULL )
	{
		CLink* pLink = pLinkList->GetNext( Position );
		if( pLink != 0 )
			pLink->ComputeHull();
	}

	static const int Steps = 2;
	static const int LayersOnStep[Steps] = { CLinkageDoc::DRAWINGLAYER, CLinkageDoc::MECHANISMLAYER };

	for( int Step = 0; Step < 2; ++Step )
	{
		int StepLayers = LayersOnStep[Step];
		Position = pLinkList->GetHeadPosition();
		while( Position != NULL )
		{
			CLink* pLink = pLinkList->GetNext( Position );
			if( pLink != 0  && ( pLink->GetLayers() & StepLayers ) != 0 )
				DrawLink( pRenderer, pDoc->GetGearConnections(), m_SelectedViewLayers, pLink, false, false, true );
		}
	}

	ConnectorList* pConnectors = pDoc->GetConnectorList();

	for( int Step = 0; Step < 2; ++Step )
	{
		int StepLayers = LayersOnStep[Step];

		Position = pLinkList->GetHeadPosition();
		while( Position != NULL )
		{
			CLink* pLink = pLinkList->GetNext( Position );
			if( pLink != 0 && ( pLink->GetLayers() & StepLayers ) != 0 )
				DrawLink( pRenderer, pDoc->GetGearConnections(), m_SelectedViewLayers, pLink, m_bShowLabels, true, false );
		}

		if( m_MouseAction == ACTION_STRETCH && Step == 1 )
		{
			/*
			 * Draw a box just for stretching that shows the stretch extents.
			 */

			CLinkageDoc* pDoc = GetDocument();
			ASSERT_VALID(pDoc);
			CFRect Rect = GetDocumentAdjustArea( true );
			if( fabs( Rect.Height() ) > 0 && fabs( Rect.Width() ) > 0 )
			{
				CPen Pen( PS_DOT, 1, COLOR_STRETCHDOTS );
				CPen *pOldPen = pRenderer->SelectObject( &Pen );
				pRenderer->DrawRect( Scale( Rect ) );
				pRenderer->SelectObject( pOldPen );
			}
		}

		if( Step == 1 )
		{
			Position = pConnectors->GetHeadPosition();
			while( Position != NULL )
			{
				CConnector* pConnector = pConnectors->GetNext( Position );
				if( pConnector == 0 )
					continue;

				DrawMotionPath( pRenderer, pConnector );
			}
		}

		Position = pConnectors->GetHeadPosition();
		while( Position != NULL )
		{
			CConnector* pConnector = pConnectors->GetNext( Position );
			if( pConnector != 0 && ( pConnector->GetLayers() & StepLayers ) != 0 )
				DrawSliderTrack( pRenderer, m_SelectedViewLayers, pConnector );
		}

		Position = pLinkList->GetHeadPosition();
		while( Position != NULL )
		{
			CLink* pLink = pLinkList->GetNext( Position );
			if( pLink != 0 && ( pLink->GetLayers() & StepLayers ) != 0 )
				DrawLink( pRenderer, pDoc->GetGearConnections(), m_SelectedViewLayers, pLink, m_bShowLabels, false, false );
		}

		Position = pConnectors->GetHeadPosition();
		while( Position != NULL )
		{
			CConnector* pConnector = pConnectors->GetNext( Position );
			if( pConnector != 0 && ( pConnector->GetLayers() & StepLayers ) != 0 )
				DrawConnector( pRenderer, m_SelectedViewLayers, pConnector, m_bShowLabels );
		}
	}

	Position = pDoc->GetGearConnections()->GetHeadPosition();
	while( Position != 0 )
	{
		CGearConnection *pGearConnection = pDoc->GetGearConnections()->GetNext( Position );
		if( pGearConnection == 0 )
			continue;

		if( pGearConnection->m_ConnectionType == pGearConnection->CHAIN )
			DrawChain( pRenderer, m_SelectedViewLayers, pGearConnection );
	}

	Position = pLinkList->GetHeadPosition();
	while( Position != NULL )
	{
		CLink* pLink = pLinkList->GetNext( Position );
		if( pLink != 0 )
		{
			int Knobs = 0;
			CControlKnob *pControlKnob = pLink->GetControlKnobs( Knobs );
			if( Knobs > 0 && pControlKnob != 0 )
				DrawControlKnob( pRenderer, m_SelectedViewLayers, pControlKnob );
		}
	}

	Position = pConnectors->GetHeadPosition();
	while( Position != NULL )
	{
		CConnector* pConnector = pConnectors->GetNext( Position );
		if( pConnector != 0 )
		{
			int Knobs = 0;
			CControlKnob *pControlKnob = pConnector->GetControlKnobs( Knobs );
			if( Knobs > 0 && pConnector != 0 )
				DrawControlKnob( pRenderer, m_SelectedViewLayers, pControlKnob );
		}
	}

	Position = pLinkList->GetHeadPosition();
	while( Position != NULL )
	{
		CLink* pLink = pLinkList->GetNext( Position );
		if( pLink != 0  )
			DebugDrawLink( pRenderer, m_SelectedViewLayers, pLink, false, true, true );
	}

	Position = pConnectors->GetHeadPosition();
	while( Position != NULL )
	{
		CConnector* pConnector = pConnectors->GetNext( Position );
		if( pConnector == 0 )
			continue;
		DebugDrawConnector( pRenderer, m_SelectedViewLayers, pConnector, m_bShowLabels );
	}

	DrawStackedConnectors( pRenderer, m_SelectedViewLayers );

	Position = pLinkList->GetHeadPosition();
	while( Position != NULL )
	{
		CLink* pLink = pLinkList->GetNext( Position );
		if( pLink != 0 && !pLink->IsMeasurementElement() )
			DrawDimensions( pRenderer, pDoc->GetGearConnections(), m_SelectedViewLayers, pLink, true, true );
	}

	Position = pConnectors->GetHeadPosition();
	while( Position != NULL )
	{
		CConnector* pConnector = pConnectors->GetNext( Position );
		if( pConnector != 0 )
			DrawDimensions( pRenderer, m_SelectedViewLayers, pConnector, true, true );
	}

	DrawGroundDimensions( pRenderer, pDoc, m_SelectedViewLayers, 0, true, true );

	DrawSliderRadiusDimensions( pRenderer, pDoc, m_SelectedViewLayers, true, true );

	DrawSnapLines( pRenderer );

	if( m_bShowAngles )
		DrawAlignmentHintLines( pRenderer );

	if( m_bShowDebug )
		DrawDebugItems( pRenderer );

	if( m_bShowSelection && pDoc->IsSelectionAdjustable() )
		DrawAdjustmentControls( pRenderer );

	if( m_bShowData && m_bShowSelection )
		DrawData( pRenderer );

	if( m_bShowSelection && m_MouseAction == ACTION_SELECT )
	{
		CPen Pen( PS_SOLID, 1, RGB( 128, 128, 255 ) );
		CPen *pOldPen = pRenderer->SelectObject( &Pen );
		pRenderer->DrawRect( m_SelectRect );
		pRenderer->SelectObject( pOldPen );
	}

	return CFArea();
}

void CLinkageView::MovePartsLinkToOrigin( CLink *pPartsLink, CFPoint Origin, GearConnectionList *pGearConnections, bool bRotateToOptimal )
{
	if( pPartsLink->GetConnectorCount() == 1 || pPartsLink->IsGear() && pGearConnections != 0 )
	{
		std::list<double> RadiusList;
		pPartsLink->GetGearRadii( *pGearConnections, RadiusList );
		if( !RadiusList.empty() )
		{
			double LargestRadius = RadiusList.back();
			CConnector *pConnector = pPartsLink->GetConnector( 0 );
			if( pConnector != 0 )
				pConnector->SetPoint( CFPoint( Origin.x + LargestRadius, Origin.y - LargestRadius ) );
		}
		return;
	}

	int BestStartPoint = -1;
	int BestEndPoint = -1;

	int PointCount = 0;
	CFPoint *pPoints = pPartsLink->GetPoints( PointCount );
	bool bIsLine = true;
	if( PointCount > 2 )
	{
		for( int Index = 2; Index < PointCount; ++Index )
		{
			if( DistanceToLine( pPoints[0], pPoints[1], pPoints[Index] ) > 0.0001 )
			{
				bIsLine = false;
				break;
			}
		}
	}

	CConnector *pStartConnector = 0;

	if( bIsLine )
	{
		BestStartPoint = GetLongestLinearDimensionLine( pPartsLink, BestEndPoint, pPoints, PointCount, &pStartConnector );
		if( BestStartPoint < 0 )
			return;
	}
	else
	{
		PointCount = 0;
		pPoints = pPartsLink->GetHull( PointCount );

		BestStartPoint = GetLongestHullDimensionLine( pPartsLink, BestEndPoint, pPoints, PointCount, &pStartConnector );
		if( BestStartPoint < 0 )
			return;
	}

	CFLine OrientationLine( pPoints[BestStartPoint], pPoints[BestEndPoint] );
	double Angle = 0;
	if( bRotateToOptimal )
		Angle = GetAngle( pPoints[BestStartPoint], pPoints[BestEndPoint] );

	CFPoint AdditionalOffset;
	POSITION Position = pPartsLink->GetConnectorList()->GetHeadPosition();
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

CTempLink* CLinkageView::GetTemporaryGroundLink( LinkList *pDocLinks, ConnectorList *pDocConnectors, CFPoint PartOrigin )
{
	int Anchors = 0;
	POSITION Position = pDocConnectors->GetHeadPosition();
	while( Position != 0 )
	{
		CConnector *pConnector = pDocConnectors->GetNext( Position );
		if( pConnector == 0 || !pConnector->IsAnchor() )
			continue;

		++Anchors;
	}

	if( Anchors <= 1 ) // Empty or non-functining mechanism, or there's just one anchor which doesn't need a ground link part. 
		return 0;

	#if 0 // Always include the ground link.
	Position = pDocLinks->GetHeadPosition();
	while( Position != 0 )
	{
		CLink *pLink = pDocLinks->GetNext( Position );
		if( pLink == 0  )
			continue;

		int FoundAnchors = pLink->GetAnchorCount();
		if( FoundAnchors > 0 && FoundAnchors == Anchors )
			return 0; // The total anchor count all on this link so there is no need for a ground link.
	}
	#endif

	// Create the ground link.

	CTempLink *pPartsLink = new CTempLink();
	pPartsLink->SetColor( COLOR_GROUND );
	pPartsLink->SetLocked( false );
	pPartsLink->SetIdentifier( -1 );
	pPartsLink->SetName( "Ground" );
	pPartsLink->SetLayers( CLinkageDoc::MECHANISMLAYER );

	Position = pDocConnectors->GetHeadPosition();
	while( Position != 0 )
	{
		CConnector *pConnector = pDocConnectors->GetNext( Position );
		if( pConnector == 0 || !pConnector->IsAnchor() )
			continue;

		CConnector *pPartsConnector = new CConnector( *pConnector );
		pPartsConnector->SetColor( pPartsLink->GetColor() );
		pPartsConnector->SetAsAnchor( false );
		pPartsConnector->SetAsInput( false );
		pPartsConnector->SetAsDrawing( false );
		pPartsLink->AddConnector( pPartsConnector );
	}

	MovePartsLinkToOrigin( pPartsLink, PartOrigin, 0, false );

	return pPartsLink;
}

CTempLink* CLinkageView::GetTemporaryPartsLink( CLink *pLink, CFPoint PartOrigin, GearConnectionList *pGearConnections )
{
	CTempLink *pPartsLink = new CTempLink( *pLink );
	// MUST REMOVE THE COPIED CONNECTORS HERE BECAUSE THEY ARE JUST POINTER COPIES!
	pPartsLink->RemoveAllConnectors();
	pPartsLink->SetLocked( false );

	POSITION Position = pLink->GetConnectorList()->GetHeadPosition();
	while( Position != 0 )
	{
		CConnector *pConnector = pLink->GetConnectorList()->GetNext( Position );
		if( pConnector == 0 )
			continue;

		CConnector *pPartsConnector = new CConnector( *pConnector );
		pPartsConnector->SetColor( pPartsLink->GetColor() );
		pPartsConnector->SetAsAnchor( false );
		pPartsConnector->SetAsInput( false );
		pPartsConnector->SetAsDrawing( false );
		pPartsLink->AddConnector( pPartsConnector );
	}

	MovePartsLinkToOrigin( pPartsLink, PartOrigin, pGearConnections );

	return pPartsLink;
}

CFArea CLinkageView::DrawPartsList( CRenderer* pRenderer )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// get the original document area just to get the top left corner for placing the parts list contents.
	CFRect Area;
	pDoc->GetDocumentArea( Area );

	if( m_bShowGrid && pRenderer->BackgroundsAllowed() )
		DrawGrid( pRenderer, m_GridType );

	pRenderer->SelectObject( m_pUsingFont, UnscaledUnits( m_UsingFontHeight ) );
	pRenderer->SetTextColor( COLOR_TEXT );

	CFPoint StartPoint = Area.TopLeft();
	CFArea DocumentArea;
	double LastPartHeight = 0;
	double yOffset = 0.0;

	GearConnectionList *pGearConnections = pDoc->GetGearConnections();

	POSITION Position = pDoc->GetLinkList()->GetHeadPosition();
	bool bGroundLink = false;
	for( ; !bGroundLink; ( Position == 0 ? 0 : pDoc->GetLinkList()->GetNext( Position ) ) )
	{
		CLink *pPartsLink = 0;
		if( Position == 0 )
		{
			if( !m_bShowGroundDimensions )
				break;
			/*
			 * Generate a ground link for this last pass through the data.
			 */

			bGroundLink = true;
			pPartsLink = GetTemporaryGroundLink( pDoc->GetLinkList(), pDoc->GetConnectorList(), CFPoint( StartPoint.x, StartPoint.y - yOffset - LastPartHeight ) );
		}
		else
		{
			CLink *pLink = pDoc->GetLinkList()->GetAt( Position );
			if( pLink == 0 || ( pLink->GetLayers() & CLinkageDoc::DRAWINGLAYER ) != 0 )
				continue;

			if( pLink->GetConnectorCount() <= 1 && !pLink->IsGear() )
				continue;

			pPartsLink = GetTemporaryPartsLink( pLink, CFPoint( StartPoint.x, StartPoint.y - yOffset - LastPartHeight ), pGearConnections );
		}

		if( pPartsLink == 0 )
			continue;

		yOffset += LastPartHeight;

		ConnectorList* pConnectors = pPartsLink->GetConnectorList();

		DrawLink( pRenderer, pGearConnections, m_SelectedViewLayers, pPartsLink, false, false, true );
		DrawLink( pRenderer, pGearConnections, m_SelectedViewLayers, pPartsLink, m_bShowLabels, true, false );

		//POSITION Position2 = pConnectors->GetHeadPosition();
		//while( Position2 != NULL )
		//{
		//	CConnector* pConnector = pConnectors->GetNext( Position2 );
		//	if( pConnector != 0 )
		//		DrawSliderTrack( pRenderer, m_SelectedViewLayers, pConnector );
		//}

		DrawLink( pRenderer, pGearConnections, m_SelectedViewLayers, pPartsLink, m_bShowLabels, false, false );
		POSITION Position2 = pConnectors->GetHeadPosition();
		while( Position2 != NULL )
		{
			CConnector* pConnector = pConnectors->GetNext( Position2 );
			if( pConnector != 0 )
			{
				DrawConnector( pRenderer, m_SelectedViewLayers, pConnector, m_bShowLabels );
				// DrawConnector( pRenderer, m_SelectedViewLayers, pConnector, m_bShowLabels, false, false, false, true );
			}
		}

		CFArea PartArea;
		pPartsLink->GetArea( *pGearConnections, PartArea );

		if( bGroundLink )
			PartArea += DrawGroundDimensions( pRenderer, pDoc, m_SelectedViewLayers, pPartsLink, true, true );
		else
			PartArea += DrawDimensions( pRenderer, pGearConnections, m_SelectedViewLayers, pPartsLink, true, true );
		Position2 = pConnectors->GetHeadPosition();
		while( Position2 != NULL )
		{
			CConnector* pConnector = pConnectors->GetNext( Position2 );
			if( pConnector != 0 )
			PartArea += DrawDimensions( pRenderer, m_SelectedViewLayers, pConnector, true, true );
		}

		DocumentArea += PartArea;

		LastPartHeight = abs( PartArea.Height() ) + Unscale( OFFSET_INCREMENT );

		ASSERT( pPartsLink != 0 );
		delete pPartsLink;
	}

	/*
	 * Create a ground link if no other single link holds all of the ground connectors.
	 */

	return DocumentArea;
}

void CLinkageView::DrawMotionPath( CRenderer *pRenderer, CConnector *pConnector )
{
	// Draw the drawing stuff before the Links and connectors
	int DrawPoint = 0;
	int PointCount = 0;
	int MaxPoints = 0;

	pRenderer->SetBkMode( TRANSPARENT );
	CFPoint *pPoints = pConnector->GetMotionPath( DrawPoint, PointCount, MaxPoints );
	if( pPoints != 0 && PointCount > 1 )
	{
		COLORREF Color = COLOR_MOTIONPATH;
		CPen Pen( PS_SOLID, 1, Color );
		CPen *pOldPen = pRenderer->SelectObject( &Pen );

		pRenderer->MoveTo( Scale( CFPoint( pPoints[DrawPoint].x, pPoints[DrawPoint].y ) ) );
		++DrawPoint;
		for( int Counter = 1; Counter < PointCount; ++Counter )
		{
			if( DrawPoint > MaxPoints )
				DrawPoint = 0;
			pRenderer->LineTo( Scale( CFPoint( pPoints[DrawPoint].x, pPoints[DrawPoint].y ) ) );
			++DrawPoint;
		}
		if( m_bSimulating )
			pRenderer->LineTo( Scale( pConnector->GetPoint() ) );
	}
}

BOOL StretchBltPlus(
  HDC hdcDest,
  int nXOriginDest,
  int nYOriginDest,
  int nWidthDest,
  int nHeightDest,
  HDC hdcSrc,
  int nXOriginSrc,
  int nYOriginSrc,
  int nWidthSrc,
  int nHeightSrc,
  DWORD dwRop
)
{
	  Graphics dest(hdcDest);
	  Graphics src(hdcSrc);
	  HBITMAP hBmp = (HBITMAP)GetCurrentObject( hdcSrc, OBJ_BITMAP );
	  Image *imx = Bitmap::FromHBITMAP(hBmp,NULL);
	  RectF r( (Gdiplus::REAL)nXOriginDest, (Gdiplus::REAL)nYOriginDest, (Gdiplus::REAL)nWidthDest, (Gdiplus::REAL)nHeightDest );
	  dest.SetInterpolationMode( InterpolationModeHighQuality );
	  dest.DrawImage( imx, r, (Gdiplus::REAL)nXOriginSrc, (Gdiplus::REAL)nYOriginSrc, (Gdiplus::REAL)nWidthSrc, (Gdiplus::REAL)nHeightSrc, UnitPixel );
	  delete imx;
	  return TRUE;
}

void CLinkageView::OnPrint(CDC* pDC, CPrintInfo* pPrintInfo)
{
	ASSERT_VALID(pDC);

	// Override and set printing variables based on page number
	OnDraw(pDC, pPrintInfo);                    // Call Draw
}

void CLinkageView::OnDraw( CDC* pDC )
{
		OnDraw( pDC, 0 );
}

void CLinkageView::OnDraw( CDC* pDC, CPrintInfo *pPrintInfo )
{
   	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	#if defined( LINKAGE_USE_DIRECT2D )
		CRenderer Renderer( pDC->IsPrinting() ? CRenderer::WINDOWS_PRINTER_GDI : CRenderer::WINDOWS_D2D );
	#else
		CRenderer Renderer( pDC->IsPrinting() ? CRenderer::WINDOWS_PRINTER_GDI : CRenderer::WINDOWS_GDI );
	#endif

	/*
	 * Forcing a scale that is not 1.0 will result in slow framerates during editing
	 * and simulation. The forced scale value is really only intended to be used
	 * when saving a video animation of the mechanism.
	 */

	// Just a note for later experimentation...
	// The 8th parameter below is the unscalled unit size and it should be possible to set that to a large value to
	// get thicker lines as well as larget connector circles, etc.
	// When changing it, it does appear to alter the connector ciecle size but not he line thicknesses. I tried
	// to change the line thickness in a pen creation call and it worked like expected.
	// The trick is to called UnscaledUnits() on the number passed in to the call to create the pen object. Then the
	// thickness of everything can be altered. Maybe it's useless but it might be handy to do all of that in the floating
	// point world and then allow the mechanism scale to be adjucted for video capture, sort of like when the video
	// is captured using a separate draw operation to a separate renderer object. Give it a try sometime.

	CBitmap Bitmap;
    PrepareRenderer( Renderer, 0, &Bitmap, pDC, 1.0, false, 0.0, 1.0, !pDC->IsPrinting(), false, m_bPrintFullSize, pPrintInfo == 0 ? 0 : pPrintInfo->m_nCurPage - 1 );

	if( m_pBackgroundBitmap != 0 )
	{
		CFRect Rect( -(m_pBackgroundBitmap->width/2), -(m_pBackgroundBitmap->height/2), -(m_pBackgroundBitmap->width/2) + m_pBackgroundBitmap->width, -(m_pBackgroundBitmap->height/2) + m_pBackgroundBitmap->height );
		Rect = Scale( Rect );

		Renderer.DrawRendererBitmap( m_pBackgroundBitmap, Rect, pDoc->GetBackgroundTransparency() );
	}

	DoDraw( &Renderer );

	if( pDC->IsPrinting() )
	{
		if( pPrintInfo != 0 )
		{
			CString Number;
			Number.Format( "Page %d", pPrintInfo->m_nCurPage ); 
			// Print page numbers. Give it a try...
			pDC->SelectClipRgn( 0 );
			int x = m_DrawingRect.left + m_DrawingRect.Width() / 2;
			int y = m_DrawingRect.bottom;
			Renderer.SetTextAlign( TA_CENTER | TA_TOP ); // Using TA_TOP since the bottom of the drawing rect is where there is room for a page number.
			Renderer.TextOutA( x, y, Number );
		}
		//CFRect Rect( -(m_pBackgroundBitmap->width/2), -(m_pBackgroundBitmap->height/2), -(m_pBackgroundBitmap->width/2) + m_pBackgroundBitmap->width, -(m_pBackgroundBitmap->height/2) + m_pBackgroundBitmap->height );
		//Rect = Scale( Rect );

		//pPrintInfo == 0 ? 0 : pPrintInfo->m_nCurPage

		/*
		 * Printing is done using the print DC directly. There is no
		 * copy operating and no video creation so there is
		 * no further action to take in this function.
		 */
		Renderer.Detach();
		return;
	}

	if( m_bRecordingVideo && m_RecordQuality == 0 )
	{
		/*
		 * Copy the image used for the window directly to the video
		 * file as-is. No extra draw operation is needed for a
		 * standard quality video. The image will lose data if the
		 * window on the screen does not contain the entire video
		 * capture area.
		 */

		Renderer.EndDraw();
		SaveVideoFrame( &Renderer, m_DrawingRect );
		Renderer.BeginDraw();
	}

	if( m_bShowAnicrop )
		DrawAnicrop( &Renderer );

	DrawRuler( &Renderer );

	Renderer.EndDraw();


	/*
	 * Copy the image to the window.
	 */

	if( Renderer.GetScale() == 1.0 )
		pDC->BitBlt( 0, 0, m_DrawingRect.Width(), m_DrawingRect.Height(), Renderer.GetDC(), 0, 0, SRCCOPY );
	else
		StretchBltPlus( pDC->GetSafeHdc(),
		                0, 0, m_DrawingRect.Width(), m_DrawingRect.Height(),
		                Renderer.GetSafeHdc(),
						0, 0, (int)( m_DrawingRect.Width() * Renderer.GetScale() ), (int)( m_DrawingRect.Height() * Renderer.GetScale() ),
						SRCCOPY );

	if( m_bRecordingVideo && m_RecordQuality > 0 )
	{
		/*
		 * Redraw the entire image at a scaled size to get a high quality anti-aliased
		 * image in the video. Convert to 1080p.
		 */

		CRenderer VideoRenderer( CRenderer::WINDOWS_GDI );
		CBitmap VideoBitmap;

		CRect VideoRect( 0, 0, ANIMATIONWIDTH, ANIMATIONHEIGHT );

		PrepareRenderer( VideoRenderer, &VideoRect, &VideoBitmap, pDC, 4.0, false, 0.0, 1.0, true, true, false, 0 );

		m_Zoom = m_ScreenZoom * m_DPIScale;
		m_ScrollPosition.x = m_ScreenScrollPosition.x * m_DPIScale;
		m_ScrollPosition.y = m_ScreenScrollPosition.y * m_DPIScale;
		double SaveDPIScale = m_DPIScale;
		m_DPIScale = 1.0;

		DoDraw( &VideoRenderer );

		m_DPIScale = SaveDPIScale;

		SaveVideoFrame( &VideoRenderer, VideoRect );
		
		Renderer.EndDraw();
	}
}

void CLinkageView::DrawAnicrop( CRenderer *pRenderer )
{
	if( !m_bShowAnicrop )
		return;

	double UseWidth = ( ANIMATIONWIDTH + 1 ) / m_DPIScale;  // the renderer will scale up the rectangle and it will end up the proper pixel size.
	double UseHeight = ( ANIMATIONHEIGHT + 1 ) / m_DPIScale;  // the renderer will scale up the rectangle and it wil end up the proper pixel size.

	double x = ( m_DrawingRect.Width() / 2  / m_DPIScale) - ( UseWidth / 2 );
	double y = ( m_DrawingRect.Height() / 2 / m_DPIScale ) - ( UseHeight / 2 );

	CPen Blue( PS_SOLID, 1, COLOR_ANICROP );
	CPen *pOldPen = pRenderer->SelectObject( &Blue );
	pRenderer->DrawRect( x, y, x+UseWidth, y+UseHeight );

	//UseWidth = ( ANIMATIONWIDTH + 1 ) / 2 / m_DPIScale;
	//UseHeight = ( ANIMATIONHEIGHT + 1 ) / 2 / m_DPIScale;

	//x = ( m_DrawingRect.Width() / 2 / m_DPIScale ) - ( UseWidth / 2 );
	//y = ( m_DrawingRect.Height() / 2 / m_DPIScale ) - ( UseHeight / 2 );

	//CPen BlueDots( PS_DOT, 1, RGB( 196, 196, 255 ) );
	//pRenderer->SelectObject( &BlueDots );
	//pRenderer->DrawRect( x, y, x+UseWidth, y+UseHeight );

	pRenderer->SelectObject( pOldPen );
}

void CLinkageView::DrawRuler( CRenderer *pRenderer )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if( !m_bShowDimensions || pDoc->GetConnectorList()->GetCount() > 0 )
		return;

	CPen Pen( PS_SOLID, 1, RGB( 196, 196, 196 ) );
	CPen *pOldPen = pRenderer->SelectObject( &Pen );
	CFont *pOriginalFont = pRenderer->SelectObject( m_pUsingFont, UnscaledUnits( m_UsingFontHeight ) );
	COLORREF OldFontColor = pRenderer->GetTextColor();
	pRenderer->SetTextColor( RGB( 96, 96, 96 ) );

	static const int SCALE_LINE_GAPLENGTH = 50;
	static const int SCALE_LINE_LENGTH = 500;
	static const int SCALE_LINE_SEGMENTS = 3;
	static const int SCALE_LINE_TICK_HEIGHT = 10;

	double DocScale = pDoc->GetUnitScale();

	double GapDistance = Unscale( SCALE_LINE_GAPLENGTH ) * DocScale;

	// Get the next 1, 5, 10, 50, 100 type of number that is above the
	// distance we got using a base pixel length. This will be the value
	// to use for the measurement line tick marks.
	int Test = (int)( GapDistance * 1000 );
	int Adjustment = -4;
	while( Test != 0 )
	{
		++Adjustment;
		Test /= 10;
	}
	double NewValue = GapDistance / pow( 10.0, Adjustment );
	NewValue += .9999;
	NewValue = (int)( NewValue );
	if( NewValue > 5.0 )
		NewValue = 10;
	else if( NewValue > 1.0 )
		NewValue = 5;
	GapDistance = NewValue * pow( 10.0, Adjustment );

	double PixelWidth = Scale( GapDistance / DocScale );
	int Sections = (int)( m_DrawingRect.Width() / 2 / PixelWidth );
	--Sections;
	PixelWidth *= Sections;

	for( int Counter = 0; Counter <= Sections; ++Counter )
	{
		CFLine Line( Counter * GapDistance / DocScale, 0, ( Counter + 1 ) * GapDistance / DocScale, 0 );
		Line = Scale( Line );

		CFLine Tick1( Line.GetStart(), CFPoint( Line.GetStart().x, Line.GetStart().y + SCALE_LINE_TICK_HEIGHT ) );

		pRenderer->DrawLine( Tick1 );

		if( Counter < Sections )
		{
			CFLine Tick2( Line.GetStart(), CFPoint( Line.GetStart().x, Line.GetStart().y + SCALE_LINE_TICK_HEIGHT ) );
			pRenderer->DrawLine( Tick2 );
			pRenderer->DrawLine( Line );
		}

		CString Measure;
		Measure.Format( "%.3lf", GapDistance * Counter );
		pRenderer->SetTextAlign( TA_CENTER | TA_TOP );
		pRenderer->TextOut( Line.GetStart().x, Line.GetStart().y + UnscaledUnits( SCALE_LINE_TICK_HEIGHT + 1 ), Measure );
	}

	pRenderer->SelectObject( pOldPen );
	pRenderer->SelectObject( pOriginalFont, UnscaledUnits( m_UsingFontHeight ) );
	pRenderer->SetTextColor( OldFontColor );
}

bool CLinkageView::SelectVideoBox( UINT nFlags, CPoint Point )
{
	if( m_VisibleAdjustment == ADJUSTMENT_NONE || m_MouseAction != ACTION_NONE )
		return false;

	CFRect Rect( 0, 0, 0, 0 );
	m_MouseAction = ACTION_NONE;

	CFPoint FPoint = Point;

	/*
	 * The line of code below gets screwed up by the compiler if the option
	 * is set to allow any appropriate function to be inline. Only functions
	 * marked as _inline shuold be allowed to avoid this compiler bug.
	 */
	GetAdjustmentControlRect( AC_ROTATE, Rect );
	Rect.InflateRect( 2, 2 );
	if( Rect.PointInRect( FPoint ) )
		m_MouseAction = ACTION_RECENTER;

	CFRect AdjustmentRect = Scale( m_SelectionContainerRect );

	MouseAction DoMouseAction = ACTION_NONE;
	if( m_VisibleAdjustment == ADJUSTMENT_ROTATE )
		DoMouseAction = ACTION_ROTATE;
	else
		DoMouseAction = ACTION_STRETCH;

	bool bVertical = fabs( m_SelectionContainerRect.Height() ) > 0;
	bool bHorizontal = fabs( m_SelectionContainerRect.Width() ) > 0;
	bool bBoth = bHorizontal && bVertical;

	GetAdjustmentControlRect( AC_TOP_LEFT, Rect );
	Rect.InflateRect( 2, 2 );
	if( bBoth && Rect.PointInRect( FPoint ) )
	{
		m_StretchingControl = AC_TOP_LEFT;
		m_DragOffset = FPoint - AdjustmentRect.TopLeft();
		m_MouseAction = DoMouseAction;
	}
	GetAdjustmentControlRect( AC_TOP_RIGHT, Rect );
	Rect.InflateRect( 2, 2 );
	if( bBoth && Rect.PointInRect( FPoint ) )
	{
		m_StretchingControl = AC_TOP_RIGHT;
		m_DragOffset = FPoint - CFPoint( AdjustmentRect.right, AdjustmentRect.top );
		m_MouseAction = DoMouseAction;
	}
	GetAdjustmentControlRect( AC_BOTTOM_RIGHT, Rect );
	Rect.InflateRect( 2, 2 );
	if( bBoth && Rect.PointInRect( FPoint ) )
	{
		m_StretchingControl = AC_BOTTOM_RIGHT;
		m_DragOffset = FPoint - AdjustmentRect.BottomRight();
		m_MouseAction = DoMouseAction;
	}
	GetAdjustmentControlRect( AC_BOTTOM_LEFT, Rect );
	Rect.InflateRect( 2, 2 );
	if( bBoth && Rect.PointInRect( FPoint ) )
	{
		m_StretchingControl = AC_BOTTOM_LEFT;
		m_DragOffset = FPoint - CFPoint( AdjustmentRect.left, AdjustmentRect.bottom );
		m_MouseAction = DoMouseAction;
	}

	if( m_VisibleAdjustment == ADJUSTMENT_STRETCH )
	{
		GetAdjustmentControlRect( AC_TOP, Rect );
		Rect.InflateRect( 2, 2 );
		if( bVertical && Rect.PointInRect( FPoint ) )
		{
			m_StretchingControl = AC_TOP;
			m_DragOffset = FPoint - CFPoint( ( AdjustmentRect.left + AdjustmentRect.right ) / 2, AdjustmentRect.top );
			m_MouseAction = ACTION_STRETCH;
		}
		GetAdjustmentControlRect( AC_RIGHT, Rect );
		Rect.InflateRect( 2, 2 );
		if( bHorizontal && Rect.PointInRect( FPoint ) )
		{
			m_StretchingControl = AC_RIGHT;
			m_DragOffset = FPoint - CFPoint( AdjustmentRect.right, ( AdjustmentRect.top + AdjustmentRect.bottom ) / 2 );
			m_MouseAction = ACTION_STRETCH;
		}
		GetAdjustmentControlRect( AC_BOTTOM, Rect );
		Rect.InflateRect( 2, 2 );
		if( bVertical && Rect.PointInRect( FPoint ) )
		{
			m_StretchingControl = AC_BOTTOM;
			m_DragOffset = FPoint - CFPoint( ( AdjustmentRect.left + AdjustmentRect.right ) / 2, AdjustmentRect.bottom );
			m_MouseAction = ACTION_STRETCH;
		}
		GetAdjustmentControlRect( AC_LEFT, Rect );
		Rect.InflateRect( 2, 2 );
		if( bHorizontal && Rect.PointInRect( FPoint ) )
		{
			m_StretchingControl = AC_LEFT;
			m_DragOffset = FPoint - CFPoint( AdjustmentRect.left, ( AdjustmentRect.top + AdjustmentRect.bottom ) / 2 );
			m_MouseAction = ACTION_STRETCH;
		}
	}

	if( m_MouseAction == ACTION_NONE )
		return false;
	else
	{
		InvalidateRect( 0 );
		return true;
	}
}

bool CLinkageView::GrabAdjustmentControl( CFPoint Point, CFPoint GrabPoint, _AdjustmentControl CheckControl, MouseAction DoMouseAction, _AdjustmentControl *pSelectControl, CFPoint *pDragOffset, MouseAction *pMouseAction )
{
	CFRect Rect( 0, 0, 0, 0 );
	GetAdjustmentControlRect( CheckControl, Rect );
	Rect.InflateRect( 2, 2 );
	if( Rect.PointInRect( Point ) )
	{
		*pSelectControl = CheckControl;
		*pDragOffset = Point - GrabPoint;
		*pMouseAction = DoMouseAction;
		return true;
	}
	return false;
}

CFPoint CLinkageView::AdjustClientAreaPoint( CPoint point )
{
	#if defined( LINKAGE_USE_DIRECT2D )
		return CFPoint( point.x / m_DPIScale, point.y / m_DPIScale );
	#else
		return CFPoint( point.x, point.y );
	#endif
}

bool CLinkageView::SelectAdjustmentControl( UINT nFlags, CFPoint Point )
{
	if( m_VisibleAdjustment == ADJUSTMENT_NONE || m_MouseAction != ACTION_NONE )
		return false;

	CFRect Rect( 0, 0, 0, 0 );
	m_MouseAction = ACTION_NONE;

	CFPoint FPoint = Point;

	/*
	 * The line of code below gets screwed up by the compiler if the option
	 * is set to allow any appropriate function to be inline. Only functions
	 * marked as _inline should be allowed to avoid this compiler bug.
	 */
	GetAdjustmentControlRect( AC_ROTATE, Rect );
	Rect.InflateRect( 2, 2 );
	if( Rect.PointInRect( FPoint ) )
		m_MouseAction = ACTION_RECENTER;

	CFRect AdjustmentRect = Scale( m_SelectionContainerRect );

	MouseAction DoMouseAction = ACTION_NONE;
	if( m_VisibleAdjustment == ADJUSTMENT_ROTATE )
		DoMouseAction = ACTION_ROTATE;
	else
		DoMouseAction = ACTION_STRETCH;

	bool bVertical = fabs( m_SelectionContainerRect.Height() ) > 0;
	bool bHorizontal = fabs( m_SelectionContainerRect.Width() ) > 0;

	if( m_VisibleAdjustment == ADJUSTMENT_ROTATE || ( bHorizontal && bVertical  ) )
	{
		GrabAdjustmentControl( FPoint, AdjustmentRect.TopLeft(), AC_TOP_LEFT, DoMouseAction, &m_StretchingControl, &m_DragOffset, &m_MouseAction );
		GrabAdjustmentControl( FPoint, CFPoint( AdjustmentRect.right, AdjustmentRect.top ), AC_TOP_RIGHT, DoMouseAction, &m_StretchingControl, &m_DragOffset, &m_MouseAction );
		GrabAdjustmentControl( FPoint, AdjustmentRect.BottomRight(), AC_BOTTOM_RIGHT, DoMouseAction, &m_StretchingControl, &m_DragOffset, &m_MouseAction );
		GrabAdjustmentControl( FPoint,  CFPoint( AdjustmentRect.left, AdjustmentRect.bottom ), AC_BOTTOM_LEFT, DoMouseAction, &m_StretchingControl, &m_DragOffset, &m_MouseAction );
	}

	if( m_VisibleAdjustment == ADJUSTMENT_STRETCH )
	{
		if( bVertical )
		{
			GrabAdjustmentControl( FPoint, CFPoint( ( AdjustmentRect.left + AdjustmentRect.right ) / 2 ), AC_TOP, ACTION_STRETCH, &m_StretchingControl, &m_DragOffset, &m_MouseAction );
			GrabAdjustmentControl( FPoint, CFPoint( ( AdjustmentRect.left + AdjustmentRect.right ) / 2, AdjustmentRect.bottom ), AC_BOTTOM, ACTION_STRETCH, &m_StretchingControl, &m_DragOffset, &m_MouseAction );
		}
		if( bHorizontal )
		{
			GrabAdjustmentControl( FPoint, CFPoint( AdjustmentRect.right, ( AdjustmentRect.top + AdjustmentRect.bottom ) / 2 ), AC_RIGHT, ACTION_STRETCH, &m_StretchingControl, &m_DragOffset, &m_MouseAction );
			GrabAdjustmentControl( FPoint, CFPoint( AdjustmentRect.left, ( AdjustmentRect.top + AdjustmentRect.bottom ) / 2 ), AC_LEFT, ACTION_STRETCH, &m_StretchingControl, &m_DragOffset, &m_MouseAction );
		}
	}

	if( m_MouseAction == ACTION_NONE )
		return false;
	else
	{
		InvalidateRect( 0 );
		return true;
	}
}

void CLinkageView::MarkSelection( bool bSelectionChanged, bool bUpdateRotationCenter )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if( pDoc->IsSelectionAdjustable() )
	{
		if( bSelectionChanged )
		{
			m_bChangeAdjusters = false;
			m_VisibleAdjustment = ADJUSTMENT_STRETCH;
			m_SelectionContainerRect = GetDocumentArea( false, true );
			m_SelectionAdjustmentRect = GetDocumentAdjustArea( true );
			if( bUpdateRotationCenter )
				m_SelectionRotatePoint.SetPoint( ( m_SelectionContainerRect.left + m_SelectionContainerRect.right ) / 2,
													( m_SelectionContainerRect.top + m_SelectionContainerRect.bottom ) / 2 );
		}
		else
			m_bChangeAdjusters = true;
	}
	else
		m_VisibleAdjustment = ADJUSTMENT_NONE;
}

bool CLinkageView::FindDocumentItem( CFPoint Point, CLink *&pFoundLink, CConnector *&pFoundConnector )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CFPoint AdjustedPoint = Unscale( Point );

	double GrabDistance = Unscale( GRAB_DISTANCE ); // The mouse point has already been adjusted for the DPI so the grab distance is not.

	return pDoc->FindElement( AdjustedPoint, GrabDistance, 0, pFoundLink, pFoundConnector );
}

bool CLinkageView::SelectDocumentItem( UINT nFlags, CFPoint point )
{
	if( m_MouseAction != ACTION_NONE )
		return false;

	if( GetAsyncKeyState( VK_MENU ) & 0x8000 )
		return false;
		
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	bool DepthSelect = ( nFlags & ( MK_CONTROL | MK_SHIFT ) ) == ( MK_CONTROL | MK_SHIFT );
	bool bMultiSelect = ( nFlags & ( MK_CONTROL | MK_SHIFT ) ) != 0 && !DepthSelect;
	if( !DepthSelect )
		m_SelectionDepth = 0;

	CFPoint AdjustedPoint = Unscale( point );

	double GrabDistance = Unscale( GRAB_DISTANCE ); // The mouse point has already been adjusted for the DPI so the grab distance is not.

	bool bSelectionChanged = false;
	m_MouseAction = pDoc->SelectElement( AdjustedPoint, GrabDistance, 0, bMultiSelect, m_SelectionDepth, bSelectionChanged ) ? ACTION_DRAG : ACTION_NONE;

	if( DepthSelect )
	{
		if( m_MouseAction != ACTION_NONE )
			++m_SelectionDepth;
		else
			m_SelectionDepth = 0;
	}

	SelectionChanged();

	MarkSelection( bSelectionChanged );
	ShowSelectedElementCoordinates();

	if( m_MouseAction == ACTION_NONE )
		return false;
	else
	{
		InvalidateRect( 0 );
		return true;
	}
}

bool CLinkageView::DragSelectionBox( UINT nFlags, CFPoint point )
{
	if( m_MouseAction != ACTION_NONE )
		return false;

	/*
	 * Assume nothing else is under the mouse cursor at this point
	 * in the processing. There would be some mouse action set if
	 * something else has been selected before noe.
	 */

	m_MouseAction = ACTION_SELECT;
	m_SelectRect.SetRect( point.x, point.y, point.x, point.y );
	InvalidateRect( 0 );

	return true;
}

void CLinkageView::OnLButtonDown(UINT nFlags, CPoint MousePoint)
{
	SetFocus();

	CFPoint point = AdjustClientAreaPoint( MousePoint );

	if( m_bSimulating || !m_bAllowEdit )
		return;

	SetCapture();

	m_PreviousDragPoint = point;
	m_DragStartPoint = point;
	m_DragOffset.SetPoint( 0, 0 );
	m_bMouseMovedEnough	= false;
	m_MouseAction = ACTION_NONE;
	m_bSuperHighlight = 0;

	CFPoint DocumentPoint = Unscale( point );
	SetLocationAsStatus( DocumentPoint );

	if( m_bAllowEdit && SelectAdjustmentControl( nFlags, point ) )
		return;

	if( SelectDocumentItem( nFlags, point ) )
		return;

	if( DragSelectionBox( nFlags, point ) )
		return;
}

void CLinkageView::OnButtonUp(UINT nFlags, CPoint MousePoint)
{
	CFPoint point = AdjustClientAreaPoint( MousePoint );

	ReleaseCapture();

    if( m_bChangeAdjusters )
		OnMouseEndChangeAdjusters( nFlags, point );

	switch( m_MouseAction )
	{
		case ACTION_SELECT: OnMouseEndSelect( nFlags, point ); break;
		case ACTION_DRAG: OnMouseEndDrag( nFlags, point ); break;
		case ACTION_PAN: OnMouseEndPan( nFlags, point ); break;
		case ACTION_ROTATE: OnMouseEndRotate( nFlags, point ); break;
		case ACTION_RECENTER: OnMouseEndRecenter( nFlags, point ); break;
		case ACTION_STRETCH: OnMouseEndStretch( nFlags, point ); break;
		default: break;
	}
	m_MouseAction = ACTION_NONE;

	CFPoint DocumentPoint = Unscale( point );
	//SetLocationAsStatus( DocumentPoint );

	SetScrollExtents();
}

void CLinkageView::OnLButtonUp(UINT nFlags, CPoint point)
{
	OnButtonUp( nFlags, point );
}

void CLinkageView::OnMouseMoveSelect(UINT nFlags, CFPoint point)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	m_SelectRect.SetRect( m_SelectRect.TopLeft(), point );
	InvalidateRect( 0 );
}

double CLinkageView::CalculateDefaultGrid( void )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	double GapDistance = Unscale( 18 ) * pDoc->GetUnitScale();

	// Get the next 1, 5, 10, 50, 100 type of number that is above the
	// distance we got using a base pixel length. This will be the value
	// to use for the snap.
	int Test = (int)( GapDistance * 1000 );
	int Adjustment = -4;
	while( Test != 0 )
	{
		++Adjustment;
		Test /= 10;
	}
	double NewValue = GapDistance / pow( 10.0, Adjustment );
	NewValue += .9999;
	NewValue = (int)( NewValue );
	if( NewValue > 5.0 )
		NewValue = 10;
	else if( NewValue > 1.0 )
		NewValue = 5;
	GapDistance = NewValue * pow( 10.0, Adjustment );
	if( GapDistance < 0.01 )
		GapDistance = 0.01;

	return GapDistance;
}

void CLinkageView::OnMouseMoveDrag(UINT nFlags, CFPoint point)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CFPoint AdjustedPoint = Unscale( point );

	bool bControlKey = ( nFlags & ( MK_CONTROL ) ) != 0;
	bool bShiftKey = ( nFlags & ( MK_SHIFT ) ) != 0;

	bool bGridSnap = m_bGridSnap;
	bool bElementSnap = m_bSnapOn;

	if( m_bGridSnap || m_bSnapOn )
	{
		// Either is on so bToggleSnap==true disables both.
		bGridSnap = bGridSnap && !bControlKey && !bShiftKey;
		bElementSnap = bElementSnap && !bControlKey && !bShiftKey;
	}
	else
	{
		// Both are off so bToggleSnap==true enables one or both depending on the key.
		bGridSnap = bShiftKey;
		bElementSnap = bControlKey;
	}

	CFPoint ReferencePoint = AdjustedPoint;

	double xGapDistance = m_GridType == 0 ? CalculateDefaultGrid() : ( m_xUserGrid * pDoc->GetUnitScale() );
	double yGapDistance = m_GridType == 0 ? xGapDistance : ( m_yUserGrid * pDoc->GetUnitScale() );

	if( m_bAllowEdit )
	{
		if( pDoc->MoveSelected( AdjustedPoint, bElementSnap, bGridSnap,  xGapDistance / pDoc->GetUnitScale(),  yGapDistance / pDoc->GetUnitScale(), Unscale( VIRTUAL_PIXEL_SNAP_DISTANCE ), ReferencePoint, true ) )
			InvalidateRect( 0 );
	}

	SetLocationAsStatus( ReferencePoint );
}

void CLinkageView::OnMouseMovePan(UINT nFlags, CFPoint point)
{
	m_ScreenScrollPosition.x -= point.x - m_PreviousDragPoint.x;
	m_ScreenScrollPosition.y -= point.y - m_PreviousDragPoint.y;
	InvalidateRect( 0 );
}

void CLinkageView::OnMouseMoveRotate(UINT nFlags, CFPoint point)
{
	CFPoint DragStartPoint = Unscale( m_DragStartPoint );
	CFPoint CurrentPoint = Unscale( point );

	double Angle1 = GetAngle( m_SelectionRotatePoint, DragStartPoint );
	double Angle2 = GetAngle( m_SelectionRotatePoint, CurrentPoint );
	double Change = Angle2 - Angle1;
	while( Change < -180 )
		Change += 360;
	while( Change > 180 )
		Change -= 360;
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if( pDoc->RotateSelected( m_SelectionRotatePoint, Change ) )
	{
		InvalidateRect( 0 );
		CString Temp;
		Temp.Format( "%.3lf°", Change );
		SetStatusText( Temp );
	}
}

class CSnapConnector : public ConnectorListOperation
{
	public:
	CConnector *m_pSnapConnector;
	double m_Distance;
	CFPoint m_Point;
	double m_SnapDistance;
	CSnapConnector( const CFPoint &Point, double SnapDistance ) { m_Distance = DBL_MAX; m_pSnapConnector = 0; m_Point = Point; m_SnapDistance = SnapDistance; }
	bool operator()( class CConnector *pConnector, bool bFirst, bool bLast )
	{
		double TestDistance = m_Point.DistanceToPoint( pConnector->GetPoint() );
		if( TestDistance < m_Distance && TestDistance <= m_SnapDistance )
		{
			m_Distance = TestDistance;
			m_pSnapConnector = pConnector;
		}
		return true;
	}
	double GetDistance( void ) { return m_Distance; }
	CConnector *GetConnector( void ) { return m_pSnapConnector; }
};

void CLinkageView::OnMouseMoveRecenter(UINT nFlags, CFPoint point)
{
	// Snap to connectors

	CFPoint AdjustedPoint = Unscale( point );

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CSnapConnector Snap( AdjustedPoint, Unscale( VIRTUAL_PIXEL_SNAP_DISTANCE ) );
	pDoc->GetConnectorList()->Iterate( Snap );

	CConnector* pSnapConnector = Snap.GetConnector();

	if( pSnapConnector != 0 )
		m_SelectionRotatePoint = pSnapConnector->GetPoint();
	else
		m_SelectionRotatePoint = AdjustedPoint;

	InvalidateRect( 0 );
}

void CLinkageView::OnMouseMoveStretch(UINT nFlags, CFPoint point, bool bEndStretch )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CFPoint AdjustedPoint = Unscale( point );
	CLinkageDoc::_Direction Direction = CLinkageDoc::DIAGONAL;

	CFRect NewRect = m_SelectionAdjustmentRect;
	switch( m_StretchingControl )
	{
		case AC_TOP: NewRect.top = AdjustedPoint.y; Direction = CLinkageDoc::VERTICAL; break;
		case AC_RIGHT: NewRect.right = AdjustedPoint.x; Direction = CLinkageDoc::HORIZONTAL; break;
		case AC_BOTTOM: NewRect.bottom = AdjustedPoint.y; Direction = CLinkageDoc::VERTICAL; break;
		case AC_LEFT: NewRect.left = AdjustedPoint.x; Direction = CLinkageDoc::HORIZONTAL; break;

		case AC_TOP_LEFT:
		case AC_TOP_RIGHT:
		case AC_BOTTOM_LEFT:
		case AC_BOTTOM_RIGHT:
		{
			Direction = CLinkageDoc::DIAGONAL;
			double NewHeight;
			double NewWidth;
			switch( m_StretchingControl )
			{
				case AC_TOP_LEFT:
					NewRect.left = AdjustedPoint.x;
					NewRect.top = AdjustedPoint.y;
					NewHeight = m_SelectionAdjustmentRect.Width() == 0 ? 0 : m_SelectionAdjustmentRect.Height() * NewRect.Width() / m_SelectionAdjustmentRect.Width();
					NewWidth = m_SelectionAdjustmentRect.Height() == 0 ? 0 : m_SelectionAdjustmentRect.Width() * NewRect.Height() / m_SelectionAdjustmentRect.Height();

					if( fabs( NewWidth ) > fabs( NewRect.Width() ) )
						NewRect.left = NewRect.right - NewWidth;
					else
						NewRect.top = NewRect.bottom - NewHeight;
					break;
				case AC_TOP_RIGHT:
					NewRect.right = AdjustedPoint.x;
					NewRect.top = AdjustedPoint.y;
					NewHeight = m_SelectionAdjustmentRect.Width() == 0 ? 0 : m_SelectionAdjustmentRect.Height() * NewRect.Width() / m_SelectionAdjustmentRect.Width();
					NewWidth = m_SelectionAdjustmentRect.Height() == 0 ? 0 : m_SelectionAdjustmentRect.Width() * NewRect.Height() / m_SelectionAdjustmentRect.Height();

					if( fabs( NewWidth ) > fabs( NewRect.Width() ) )
						NewRect.right = NewRect.left + NewWidth;
					else
						NewRect.top = NewRect.bottom - NewHeight;
					break;
				case AC_BOTTOM_LEFT:
					NewRect.left = AdjustedPoint.x;
					NewRect.bottom = AdjustedPoint.y;
					NewHeight = m_SelectionAdjustmentRect.Width() == 0 ? 0 : m_SelectionAdjustmentRect.Height() * NewRect.Width() / m_SelectionAdjustmentRect.Width();
					NewWidth = m_SelectionAdjustmentRect.Height() == 0 ? 0 : m_SelectionAdjustmentRect.Width() * NewRect.Height() / m_SelectionAdjustmentRect.Height();

					if( fabs( NewWidth ) > fabs( NewRect.Width() ) )
						NewRect.left = NewRect.right - NewWidth;
					else
						NewRect.bottom = NewRect.top + NewHeight;
					break;
				case AC_BOTTOM_RIGHT:
					NewRect.right = AdjustedPoint.x;
					NewRect.bottom = AdjustedPoint.y;
					NewHeight = m_SelectionAdjustmentRect.Width() == 0 ? 0 : m_SelectionAdjustmentRect.Height() * NewRect.Width() / m_SelectionAdjustmentRect.Width();
					NewWidth = m_SelectionAdjustmentRect.Height() == 0 ? 0 : m_SelectionAdjustmentRect.Width() * NewRect.Height() / m_SelectionAdjustmentRect.Height();

					if( fabs( NewWidth ) > fabs( NewRect.Width() ) )
						NewRect.right = NewRect.left + NewWidth;
					else
						NewRect.bottom = NewRect.top + NewHeight;
					break;
			}
			break;
		}
	}

	if( !pDoc->StretchSelected( m_SelectionAdjustmentRect, NewRect, Direction ) )
		return;

	double Ratio1 = NewRect.Width() / m_SelectionAdjustmentRect.Width();
	double Ratio2 = NewRect.Height() / m_SelectionAdjustmentRect.Height();
	if( isnan( Ratio1 ) || ( !isnan( Ratio2 ) && Ratio2 != 1.0 ) ) // Take the second if it's better than the first.
		Ratio1 = Ratio2;

	CString Temp;
	Temp.Format( "%.3lf%%", Ratio1 * 100 );
	SetStatusText( isnan( Ratio1 ) ? "" : Temp );


	if( bEndStretch )
	{
		CFPoint Scale( NewRect.Width() / m_SelectionAdjustmentRect.Width(), NewRect.Height() / m_SelectionAdjustmentRect.Height() );

		CFPoint OldPoint = m_SelectionRotatePoint;
		CFPoint NewPoint;
		m_SelectionRotatePoint.x = NewRect.left + ( ( OldPoint.x - m_SelectionAdjustmentRect.left ) * Scale.x );
		m_SelectionRotatePoint.y = NewRect.top + ( ( OldPoint.y - m_SelectionAdjustmentRect.top ) * Scale.y );
	}
	InvalidateRect( 0 );
}

void CLinkageView::OnMouseEndChangeAdjusters(UINT nFlags, CFPoint point)
{
	m_bChangeAdjusters = false;
	if( m_bAllowEdit )
	{
		if( m_VisibleAdjustment == ADJUSTMENT_STRETCH )
			m_VisibleAdjustment = ADJUSTMENT_ROTATE;
		else if( m_VisibleAdjustment == ADJUSTMENT_ROTATE )
			m_VisibleAdjustment = ADJUSTMENT_STRETCH;
	}
}

void CLinkageView::OnMouseEndDrag(UINT nFlags, CFPoint point)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pDoc->FinishChangeSelected();

	int TotalSelected = pDoc->GetSelectedLinkCount( false ) + pDoc->GetSelectedConnectorCount();
	if( TotalSelected > 0 )
	{
		CFPoint TempPoint( m_SelectionContainerRect.left, m_SelectionContainerRect.top );
		m_SelectionRotatePoint -= TempPoint;
		m_SelectionContainerRect = GetDocumentArea( false, true );
		TempPoint.SetPoint( m_SelectionContainerRect.left, m_SelectionContainerRect.top );
		m_SelectionRotatePoint += TempPoint;

		ShowSelectedElementStatus();
	}

	if( m_bMouseMovedEnough && m_bSnapOn && m_bAutoJoin &&
	    pDoc->GetSelectedLinkCount( true ) == 0 && pDoc->GetSelectedConnectorCount() == 1 )
	{
		pDoc->AutoJoinSelected();
	}

	m_SelectionContainerRect = GetDocumentArea( false, true );
	m_SelectionAdjustmentRect = GetDocumentAdjustArea( true );
	ShowSelectedElementCoordinates();
	SelectionChanged();

	InvalidateRect( 0 );
}

void CLinkageView::ShowSelectedElementStatus( void )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CElement *pElement = 0;

	if( pDoc->GetSelectedConnectorCount() == 1 )
	{
		pElement = pDoc->GetSelectedConnector( 0 );
	}
	else if( pDoc->GetSelectedLinkCount( false ) == 1 )
	{
		pElement = pDoc->GetSelectedLink( 0, false );
	}
	else
		return;

	if( pElement == 0 )
		return;

	//CString String;
	//if( pElement->IsLink() )
	//	String = "Link ";
	//else if( pElement->IsConnector() )
	//	String = "Connector ";
	//
	//String += pElement->GetIdentifierString( m_bShowDebug );

	// SetStatusText( GetElementFullDescription( pElement ) );
}

CString CLinkageView::GetElementFullDescription( CElement *pElement )
{
	CString String = pElement->GetTypeString();
	String += " ";
	String += pElement->GetIdentifierString( m_bShowDebug );
	return String;
}

void CLinkageView::OnMouseEndSelect(UINT nFlags, CFPoint point)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	/*
	 * Normalize the rectangle for selection. Other code might not handle
	 * it otherwise.
	 */

	if( m_SelectRect.left > m_SelectRect.right )
	{
		double Temp = m_SelectRect.left;
		m_SelectRect.left = m_SelectRect.right;
		m_SelectRect.right = Temp;
	}
	if( m_SelectRect.top > m_SelectRect.bottom )
	{
		double Temp = m_SelectRect.top;
		m_SelectRect.top = m_SelectRect.bottom;
		m_SelectRect.bottom = Temp;
	}

	CFRect Rect = Unscale( m_SelectRect );

	m_bChangeAdjusters = false;
	bool bMultiSelect = ( nFlags & ( MK_CONTROL | MK_SHIFT ) ) != 0;

	bool bSelectionChanged = false;
	m_MouseAction = pDoc->SelectElement( Rect, 0, bMultiSelect, bSelectionChanged ) ? ACTION_DRAG : ACTION_NONE;

	if( pDoc->IsSelectionAdjustable() )
    {
		//SetStatusText( m_MouseAction == ACTION_ROTATE ? "0.0000°" : ( m_MouseAction == ACTION_STRETCH ? "100.000%" : "" ) );

		if( bSelectionChanged )
		{
			m_VisibleAdjustment = ADJUSTMENT_STRETCH;
			m_SelectionContainerRect = GetDocumentArea( false, true );
			m_SelectionAdjustmentRect = GetDocumentAdjustArea( true );
			m_SelectionRotatePoint.SetPoint( ( m_SelectionContainerRect.left + m_SelectionContainerRect.right ) / 2,
											 ( m_SelectionContainerRect.top + m_SelectionContainerRect.bottom ) / 2 );
		}
	}
	else
		m_VisibleAdjustment = ADJUSTMENT_NONE;

	ShowSelectedElementStatus();

	ShowSelectedElementCoordinates();
	SelectionChanged();
	InvalidateRect( 0 );
}

void CLinkageView::OnMouseEndPan(UINT nFlags, CFPoint point)
{
	InvalidateRect( 0 );
}

void CLinkageView::OnMouseEndRotate(UINT nFlags, CFPoint point)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->FinishChangeSelected();
	m_SelectionContainerRect = GetDocumentArea( false, true );
	m_SelectionAdjustmentRect = GetDocumentAdjustArea( true );
	// SelectionChanged(); // Is it good to reset the and in the status bar to zero? maybe not so comment this out.
	InvalidateRect( 0 );
}

void CLinkageView::OnMouseEndRecenter(UINT nFlags, CFPoint point)
{
	InvalidateRect( 0 );
}

void CLinkageView::OnMouseEndStretch(UINT nFlags, CFPoint point)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->FinishChangeSelected();
	m_SelectionContainerRect = GetDocumentArea( false, true );
	m_SelectionAdjustmentRect = GetDocumentAdjustArea( true );
	// SelectionChanged(); // Is it good to reset the stretch to 100% in the status bar? Maybe not so comment this out.
	InvalidateRect( 0 );
}

void CLinkageView::SetLocationAsStatus( CFPoint &Point )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if( pDoc->IsAnySelected() )
		return;

	CString Location;
	double DocumentScale = pDoc->GetUnitScale();

	Location.Format( "%.3lf,%.3lf", Point.x * DocumentScale, Point.y * DocumentScale );
	SetStatusText( Location );
}

void CLinkageView::OnMouseMove(UINT nFlags, CPoint MousePoint)
{
	CFPoint point = AdjustClientAreaPoint( MousePoint );

	if( m_PreviousDragPoint == point )
		return;

	if( m_bSimulating && m_MouseAction != ACTION_PAN )
		return;

	if( !m_bMouseMovedEnough )
	{
		if( abs( point.x - m_DragStartPoint.x ) < 2
			&& abs( point.y - m_DragStartPoint.y ) < 2 )
			return;

		m_bMouseMovedEnough = true;

		if( m_MouseAction == ACTION_DRAG
			|| m_MouseAction == ACTION_ROTATE
			|| m_MouseAction == ACTION_STRETCH )
		{
			CLinkageDoc* pDoc = GetDocument();
			ASSERT_VALID(pDoc);
			pDoc->StartChangeSelected();
		}
	}

	if( m_MouseAction != ACTION_DRAG && m_MouseAction != ACTION_ROTATE && m_MouseAction != ACTION_STRETCH )
	{
		CFPoint DocumentPoint = Unscale( point );
		SetLocationAsStatus( DocumentPoint );
	}

	if( m_MouseAction == ACTION_NONE )
		return;

	switch( m_MouseAction )
	{
		case ACTION_SELECT: OnMouseMoveSelect( nFlags, point ); break;
		case ACTION_DRAG: OnMouseMoveDrag( nFlags, point ); break;
		case ACTION_PAN: OnMouseMovePan( nFlags, point ); break;
		case ACTION_ROTATE: OnMouseMoveRotate( nFlags, point ); break;
		case ACTION_RECENTER: OnMouseMoveRecenter( nFlags, point ); break;
		case ACTION_STRETCH: OnMouseMoveStretch( nFlags, point, false ); break;
		default: break;
	}

	m_bChangeAdjusters = false;

	m_PreviousDragPoint = point;
}

void CLinkageView::OnMechanismReset()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pDoc->Reset( false );
	m_Simulator.Reset();
	UpdateForDocumentChange();
}

DWORD TickCount;

void CALLBACK TimeProc( UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2 )
{
	TickCount = GetTickCount();
	CLinkageView* pView = (CLinkageView*)dwUser;
	pView->SendMessage( WM_TIMER, 0, 0 );
}

void CALLBACK TimerQueueCallback( void *pParameter, BOOLEAN bTimerOrWaitFired )
{
	TickCount = GetTickCount();
	CLinkageView* pView = (CLinkageView*)pParameter;
	pView->SendMessage( WM_TIMER, 0, 0 );
}

bool CLinkageView::InitializeVideoFile( const char *pVideoFile, const char *pCompressorString )
{
	CString FCC = "CVID";
	const char *pFCC = strchr( pCompressorString, '(' );
	if( pFCC != 0 )
	{
		FCC.Truncate( 0 );
		FCC.Append( pFCC+1, 4 );
	}

	m_pAvi = new CAviFile( pVideoFile, mmioFOURCC( FCC[0], FCC[1], FCC[2], FCC[3] ), 30 );

	return true;
}

void CLinkageView::SaveVideoFrame( CRenderer *pRenderer, CRect &CaptureRect )
{
	if( m_pAvi == 0 )
		return;

	bool bHDScaling = m_RecordQuality > 0;

	double ScaleFactor = pRenderer->GetScale();

	int Width = bHDScaling ? 1920 : ANIMATIONWIDTH;
	int Height = bHDScaling ? 1080 : ANIMATIONHEIGHT;

	CDIBSection *pVideoBitmap = new CDIBSection;
	if( pVideoBitmap == 0 )
		return;

	if( !pVideoBitmap->CreateDibSection( pRenderer->GetDC(), Width, Height ) )
		return;

	if( !bHDScaling && ( CaptureRect.Width() < Width || CaptureRect.Height() < Height ) )
		pVideoBitmap->m_DC.PatBlt( 0, 0, (int)( Width * ScaleFactor ), (int)( Height * ScaleFactor ), WHITENESS );

	int x = bHDScaling ? 0 : ( CaptureRect.Width() / 2 ) - ( Width / 2 );
	int y = bHDScaling ? 0 : ( CaptureRect.Height() / 2 ) - ( Height / 2 );

	if( ScaleFactor == 1.0 && !bHDScaling )
		pVideoBitmap->m_DC.BitBlt( 0, 0, Width, Height, pRenderer->GetDC(), x, y, SRCCOPY );
	else
	{
		StretchBltPlus( pVideoBitmap->m_DC.GetSafeHdc(),
		                0, 0, Width, Height,
		                pRenderer->GetSafeHdc(),
						(int)( x * ScaleFactor ), (int)( y * ScaleFactor ),
						(int)( ANIMATIONWIDTH * ScaleFactor ), (int)( ANIMATIONHEIGHT * ScaleFactor ),
						SRCCOPY );
	}

	DWORD Ticks = GetTickCount();
	while( m_pVideoBitmapQueue != 0 && GetTickCount() < Ticks + 2000 )
		Sleep( 1 );

	if( m_pVideoBitmapQueue == 0 && StartVideoThread() )
	{
		m_pAvi->AppendNewFrame( pVideoBitmap, 1 );
		delete pVideoBitmap;
		//m_pVideoBitmapQueue = pVideoBitmap;
		//SetEvent( m_hVideoFrameEvent );
	}
	else
		delete pVideoBitmap;
}

void CLinkageView::VideoThreadFunction( void )
{
	CoInitialize( 0 );

	while( !m_bRequestAbort )
	{
		if( WaitForSingleObject( m_hVideoFrameEvent, 500 ) != WAIT_TIMEOUT )
		{
			if( m_pVideoBitmapQueue != 0 )
			{
				m_pAvi->AppendNewFrame( m_pVideoBitmapQueue, 1 );
				delete m_pVideoBitmapQueue;
				m_pVideoBitmapQueue = 0;
			}
		}
	}

	CoUninitialize();

	m_bProcessingVideoThread = false;
}

unsigned int __stdcall VideoThreadFunction( void* pInfo )
{
	if( pInfo == 0 )
		return 0;

	CLinkageView *pView = (CLinkageView*)pInfo;
	pView->VideoThreadFunction();

	return 0;
}

bool CLinkageView::StartVideoThread( void )
{
	if( m_bProcessingVideoThread )
		return true;

	m_bProcessingVideoThread = true;
	uintptr_t ThreadHandle;
	ThreadHandle = _beginthreadex( NULL, 0, ::VideoThreadFunction, this, 0, NULL );
	if( ThreadHandle == 0 )
	{
		m_bProcessingVideoThread = false;
		return false;
	}
	CloseHandle( (HANDLE)ThreadHandle );
	return true;
}

void CLinkageView::CloseVideoFile( void )
{
	if( !m_bRecordingVideo )
		return;

	m_bRecordingVideo = false;

	DWORD Ticks = GetTickCount();
	while( m_pVideoBitmapQueue != 0 && GetTickCount() < Ticks + 60000 )
		Sleep( 1 );

	if( m_pAvi != 0 )
	{
		delete m_pAvi;
		m_pAvi = 0;
	}
}

bool CLinkageView::AnyAlwaysManual( void )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	ConnectorList* pConnectors = pDoc->GetConnectorList();
	POSITION Position = pConnectors->GetHeadPosition();
	while( Position != NULL )
	{
		CConnector* pConnector = pConnectors->GetNext( Position );
		if( pConnector != 0 && pConnector->IsInput() && pConnector->IsAlwaysManual() )
			return true;
	}

	LinkList* pLinkList = pDoc->GetLinkList();
	Position = pLinkList->GetHeadPosition();
	while( Position != NULL )
	{
		CLink* pLink = pLinkList->GetNext( Position );
		if( pLink != 0 && pLink->IsActuator() && pLink->IsAlwaysManual() )
			return true;
	}

	return false;
}

DWORD TimeArray[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int TimerArrayIndex = 1;
DWORD LastTickTimeArray = 0;
DWORD FirstTimeArrayTick = 0;

void CLinkageView::StartMechanismSimulate( enum _SimulationControl SimulationControl, int StartStep )
{
	SetFocus();

	if( m_bSimulating )
		return;

	if( m_TimerID != 0 )
		timeKillEvent( m_TimerID );

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	//pDoc->SelectElement();
	InvalidateRect( 0 );
	m_MouseAction = ACTION_NONE;

	DebugItemList.Clear();

	m_bWasValid = true;
	if( m_bRecordingVideo )
		SetStatusText( "Recording simulation video." );
	else
		SetStatusText( "Running simulation." );

	pDoc->Reset( true );

	m_Simulator.Reset();
	m_Simulator.Options( m_bUseMoreMomentum );
	m_bSimulating = true;
	m_SimulationControl = SimulationControl;
	m_SimulationSteps = StartStep;
	SetScrollExtents( false );
	m_ControlWindow.ShowWindow( m_ControlWindow.GetControlCount() > 0 ? SW_SHOWNORMAL : SW_HIDE );

	TimeArray[0] = GetTickCount();
	LastTickTimeArray = GetTickCount();
	FirstTimeArrayTick = GetTickCount();
	TimerArrayIndex = 0;

	//StartTimer();

	m_TimerID = timeSetEvent( 33, 1, TimeProc, (DWORD_PTR)this, 0 );
}

void CLinkageView::StopMechanismSimulate( bool KeepCurrentPositions )
{
	if( !m_bSimulating )
		return;

	CloseVideoFile();

	ClearDebugItems();

	if( m_TimerID != 0 )
		timeKillEvent( m_TimerID );

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	//pDoc->SelectElement();
	InvalidateRect( 0 );
	m_MouseAction = ACTION_NONE;

	SetStatusText();
	m_bSimulating = false;
	m_SimulationSteps = 0;
	SetScrollExtents();
	m_ControlWindow.ShowWindow( SW_HIDE );

	pDoc->Reset( false, KeepCurrentPositions );
	m_Simulator.Reset();

	UpdateForDocumentChange();
}

bool CLinkageView::CanSimulate( void )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CString Error;
	if( !pDoc->CanSimulate( Error ) )
	{
		AfxMessageBox( Error );
		return false;
	}
	return true;
}

void CLinkageView::OnMechanismSimulate()
{
	if( !CanSimulate() )
		return;
	ConfigureControlWindow( AUTO );
	StartMechanismSimulate( AUTO );
}

void CLinkageView::ConfigureControlWindow( enum _SimulationControl SimulationControl )
{
	m_ControlWindow.Clear();

	if( SimulationControl == GLOBAL )
		m_ControlWindow.AddControl( -1, "Mechanism", -1 );

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	ConnectorList* pConnectors = pDoc->GetConnectorList();
	POSITION Position = pConnectors->GetHeadPosition();
	while( Position != NULL )
	{
		CConnector* pConnector = pConnectors->GetNext( Position );
		if( pConnector == 0 )
			continue;
		if( !pConnector->IsInput() )
			continue;
		if( SimulationControl == INDIVIDUAL || pConnector->IsAlwaysManual() )
		{
			CString String = GetElementFullDescription( pConnector );
			//String.Format( "Connector %s", (const char*)pConnector->GetIdentifierString( m_bShowDebug ) );
			m_ControlWindow.AddControl( 10000 + pConnector->GetIdentifier(), String, 10000 + pConnector->GetIdentifier(), true );
		}
	}

	LinkList* pLinkList = pDoc->GetLinkList();
	Position = pLinkList->GetHeadPosition();
	while( Position != NULL )
	{
		CLink* pLink = pLinkList->GetNext( Position );
		if( pLink == 0 )
			continue;
		if( !pLink->IsActuator() )
			continue;
		if( SimulationControl == INDIVIDUAL || pLink->IsAlwaysManual() )
		{
			CString String = GetElementFullDescription( pLink );
			//String.Format( "Link %s", pLink->GetIdentifierString( m_bShowDebug ) );
			m_ControlWindow.AddControl( pLink->GetIdentifier(), String, pLink->GetIdentifier(), false, pLink->GetCPM() >= 0 ? 0.0 : 1.0 );
		}
	}

	int Height = m_ControlWindow.GetDesiredHeight();
	if( Height > m_DrawingRect.Height() )
		Height = m_DrawingRect.Height();
	m_ControlWindow.SetWindowPos( 0, 0, m_DrawingRect.Height() - Height, m_DrawingRect.Width(), m_DrawingRect.Height(), SWP_NOZORDER );
}

void CLinkageView::OnControlWindow()
{
}

void CLinkageView::OnUpdateMechanismQuicksim(CCmdUI *pCmdUI)
{
	pCmdUI->Enable( !m_bSimulating && m_bAllowEdit );
}

void CLinkageView::OnUpdateMechanismSimulate(CCmdUI *pCmdUI)
{
	pCmdUI->Enable( !m_bSimulating && m_bAllowEdit );
}

void CLinkageView::StepSimulation( enum _SimulationControl SimulationControl )
{
	int ControlCount = m_ControlWindow.GetControlCount();
	if( ( SimulationControl == GLOBAL || SimulationControl == INDIVIDUAL ) && ControlCount == 0 )
		return;

	int *pControlIDs = 0;
	double *pPositions = 0;
	if( ControlCount > 0 )
	{
		pControlIDs = new int[ControlCount];
		if( pControlIDs == 0 )
			return;
		pPositions = new double[ControlCount];
		if( pPositions == 0 )
		{
			delete [] pControlIDs;
			return;
		}
		for( int Index = 0; Index < ControlCount; ++Index )
		{
			pControlIDs[Index] = m_ControlWindow.GetControlId( Index );
			pPositions[Index] = m_ControlWindow.GetControlPosition( Index );
		}
	}

	CLinkageDoc* pDoc = GetDocument();

	DWORD SimulationTimeLimit = 100;
	DWORD SimulationStartTicks = GetTickCount();
	double ForceCPM = 0;

	bool bSetToAbsoluteStep = false;

	if( SimulationControl == AUTO )
		m_SimulationSteps = 1;
	else if( SimulationControl == ONECYCLE )
	{
		if( m_SimulationSteps >= m_PauseStep )
			return;
		++m_SimulationSteps;
		bSetToAbsoluteStep = true;
		ForceCPM = m_ForceCPM;
	}
	else if( SimulationControl == ONECYCLEX )
	{
		// This is only used to show the end of the simuation after the pause step is reached.
		return;
	}
	else if( SimulationControl == GLOBAL )
	{
		static const int MAX_MANUAL_STEPS = 800;
		int Steps = min( m_Simulator.GetSimulationSteps( pDoc ), MAX_MANUAL_STEPS );
		double Position = m_ControlWindow.GetControlPosition( 0 );
		m_SimulationSteps = (int)( Steps * Position );
		bSetToAbsoluteStep = true;
	}
	else if( SimulationControl == INDIVIDUAL )
	{
//		if( ControlCount > 0 )
//			m_SimulationSteps = m_Simulator.GetUnfinishedSimulationSteps();
	}

	//if( SimulationControl != INDIVIDUAL )
	//	ControlCount = 0;

	if( m_SimulationSteps != 0 || SimulationControl == INDIVIDUAL || bSetToAbsoluteStep )
	{
		ClearDebugItems();
		m_Simulator.SimulateStep( pDoc, m_SimulationSteps, bSetToAbsoluteStep, pControlIDs, pPositions, ControlCount, AnyAlwaysManual() && SimulationControl != INDIVIDUAL, ForceCPM );
	}

	if( !bSetToAbsoluteStep )
		m_SimulationSteps = 0;

	if( pControlIDs != 0 )
		delete [] pControlIDs;
	if( pPositions != 0 )
		delete [] pPositions;
}

void CLinkageView::OnTimer(UINT_PTR nIDEvent)
{
	if( !m_bSimulating )
		return;

	DWORD Temp = GetTickCount();
	if( TimerArrayIndex < 9  )
	{
		TimeArray[TimerArrayIndex] = Temp - LastTickTimeArray;
		LastTickTimeArray = Temp;
		++TimerArrayIndex;
	}
	else
	{
		FirstTimeArrayTick = Temp - FirstTimeArrayTick;
		TimeArray[9] = 0;
		CString Derp;
		Derp.Format( "%u", FirstTimeArrayTick );
		//AfxMessageBox( Derp );
	}

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// Step the simuation as long as the video is not getting extra pause frames before or after.
	if( !m_bRecordingVideo || !m_bUseVideoCounters || ( m_VideoStartFrames == 0 && m_VideoRecordFrames != 0 ) )
		StepSimulation( m_SimulationControl );

	// Redraw without invalidation. This makes things a little more exact.
	//InvalidateRect( 0 );

	CClientDC dc( this );
	OnDraw( &dc );

	if( m_bRecordingVideo && m_pAvi != 0 && m_pAvi->GetLastError() != 0 )
	{
		CString AviError = m_pAvi->GetLastErrorMessage();
		StopMechanismSimulate();
		AfxMessageBox( AviError, MB_ICONEXCLAMATION | MB_OK );
		return;
	}

	if( !m_Simulator.IsSimulationValid() && m_bWasValid )
	{
		SetStatusText( "Invalid mechanism. Simulation paused." );
		m_bWasValid = false;
	}

	if( m_bRecordingVideo )
	{
		TickCount = 0;
		if( m_bUseVideoCounters )
		{
			CString Status;
			int TotalFrames = m_VideoStartFrames + m_VideoRecordFrames + m_VideoEndFrames;
			double Seconds = (double)TotalFrames / 30.0;
			Status.Format( "Recording simulation video. %.1lf more second%s of video to go.", Seconds, Seconds == 1.0 ? "" : "s" );

			if( m_VideoStartFrames > 0 )
				--m_VideoStartFrames;
			else if( m_VideoRecordFrames > 0 )
				--m_VideoRecordFrames;
			else if( m_VideoEndFrames > 0 )
				--m_VideoEndFrames;

			if( !Status.IsEmpty() )
				SetStatusText( Status );

			if( m_VideoStartFrames == 0 && m_VideoRecordFrames == 0 && m_VideoEndFrames == 0 )
				StopMechanismSimulate();
		}
	}

	if( !m_bSimulating )
		return;

	DWORD Derf = GetTickCount();
	DWORD Adjust = Derf - TickCount;
	if( Adjust > 32 )
		Adjust = 32;
	TickCount = Derf;

	m_TimerID = timeSetEvent( 33 - Adjust, 1, TimeProc, (DWORD_PTR)this, 0 );
}

void CLinkageView::OnUpdateMechanismReset(CCmdUI *pCmdUI)
{
	pCmdUI->Enable( !m_bSimulating && m_bAllowEdit );
}

void CLinkageView::OnUpdateAlignButton(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Selected = pDoc->GetSelectedConnectorCount();
	if( pDoc->GetSelectedLinkCount( true ) > 0 )
		Selected += 2;

	bool bEnable = pDoc->GetAlignConnectorCount() > 1
				   || Selected > 1
				   || pDoc->IsSelectionAngleable()
				   || pDoc->IsSelectionRectangle()
				   || pDoc->IsSelectionAdjustable()
				   || pDoc->IsSelectionLengthable()
				   || pDoc->IsSelectionLineable()
				   || pDoc->IsSelectionMeshableGears()
				   || pDoc->IsSelectionPositionable()
				   || pDoc->IsSelectionRotatable()
				   || pDoc->IsSelectionScalable();


	pCmdUI->Enable( !m_bSimulating && bEnable && m_bAllowEdit );
}

void CLinkageView::OnUpdateEdit(CCmdUI *pCmdUI)
{
	pCmdUI->Enable( !m_bSimulating && m_bAllowEdit );
}

void CLinkageView::OnUpdateNotSimulating(CCmdUI *pCmdUI)
{
	pCmdUI->Enable( !m_bSimulating );
}

void CLinkageView::OnUpdateEditSelected(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	int Selected = pDoc->GetSelectedConnectorCount() + pDoc->GetSelectedLinkCount( false );
	pCmdUI->Enable( !m_bSimulating && Selected != 0  && m_bAllowEdit );
}

void CLinkageView::OnUpdateEditConnect(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( !m_bSimulating && pDoc->IsSelectionConnectable() && m_bAllowEdit );
}

void CLinkageView::OnUpdateEditFasten(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( !m_bSimulating && pDoc->IsSelectionFastenable() && m_bAllowEdit );
}

void CLinkageView::OnUpdateEditUnfasten(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( !m_bSimulating && pDoc->IsSelectionUnfastenable() && m_bAllowEdit );
}

void CLinkageView::OnUpdateEditJoin(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( !m_bSimulating && pDoc->IsSelectionJoinable() && m_bAllowEdit );
}

void CLinkageView::OnUpdateEditLock(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( !m_bSimulating && pDoc->IsSelectionLockable() && m_bAllowEdit );
}

void CLinkageView::OnUpdateEditCombine(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( !m_bSimulating && pDoc->IsSelectionCombinable() && m_bAllowEdit );
}

void CLinkageView::OnEditCombine()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->CombineSelected();
	InvalidateRect( 0 );
}

afx_msg void CLinkageView::OnEditmakeAnchor()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->MakeAnchorSelected();
	InvalidateRect( 0 );
}

afx_msg void CLinkageView::OnUpdateEditmakeAnchor(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( !m_bSimulating && pDoc->IsSelectionMakeAnchor() && m_bAllowEdit );
}

void CLinkageView::OnEditConnect()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CLink *pLink = pDoc->ConnectSelected();
	if( pLink != 0 && m_bNewLinksSolid )
		pLink->SetSolid( true );
	InvalidateRect( 0 );
}

void CLinkageView::OnEditSelectLink()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if( pDoc->SelectLinkFromConnectors() )
		InvalidateRect( 0 );
}

void CLinkageView::OnUpdateEditSelectLink(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( !m_bSimulating && pDoc->GetSelectedConnectorCount() > 1 && pDoc->GetSelectedLinkCount( false ) == 0  && m_bAllowEdit );
}

void CLinkageView::OnEditFasten()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->FastenSelected();
	InvalidateRect( 0 );
}

void CLinkageView::OnEditUnfasten()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->UnfastenSelected();
	InvalidateRect( 0 );
}


void CLinkageView::OnUpdateEditSetLocation(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( !m_bSimulating && pDoc->IsSelectionPositionable() && m_bAllowEdit );
}

void CLinkageView::OnEditSetLocation()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	double DocumentScale = pDoc->GetUnitScale();

	CLocationDialog dlg;
	CConnector *pConnector = pDoc->GetSelectedConnector( 0 );
	if( pConnector == 0 )
		return;

	dlg.m_PromptText = GetElementFullDescription( pConnector );
	dlg.m_xPosition = pConnector->GetPoint().x * DocumentScale;
	dlg.m_yPosition = pConnector->GetPoint().y * DocumentScale;
	
	if( dlg.DoModal() == IDOK )
	{
		if( dlg.m_xPosition != pConnector->GetPoint().x ||
		    dlg.m_yPosition != pConnector->GetPoint().y )
		{
			pDoc->PushUndo();
			pConnector->SetPoint( dlg.m_xPosition / DocumentScale, dlg.m_yPosition / DocumentScale );
			pDoc->SetModifiedFlag( true );
			SetScrollExtents();
			InvalidateRect( 0 );
		}
	}
}

void CLinkageView::OnUpdateEditSetLength(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( !m_bSimulating && pDoc->IsSelectionLengthable() && m_bAllowEdit );
}

void CLinkageView::OnEditSetLength()
{
	CLengthDistanceDialog dlg;
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	dlg.m_Distance = pDoc->GetSelectedElementCoordinates( 0 );
	if( dlg.DoModal() == IDOK )
	{
		pDoc->SetSelectedElementCoordinates( &m_SelectionRotatePoint, dlg.m_Distance );
		UpdateForDocumentChange();
	}
}

void CLinkageView::OnUpdateEditSetAngle(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( !m_bSimulating && pDoc->IsSelectionTriangle() && m_bAllowEdit );
}

void CLinkageView::OnEditSetAngle()
{
	CAngleDialog dlg;
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	dlg.m_Angle = pDoc->GetSelectedElementCoordinates( 0 );
	if( dlg.DoModal() == IDOK )
	{
	}
}

void CLinkageView::OnUpdateEditRotate(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( !m_bSimulating && pDoc->IsSelectionRotatable() && m_bAllowEdit );
}

void CLinkageView::OnEditRotate()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CRotateDialog dlg;
	dlg.m_Angle = "0.0";
	if( dlg.DoModal() == IDOK )
	{
		CString Temp = dlg.m_Angle + "*";
		pDoc->SetSelectedElementCoordinates( &m_SelectionRotatePoint, Temp );
		UpdateForDocumentChange();
	}
}

void CLinkageView::OnUpdateEditScale(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( !m_bSimulating && pDoc->IsSelectionScalable() && m_bAllowEdit );
}

void CLinkageView::OnEditScale()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CScaleDialog dlg;
	dlg.m_Scale = "100";
	if( dlg.DoModal() == IDOK )
	{
		CString Temp = dlg.m_Scale + "%";
		pDoc->SetSelectedElementCoordinates( &m_SelectionRotatePoint, Temp );
		UpdateForDocumentChange();
	}
}

void CLinkageView::OnUpdateEditSetRatio(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( !m_bSimulating && pDoc->IsSelectionMeshableGears() && m_bAllowEdit );
}

void CLinkageView::OnEditSetRatio()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if( !pDoc->IsSelectionMeshableGears() )
		return;

	CGearRatioDialog dlg;

	// We know that there an be only two links selected at this time.
	CLink *pLink1 = pDoc->GetSelectedLink( 0, false );
	CLink *pLink2 = pDoc->GetSelectedLink( 1, false );

	CGearConnection *pConnection = pDoc->GetGearRatio( pLink1, pLink2, &dlg.m_Gear1Size, &dlg.m_Gear2Size );

	if( pConnection == 0 )
	{
		dlg.m_Gear1Size = 1.0;
		dlg.m_Gear2Size = 1.0;
		dlg.m_GearChainSelection = 0;
		dlg.m_bUseRadiusValues = false;
	}
	else
	{
		dlg.m_GearChainSelection = pConnection->m_ConnectionType == CGearConnection::GEARS ? 0 : 1;
		dlg.m_bUseRadiusValues = pConnection->m_bUseSizeAsRadius;
	}

	dlg.m_Link1Name = pLink1->GetIdentifierString( m_bShowDebug );
	dlg.m_Link2Name = pLink2->GetIdentifierString( m_bShowDebug );

	if( dlg.DoModal() == IDOK )
	{
		if( pDoc->SetGearRatio( pLink1, pLink2, dlg.m_Gear1Size, dlg.m_Gear2Size, dlg.m_bUseRadiusValues && dlg.m_GearChainSelection == 1, dlg.m_GearChainSelection == 0 ? CGearConnection::GEARS : CGearConnection::CHAIN ) )
			UpdateForDocumentChange();
	}
	InvalidateRect( 0 );
}

void CLinkageView::OnEditDeleteselected()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->DeleteSelected();
	UpdateForDocumentChange();
	SelectionChanged();
}

BOOL CLinkageView::OnEraseBkgnd(CDC* pDC)
{
	return 0;
}

void CLinkageView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	SetScrollExtents();

	int Height = m_ControlWindow.GetDesiredHeight();
	if( Height > cy )
		Height = cy;
	m_ControlWindow.SetWindowPos( 0, 0, cy - Height, cx, cy, SWP_NOZORDER );

	GetClientRect( &m_DrawingRect );
}

void CLinkageView::OnUpdateButtonRun(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( ( !m_bSimulating || m_SimulationControl == STEP ) && !pDoc->IsEmpty() && m_bAllowEdit );
}

void CLinkageView::OnUpdateButtonStop(CCmdUI *pCmdUI)
{
	pCmdUI->Enable( m_bSimulating );
}

void CLinkageView::OnButtonRun()
{
	if( !CanSimulate() )
		return;
	if( m_bSimulating )
	{
		if( m_SimulationControl == STEP )
			m_SimulationControl = AUTO;
	}
	else
	{
		ConfigureControlWindow( AUTO );
		StartMechanismSimulate( AUTO );
	}
}

void CLinkageView::StopSimulation()
{
	StopMechanismSimulate();
}

void CLinkageView::OnButtonStop()
{
	StopMechanismSimulate();
}

void CLinkageView::OnButtonPin()
{
	StopMechanismSimulate( true );
}

void CLinkageView::OnUpdateButtonPin(CCmdUI *pCmdUI)
{
	pCmdUI->Enable( m_bSimulating );
}

CFPoint CLinkageView::GetDocumentViewCenter( void )
{
	CFPoint Point( ( m_DrawingRect.left + m_DrawingRect.right ) / 2, ( m_DrawingRect.top + m_DrawingRect.bottom ) / 2 );

	return Unscale( Point );
}

void CLinkageView::InsertPoint( CFPoint *pPoint )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->InsertConnector( CLinkageDoc::DRAWINGLAYER, Unscale( LINK_INSERT_PIXELS ), pPoint == 0 ? GetDocumentViewCenter() : *pPoint, pPoint != 0 );
	m_bSuperHighlight = true;

	UpdateForDocumentChange();
	SelectionChanged();
}

void CLinkageView::InsertLine( CFPoint *pPoint )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->InsertLink( CLinkageDoc::DRAWINGLAYER, Unscale( LINK_INSERT_PIXELS ), pPoint == 0 ? GetDocumentViewCenter() : *pPoint, pPoint != 0, 2, false );
	m_bSuperHighlight = true;

	UpdateForDocumentChange();
	SelectionChanged();
}

void CLinkageView::InsertMeasurement( CFPoint *pPoint )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CConnector *pConnector0 = pDoc->GetSelectedConnector( 0 );
	CConnector *pConnector1 = pDoc->GetSelectedConnector( 1 );
	bool bFastenIt = pDoc->GetSelectedConnectorCount() == 2 && pDoc->GetSelectedLinkCount( true ) == 0;
	CLink *pLink = pDoc->InsertMeasurement( CLinkageDoc::DRAWINGLAYER, Unscale( LINK_INSERT_PIXELS ), pPoint == 0 ? GetDocumentViewCenter() : *pPoint, pPoint != 0 );
	m_bSuperHighlight = true;

	if( bFastenIt && pLink != 0 )
	{
		CConnector *pStart = pLink->GetConnector( 0 );
		CConnector *pEnd = pLink->GetConnector( 1 );
		if( pStart != 0 && pEnd != 0 )
		{
			pDoc->FastenThese( pStart, pConnector0 );
			pStart->SetPoint( pConnector0->GetPoint() );
			pDoc->FastenThese( pEnd, pConnector1 );
			pEnd->SetPoint( pConnector1->GetPoint() );
		}
	}

	UpdateForDocumentChange();
	SelectionChanged();
}

void CLinkageView::InsertGear( CFPoint *pPoint )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->InsertGear( CLinkageDoc::MECHANISMLAYER, Unscale( LINK_INSERT_PIXELS ), pPoint == 0 ? GetDocumentViewCenter() : *pPoint, pPoint != 0 );
	m_bSuperHighlight = true;

	UpdateForDocumentChange();
	SelectionChanged();
}

void CLinkageView::InsertConnector( CFPoint *pPoint )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->InsertConnector( CLinkageDoc::MECHANISMLAYER, Unscale( LINK_INSERT_PIXELS ), pPoint == 0 ? GetDocumentViewCenter() : *pPoint, pPoint != 0 );
	m_bSuperHighlight = true;

	UpdateForDocumentChange();
	SelectionChanged();
}

void CLinkageView::InsertCircle( CFPoint *pPoint )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->InsertCircle( CLinkageDoc::DRAWINGLAYER, Unscale( LINK_INSERT_PIXELS ), pPoint == 0 ? GetDocumentViewCenter() : *pPoint, pPoint != 0 );
	m_bSuperHighlight = true;

	UpdateForDocumentChange();
	SelectionChanged();
}

void CLinkageView::InsertAnchor( CFPoint *pPoint )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->InsertAnchor( CLinkageDoc::MECHANISMLAYER, Unscale( LINK_INSERT_PIXELS ), pPoint == 0 ? GetDocumentViewCenter() : *pPoint, pPoint != 0, m_bNewLinksSolid );
	m_bSuperHighlight = true;

	UpdateForDocumentChange();
	SelectionChanged();
}

void CLinkageView::InsertAnchorLink( CFPoint *pPoint )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->InsertAnchorLink(  CLinkageDoc::MECHANISMLAYER, Unscale( LINK_INSERT_PIXELS ), pPoint == 0 ? GetDocumentViewCenter() : *pPoint, pPoint != 0, m_bNewLinksSolid );
	m_bSuperHighlight = true;

	UpdateForDocumentChange();
	SelectionChanged();
}

void CLinkageView::InsertRotatinganchor( CFPoint *pPoint )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->InsertRotateAnchor(  CLinkageDoc::MECHANISMLAYER, Unscale( LINK_INSERT_PIXELS ), pPoint == 0 ? GetDocumentViewCenter() : *pPoint, pPoint != 0, m_bNewLinksSolid );
	m_bSuperHighlight = true;

	UpdateForDocumentChange();
	SelectionChanged();
}

void CLinkageView::InsertLink( CFPoint *pPoint )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->InsertLink( CLinkageDoc::MECHANISMLAYER, Unscale( LINK_INSERT_PIXELS ), pPoint == 0 ? GetDocumentViewCenter() : *pPoint, pPoint != 0, 2, m_bNewLinksSolid );
	m_bSuperHighlight = true;

	UpdateForDocumentChange();
	SelectionChanged();
}

void CLinkageView::OnUpdateEditSplit(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( !m_bSimulating && pDoc->IsSelectionSplittable() && m_bAllowEdit );
}

void CLinkageView::OnUpdateEditProperties(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if( pDoc->GetSelectedLinkCount( true ) == 1  )
		pCmdUI->Enable( true );
	else if( pDoc->GetSelectedLinkCount( false ) == 1 && pDoc->GetSelectedConnectorCount() == 0 )
		pCmdUI->Enable( true );
	else if( pDoc->GetSelectedLinkCount( true ) > 1 )
		pCmdUI->Enable( true );
	else if( pDoc->GetSelectedConnectorCount() == 1 )
		pCmdUI->Enable( true );
	else
		pCmdUI->Enable( false );
}

class CSelectedConnector : public ConnectorListOperation
{
	public:
	CConnector *m_pSelectedConnector;
	CSelectedConnector() { m_pSelectedConnector = 0; }
	bool operator()( class CConnector *pConnector, bool bFirst, bool bLast )
	{
		if( pConnector->IsSelected() )
		{
			m_pSelectedConnector = pConnector;
			return false;
		}
		return true;
	}
	CConnector *GetConnector( void ) { return m_pSelectedConnector; }
};

void CLinkageView::EditProperties( CConnector *pConnector, CLink *pLink, bool bSelectedLinks )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if( pLink != 0 || bSelectedLinks )
	{
		int Layers = 0;
		int Count = 0;
		if( bSelectedLinks )
		{
			LinkList* pLinkList = pDoc->GetLinkList();
			POSITION Position = pLinkList->GetHeadPosition();
			while( Position != NULL )
			{
				pLink = pLinkList->GetNext( Position );
				if( pLink != 0 && pLink->IsSelected() )
				{
					Layers |= pLink->GetLayers();
					++Count;
				}
			}
		}
		else
		{
			Layers = pLink->GetLayers();
			Count = 1;
		}
		if( pLink != 0 )
		{
			bool bResult = false;
			if( Layers & CLinkageDoc::DRAWINGLAYER )
				bResult = LineProperties( Count > 1 ? 0 : pLink );
			else
				bResult = LinkProperties( Count > 1 ? 0 : pLink );
			if( bResult )
			{
				pDoc->SetModifiedFlag( true );
				SetScrollExtents();
				InvalidateRect( 0 );
			}
			return;
		}
	}

	if( pConnector != 0 )
	{
		bool bResult = false;
		if( pConnector->GetLayers() & CLinkageDoc::DRAWINGLAYER )
			bResult = PointProperties( pConnector );
		else
			bResult = ConnectorProperties( pConnector );
		if( bResult )
		{
			pDoc->SetModifiedFlag( true );
			SetScrollExtents();
			InvalidateRect( 0 );
		}
		return;
	}
}

void CLinkageView::OnEditProperties()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if( pDoc->GetSelectedLinkCount( true ) == 1  )
		EditProperties( 0, pDoc->GetSelectedLink( 0, true ), false );
	else if( pDoc->GetSelectedLinkCount( false ) == 1 && pDoc->GetSelectedConnectorCount() == 0 )
		EditProperties( 0, pDoc->GetSelectedLink( 0, false ), false );
	else if( pDoc->GetSelectedLinkCount( true ) > 1 )
		EditProperties( 0, 0, true );
	else if( pDoc->GetSelectedConnectorCount() == 1 )
		EditProperties( pDoc->GetSelectedConnector( 0 ), 0, false );
}

void CLinkageView::OnLButtonDblClk(UINT nFlags, CPoint MousePoint)
{
	CFPoint point = AdjustClientAreaPoint( MousePoint );

	if( m_VisibleAdjustment == ADJUSTMENT_ROTATE )
	{
		// Must have been selected before double click - go back to
		// stretch mode.
		m_VisibleAdjustment = ADJUSTMENT_STRETCH;
		InvalidateRect( 0 );
		UpdateWindow();
	}
	OnEditProperties();
}

void CLinkageView::OnEditJoin()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->JoinSelected( true );
	InvalidateRect( 0 );
}

void CLinkageView::OnEditLock()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->LockSelected();
	InvalidateRect( 0 );
}

void CLinkageView::OnEditSelectall()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->SelectAll();
	int TotalSelected = pDoc->GetSelectedLinkCount( false ) + pDoc->GetSelectedConnectorCount();
	if( TotalSelected > 0 )
	{
		m_SelectionContainerRect = GetDocumentArea( false, true );
		m_SelectionAdjustmentRect = GetDocumentAdjustArea( true );
		m_SelectionRotatePoint.SetPoint( ( m_SelectionContainerRect.left + m_SelectionContainerRect.right ) / 2,
		                                 ( m_SelectionContainerRect.top + m_SelectionContainerRect.bottom ) / 2 );
		m_VisibleAdjustment = ADJUSTMENT_STRETCH;
	}
	ShowSelectedElementStatus();
	UpdateForDocumentChange();
	SelectionChanged();
}

void CLinkageView::OnUpdateSelectall( CCmdUI *pCmdUI )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( m_bAllowEdit && !m_bSimulating && !pDoc->IsEmpty() );
}

void CLinkageView::OnEditSelectElements()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CSelectElementsDialog dlg;
	dlg.m_bShowDebug = m_bShowDebug;
	dlg.m_pDocument = pDoc;
	pDoc->FillElementList( dlg.m_AllElements );
	if( dlg.DoModal() == IDOK )
	{
		InvalidateRect( 0 );
	}
}

void CLinkageView::OnUpdateSelectElements( CCmdUI *pCmdUI )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( m_bAllowEdit && !m_bSimulating && !pDoc->IsEmpty() );
}

void CLinkageView::OnAlignHorizontal()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->AlignSelected( pDoc->HORIZONTAL );
	UpdateForDocumentChange();
}

void CLinkageView::OnUpdateAlignHorizontal( CCmdUI *pCmdUI )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( !m_bSimulating && pDoc->GetAlignConnectorCount() > 1 && m_bAllowEdit );
}

void CLinkageView::OnFlipHorizontal()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->AlignSelected( pDoc->FLIPHORIZONTAL );
	UpdateForDocumentChange();
}

void CLinkageView::OnUpdateFlipHorizontal( CCmdUI *pCmdUI )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	int Selected = pDoc->GetSelectedConnectorCount();
	if( pDoc->GetSelectedLinkCount( true ) > 0 )
		Selected += 2;
	pCmdUI->Enable( !m_bSimulating && Selected > 1 && m_bAllowEdit );
}

void CLinkageView::OnFlipVertical()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->AlignSelected( pDoc->FLIPVERTICAL );
	UpdateForDocumentChange();
}

void CLinkageView::OnUpdateFlipVertical( CCmdUI *pCmdUI )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	int Selected = pDoc->GetSelectedConnectorCount();
	if( pDoc->GetSelectedLinkCount( true ) > 0 )
		Selected += 2;
	pCmdUI->Enable( !m_bSimulating && Selected > 1 && m_bAllowEdit );
}

void CLinkageView::OnRotateToMeet()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->MeetSelected();
	UpdateForDocumentChange();
}

void CLinkageView::OnUpdateRotateToMeet( CCmdUI *pCmdUI )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( !m_bSimulating && pDoc->IsSelectionMeetable() && m_bAllowEdit );
}

void CLinkageView::OnAlignVertical()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->AlignSelected( pDoc->VERTICAL );
	UpdateForDocumentChange();
}

void CLinkageView::OnUpdateAlignVertical( CCmdUI *pCmdUI )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( !m_bSimulating && pDoc->GetAlignConnectorCount() > 1 && m_bAllowEdit );
}

void CLinkageView::OnAlignLineup()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->AlignSelected( pDoc->INLINE );
	UpdateForDocumentChange();
}

void CLinkageView::OnUpdateAlignLineup( CCmdUI *pCmdUI )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( pDoc->GetAlignConnectorCount() > 2 && m_bAllowEdit );
}

void CLinkageView::OnAlignEvenSpace()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->AlignSelected( pDoc->INLINESPACED );
	UpdateForDocumentChange();
}

void CLinkageView::OnUpdateAlignEvenSpace( CCmdUI *pCmdUI )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( pDoc->GetAlignConnectorCount() > 2 && m_bAllowEdit );
}

void CLinkageView::OnAlignRightAngle()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->MakeRightAngleSelected();
	UpdateForDocumentChange();
}

void CLinkageView::OnUpdateAlignRightAngle( CCmdUI *pCmdUI )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CConnector *pThird = pDoc->GetSelectedConnector( 2 );
	pCmdUI->Enable( !m_bSimulating && pDoc->IsSelectionTriangle() && pThird != 0 && !pThird->IsSlider() && m_bAllowEdit );
}

void CLinkageView::OnAlignSetAngle()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CAngleDialog dlg;
	dlg.m_Angle = pDoc->GetSelectedElementAngle();
	if( dlg.DoModal() == IDOK )
	{
		if( dlg.m_Angle.Find( "*" ) == dlg.m_Angle.GetLength() - 2 )
			pDoc->SetSelectedElementCoordinates( &m_SelectionRotatePoint, dlg.m_Angle );
		else
			pDoc->MakeSelectedAtAngle( atof( dlg.m_Angle ) );
		UpdateForDocumentChange();
	}

	/*

	if( !pDoc->IsSelectionTriangle() )
		return;

	CConnector *pConnector0 = (CConnector*)pDoc->GetSelectedConnector( 0 );
	CConnector *pConnector1 = (CConnector*)pDoc->GetSelectedConnector( 1 );
	CConnector *pConnector2 = (CConnector*)pDoc->GetSelectedConnector( 2 );

	if( pConnector2 != 0 && pConnector2->IsSlider() )
		return;

	double Angle = GetAngle( pConnector1->GetOriginalPoint(), pConnector0->GetOriginalPoint(), pConnector2->GetOriginalPoint() );
	if( Angle > 180 )
		Angle = 360 - Angle;

	CAngleDialog dlg;
	dlg.m_Angle = Angle;

	if( dlg.DoModal() )
	{
		pDoc->MakeSelectedAtAngle( dlg.m_Angle );
		UpdateForDocumentChange();
	}
	*/
}

void CLinkageView::OnUpdateAlignSetAngle( CCmdUI *pCmdUI )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	// CConnector *pThird = pDoc->GetSelectedConnector( 2 );
	pCmdUI->Enable( !m_bSimulating && pDoc->IsSelectionAngleable() && m_bAllowEdit );
}

void CLinkageView::OnAddConnector()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if( pDoc->AddConnectorToSelected( Scale( GRAB_DISTANCE * 4 ) ) )
		UpdateForDocumentChange();
}

void CLinkageView::OnUpdateAddConnector( CCmdUI *pCmdUI )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( pDoc->CanAddConnector() && m_bAllowEdit );
}

void CLinkageView::OnAlignRectangle()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->MakeParallelogramSelected( true );
	UpdateForDocumentChange();
}

void CLinkageView::OnUpdateAlignRectangle( CCmdUI *pCmdUI )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( !m_bSimulating && pDoc->IsSelectionRectangle() && m_bAllowEdit );
}

void CLinkageView::OnAlignParallelogram()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->MakeParallelogramSelected( false );
	UpdateForDocumentChange();
}

void CLinkageView::OnUpdateAlignParallelogram( CCmdUI *pCmdUI )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( !m_bSimulating && pDoc->IsSelectionRectangle() && m_bAllowEdit );
}

void CLinkageView::OnEditSplit()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->SplitSelected();
	UpdateForDocumentChange();
}

void CLinkageView::InsertDouble( CFPoint *pPoint )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pDoc->InsertLink(  CLinkageDoc::MECHANISMLAYER, Unscale( LINK_INSERT_PIXELS ), pPoint == 0 ? GetDocumentViewCenter() : *pPoint, pPoint != 0, 2, m_bNewLinksSolid );
	m_bSuperHighlight = true;
	UpdateForDocumentChange();
	SelectionChanged();
}

void CLinkageView::InsertTriple( CFPoint *pPoint )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->InsertLink(  CLinkageDoc::MECHANISMLAYER, Unscale( LINK_INSERT_PIXELS ), pPoint == 0 ? GetDocumentViewCenter() : *pPoint, pPoint != 0, 3, m_bNewLinksSolid );
	m_bSuperHighlight = true;
	UpdateForDocumentChange();
	SelectionChanged();
}

void CLinkageView::InsertQuad( CFPoint *pPoint )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->InsertLink(  CLinkageDoc::MECHANISMLAYER, Unscale( LINK_INSERT_PIXELS ), pPoint == 0 ? GetDocumentViewCenter() : *pPoint, pPoint != 0, 4, m_bNewLinksSolid );
	m_bSuperHighlight = true;
	UpdateForDocumentChange();
	SelectionChanged();
}

void CLinkageView::OnViewLabels()
{
	m_bShowLabels = !m_bShowLabels;
	SaveSettings();
	InvalidateRect( 0 );
}

void CLinkageView::OnViewAngles()
{
	m_bShowAngles = !m_bShowAngles;
	SaveSettings();
	InvalidateRect( 0 );
}

void CLinkageView::OnUpdateViewLabels(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck( m_bShowLabels );
	pCmdUI->Enable( !m_bSimulating );
}

void CLinkageView::OnUpdateViewAngles(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck( m_bShowAngles );
	pCmdUI->Enable( !m_bSimulating );
}

void CLinkageView::OnEditSnap()
{
	m_bSnapOn = !m_bSnapOn;
	SaveSettings();
}

void CLinkageView::OnUpdateEditSnap(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck( m_bSnapOn );
	pCmdUI->Enable( !m_bSimulating );
}

void CLinkageView::OnEditGridSnap()
{
	m_bGridSnap = !m_bGridSnap;
	SaveSettings();
}

void CLinkageView::OnUpdateEditGridSnap(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck( m_bGridSnap );
	pCmdUI->Enable( !m_bSimulating );
}

void CLinkageView::OnEditAutoJoin()
{
	m_bAutoJoin = !m_bAutoJoin;
	SaveSettings();
}

void CLinkageView::OnUpdateEditAutoJoin(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck( m_bAutoJoin );
	pCmdUI->Enable( !m_bSimulating && m_bSnapOn );
}

void CLinkageView::OnViewData()
{
	m_bShowData = !m_bShowData;
	SaveSettings();
	InvalidateRect( 0 );
}

void CLinkageView::OnUpdateViewData(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck( m_bShowData );
	pCmdUI->Enable( !m_bSimulating );
}

void CLinkageView::OnViewDimensions()
{
	m_bShowDimensions = !m_bShowDimensions;
	SaveSettings();
	InvalidateRect( 0 );
}

void CLinkageView::OnUpdateViewDimensions(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck( m_bShowDimensions );
	pCmdUI->Enable( !m_bSimulating );
}

void CLinkageView::OnViewGroundDimensions()
{
	m_bShowGroundDimensions = !m_bShowGroundDimensions;
	SaveSettings();
	InvalidateRect( 0 );
}

void CLinkageView::OnUpdateViewGroundDimensions(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck( m_bShowGroundDimensions );
	pCmdUI->Enable( !m_bSimulating );
}

void CLinkageView::OnViewDrawingLayerDimensions()
{
	m_bShowDrawingLayerDimensions = !m_bShowDrawingLayerDimensions;
	SaveSettings();
	InvalidateRect( 0 );
}

void CLinkageView::OnUpdateViewDrawingLayerDimensions(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck( m_bShowDrawingLayerDimensions );
	pCmdUI->Enable( !m_bSimulating );
}

void CLinkageView::OnViewSolidLinks()
{
	m_bNewLinksSolid = !m_bNewLinksSolid;
	SaveSettings();
	InvalidateRect( 0 );
}

void CLinkageView::OnUpdateViewSolidLinks(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck( m_bNewLinksSolid );
	pCmdUI->Enable( !m_bSimulating );
}

void CLinkageView::OnViewMechanism()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if( m_SelectedViewLayers & CLinkageDoc::MECHANISMLAYERS )
		m_SelectedViewLayers &= ~CLinkageDoc::MECHANISMLAYERS;
	else
		m_SelectedViewLayers |= CLinkageDoc::MECHANISMLAYERS;
	SaveSettings();

	pDoc->SetViewLayers( m_SelectedViewLayers );

	pDoc->FinishChangeSelected();
	m_SelectionContainerRect = GetDocumentArea( false, true );
	m_SelectionAdjustmentRect = GetDocumentAdjustArea( true );
	InvalidateRect( 0 );
}

void CLinkageView::OnUpdateViewMechanism(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pCmdUI->SetCheck( ( m_SelectedViewLayers & CLinkageDoc::MECHANISMLAYERS ) != 0 ? 1 : 0 );
	pCmdUI->Enable( !m_bSimulating );
}

void CLinkageView::OnViewDrawing()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if( m_SelectedViewLayers & CLinkageDoc::DRAWINGLAYER )
		m_SelectedViewLayers &= ~CLinkageDoc::DRAWINGLAYER;
	else
		m_SelectedViewLayers |= CLinkageDoc::DRAWINGLAYER;
	SaveSettings();

	pDoc->SetViewLayers( m_SelectedViewLayers );

	pDoc->FinishChangeSelected();
	m_SelectionContainerRect = GetDocumentArea( false, true );
	m_SelectionAdjustmentRect = GetDocumentAdjustArea( true );
	InvalidateRect( 0 );
}

void CLinkageView::OnUpdateViewDrawing(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pCmdUI->SetCheck( ( m_SelectedViewLayers & CLinkageDoc::DRAWINGLAYER ) != 0 ? 1 : 0 );
	pCmdUI->Enable( !m_bSimulating );
}

void CLinkageView::OnEditDrawing()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if( m_SelectedEditLayers & CLinkageDoc::DRAWINGLAYER )
		m_SelectedEditLayers &= ~CLinkageDoc::DRAWINGLAYER;
	else
		m_SelectedEditLayers |= CLinkageDoc::DRAWINGLAYER;
	SaveSettings();

	pDoc->SetEditLayers( m_SelectedEditLayers );

	pDoc->FinishChangeSelected();
	m_SelectionContainerRect = GetDocumentArea( false, true );
	m_SelectionAdjustmentRect = GetDocumentAdjustArea( true );
	InvalidateRect( 0 );
}

void CLinkageView::OnUpdateEditDrawing(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pCmdUI->SetCheck( ( m_SelectedEditLayers & CLinkageDoc::DRAWINGLAYER ) != 0 ? 1 : 0 );
	pCmdUI->Enable( !m_bSimulating );
}

void CLinkageView::OnEditMechanism()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int CurrentEditLayers = m_SelectedEditLayers;

	if( m_SelectedEditLayers & CLinkageDoc::MECHANISMLAYERS )
		m_SelectedEditLayers &= ~CLinkageDoc::MECHANISMLAYERS;
	else
		m_SelectedEditLayers |= CLinkageDoc::MECHANISMLAYERS;
	SaveSettings();

	pDoc->SetEditLayers( m_SelectedEditLayers );

	pDoc->FinishChangeSelected();
	m_SelectionContainerRect = GetDocumentArea( false, true );
	m_SelectionAdjustmentRect = GetDocumentAdjustArea( true );
	InvalidateRect( 0 );
}

void CLinkageView::OnUpdateEditMechanism(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pCmdUI->SetCheck( ( m_SelectedEditLayers & CLinkageDoc::MECHANISMLAYERS ) != 0 ? 1 : 0 );
	pCmdUI->Enable( !m_bSimulating );
}

void CLinkageView::OnViewDebug()
{
	m_bShowDebug = !m_bShowDebug;
	SaveSettings();
	InvalidateRect( 0 );
}

void CLinkageView::OnViewBold()
{
	m_bShowBold = !m_bShowBold;
	SaveSettings();
	InvalidateRect( 0 );
}

void CLinkageView::OnUpdateViewAnicrop(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck( m_bShowAnicrop );
	pCmdUI->Enable( !m_bSimulating );
}

void CLinkageView::OnViewAnicrop()
{
	m_bShowAnicrop = !m_bShowAnicrop;
	SaveSettings();
	InvalidateRect( 0 );
}

void CLinkageView::OnUpdateViewLargeFont(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck( m_bShowLargeFont );
	pCmdUI->Enable( !m_bSimulating );
}

void CLinkageView::OnViewLargeFont()
{
	m_bShowLargeFont = !m_bShowLargeFont;
	SetupFont();
	SaveSettings();
	InvalidateRect( 0 );
}

void CLinkageView::OnUpdateEditGrid(CCmdUI *pCmdUI)
{
	pCmdUI->Enable( !m_bSimulating );
}

void CLinkageView::OnUpdateViewShowGrid(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck( m_bShowGrid );
	pCmdUI->Enable( !m_bSimulating );
}

void CLinkageView::OnMoreMomentum()
{
	m_bUseMoreMomentum = !m_bUseMoreMomentum;
	SaveSettings();
	InvalidateRect( 0 );
}

void CLinkageView::OnUpdateMoreMomentum(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck( m_bUseMoreMomentum );
	pCmdUI->Enable( !m_bSimulating );
}

void CLinkageView::OnViewShowGrid()
{
	m_bShowGrid = !m_bShowGrid;
	SaveSettings();
	InvalidateRect( 0 );
}

void CLinkageView::OnEditGrid()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	double DocumentScale = pDoc->GetUnitScale();
	
	CUserGridDialog dlg;
	dlg.m_GridTypeSelection = m_GridType;
	if( m_xUserGrid == 0 )
		m_xUserGrid = 10 / DocumentScale;
	if( m_yUserGrid == 0 )
		m_yUserGrid = 10 / DocumentScale;
	dlg.m_HorizontalSpacing = m_xUserGrid * DocumentScale;
	dlg.m_VerticalSpacing = m_yUserGrid * DocumentScale;
	dlg.m_bShowUserGrid = m_bShowGrid ? 1 : 0;

	if( dlg.DoModal() == IDOK )
	{
		bool bDirtyDocument = m_GridType != dlg.m_GridTypeSelection || m_xUserGrid != dlg.m_HorizontalSpacing / DocumentScale || m_yUserGrid != dlg.m_VerticalSpacing / DocumentScale;
		m_bShowGrid = dlg.m_bShowUserGrid != 0;	
		m_GridType = dlg.m_GridTypeSelection;
		m_xUserGrid = dlg.m_HorizontalSpacing / DocumentScale;
		m_yUserGrid = dlg.m_VerticalSpacing / DocumentScale;

		if( m_GridType == 1 )
		{
			CFPoint Temp( m_xUserGrid, m_yUserGrid );
			pDoc->SetGrid( Temp );
		}
		else
			pDoc->SetGrid();

		// Only treat grid changes as modifying the document if the 
		if( !pDoc->IsEmpty() && bDirtyDocument )
			pDoc->SetModifiedFlag( true );

		SaveSettings();
		InvalidateRect( 0 );
	}
}

void CLinkageView::OnUpdateViewParts(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck( m_bShowParts );
	pCmdUI->Enable( !m_bSimulating );
}

void CLinkageView::OnViewParts()
{
	m_bShowParts = !m_bShowParts;
	m_bAllowEdit = !m_bShowParts;
	if( m_bShowParts )
	{
		CLinkageDoc* pDoc = GetDocument();
		ASSERT_VALID(pDoc);
		//pDoc->SelectElement();
	}
	GetSetViewSpecificDimensionVisbility( false );
	SaveSettings();
	InvalidateRect( 0 );
}

void CLinkageView::OnUpdateViewDebug(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck( m_bShowDebug );
	pCmdUI->Enable( !m_bSimulating );
}

void CLinkageView::OnUpdateViewBold(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck( m_bShowBold );
	pCmdUI->Enable( !m_bSimulating );
}

void CLinkageView::OnEditCut()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->Copy( true );
	UpdateForDocumentChange();
	SelectionChanged();
}

void CLinkageView::OnEditCopy()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->Copy( false );
}

void CLinkageView::OnEditPaste()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->Paste();
	m_VisibleAdjustment = ADJUSTMENT_STRETCH;
	m_SelectionContainerRect = GetDocumentArea( false, true );
	m_SelectionAdjustmentRect = GetDocumentAdjustArea( true );
	m_SelectionRotatePoint.SetPoint( ( m_SelectionContainerRect.left + m_SelectionContainerRect.right ) / 2,
									 ( m_SelectionContainerRect.top + m_SelectionContainerRect.bottom ) / 2 );
	UpdateForDocumentChange();
	SelectionChanged();
}

void CLinkageView::OnUpdateEditPaste(CCmdUI *pCmdUI)
{
    UINT CF_Linkage = RegisterClipboardFormat( "RECTORSQUID_Linkage_CLIPBOARD_XML_FORMAT" );
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( !m_bSimulating && ::IsClipboardFormatAvailable( CF_Linkage ) != 0 && m_bAllowEdit );
}

void CLinkageView::OnRButtonDown(UINT nFlags, CPoint MousePoint)
{
	SetFocus();
	
	CFPoint point = AdjustClientAreaPoint( MousePoint );

	SetCapture();
	m_bSuperHighlight = 0;
	m_MouseAction = ACTION_PAN;
	m_PreviousDragPoint = point;
	m_DragStartPoint = point;
	m_bMouseMovedEnough	= false;
}

void CLinkageView::OnMButtonDown(UINT nFlags, CPoint MousePoint)
{
	CFPoint point = AdjustClientAreaPoint( MousePoint );

	SetCapture();
	m_bSuperHighlight = 0;
	m_MouseAction = ACTION_PAN;
	m_PreviousDragPoint = point;
	m_DragStartPoint = point;
	m_bMouseMovedEnough	= false;
}

void CLinkageView::OnRButtonUp(UINT nFlags, CPoint MousePoint)
{
	ReleaseCapture();
	m_MouseAction = ACTION_NONE;
	SetScrollExtents();

	if( m_bSimulating )
		return;

	if( !m_bMouseMovedEnough && m_bShowParts )
	{
		if( AfxMessageBox( "The Parts List cannot be modified. Would you like to turn off the Parts List view?", MB_YESNO ) == IDYES )
			OnViewParts();
		return;
	}

	if( !m_bMouseMovedEnough && m_bAllowEdit )
	{
		CLinkageDoc* pDoc = GetDocument();
		ASSERT_VALID(pDoc);
		CLink *pLink = 0;
		CConnector *pConnector = 0;
		CFPoint point = AdjustClientAreaPoint( MousePoint );
		if( FindDocumentItem( point, pLink, pConnector ) )
		{
			bool bResult = false;
			if( pConnector != 0 )
				EditProperties( pConnector, 0, false );
			else if( pLink != 0 )
				EditProperties( 0, pLink, false );
		}
		else
		{
			m_PopupPoint = point;
			ClientToScreen( &MousePoint );
			if( m_pPopupGallery != 0 )
			{
				m_pPopupGallery->ShowGallery( this, MousePoint.x, MousePoint.y, 0 );
			}
		}
	}
}

void CLinkageView::OnMButtonUp(UINT nFlags, CPoint MousePoint)
{
	ReleaseCapture();
	m_MouseAction = ACTION_NONE;
	SetScrollExtents();

	if( m_bSimulating )
		return;
}

BOOL CLinkageView::OnMouseWheel(UINT nFlags, short zDelta, CPoint MousePoint)
{
//	if( m_bSimulating )
//		return TRUE;

	if( zDelta == 0 )
		return TRUE;
	if( zDelta > 0 && m_ScreenZoom >= 256 )
		return TRUE;
	else if( zDelta < 0 && m_ScreenZoom <= .0005 )
		return TRUE;

	ScreenToClient( &MousePoint );
	CFPoint pt = AdjustClientAreaPoint( MousePoint );

	m_Zoom = m_ScreenZoom;
	CFPoint AdjustedPoint = Unscale( pt );

	if( zDelta > 0 )
		m_ScreenZoom *= ZOOM_AMOUNT;
	else if( zDelta < 0 )
		m_ScreenZoom /= ZOOM_AMOUNT;

	m_Zoom = m_ScreenZoom;
	CFPoint AdjustedPoint2 = Unscale( pt );

	double dx = ( AdjustedPoint2.x - AdjustedPoint.x ) * m_ScreenZoom;
	double dy = ( AdjustedPoint2.y - AdjustedPoint.y ) * m_ScreenZoom;

	m_ScreenScrollPosition.x -= (int)dx;
	m_ScreenScrollPosition.y += (int)dy;

	m_ScrollPosition = m_ScreenScrollPosition;

	SetScrollExtents();
	InvalidateRect( 0 );
	return TRUE;
}

CPoint CLinkageView::GetDefaultUnscalePoint( void )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	return CPoint( m_DrawingRect.Width() / 2, m_DrawingRect.Height() / 2 );
}

void CLinkageView::OnViewZoomin()
{
	//if( m_bSimulating )
	//	return;

	CPoint Point = GetDefaultUnscalePoint();
	ClientToScreen( &Point );
	OnMouseWheel( 0, 120, Point );
}

void CLinkageView::OnViewZoomout()
{
	//if( m_bSimulating )
	//	return;

	CPoint Point = GetDefaultUnscalePoint();
	ClientToScreen( &Point );
	OnMouseWheel( 0, -120, Point );
}

void CLinkageView::GetDocumentViewArea( CFRect &Rect, CLinkageDoc *pDoc )
{
	if( pDoc == 0 )
	{
		CLinkageDoc* pDoc = GetDocument();
		ASSERT_VALID(pDoc);
	}

	CFRect Area = GetDocumentArea();
	CFRect PixelRect = Area;
}

void CLinkageView::ZoomToFit( CFRect Container, bool bAllowEnlarge, double MarginScale, double UnscaledUnitSize, bool bShrinkToZoomStep )
{
	/*
	 * Adjust the scroll offset and zoom to fit the document to the specified area.
	 */

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	
	//m_YUpDirection = ( Renderer.GetYOrientation() < 0 ) ? -1 : 1;
	m_BaseUnscaledUnit = UnscaledUnitSize;
	m_ConnectorRadius = CONNECTORRADIUS * UnscaledUnitSize;
	m_ConnectorTriangle = CONNECTORTRIANGLE * UnscaledUnitSize;
	m_ArrowBase = ANCHORBASE * UnscaledUnitSize;
	m_Zoom = 1.0;
	m_ScrollPosition.SetPoint( 0, 0 );

	if( pDoc->IsEmpty() && m_pBackgroundBitmap == 0 )
		return;

	double cx = Container.Width();
	double cy = Container.Height();

	double xMargin = cx * MarginScale;
	double yMargin = cy * MarginScale;
	cx -= xMargin;
	cy -= yMargin;

	// Connector radius on every side of the mechanism. This is wrong (more than needed) if there are just drawing elements at the edges of the mechanism.
	cx -= CONNECTORRADIUS * 2;
	cy -= CONNECTORRADIUS * 2;

	cx -= 1;
	cy -= 1;

	double cxOriginal = cx;
	double cyOriginal = cy;

	/*
	* Start with a zoom of 1. The document dimensions are the same as the DPI adjusted pixel dimensions.
	* Take the dimensioned document area and the non-dimensioned document area and the difference is how
	* much room is need around the undimensioned document for the dimensions IF THERE ARE NO DIAGONAL MEASUREMENT LINES.
	* The accuracy of the later calculated zoom is tested for cases where diagonal measurement lines have caused
	* an innaccurate result.
	*/

	CFRect NoDimArea = GetDocumentArea( false );
	NoDimArea.Normalize();
	CFRect DimArea = GetDocumentArea( m_bShowDimensions );
	DimArea.Normalize();

	// Take drawing rect and shrink it by the difference between the dimensioned and non-dimensioned document. This 
	// is a good starting point to determining how much room is needed for the dimension lines.
	cx -= ( DimArea.Width() - NoDimArea.Width() ) * 1;
	cy -= ( DimArea.Height() - NoDimArea.Height() ) * 1;

	double xRatio = cx / NoDimArea.Width();
	double yRatio = cy / NoDimArea.Height();
	double Ratio = min( xRatio, yRatio );

	m_Zoom = Ratio;

	/*
	* Now that there is a zoom level that might work, check to see if the dimensions actual fit.
	* it is possible that the dimensioned document now is too big or too small because of diagonal
	* measurement lines. Try to find a better fit based on this starting zoom level.
	*/

	cx = cxOriginal;
	cy = cyOriginal;

	CFRect Area = GetDocumentArea( m_bShowDimensions );
	Area.Normalize();
	xRatio = cx / Area.Width();
	yRatio = cy / Area.Height();
	Ratio = min( xRatio, yRatio );

	int Direction = Ratio - m_Zoom >= 0 ? 1 : -1;
	while( fabs( Ratio - m_Zoom ) > 0.01 )
	{
		int NewDirection = Ratio - m_Zoom >= 0 ? 1 : -1;
		if( NewDirection != Direction )
			break;
		double Difference = Ratio - m_Zoom;
		m_Zoom += Difference * 0.5;

		Area = GetDocumentArea( m_bShowDimensions );
		Area.Normalize();
		xRatio = cx / Area.Width();
		yRatio = cy / Area.Height();
		Ratio = min( xRatio, yRatio );
	}

	if( bShrinkToZoomStep )
	{
		m_Zoom = pow( ZOOM_AMOUNT, floor( log( m_Zoom ) / log( ZOOM_AMOUNT ) ) );
	}

	if( m_Zoom > 1.0 && !bAllowEnlarge )
		m_Zoom = 1.0;

	Area = GetDocumentArea( m_bShowDimensions );
	Area.Normalize();

	// Center the mechanism.
	m_ScrollPosition.x = m_Zoom * ( Area.left + Area.Width() / 2 );
	m_ScrollPosition.y = m_Zoom * -( Area.top + Area.Height() / 2 );
}

void CLinkageView::OnMenuZoomfit()
{
	if( m_bSimulating )
		return;

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if( pDoc->IsEmpty() && m_pBackgroundBitmap == 0 )
	{
		m_ScreenZoom = 1;
		m_ScreenScrollPosition.x = 0;
		m_ScreenScrollPosition.y = 0;
		SetScrollExtents();
		InvalidateRect( 0 );
		return;
	}

	/*
	 * Render using a null renderer using the scale-to-fit flag to scale the document to fit the window.
	 * The regular drawing later will have the zoom and scroll positions set from this one-time null rendering.
	 */ 
	CRenderer NullRenderer( CRenderer::NULL_RENDERER );
	PrepareRenderer( NullRenderer, 0, 0, 0, 1.0, true, 0.0, 1.0, true, false, m_bPrintFullSize, 0 );

	m_ScreenZoom = m_Zoom;
	m_ScreenScrollPosition = m_ScrollPosition;

	SetScrollExtents();
	InvalidateRect( 0 );
}

void CLinkageView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	
	pDoc->SetViewLayers( m_SelectedViewLayers );
	pDoc->SetEditLayers( m_SelectedEditLayers );
}

void CLinkageView::OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint )
{
	SetScrollExtents();

	CMFCRibbonBar *pRibbon = ((CFrameWndEx*) AfxGetMainWnd())->GetRibbonBar();
	if( pRibbon == 0 )
		return;
	CMFCRibbonComboBox * pComboBox = DYNAMIC_DOWNCAST( CMFCRibbonComboBox, pRibbon->FindByID( ID_VIEW_UNITS ) );
	if( pComboBox == 0 )
		return;

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CLinkageDoc::_Units Units = pDoc->GetUnits();

	CFPoint Temp( m_xUserGrid, m_yUserGrid );
	if( pDoc->GetGrid( Temp ) )
	{
		m_GridType = 1;
		m_xUserGrid = Temp.x;
		m_yUserGrid = Temp.y;
	}
	else if( !pDoc->GetPathName().IsEmpty() || !pDoc->IsEmpty() )
		m_GridType = 0;

	pComboBox->SelectItem( (DWORD_PTR)Units );

	if( lHint == 0 )
	{
		// Deal with a background bitmap that is stored in both the document and in binary form in this view object.
		if( m_pBackgroundBitmap != 0 )
		{
			delete m_pBackgroundBitmap;
			m_pBackgroundBitmap = 0;
		}

		std::string& BitmapData = pDoc->GetBackgroundImage();
		if( BitmapData.length() > 0 )
		{
			std::vector<byte> buffer = base64_decode( BitmapData );
			m_pBackgroundBitmap = ImageBytesToRendererBitmap( buffer, (size_t)buffer.size() );
		}
	}

	UpdateForDocumentChange();

	CView::OnUpdate( pSender, lHint, pHint );
}

void CLinkageView::OnFilePrintSetup()
{
	//SetPrinterOrientation();

	CLinkageApp *pApp = (CLinkageApp*)AfxGetApp();
	if( pApp != 0 )
		pApp->OnFilePrintSetup();
}

void CLinkageView::OnPrintFull()
{
	m_bPrintFullSize = !m_bPrintFullSize;
	SaveSettings();
}

void CLinkageView::OnUpdatePrintFull( CCmdUI *pCmdUI )
{
	pCmdUI->SetCheck( m_bPrintFullSize );
}

BOOL CLinkageView::OnPreparePrinting( CPrintInfo* pInfo )
{

	/*


	This line of code below will show the print dialog box if actually printing. The problem
	is that tehre is no way to know if the printer selection changes. Because of that, there
	is also no way to change the page count depending on the paper size for the selected printer.
	This is a huge flaw in the built-in printing process and one that can be fixded by creating a
	fully custum print dialog, or something like that.

	For now, show the print dialog first then give a warning of the document needs more than some 
	hard-coded number of pages. if the user tries to print a huge machine actual size and has 
	hundreds of pages to print then they need to apporve of that!

	None of this is a problem if they print to fit on the page.

	*/

	pInfo->SetMinPage( 1 );
	pInfo->SetMaxPage( m_bPrintFullSize ? 1000 : 1 );
	pInfo->m_pPD->m_pd.Flags |= PD_NOPAGENUMS;
	BOOL bResult = DoPreparePrinting( pInfo );
	if( bResult == FALSE )
		return FALSE;

	if( pInfo->m_pPD == 0 )
		return bResult;

	/*
	 * Calculate page count and orientation. Both are automatic.
	 */
	CDC dc;
	dc.Attach( pInfo->m_pPD->m_pd.hDC );

	CLinkageApp *pApp = (CLinkageApp*)AfxGetApp();
	if( pApp == 0 )
		return bResult;

	m_Zoom = 1.0;

	CFRect Area = GetDocumentArea( m_bShowDimensions );
	if( m_bPrintFullSize )
	{
		int Count = GetPrintPageCount( &dc, m_bPrintFullSize, Area, m_PrintOrientationMode, m_PrintWidthInPages );
		pInfo->SetMaxPage( Count );
	}
	else
	{
		if( fabs( Area.Width() ) >= fabs( Area.Height() ) )
			m_PrintOrientationMode = DMORIENT_LANDSCAPE;
		else
			m_PrintOrientationMode = DMORIENT_PORTRAIT;
		pInfo->SetMaxPage( 1 );
	}

	pApp->SetPrinterOrientation( m_PrintOrientationMode );
	DEVMODE* pDevMode = pInfo->m_pPD->GetDevMode();
	pDevMode->dmOrientation = m_PrintOrientationMode;
	pDevMode->dmFields |= DM_ORIENTATION;
	dc.ResetDC( pDevMode );

	dc.Detach();

	return bResult;
}

void CLinkageView::OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo)
{
	if( m_bSimulating )
		StopMechanismSimulate();
}

void CLinkageView::SetZoom( double Zoom )
{
	if( Zoom == m_ScreenZoom )
		return;
	if( Zoom > 256 )
		Zoom = 256;
	else if( Zoom <= 0.0005 )
		Zoom = 0.0005;
	m_ScreenZoom = Zoom;
	m_Zoom = m_ScreenZoom;
	SetScrollExtents();
	InvalidateRect( 0 );
}

void CLinkageView::SetOffset( CFPoint Offset )
{
	if( Offset == m_ScreenScrollPosition )
		return;
	m_ScreenScrollPosition = Offset;
	m_ScrollPosition = m_ScreenScrollPosition;
	SetScrollExtents();
	InvalidateRect( 0 );
}

void CLinkageView::OnSelectNext( void )
{
	SetFocus();
	
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if( pDoc->SelectNext( pDoc->NEXT ) )
	{
		ShowSelectedElementStatus();
		UpdateForDocumentChange();
		SelectionChanged();
	}
}

void CLinkageView::OnSelectPrevious( void )
{
	SetFocus();
	
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if( pDoc->SelectNext( pDoc->PREVIOUS ) )
	{
		ShowSelectedElementStatus();
		UpdateForDocumentChange();
		SelectionChanged();
	}
}

void CLinkageView::Nudge( double Horizontal, double Vertical )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	double xNudge = Unscale( 1.0 / m_DPIScale ) * Horizontal;
	double yNudge = Unscale( 1.0 / m_DPIScale ) * Vertical;
	pDoc->MoveSelected( CFPoint( xNudge, yNudge ) );
	UpdateForDocumentChange();
}

static const int BigNudge = 10;

void CLinkageView::OnNudgeLeft( void )
{
	if( m_bSimulating )
		OnSimulateStep( 0, false );
	else if( m_bAllowEdit )
		Nudge( -1, 0 );
}

void CLinkageView::OnNudgeRight( void )
{
	if( m_bSimulating )
		OnSimulateStep( 1, false );
	else if( m_bAllowEdit )
		Nudge( 1, 0 );
}

void CLinkageView::OnNudgeUp( void )
{
	if( !m_bSimulating && m_bAllowEdit )
		Nudge( 0, 1 );
}

void CLinkageView::OnNudgeDown( void )
{
	if( !m_bSimulating && m_bAllowEdit )
		Nudge( 0, -1 );
}

void CLinkageView::OnNudgeBigLeft( void )
{
	if( m_bSimulating )
		OnSimulateStep( 0, true );
	else if( m_bAllowEdit )
		Nudge( -1 * BigNudge, 0 );
}

void CLinkageView::OnNudgeBigRight( void )
{
	if( m_bSimulating )
		OnSimulateStep( 1, true );
	else if( m_bAllowEdit )
		Nudge( 1 * BigNudge, 0 );
}

void CLinkageView::OnNudgeBigUp( void )
{
	if( !m_bSimulating && m_bAllowEdit )
		Nudge( 0, 1 * BigNudge );
}

void CLinkageView::OnNudgeBigDown( void )
{
	if( !m_bSimulating && m_bAllowEdit )
		Nudge( 0, -1 * BigNudge );
}

void CLinkageView::OnMechanismQuicksim()
{
	if( m_TimerID != 0 )
		timeKillEvent( m_TimerID );

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	//pDoc->SelectElement();
	InvalidateRect( 0 );
	m_MouseAction = ACTION_NONE;

	if( m_bSimulating )
	{
		m_bSimulating = false;
		m_TimerID = 0;
		OnMechanismReset();
	}

	// Always do the quick simulation even if it was already simulating.

	pDoc->Reset( true );
	m_Simulator.Reset();

	int Steps = m_Simulator.GetSimulationSteps( pDoc );

	if( Steps == 0 )
		return;

	++Steps; // There is always a last step needed to get back to the start location.

	bool bGoodSimulation = true;
	int Counter = 0;
	for( ; Counter < Steps && bGoodSimulation; ++Counter )
		bGoodSimulation = m_Simulator.SimulateStep( pDoc, 1, false, 0, 0, 0, false, 0 );

	// Do one more step so that the final resting place for the drawing connectors are included in the motion path.
	if( !bGoodSimulation )
		m_Simulator.SimulateStep( pDoc, 1, false, 0, 0, 0, false, 0 );

	DebugItemList.Clear();

	//pDoc->Reset( !bGoodSimulation );
	pDoc->Reset( false );
	m_Simulator.Reset();

	InvalidateRect( 0 );
}

void CLinkageView::DebugDrawConnector( CRenderer* pRenderer, unsigned int OnLayers, CConnector* pConnector, bool bShowlabels, bool bUnused, bool bHighlight, bool bDrawConnectedLinks )
{
	if( !m_bShowDebug )
		return;

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CPen CustomPen;
	CustomPen.CreatePen( PS_USERSTYLE, 1, RGB( 0, 0, 0 ) );
	CPen *pOldPen = pRenderer->SelectObject( &CustomPen );

	if( pConnector != 0 )
	{
		CFPoint Point = pConnector->GetPoint();
		Point = Scale( Point );

		pRenderer->SetTextColor( RGB( 25, 0, 0 ) );
		pRenderer->SetBkMode( OPAQUE );
		pRenderer->SetTextAlign( TA_LEFT | TA_TOP );
		pRenderer->SetBkColor( RGB( 255, 255, 255 ) );

		CString String;
		double DocumentScale = pDoc->GetUnitScale();
		String.Format( "%s/M %.6lf", pDoc->GetUnitsString( pDoc->GetUnits(), true ), pConnector->GetSpeed() * pDoc->GetUnitScale() );

		double Radius = m_ConnectorRadius * 5;

		pRenderer->TextOut( Point.x + Radius, Point.y - ( UnscaledUnits( m_UsingFontHeight + 1 ) * 2 ), String );
	}

	pRenderer->SelectObject( pOldPen );

}

double CLinkageView::AdjustYCoordinate( double y )
{
	return y * m_YUpDirection;
}

void CLinkageView::DrawSliderTrack( CRenderer* pRenderer, unsigned int OnLayers, CConnector* pConnector )
{
	if( ( pConnector->GetLayers() & OnLayers ) == 0 )
		return;

	if( !pConnector->IsSlider() )
		return;

	CConnector *Limit1;
	CConnector *Limit2;

	if( !pConnector->GetSlideLimits( Limit1, Limit2 ) )
		return;

	COLORREF Color = RGB( 190, 165, 165 );

	// Sort the points so that other sliders on these same points draw from the same location.
	CLink * pCommon = Limit1->GetSharingLink( Limit2 );
	if( pCommon != 0 )
	{
		if( !pCommon->IsSolid() && pConnector->GetSlideRadius() == 0.0 )
		{
			int PointCount = 0;
			CFPoint* Points = pCommon->GetHull( PointCount );
			int Found1 = -2; // -2 keeps the test later from accidentially thinking the points are along the hull.
			int Found2 = -2;
			for( int Index = 0; Index < PointCount; ++Index )
			{
				if( Limit1->GetPoint() == Points[Index] )
					Found1 = Index;
				if( Limit2->GetPoint() == Points[Index] )
					Found2 = Index;
				if( Found1 >= 0 && Found2 >= 0 )
					break;
			}
			if( Found1 == Found2 + 1 || Found1 == Found2 - 1
			    || ( Found1 == 0 && Found2 == PointCount - 1 )
			    || ( Found2 == 0 && Found1 == PointCount - 1 ) )
			{
				// The points are along the hull of a non-solid link so there is already a line being drawn between them.
				return;
			}
		}

		Color = pCommon->GetColor();
	}

	CFPoint Point1 = Limit1->GetPoint();
	CFPoint Point2 = Limit2->GetPoint();

	//if( Point2.x < Point1.x || ( Point2.x == Point1.x && Point2.y < Point1.y ) )
	//{
	//	CFPoint Temp = Point1;
	//	Point1 = Point2;
	//	Point2 = Temp;
	//}

	CPen DotPen( PS_DOT, 1, Color );
	CPen *pOldPen = pRenderer->SelectObject( &DotPen );

	if( pConnector->GetOriginalSlideRadius() != 0.0 )
	{
		CFArc SliderArc;
		if( !pConnector->GetSliderArc( SliderArc ) )
			return;

		SliderArc = Scale( SliderArc );
		pRenderer->Arc( SliderArc );
	}
	else
		pRenderer->DrawLine( Scale( CFPoint( Point1.x, Point1.y ) ), Scale( CFPoint( Point2.x, Point2.y ) ) );

	pRenderer->SelectObject( pOldPen );
}

double CLinkageView::UnscaledUnits( double Count )
{
	return Count * m_BaseUnscaledUnit;
}

static CString GetErrorText( CElement::_ElementError Error )
{
	switch( Error )
	{
		case CElement::SUCCESS: return "";
		case CElement::UNDEFINED: return " Unable to simulate this element.";
		case CElement::MANGLE: return "The link is being mangled (stretch, compressed, altered).";
		case CElement::RANGE: return "The connector is being moved outside of its valid range.";
		case CElement::CONFLICT: return "Other elements are making conflicting attempts to move this element.";
		case CElement::FLOPPY: return "The result is unpredictable (floppy) for this element.";
	}
	return "";
}

void CLinkageView::DrawFailureMarks( CRenderer* pRenderer, unsigned int OnLayers, CFPoint Point, double Radius, CElement::_ElementError Error )
{
	if( Error == CElement::SUCCESS )
		return;
	CPen RedPen( PS_SOLID, 3, RGB( 255, 32, 32 ) );
	CPen *pOldPen = pRenderer->SelectObject( &RedPen );
	double Distance1 = Radius + UnscaledUnits( 4 );
	double Distance2 = Radius + UnscaledUnits( 9 );

	pRenderer->MoveTo( Point.x + Distance1, Point.y );
	pRenderer->LineTo( Point.x + Distance2, Point.y );
	pRenderer->MoveTo( Point.x - Distance1, Point.y );
	pRenderer->LineTo( Point.x - Distance2, Point.y );
	pRenderer->MoveTo( Point.x, Point.y - AdjustYCoordinate( Distance1 ) );
	pRenderer->LineTo( Point.x, Point.y - AdjustYCoordinate( Distance2 ) );
	pRenderer->MoveTo( Point.x, Point.y + AdjustYCoordinate( Distance1 ) );
	pRenderer->LineTo( Point.x, Point.y + AdjustYCoordinate( Distance2 ) );

	Distance1 = (int)( ( ( (double)Radius + 4.0 ) / 1.414 ) * m_BaseUnscaledUnit );
	Distance2 = (int)( ( ( (double)Radius + 9.9 ) / 1.414 ) * m_BaseUnscaledUnit );

	pRenderer->MoveTo( Point.x + Distance1, Point.y - AdjustYCoordinate( Distance1 ) );
	pRenderer->LineTo( Point.x + Distance2, Point.y - AdjustYCoordinate( Distance2 ) );
	pRenderer->MoveTo( Point.x - Distance1, Point.y + AdjustYCoordinate( Distance1 ) );
	pRenderer->LineTo( Point.x - Distance2, Point.y + AdjustYCoordinate( Distance2 ) );
	pRenderer->MoveTo( Point.x + Distance1, Point.y + AdjustYCoordinate( Distance1 ) );
	pRenderer->LineTo( Point.x + Distance2, Point.y + AdjustYCoordinate( Distance2 ) );
	pRenderer->MoveTo( Point.x - Distance1, Point.y - AdjustYCoordinate( Distance1 ) );
	pRenderer->LineTo( Point.x - Distance2, Point.y - AdjustYCoordinate( Distance2 ) );

	pRenderer->SetTextAlign( TA_TOP | TA_LEFT );
	pRenderer->SetBkMode( TRANSPARENT );
	pRenderer->TextOut( Point.x + UnscaledUnits( 11 ), Point.y - ( UnscaledUnits( m_UsingFontHeight + 1 ) / 2 ), GetErrorText( Error ) );

	pRenderer->SelectObject( pOldPen );
}

void CLinkageView::DrawConnector( CRenderer* pRenderer, unsigned int OnLayers, CConnector* pConnector, bool bShowlabels, bool bUnused, bool bHighlight, bool bDrawConnectedLinks, bool bControlKnob )
{
	if( ( pConnector->GetLayers() & OnLayers ) == 0 )
		return;

	// Draw only the connector in the proper fashion
	CPen Pen;
	CPen BlackPen( PS_SOLID, 1, RGB( 0, 0, 0 ) );
	CPen* pOldPen = 0;
	CBrush Brush;
	CBrush *pOldBrush = 0;
	COLORREF Color;
	bool bSkipConnectorDraw = false;
	bool bLinkSelected = false;

	POSITION Position = pConnector->GetLinkList()->GetHeadPosition();
	while( Position != 0 )
	{
		CLink* pLink = pConnector->GetLinkList()->GetNext( Position );
		if( pLink != 0 )
		{
			if( pLink->IsSelected() )
				bLinkSelected = true;
		}
	}

	if( !pConnector->IsOnLayers( CLinkageDoc::MECHANISMLAYERS ) )
	{
		Color = pConnector->IsAlone() ? pConnector->GetColor() : COLOR_DRAWINGDARK;
		bSkipConnectorDraw = !bControlKnob;
		POSITION Position = pConnector->GetLinkList()->GetHeadPosition();
		while( Position != 0 )
		{
			CLink* pLink = pConnector->GetLinkList()->GetNext( Position );
			if( pLink != 0 && !pLink->IsMeasurementElement() )
			{
				bSkipConnectorDraw = false;
				break;
			}
		}
	}
	else if( pRenderer->GetColorCount() == 2 )
		Color = RGB( 0, 0, 0 );
	else
	{
		Color = pConnector->GetColor();
		int UsefulLinkCount = 0;
		POSITION Position = pConnector->GetLinkList()->GetHeadPosition();
		if( Position != 0 && pConnector->GetLinkList()->GetCount() == 1 )
		{
			CLink* pLink = pConnector->GetLinkList()->GetNext( Position );
			if (pLink != 0 && pLink->GetConnectorCount() > 1)
			{
				++UsefulLinkCount;
				Color = pLink->GetColor();
			}
		}
		if( UsefulLinkCount > 1 )
			Color = RGB(0, 0, 0);
	}

	if( ( m_SelectedEditLayers & CLinkageDoc::MECHANISMLAYERS ) == 0 )
		Color = LightenColor( DesaturateColor( Color, 0.5 ), 0.6 );

	CFPoint Point = pConnector->GetPoint();
	Point = Scale( Point );

	if( !bSkipConnectorDraw )
	{
		if( !pConnector->IsOnLayers( CLinkageDoc::MECHANISMLAYERS ) )
		{
			Pen.CreatePen( PS_SOLID, 1, Color );
			Brush.CreateSolidBrush( Color );
		}
		else if( pConnector->IsSelected() )
		{
			Pen.CreatePen( PS_SOLID, 1, Color );
			Brush.CreateSolidBrush( Color );
		}
		else
		{
			Pen.CreatePen( PS_SOLID, 1, Color );
			Brush.CreateStockObject( NULL_BRUSH );
		}

		if( bShowlabels && !bControlKnob )
		{
			/*
			 * Don't draw generic ID values for drawing elements. Only
			 * mechanism elements can show an ID string when no name is
			 * assigned by the user.
			 */

			double dx = m_ConnectorRadius - UnscaledUnits( 1 );
			double dy = m_ConnectorRadius - UnscaledUnits( 2 );
			if( pConnector->IsInput() )
			{
				dx += UnscaledUnits( 2 );
				dy += UnscaledUnits( 2 );
			}
			if( pConnector->IsDrawing() )
			{
				dx += UnscaledUnits( 3 );
				dy -= UnscaledUnits( m_UsingFontHeight + 1 );
			}

			if( pConnector->IsLocked() )
			{
				CFPoint LockLocation( Point.x + dx, Point.y + AdjustYCoordinate( dy ) );
				LockLocation.x += m_ConnectorRadius;
				LockLocation.y += AdjustYCoordinate( m_ConnectorRadius );
				DrawLock( pRenderer, LockLocation );
				dx += UnscaledUnits( 12 );
			}

			CString &Name = pConnector->GetName();
			if( !Name.IsEmpty() || ( pConnector->GetLayers() & CLinkageDoc::MECHANISMLAYERS ) != 0 || m_bShowDebug )
			{
				CString Number;
				if( pConnector->IsAlone() && m_bShowDebug )
					Number = pConnector->GetLink( 0 )->GetIdentifierString( m_bShowDebug ) + "  ";
				Number += pConnector->GetIdentifierString( m_bShowDebug );

				pRenderer->SetTextAlign( TA_BOTTOM | TA_LEFT );
				pRenderer->SetBkMode( TRANSPARENT );

				pRenderer->TextOut( Point.x + dx, Point.y + AdjustYCoordinate( dy ), Number );

				if( pConnector->IsInput() )
				{
					pRenderer->SetTextAlign( TA_TOP | TA_CENTER );
					pRenderer->SetBkMode( TRANSPARENT );
					CString Speed;
					double RPM = pConnector->GetRPM();
					if( fabs( RPM - floor( RPM ) ) < .0001 )
						Speed.Format( "%d", (int)RPM );
					else
						Speed.Format( "%.2lf", RPM );
					if( pConnector->IsAlwaysManual() )
						Speed += "m";
					double yOffset = m_ConnectorRadius + UnscaledUnits( 2 );
					if( pConnector->IsAnchor() )
						yOffset = ( m_ConnectorTriangle * UnscaledUnits( 2 ) ) + m_ArrowBase;
					pRenderer->TextOut( Point.x, Point.y - AdjustYCoordinate( yOffset ), Speed );
				}
			}
		}

		pOldPen = pRenderer->SelectObject( &Pen );
		pOldBrush = pRenderer->SelectObject( &Brush );

		if( !pConnector->IsOnLayers( CLinkageDoc::MECHANISMLAYERS ) )
		{
			if( bControlKnob )
			{
				double Radius = m_ConnectorRadius;
				Radius -= UnscaledUnits( 1 );
				CFCircle Circle( Point, Radius );
				pRenderer->Circle( Circle );
			}
			else if( pConnector->IsAlone() )
			{
				pRenderer->MoveTo( Point.x, Point.y + AdjustYCoordinate( m_ConnectorRadius ) );
				pRenderer->LineTo( Point.x, Point.y - AdjustYCoordinate( m_ConnectorRadius ) );
				pRenderer->MoveTo( Point.x - m_ConnectorRadius, Point.y );
				pRenderer->LineTo( Point.x + m_ConnectorRadius, Point.y );
			}
			else
			{
				CFCircle Circle( Point, UnscaledUnits( 1 ) );
				pRenderer->Circle( Circle );
			}
		}
		else if( pConnector->IsSlider() && !bControlKnob )
		{
			double Angle = 0.0;
			if( pConnector->GetSlideRadius() != 0 )
			{
				CFArc Arc;
				if( pConnector->GetSliderArc( Arc ) )
					Angle = GetAngle( pConnector->GetPoint(), Arc.GetCenter() );
			}
			else
			{
				CFPoint Point1;
				CFPoint Point2;
				if( pConnector->GetSlideLimits( Point1, Point2 ) )
					Angle = GetAngle( Point1, Point2 );
			}
			pRenderer->DrawRect( -Angle, Point.x - m_ConnectorRadius, Point.y + AdjustYCoordinate( m_ConnectorRadius ), Point.x + m_ConnectorRadius, Point.y - AdjustYCoordinate( m_ConnectorRadius ) );
		}
		else
		{
			double Radius = m_ConnectorRadius;
			if( bControlKnob )
				Radius -= UnscaledUnits( 1 );

			pRenderer->Arc( Point.x - Radius, Point.y + AdjustYCoordinate( Radius ), Point.x + Radius, Point.y - AdjustYCoordinate( Radius ), Point.x, Point.y + AdjustYCoordinate( Radius ), Point.x, Point.y + AdjustYCoordinate( Radius ) );
		}

		if( pConnector->IsAnchor() )
		{
			pRenderer->MoveTo( Point.x, Point.y );
			pRenderer->LineTo( Point.x - m_ConnectorTriangle, Point.y - AdjustYCoordinate( m_ConnectorTriangle * 2 ) );
			pRenderer->LineTo( Point.x + m_ConnectorTriangle, Point.y - AdjustYCoordinate( m_ConnectorTriangle * 2 ) );
			pRenderer->MoveTo( Point.x, Point.y );
			pRenderer->LineTo( Point.x + m_ConnectorTriangle, Point.y - AdjustYCoordinate( m_ConnectorTriangle * 2 ) );

			// Little line thingies.
			for( double Counter = 0; Counter <= m_ConnectorTriangle * 2; Counter += UnscaledUnits( 3 ) )
			{
				pRenderer->MoveTo( Point.x - m_ConnectorTriangle + Counter - UnscaledUnits( 2 ), Point.y - AdjustYCoordinate( m_ConnectorTriangle + m_ConnectorTriangle + m_ArrowBase ) );
				pRenderer->LineTo( Point.x - m_ConnectorTriangle + Counter, Point.y - AdjustYCoordinate( m_ConnectorTriangle + m_ConnectorTriangle ) );
			}
		}

		if( pConnector->IsInput() )
		{
			double Radius = ( m_ConnectorRadius + UnscaledUnits( 2 ) );
			double LimitRadius = ( m_ConnectorRadius + UnscaledUnits( 4 ) );
			pRenderer->Arc( Point.x - Radius, Point.y + AdjustYCoordinate( Radius ), Point.x + Radius, Point.y - AdjustYCoordinate( Radius ), Point.x, Point.y + AdjustYCoordinate( Radius ), Point.x, Point.y + AdjustYCoordinate( Radius ) );

			double MarkingAngle = 0.0;
			if( pConnector->GetLimitAngle() > 0 )
			{
				MarkingAngle = OscillatedAngle( pConnector->GetStartOffset() * pConnector->GetDirection(), pConnector->GetLimitAngle() );

				CFPoint Start( Point.x, Point.y + AdjustYCoordinate( LimitRadius ) );
				CFPoint End( Point.x, Point.y + AdjustYCoordinate( LimitRadius ) );
				if( pConnector->GetRPM() < 0 )
					End.RotateAround( Point, -pConnector->GetLimitAngle() );
				else
					Start.RotateAround( Point, pConnector->GetLimitAngle() );

				CFArc LimitArc( Point, LimitRadius, Start, End );
				pRenderer->Arc( LimitArc );
			}

			// Draw a small line to show rotation angle.
			CFPoint StartPoint( Point.x, Point.y + AdjustYCoordinate( m_ConnectorRadius ) );
			CFPoint EndPoint( Point.x, Point.y + AdjustYCoordinate( m_ConnectorRadius + UnscaledUnits( 2 ) ) );
			StartPoint.RotateAround( Point, -( pConnector->GetRotationAngle() + MarkingAngle ) );
			EndPoint.RotateAround( Point, -( pConnector->GetRotationAngle() + MarkingAngle ) );
			pRenderer->MoveTo( StartPoint );
			pRenderer->LineTo( EndPoint );
		}

		if( pConnector->IsDrawing() && bShowlabels )
		{
			pRenderer->SelectObject( &BlackPen );
			CFPoint Pencil( Point );
			Pencil.x += m_ConnectorRadius;
			Pencil.y += AdjustYCoordinate( m_ConnectorRadius );

			if( pConnector->IsInput() )
			{
				/*
				 * no one ever sets an input to draw but make the pencil look
				 * good if they do it.
				 */
				Pencil.x += UnscaledUnits( 2 );
				Pencil.y -= UnscaledUnits( 2 );
			}

			pRenderer->MoveTo( Pencil.x, Pencil.y );
			pRenderer->LineTo( Pencil.x, Pencil.y + AdjustYCoordinate( UnscaledUnits( 2 ) ) );
			pRenderer->LineTo( Pencil.x + UnscaledUnits( 3 ), Pencil.y + AdjustYCoordinate( UnscaledUnits( 5 ) ) );
			pRenderer->LineTo( Pencil.x + UnscaledUnits( 5 ), Pencil.y + AdjustYCoordinate( UnscaledUnits( 3 ) ) );
			pRenderer->LineTo( Pencil.x + UnscaledUnits( 2 ), Pencil.y );
			pRenderer->LineTo( Pencil.x, Pencil.y );
			pRenderer->SelectObject( &Pen );
		}

		if( !pConnector->IsPositionValid() )
			DrawFailureMarks( pRenderer, OnLayers, Point, m_ConnectorRadius, pConnector->GetSimulationError() );
	}

	double Radius = pConnector->GetDrawCircleRadius();
	if( Radius != 0 )
	{
		Radius = Scale( Radius );
		CFCircle Circle( Point, Radius );
		CBrush *pBrush = (CBrush*)pRenderer->SelectStockObject( NULL_BRUSH );
		CPen CirclePen(  PS_SOLID, pConnector->GetLineSize(), pConnector->GetColor() );
		CPen *pPen = pRenderer->SelectObject( &CirclePen );
		CFArc Arc( Circle.GetCenter(), Circle.r, Circle.GetCenter(), Circle.GetCenter() );
		pRenderer->Arc( Arc );
		pRenderer->SelectObject( pPen );
		pRenderer->SelectObject( pBrush );
	}

	if( m_bShowSelection )
	{
		if( pConnector->IsSelected() || bHighlight )
		{
			CLinkageDoc* pDoc = GetDocument();
			ASSERT_VALID(pDoc);
			CBrush Brush( pDoc->GetLastSelectedConnector() == pConnector ? COLOR_LASTSELECTION : COLOR_BLACK );
			static const int BOX_SIZE = 5;
			int Adjustment = ( BOX_SIZE - 1 ) / 2;
			CFRect Rect( Point.x - Adjustment,  Point.y + AdjustYCoordinate( Adjustment ),  Point.x + Adjustment,  Point.y - AdjustYCoordinate( Adjustment ) );

			#if defined( LINKAGE_USE_DIRECT2D )
				// Selection is only shown on-screen and Direct2D makes the rectangle look too small. So enlarge it.
				Rect.InflateRect( 1, 1 );
			#endif
			pRenderer->FillRect( &Rect, &Brush );

			Rect.SetRect( Point.x,  Point.y,  Point.x,  Point.y );
			//Rect = Scale( Rect );
			Rect.InflateRect( 2 * m_ConnectorRadius, 2 * m_ConnectorRadius );

			CPen GrayPen;
			GrayPen.CreatePen( PS_SOLID, 1, COLOR_MINORSELECTION );
			pRenderer->SelectObject( &GrayPen );

			if( !bLinkSelected )
				pRenderer->DrawRect( Rect );

			DrawFasteners( pRenderer, OnLayers, pConnector );
		}
		if( pConnector->IsLinkSelected() && !pConnector->IsAlone() )
		{
			CPen GrayPen;
			GrayPen.CreatePen( PS_SOLID, 1, COLOR_MINORSELECTION ) ;
			pRenderer->SelectObject( &GrayPen );
			pRenderer->Arc( CFArc( Point, m_ConnectorRadius * 2, Point, Point ) );
		}
	}

	if( pOldPen != 0 )
		pRenderer->SelectObject( pOldPen );
	if( pOldBrush != 0 )
		pRenderer->SelectObject( pOldBrush );
}

void CLinkageView::DrawControlKnob( CRenderer* pRenderer, unsigned int OnLayers, CControlKnob* pControlKnob )
{
	CElement *pParent = pControlKnob->GetParent();

	if( pParent == 0 || ( pParent->GetLayers() & OnLayers ) == 0 )
		return;

	if( !pParent->IsSelected() && pControlKnob->IsShowOnParentSelect() )
		return;

	if( pControlKnob->IsSelected() )
		return; // hidden when being dragged.

	if( m_MouseAction != ACTION_NONE )
		return; // Don't draw if selection is not finished (user hasnt released mouse button yet).

	// Draw only the connector in the proper fashion
	CPen Pen;
	CPen BlackPen( PS_SOLID, 1, RGB( 0, 0, 0 ) );
	CPen* pOldPen = 0;
	CBrush Brush;
	CBrush *pOldBrush = 0;
	COLORREF Color;
	bool bSkipConnectorDraw = false;

	if( !pParent->IsOnLayers( CLinkageDoc::MECHANISMLAYERS ) )
		Color = pParent->GetColor();
	else if( pRenderer->GetColorCount() == 2 )
		Color = RGB( 0, 0, 0 );
	else
		Color = pParent->GetColor();

	CFPoint Point = pControlKnob->GetPoint();
	Point = Scale( Point );
	Pen.CreatePen( PS_SOLID, 1, Color );

	if( pControlKnob->IsSelected() )
		Brush.CreateSolidBrush( COLOR_ADJUSTMENTKNOBS );
	else
		Brush.CreateStockObject( NULL_BRUSH );

	pOldPen = pRenderer->SelectObject( &Pen );
	pOldBrush = pRenderer->SelectObject( &Brush );

	double Radius = m_ConnectorRadius;
	Radius -= UnscaledUnits( 1 );
	CFCircle Circle( Point, Radius );
	//pRenderer->Arc( Point.x - Radius, Point.y + AdjustYCoordinate( Radius ), Point.x + Radius, Point.y - AdjustYCoordinate( Radius ), Point.x, Point.y + AdjustYCoordinate( Radius ), Point.x, Point.y + AdjustYCoordinate( Radius ) );
	//if( pControlKnob->IsSelected() )
		pRenderer->Circle( Circle );

	if( m_bShowSelection && pParent->IsSelected() && 0 )
	{
		CLinkageDoc* pDoc = GetDocument();
		ASSERT_VALID(pDoc);
		CBrush Brush( COLOR_BLACK );
		static const int BOX_SIZE = 5;
		int Adjustment = ( BOX_SIZE - 1 ) / 2;
		CFRect Rect( Point.x - Adjustment,  Point.y + AdjustYCoordinate( Adjustment ),  Point.x + Adjustment,  Point.y - AdjustYCoordinate( Adjustment ) );

		#if defined( LINKAGE_USE_DIRECT2D )
			// Selection is only shown on-screen and Direct2D makes the rectangle look too small. So enlarge it.
			Rect.InflateRect( 1, 1 );
		#endif
		pRenderer->FillRect( &Rect, &Brush );

		Rect.SetRect( Point.x,  Point.y,  Point.x,  Point.y );
		//Rect = Scale( Rect );
		Rect.InflateRect( 2 * m_ConnectorRadius, 2 * m_ConnectorRadius );

		CPen GrayPen;
		GrayPen.CreatePen( PS_SOLID, 1, COLOR_MINORSELECTION ) ;
		pRenderer->SelectObject( &GrayPen );

		pRenderer->DrawRect( Rect );
	}

	if( pOldPen != 0 )
		pRenderer->SelectObject( pOldPen );
	if( pOldBrush != 0 )
		pRenderer->SelectObject( pOldBrush );
}

void CLinkageView::DebugDrawLink( CRenderer* pRenderer, unsigned int OnLayers, CLink *pLink, bool bShowlabels, bool bDrawHighlight, bool bDrawFill )
{
	if( !m_bShowDebug )
		return;

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CPen CustomPen;
	CustomPen.CreatePen( PS_USERSTYLE, 1, RGB( 0, 0, 0 ) );
	CPen *pOldPen = pRenderer->SelectObject( &CustomPen );

	CFPoint AveragePoint;
	pLink->GetAveragePoint( *pDoc->GetGearConnections(), AveragePoint );
	CFPoint Point = Scale( AveragePoint );

	pRenderer->SetTextColor( RGB( 25, 0, 0 ) );
	pRenderer->SetBkMode( OPAQUE );
	pRenderer->SetTextAlign( TA_LEFT | TA_TOP );
	pRenderer->SetBkColor( RGB( 255, 255, 255 ) );

	CString String;
	double DocumentScale = pDoc->GetUnitScale();
	String.Format( "RPM %.6lf", pLink->GetRPM() );

	double Radius = m_ConnectorRadius * 5;

	pRenderer->TextOut( Point.x + Radius, Point.y - ( UnscaledUnits( m_UsingFontHeight + 1 ) / 2 ), String );

	pRenderer->SelectObject( pOldPen );
}

void CLinkageView::DrawFasteners( CRenderer* pRenderer, unsigned int OnLayers, CElement *pElement )
{
	ElementList TempList;
	CElementItem *pTempItem = 0;

	if( pElement == 0 )
		return;

	CElementItem *pFastenedTo = pElement->GetFastenedTo();
	if( pFastenedTo != 0 && pFastenedTo->GetElement() != 0 && !pFastenedTo->GetElement()->IsSelected() )
		TempList.AddHead( pFastenedTo );

	ElementList *pList = pElement->GetFastenedElementList();
	if( pList != 0 )
	{
		POSITION Position = pList->GetHeadPosition();
		while( Position != 0 )
		{
			CElementItem *pItem = pList->GetNext( Position );
			if( pItem != 0 )
			{
				if( ( pItem->GetConnector() != 0 && !pItem->GetConnector()->IsSelected() )
					|| ( pItem->GetLink() != 0 && !pItem->GetLink()->IsSelected() ) )
					TempList.AddHead( pItem );
			}
		}
	}

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CBrush Brush;
	Brush.CreateSolidBrush( COLOR_MINORSELECTION );
	LOGBRUSH lBrush = { 0 };
	Brush.GetLogBrush( &lBrush );
    DWORD Style[] = { 2, 1 };
	CPen CustomPen;
	CustomPen.CreatePen( PS_USERSTYLE, 1, &lBrush, 2, Style );

	CPen *pOldPen = pRenderer->SelectObject( &CustomPen, Style, 2, 1, COLOR_MINORSELECTION );

	POSITION Position = TempList.GetHeadPosition();
	while( Position != 0 )
	{
		CElementItem *pItem = TempList.GetNext( Position );
		if( pItem == 0 )
			continue;

		CFRect AreaRect;
		if( pItem->GetConnector() != 0 )
			pItem->GetConnector()->GetArea( AreaRect );
		else
			pItem->GetLink()->GetArea( *pDoc->GetGearConnections(), AreaRect );

//		if( pFastenedTo->GetConnectorList()->GetCount() > 1 || pFastenedTo->IsGear() )
//		{
			AreaRect = Scale( AreaRect );
			AreaRect.InflateRect( 2 + m_ConnectorRadius, 2 + m_ConnectorRadius );
			pRenderer->DrawRect( AreaRect );
//		}
	}

	pRenderer->SelectObject( pOldPen );

	if( pTempItem != 0 )
		delete pTempItem;
}

void CLinkageView::DrawActuator( CRenderer* pRenderer, unsigned int OnLayers, CLink *pLink, bool bDrawFill )
{
	if( pLink->GetConnectorCount() != 2 )
		return;

	if( ( pLink->GetLayers() & OnLayers ) == 0 )
		return;

	COLORREF Color;

	if( pRenderer->GetColorCount() == 2 )
		Color = RGB( 0, 0, 0 );
	else
		Color = pLink->GetColor(); //Colors[pLink->GetIdentifier() % COLORS_COUNT];

	double Red = GetRValue( Color ) / 255.0;
	double Green = GetGValue( Color ) / 255.0;
	double Blue = GetBValue( Color ) / 255.0;
	if( Red < 1.0 )
		Red += ( 1.0 - Red ) * .7;
	if( Green < 1.0 )
		Green += ( 1.0 - Green ) * .7;
	if( Blue < 1.0 )
		Blue += ( 1.0 - Blue ) * .7;

	COLORREF SecondColor = RGB( (int)( Red * 255 ), (int)( Green * 255 ), (int)( Blue * 255 ) );

	if( ( m_SelectedEditLayers & CLinkageDoc::MECHANISMLAYERS ) == 0 )
	{
		Color = LightenColor( DesaturateColor( Color, 0.5 ), 0.6 );
		SecondColor = LightenColor( DesaturateColor( SecondColor, 0.5 ), 0.6 );
	}

	int PointCount = 0;
	CFPoint* Points = pLink->GetHull( PointCount );

	CFPoint StrokePoint;
	if( !pLink->GetStrokePoint( StrokePoint ) )
		StrokePoint = Points[1];

	CFLine Line( Points[0], Points[1] );

	int StrokeLineSize = pLink->GetLineSize();

	if( pLink->IsSolid() )
	{
		CFPoint TempPoints[2];
		TempPoints[0] = Scale( Points[0] );
		TempPoints[1] = Scale( StrokePoint );

		pRenderer->LinkageDrawExpandedPolygon( TempPoints, 0, 2, Color, bDrawFill, pLink->IsSolid() ? m_ConnectorRadius + 3 : 1 );

		StrokeLineSize = 3;
	}
	else
	{
		double LineSize = pLink->GetLineSize() * 5;
		CPen Pen;
		Pen.CreatePen( PS_SOLID, (int)( LineSize ), SecondColor );
		CBrush Brush;
		Brush.CreateSolidBrush( SecondColor );
		CPen *pOldPen = pRenderer->SelectObject( &Pen );
		CBrush *pOldBrush = pRenderer->SelectObject( &Brush );

		CFLine NewLine;
		NewLine = Line;
		NewLine.SetLength( pLink->GetStroke() );

		double Radius = UnscaledUnits( LineSize / 2 );
		CFCircle Circle( Scale( NewLine.GetStart() ), Radius );
		pRenderer->Circle( Circle );
		Circle.SetCircle( Scale( NewLine.GetEnd() ), Radius );
		pRenderer->Circle( Circle );

		NewLine = Scale( NewLine );
		if( NewLine.GetLength() >= Radius * 2 )
		{
			NewLine.MoveEnds( Radius, -Radius );
			pRenderer->MoveTo( NewLine.GetStart() );
			pRenderer->LineTo( NewLine.GetEnd() );
		}

		pRenderer->SelectObject( pOldPen );
		pRenderer->SelectObject( pOldBrush );
	}

	double ExtendDistance = OscillatedDistance( pLink->GetTempActuatorExtension() + pLink->GetStartOffset(), pLink->GetStroke() );

	Line.ReverseDirection();
	if( pLink->GetCPM() >= 0 )
		Line.SetLength( Line.GetLength() - ExtendDistance );
	else
		Line.SetLength( Line.GetLength() - pLink->GetStroke() + ExtendDistance );

	CPen Pen;
	Pen.CreatePen( PS_SOLID, StrokeLineSize, Color );
	CPen *pOldPen = pRenderer->SelectObject( &Pen );

	pRenderer->MoveTo( Scale( Line.GetStart() ) );
	pRenderer->LineTo( Scale( Line.GetEnd() ) );

	CPen LinePen;
	LinePen.CreatePen( PS_SOLID, 1, Color );
	pRenderer->SelectObject( &LinePen );

	CFCircle StrokePointCircle( Scale( StrokePoint ), m_ConnectorRadius - UnscaledUnits( 1 ) );
	pRenderer->Arc( StrokePointCircle );

	pRenderer->SelectObject( pOldPen );

	Points = 0;
}

void CLinkageView::DrawStackedConnectors( CRenderer* pRenderer, unsigned int OnLayers )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if( ( OnLayers & CLinkageDoc::MECHANISMLAYERS ) == 0 )
		return;

	if( m_bSimulating )
		return;

	CPen Pen;
	Pen.CreatePen( PS_SOLID, 1, RGB( 128, 128, 128 ) );
	CPen *pOldPen = pRenderer->SelectObject( &Pen );

	ConnectorList* pConnectors = pDoc->GetConnectorList();
	POSITION Position = pConnectors->GetHeadPosition();
	while( Position != NULL )
	{
		CConnector* pConnector = pConnectors->GetNext( Position );
		if( pConnector == 0 )
			continue;

		// Test for mechanism layers because all other layers don't need the stacking indicator.
		if( ( pConnector->GetLayers() & CLinkageDoc::MECHANISMLAYERS  ) == 0 )
			continue;

		POSITION Position2 = pConnectors->GetHeadPosition();
		while( Position2 != NULL )
		{
			CConnector* pConnector2 = pConnectors->GetNext( Position2 );
			if( pConnector2 == 0 || pConnector2 == pConnector )
				continue;

			if( ( pConnector->GetLayers() & pConnector2->GetLayers() ) == 0 )
				continue;

			CFPoint Point1 = Scale( pConnector->GetPoint() );
			CFPoint Point2 = Scale( pConnector2->GetPoint() );

			// Compare pixel locatons - convert to integers to detect pixel variations.
			if( (int)Point1.x == (int)Point2.x && (int)Point1.y == (int)Point2.y )
			{
				double Radius = ( m_ConnectorRadius + UnscaledUnits( 2 ) );
				if( pConnector->IsInput() || pConnector2->IsInput() )
					Radius += UnscaledUnits( 2 );
				pRenderer->Arc( Point1.x - Radius, Point1.y + AdjustYCoordinate( Radius ), Point1.x + Radius, Point1.y - AdjustYCoordinate( Radius ), Point1.x - Radius, Point1.y + AdjustYCoordinate( Radius ), Point1.x + Radius, Point1.y - AdjustYCoordinate( Radius ) );
			}
		}
	}

	pRenderer->SelectObject( pOldPen );
}

CFArea CLinkageView::DrawSliderRadiusDimensions( CRenderer* pRenderer, CLinkageDoc *pDoc, unsigned int OnLayers, bool bDrawLines, bool bDrawText )
{
	if( !m_bShowDimensions )
		return CFArea();

	if( ( OnLayers & CLinkageDoc::MECHANISMLAYERS ) == 0 )
		return CFArea();

	CFArea DimensionsArea;

	CList<CFArc> ArcList; 

	CPen Pen;
	Pen.CreatePen( PS_SOLID, 1, RGB( 192, 192, 192 ) );
	CPen *pOldPen = pRenderer->SelectObject( &Pen );
	COLORREF OldFontColor = pRenderer->GetTextColor();
	pRenderer->SetTextColor( RGB( 96, 96, 96 ) );

	pRenderer->SetBkMode( OPAQUE );
	pRenderer->SetTextAlign( TA_CENTER | TA_TOP );
	pRenderer->SetBkColor( RGB( 255, 255, 255 ) );

	POSITION Position = pDoc->GetConnectorList()->GetHeadPosition();
	while( Position != 0 )
	{
		CConnector *pConnector = pDoc->GetConnectorList()->GetNext( Position );
		if( pConnector == 0 || !pConnector->IsSlider() )
			continue;

		if( pConnector->GetOriginalSlideRadius() != 0.0 )
		{
			CFArc SliderArc;
			if( !pConnector->GetSliderArc( SliderArc ) )
				continue;

			bool bAlreadyInList = false;
			POSITION Position2 = ArcList.GetHeadPosition();
			while( Position2 != 0 )
			{
				CFArc Arc = ArcList.GetNext( Position2 );
				if( SliderArc == Arc )
				{
					bAlreadyInList = true;
					break;
				}
			}

			if( bAlreadyInList )
				continue;

			// Draw this one and add it to the list.
			DimensionsArea += DrawArcRadius( pRenderer, SliderArc, bDrawLines, bDrawText );

			ArcList.AddTail( SliderArc );
		}
	}
	pRenderer->SelectObject( pOldPen );
	pRenderer->SetTextColor( OldFontColor );

	return DimensionsArea;
}

CFArea CLinkageView::DrawGroundDimensions( CRenderer* pRenderer, CLinkageDoc *pDoc, unsigned int OnLayers, CLink *pGroundLink, bool bDrawLines, bool bDrawText )
{
	if( !m_bShowDimensions || !m_bShowGroundDimensions )
		return CFArea();

	if( ( OnLayers & CLinkageDoc::MECHANISMLAYERS ) == 0 )
		return CFArea();

	POSITION Position;

	if( pGroundLink == 0 && false )
	{
		int Anchors = 0;
		Position = pDoc->GetConnectorList()->GetHeadPosition();
		while( Position != 0 )
		{
			CConnector *pConnector = pDoc->GetConnectorList()->GetNext( Position );
			if( pConnector == 0 || !pConnector->IsAnchor() )
				continue;

			++Anchors;
		}

		if( Anchors == 0 ) // Empty or non-functioning mechanism.
			return 0;

		Position = pDoc->GetLinkList()->GetHeadPosition();
		while( Position != 0 )
		{
			CLink *pLink = pDoc->GetLinkList()->GetNext( Position );
			if( pLink == 0  )
				continue;

			int FoundAnchors = pLink->GetAnchorCount();
			if( FoundAnchors > 0 && FoundAnchors == Anchors )
				return CFArea(); // The total anchor count all on this link so there is no need for a ground link.
		}
	}

	CFArea DimensionsArea;

	int ConnectorCount = 0;
	CConnector *pLeftMost = 0;
	CConnector *pBottomMost = 0;
	double xAllLeftMost = DBL_MAX;
	double yAllBottomMost = DBL_MAX;

	ConnectorList* pConnectors;
	if( pGroundLink == 0 )
		pConnectors = pDoc->GetConnectorList();
	else
		pConnectors = pGroundLink->GetConnectorList();

	Position = pConnectors->GetHeadPosition();
	while( Position != NULL )
	{
		CConnector* pConnector = pConnectors->GetNext( Position );
		if( pConnector == 0 )
			continue;

		// Keep track of the left-most and bottom-most connectors for
		// positioning the measurement lines later.
		if( m_bSimulating )
		{
			xAllLeftMost = min( xAllLeftMost, pConnector->GetOriginalPoint().x );
			yAllBottomMost = min( yAllBottomMost, pConnector->GetOriginalPoint().y );
		}
		else
		{
			// The current points are used when not simulating.
			xAllLeftMost = min( xAllLeftMost, pConnector->GetPoint().x );
			yAllBottomMost = min( yAllBottomMost, pConnector->GetPoint().y );
		}

		if( pGroundLink == 0 && !pConnector->IsAnchor() )
			continue;

		++ConnectorCount;

		if( pLeftMost == 0 || pConnector->GetPoint().x < pLeftMost->GetPoint().x )
			pLeftMost = pConnector;
		if( pBottomMost == 0 || pConnector->GetPoint().y < pBottomMost->GetPoint().y )
			pBottomMost = pConnector;
	}

	if( ConnectorCount <= 1 )
		return DimensionsArea;

	vector<ConnectorDistance> ConnectorReference( ConnectorCount );
	vector<ConnectorDistance> ConnectorReferencePerp( ConnectorCount );

	CPen Pen;
	Pen.CreatePen( PS_SOLID, 1, RGB( 192, 192, 192 ) );
	CPen *pOldPen = pRenderer->SelectObject( &Pen );
	COLORREF OldFontColor = pRenderer->GetTextColor();
	pRenderer->SetTextColor( RGB( 96, 96, 96 ) );

	/*
	 * Create line(s) for alignment and comparison of the measurement points.
	 */

	CFLine OrientationLine( CFPoint( pLeftMost->GetPoint().x, pBottomMost->GetPoint().y ), CFPoint( pLeftMost->GetPoint().x + 1000, pBottomMost->GetPoint().y ) );

	CFLine PerpOrientationLine;
	CFLine Temp = OrientationLine;
	Temp.ReverseDirection();
	Temp.PerpendicularLine( PerpOrientationLine );

	/*
	 * Create lists of connectors that are sorted according to location along the
	 * two different orientation lines. The sorting is not necessary if there are
	 * only two connectors. Neither is the perpendicular orientation line.
	 */

	Position = pConnectors->GetHeadPosition();
	int Counter = 0;
	while( Position != NULL )
	{
		CConnector* pConnector = pConnectors->GetNext( Position );
		if( pConnector == 0 || ( pGroundLink == 0 && !pConnector->IsAnchor() ) )
			continue;

		ConnectorReference[Counter].m_pConnector = pConnector;
		ConnectorReference[Counter].m_Distance = DistanceAlongLine( OrientationLine, pConnector->GetPoint() );
		ConnectorReferencePerp[Counter].m_pConnector = pConnector;
		ConnectorReferencePerp[Counter].m_Distance = DistanceAlongLine( PerpOrientationLine, pConnector->GetPoint() );
		++Counter;
	}

	/*
	 * Sort the connectors along the orientation lines.
	 */

	sort( ConnectorReference.begin(), ConnectorReference.end(), CompareConnectorDistance() );
	sort( ConnectorReferencePerp.begin(), ConnectorReferencePerp.end(), CompareConnectorDistance() );

	/*
	 * Draw Measurement Lines.
	 */

	double Offset = Scale( OrientationLine.GetStart().x - xAllLeftMost );
	Offset += OFFSET_INCREMENT;
	for( int Counter = 0; Counter < ConnectorCount; ++Counter )
	{
		if( fabs( ConnectorReferencePerp[Counter].m_Distance ) < 0.001 )
			continue;
		CFLine MeasurementLine = PerpOrientationLine;
		MeasurementLine.SetLength(  ConnectorReferencePerp[Counter].m_Distance );
		DimensionsArea += DrawMeasurementLine( pRenderer, MeasurementLine, ConnectorReferencePerp[Counter].m_pConnector->GetPoint(), pBottomMost->GetPoint(), Offset, bDrawLines, bDrawText );
		Offset += OFFSET_INCREMENT;
	}

	Offset = Scale( OrientationLine.GetStart().y - yAllBottomMost );

	if( ConnectorCount == 2 )
	{
		// Draw a possibly diagonal measurement between the anchors but only
		// if there are two of them. Any more than that and the diagonal
		// measurement will not be useful in real life.
		if( ConnectorReference[0].m_pConnector->GetSharingLink( ConnectorReference[1].m_pConnector ) == 0 || pGroundLink != 0 )
		{
			// Skip this is they are lined up horizontally or vertically
			// because there is already a measurement shown.
			if( fabs( ConnectorReference[0].m_pConnector->GetPoint().x - ConnectorReference[1].m_pConnector->GetPoint().x ) > 0.001
			    && fabs( ConnectorReference[0].m_pConnector->GetPoint().y - ConnectorReference[1].m_pConnector->GetPoint().y ) > 0.001 )
			{
				CFLine MeasurementLine( ConnectorReference[0].m_pConnector->GetPoint(), ConnectorReference[1].m_pConnector->GetPoint() );

				double UseOffset = pGroundLink == 0 ? 0 : -OFFSET_INCREMENT;
				DimensionsArea += DrawMeasurementLine( pRenderer, MeasurementLine, MeasurementLine.GetEnd(), MeasurementLine.GetStart(), UseOffset, bDrawLines, bDrawText );

				double Angle = MeasurementLine.GetAngle();

				if( Angle < 45 )
					Offset += OFFSET_INCREMENT;
			}
		}
	}

	Offset += OFFSET_INCREMENT;
	for( int Counter = 0; Counter < ConnectorCount; ++Counter )
	{
		if( fabs( ConnectorReference[Counter].m_Distance ) < 0.001 )
			continue;
		CFLine MeasurementLine = OrientationLine;
		MeasurementLine.SetLength(  ConnectorReference[Counter].m_Distance );
		DimensionsArea += DrawMeasurementLine( pRenderer, MeasurementLine, ConnectorReference[Counter].m_pConnector->GetPoint(), pLeftMost->GetPoint(), -Offset, bDrawLines, bDrawText );
		Offset += OFFSET_INCREMENT;
	}

	pRenderer->SelectObject( pOldPen );
	pRenderer->SetTextColor( OldFontColor );

	return DimensionsArea;
}

CFLine CLinkageView::CreateMeasurementEndLine( CFLine &InputLine, CFPoint &MeasurementPoint, CFPoint &MeasurementLinePoint, int InputLinePointIndex, double Offset )
{
	bool bUseOffset = Offset == 0 ? false : true;
	Offset = Unscale( bUseOffset ? Offset : 8 );

	CFLine Line = InputLine;
	if( InputLinePointIndex > 0 )
		Line.ReverseDirection();
	Line.PerpendicularLine( Line, UnscaledUnits( Offset ), InputLinePointIndex == 0 ? 1 : -1 );
	MeasurementLinePoint = bUseOffset ? Line.m_End : Line.m_Start;
	Line.m_Start = MeasurementPoint;
	if( bUseOffset )
		Line.MoveEnds( Unscale( UnscaledUnits( 3 ) ), Unscale( UnscaledUnits( 8 ) ) );
	else
		Line.MoveEnds( -UnscaledUnits( Offset ), 0 );

	return Line;
}

void CLinkageView::DrawMeasurementLine( CRenderer* pRenderer, CFLine &InputLine, double Offset, bool bDrawLines, bool bDrawText )
{
	DrawMeasurementLine( pRenderer, InputLine, InputLine[1], InputLine[0], Offset, bDrawLines, bDrawText );
}

CFArea CLinkageView::DrawMeasurementLine( CRenderer* pRenderer, CFLine &InputLine, CFPoint &FirstPoint, CFPoint &SecondPoint, double Offset, bool bDrawLines, bool bDrawText )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	double DocumentScale = pDoc->GetUnitScale();

	CFArea DimensionsArea;
	CFLine MeasurementLine;

	CFLine Line1 = CreateMeasurementEndLine( InputLine, FirstPoint, MeasurementLine.m_Start, 0, Offset );
	CFLine Line2 = CreateMeasurementEndLine( InputLine, SecondPoint, MeasurementLine.m_End, 1, Offset );

	DimensionsArea += MeasurementLine;
	DimensionsArea += Line1;
	DimensionsArea += Line2;

	if( bDrawLines )
	{
		pRenderer->DrawLine( Scale( Line1 ) );
		pRenderer->DrawLine( Scale( Line2 ) );
	}

	MeasurementLine = Scale( MeasurementLine );

	double Length = DocumentScale * InputLine.GetLength();
	double DocUnitLength = MeasurementLine.GetLength();

	CString String;
	String.Format( "%.3lf", Length );

	if( bDrawLines )
	{
		CFLine Temp = MeasurementLine;

		if( DocUnitLength > UnscaledUnits( 9 ) )
		{
			Temp.MoveEnds( UnscaledUnits( 4 ), -UnscaledUnits( 4 ) );
			MeasurementLine.MoveEnds( UnscaledUnits( 0.5 ), -UnscaledUnits( 0.5 ) );
		}

		double TextCover = 0.0;

		if( bDrawText )
		{
			// Find where the line intersects the rectangle containing the text.
			CFLine CenteringLine = MeasurementLine;
			CenteringLine.SetLength( CenteringLine.GetLength() / 2 );
			CFPoint Center = CenteringLine.GetEnd();
			CFPoint HalfExtents = pRenderer->GetTextExtent( String, String.GetLength() );
			HalfExtents.x /= 2;
			HalfExtents.y = m_UsingFontHeight / 2; // More accurate number that doesn't include descenders and other anomolies not shown with numbers.
			
			CFLine Test1( Center.x - 1, Center.y - HalfExtents.y, Center.x + 1, Center.y - HalfExtents.y );
			CFLine Test2( Center.x - HalfExtents.x, Center.y - 1, Center.x - HalfExtents.x, Center.y + 1 );

			CFPoint Intersect1;
			CFPoint Intersect2;
			bool b1 = Intersects( CenteringLine, Test1, Intersect1 );
			bool b2 = Intersects( CenteringLine, Test2, Intersect2 );
			double Nearest = 0.0;

			if( b1 && b2 )
				Nearest = min( Distance( Center, Intersect1 ), Distance( Center, Intersect2 ) );
			else if( b1 )
				Nearest = Distance( Center, Intersect1 );
			else if( b2 )
				Nearest = Distance( Center, Intersect2 );

			TextCover = Nearest * 2;

			double Cut = Nearest + UnscaledUnits( 3.0 );
			CFLine Temp2 = Temp;
			Temp2.ReverseDirection();
			if( Temp.GetLength() / 2 > Cut )
			{
				Temp.SetLength( ( Temp.GetLength() / 2 ) - Cut );
				Temp2.SetLength( ( Temp2.GetLength() / 2 ) - Cut );
				pRenderer->DrawLine( Temp );
				pRenderer->DrawLine( Temp2 );
			}
		}
		else
			pRenderer->DrawLine( Temp );

		if( DocUnitLength > UnscaledUnits( 9 ) && DocUnitLength - UnscaledUnits( 9 ) > TextCover )
		{
			pRenderer->DrawArrow( MeasurementLine.GetEnd(), MeasurementLine.GetStart(), UnscaledUnits( 3 ), UnscaledUnits( 5 ) );
			pRenderer->DrawArrow( MeasurementLine.GetStart(), MeasurementLine.GetEnd(), UnscaledUnits( 3 ), UnscaledUnits( 5 ) );
		}
	}

	if( bDrawText )
	{

		pRenderer->SetBkMode( OPAQUE );
		pRenderer->SetTextAlign( TA_CENTER | TA_TOP );
		pRenderer->SetBkColor( RGB( 255, 255, 255 ) );

		CFLine Temp( MeasurementLine );
		Temp.SetLength( MeasurementLine.GetLength() / 2 );
		pRenderer->TextOut( Temp.GetEnd().x, Temp.GetEnd().y + AdjustYCoordinate( ( UnscaledUnits( m_UsingFontHeight + 1 ) / 2 ) + 1 ), String );
	}
	return DimensionsArea;
}

CFArea CLinkageView::DrawCirclesRadius( CRenderer *pRenderer, CFPoint Center, std::list<double> &RadiusList, bool bDrawLines, bool bDrawText )
{
	if( pRenderer == 0 || RadiusList.size() == 0 )
		return CFArea();

	CFArea DimensionsArea;

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	std::list<double>::iterator it = RadiusList.end();
	--it;
	double LargestRadius = *it;
	double SecondLargest = 0.0;
	if( it != RadiusList.begin() )
	{
		--it;
		SecondLargest = *it;
	}

	if( SecondLargest > 0.0 )
	{
		if( bDrawLines )
		{
			CFPoint Point = Center;
			Point.SetPoint( Point, SecondLargest, 45 );
			CFLine MeasurementLine( Center, Point );
			MeasurementLine = Scale( MeasurementLine );

			CFLine Temp = MeasurementLine;
			if( pRenderer->GetScale() > 1.0 )
				Temp.MoveEnds( 0, -4 );

			pRenderer->DrawLine( Temp );
			pRenderer->DrawArrow( MeasurementLine.GetStart(), MeasurementLine.GetEnd(), UnscaledUnits( 3 ), UnscaledUnits( 5 ) );
		}

		if( bDrawText )
		{
			std::list<double>::iterator end = RadiusList.end();
			--end;
			for( std::list<double>::iterator it = RadiusList.begin(); it != end; ++it )
			{
				double Radius = *it;

				CFPoint Point = Center;
				Point.SetPoint( Point, Radius, 45 );
				CFLine MeasurementLine( Center, Point );
				MeasurementLine.SetLength( MeasurementLine.GetLength() / 2 );

				MeasurementLine = Scale( MeasurementLine );

				CString String;
				double DocumentScale = pDoc->GetUnitScale();
				String.Format( "R%.3lf", DocumentScale * Radius );

				pRenderer->SetTextAlign( TA_CENTER | TA_TOP );
				pRenderer->TextOut( MeasurementLine.GetEnd().x + UnscaledUnits( 2 ), MeasurementLine.GetEnd().y - ( UnscaledUnits( m_UsingFontHeight + 1 ) / 2 ) - UnscaledUnits( 1 ), String );
			}
		}
	}

	CFPoint Point1 = Center;
	Point1.SetPoint( Point1, LargestRadius, 45 );
	CFPoint Point2;
	Point2.SetPoint( Point1, Unscale( RADIUS_MEASUREMENT_OFFSET ), 45 );
	CFPoint Point3( Point2.x + Unscale( RADIUS_MEASUREMENT_OFFSET ), Point2.y );

	if( bDrawLines )
	{
		CFLine MeasurementLine( Point1, Point2 );
		MeasurementLine = Scale( MeasurementLine );
		pRenderer->DrawLine( MeasurementLine );
		pRenderer->DrawArrow( MeasurementLine.GetEnd(), MeasurementLine.GetStart(), UnscaledUnits( 3 ), UnscaledUnits( 5 ) );
		MeasurementLine.SetLine( Point2, Point3 );
		MeasurementLine = Scale( MeasurementLine );
		pRenderer->DrawLine( MeasurementLine );
	}

	if( bDrawText )
	{
		CString String;
		double DocumentScale = pDoc->GetUnitScale();
		String.Format( "R%.3lf", DocumentScale * LargestRadius );

		CFPoint Point = Scale( Point3 );

		pRenderer->SetTextAlign( TA_LEFT | TA_TOP );
		pRenderer->TextOut( Point.x + UnscaledUnits( 2 ), Point.y - ( UnscaledUnits( m_UsingFontHeight + 1 ) / 2 ) - UnscaledUnits( 1 ), String );
	}

	return DimensionsArea;
}

CFArea CLinkageView::DrawArcRadius( CRenderer *pRenderer, CFArc &Arc, bool bDrawLines, bool bDrawText )
{
	if( pRenderer == 0 )
		return CFArea();

	CFArea DimensionsArea;

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	double Angle = Arc.GetStartAngle();
	double EndAngle = Arc.GetEndAngle();
	if( Angle == EndAngle )
		Angle = 45;
	else if( Angle > EndAngle )
		Angle += ( ( 360 - Angle ) + EndAngle ) / 2;
	else
		Angle += ( EndAngle - Angle ) / 2;
		
	CFPoint Point = Arc.GetCenter();
	Point.SetPoint( Point, fabs( Arc.r ), Angle );
	CFPoint Point2;
	Point2.SetPoint( Point, Unscale( RADIUS_MEASUREMENT_OFFSET * 2 ), Angle );
	CFPoint Point3;
	CFPoint Point4;
	if( Angle <= 90 && Angle >= -90 )
	{
		Point3.SetPoint( Point2.x + Unscale( RADIUS_MEASUREMENT_OFFSET ), Point2.y );
		Point4.SetPoint( Point3.x + UnscaledUnits( 2 ), Point3.y );
		pRenderer->SetTextAlign( TA_LEFT | TA_TOP );
	}
	else
	{
		Point3.SetPoint( Point2.x - Unscale( RADIUS_MEASUREMENT_OFFSET ), Point2.y );
		Point4.SetPoint( Point3.x - UnscaledUnits( 2 ), Point3.y );
		pRenderer->SetTextAlign( TA_RIGHT | TA_TOP );
	}

	if( bDrawLines )
	{
		CFLine MeasurementLine( Point, Point2 );
		MeasurementLine = Scale( MeasurementLine );

		CFLine Temp = MeasurementLine;
		if( pRenderer->GetScale() > 1.0 )
			Temp.MoveEnds( 0, -4 );

		pRenderer->DrawLine( Temp );
		pRenderer->DrawArrow( MeasurementLine.GetEnd(), MeasurementLine.GetStart(), UnscaledUnits( 3 ), UnscaledUnits( 5 ) );
		
		MeasurementLine.SetLine( Point2, Point3 );
		MeasurementLine = Scale( MeasurementLine );
		pRenderer->DrawLine( MeasurementLine );
	}

	if( bDrawText )
	{
		CString String;
		double DocumentScale = pDoc->GetUnitScale();
		String.Format( "R%.3lf", DocumentScale * fabs( Arc.r ) );

		Point4 = Scale( Point4 );

		pRenderer->TextOut( Point4.x, Point4.y - ( UnscaledUnits( m_UsingFontHeight + 1 ) / 2 ) - UnscaledUnits( 1 ), String );
	}

	return DimensionsArea;
}

CFArea CLinkageView::DrawConnectorLinkDimensions( CRenderer* pRenderer, const GearConnectionList *pGearConnections, unsigned int OnLayers, CLink *pLink, bool bDrawLines, bool bDrawText )
{
	if( !m_bShowDimensions )
		return CFArea();

	if( ( pLink->GetLayers() & OnLayers ) == 0 )
		return CFArea();

	if( ( pLink->GetLayers() & CLinkageDoc::DRAWINGLAYER ) != 0 && !m_bShowDrawingLayerDimensions )
		return CFArea();

	CPen Pen;
	Pen.CreatePen( PS_SOLID, 1, RGB( 192, 192, 192 ) );
	CPen *pOldPen = pRenderer->SelectObject( &Pen );
	COLORREF OldFontColor = pRenderer->GetTextColor();
	pRenderer->SetTextColor( RGB( 96, 96, 96 ) );

	pRenderer->SetBkMode( OPAQUE );
	pRenderer->SetTextAlign( TA_CENTER | TA_TOP );
	pRenderer->SetBkColor( RGB( 255, 255, 255 ) );

	int ConnectorCount = (int)pLink->GetConnectorList()->GetCount();
	if( ConnectorCount <= 1 )
	{
		CConnector* pConnector = pLink->GetConnector( 0 );
		if( pConnector != 0 )
		{
			if( pLink->IsGear() )
			{
				std::list<double> RadiusList;
				pLink->GetGearRadii( *pGearConnections, RadiusList );

				DrawCirclesRadius( pRenderer, pConnector->GetPoint(), RadiusList, bDrawLines, bDrawText );
			}

			if( ( pConnector->GetLayers() & CLinkageDoc::DRAWINGLAYER ) && bDrawText )
			{
				CLinkageDoc* pDoc = GetDocument();
				ASSERT_VALID(pDoc);

				CString String;
				double DocumentScale = pDoc->GetUnitScale();
				CFPoint Point = pConnector->GetPoint();
				String.Format( "%.3lf,%.3lf", DocumentScale * Point.x, DocumentScale * Point.y );

				Point = Scale( Point );
				pRenderer->TextOut( Point.x, Point.y + m_ConnectorRadius + UnscaledUnits( 2 ), String );
			}
		}
	}

	pRenderer->SelectObject( pOldPen );
	pRenderer->SetTextColor( OldFontColor );

	return CFArea();
}

static int GetLongestHullDimensionLine( CLink *pLink, int &BestEndPoint, CFPoint *pHullPoints, int HullPointCount, CConnector **pStartConnector )
{
	int BestStartPoint = -1;
	BestEndPoint = -1;

	if( pLink->GetConnectorCount() == 2 )
	{
		BestStartPoint = 0;
		BestEndPoint = 1;
	}
	else
	{
		double LargestDistance = 0.0;
		for( int Counter = 0; Counter < HullPointCount; ++Counter )
		{
			double TestDistance = 0;
			if( Counter < HullPointCount - 1 )
				TestDistance = Distance( pHullPoints[Counter], pHullPoints[Counter+1] );
			else
				TestDistance = Distance( pHullPoints[Counter], pHullPoints[0] );
			if( TestDistance > LargestDistance )
			{
				LargestDistance = TestDistance;
				BestStartPoint = Counter;
			}
		}
		// The hull is a clockwise collection of points so the end point of
		// the measurement is a better visual choice for the start connector.
		if( BestStartPoint < HullPointCount - 1 )
			BestEndPoint = BestStartPoint + 1;
		else
			BestEndPoint = 0;
	}

	if( pStartConnector != 0 && BestStartPoint >= 0 )
	{
		*pStartConnector = 0;
		POSITION Position = pLink->GetConnectorList()->GetHeadPosition();
		int Counter = 0;
		while( Position != 0 )
		{
			CConnector *pConnector = pLink->GetConnectorList()->GetNext( Position );
			if( pConnector == 0 )
				continue;

			if( pConnector->GetPoint() == pHullPoints[BestStartPoint] )
				*pStartConnector = pConnector;
		}
	}

	return BestStartPoint;
}

static int GetLongestLinearDimensionLine( CLink *pLink, int &BestEndPoint, CFPoint *pPoints, int PointCount, CConnector **pStartConnector )
{
	int BestStartPoint = -1;
	BestEndPoint = -1;

	if( pLink->GetConnectorCount() == 2 )
	{
		BestStartPoint = 0;
		BestEndPoint = 1;
	}
	else
	{
		double LargestDistance = 0.0;
		for( int Counter = 0; Counter < PointCount; ++Counter )
		{
			for( int Counter2 = 0; Counter2 < PointCount; ++Counter2 )
			{
				if( Counter2 == Counter )
					continue;
				double TestDistance = Distance( pPoints[Counter], pPoints[Counter2] );
				if( TestDistance > LargestDistance )
				{
					LargestDistance = TestDistance;
					BestStartPoint = Counter;
					BestEndPoint = Counter2;
				}
			}
		}
	}

	if( pStartConnector != 0 && BestStartPoint >= 0 )
	{
		*pStartConnector = 0;
		POSITION Position = pLink->GetConnectorList()->GetHeadPosition();
		int Counter = 0;
		while( Position != 0 )
		{
			CConnector *pConnector = pLink->GetConnectorList()->GetNext( Position );
			if( pConnector == 0 )
				continue;

			if( pConnector->GetPoint() == pPoints[BestStartPoint] )
				*pStartConnector = pConnector;
		}
	}

	return BestStartPoint;
}

CFArea CLinkageView::DrawDimensions( CRenderer* pRenderer, const GearConnectionList *pGearConnections, unsigned int OnLayers, CLink *pLink, bool bDrawLines, bool bDrawText )
{
	if( !m_bShowDimensions )
		return CFArea();

	if( ( pLink->GetLayers() & OnLayers ) == 0 )
		return CFArea();

	if( ( pLink->GetLayers() & CLinkageDoc::DRAWINGLAYER ) != 0 && !m_bShowDrawingLayerDimensions )
		return CFArea();

	CFArea DimensionsArea;

	int ConnectorCount = (int)pLink->GetConnectorList()->GetCount();
	if( ConnectorCount <= 1 )
		return DrawConnectorLinkDimensions( pRenderer, pGearConnections, OnLayers, pLink, bDrawLines, bDrawText );

	CPen Pen;
	Pen.CreatePen( PS_SOLID, 1, RGB( 192, 192, 192 ) );
	CPen *pOldPen = pRenderer->SelectObject( &Pen );
	COLORREF OldFontColor = pRenderer->GetTextColor();
	pRenderer->SetTextColor( RGB( 96, 96, 96 ) );

	pRenderer->SetBkMode( OPAQUE );
	pRenderer->SetTextAlign( TA_CENTER | TA_TOP );
	pRenderer->SetBkColor( RGB( 255, 255, 255 ) );

	//double LargestDistance = 0.0;

	/*
	 * Determine if the link is drawn as what appears to be a line (small deviations are ignored).
	 * If the link is a line, pick the points farthest apart as the basis for the measurement lines. Otherwise...
	 * Find two adjacent hull locations that have the largest distance between
	 * them. This is used to pick the orientation of the measurement lines.
	 * these points are not used for any other purpose.
	 */

	int BestStartPoint = -1;
	int BestEndPoint = -1;

	int PointCount = 0;
	CFPoint *pPoints = pLink->GetPoints( PointCount );
	bool bIsLine = true;
	for( int Index = 2; Index < PointCount; ++Index )
	{
		if( DistanceToLine( pPoints[0], pPoints[1], pPoints[Index] ) > 0.0001 )
		{
			bIsLine = false;
			break;
		}
	}

	CConnector *pStartConnector = 0;

	if( bIsLine )
	{
		BestStartPoint = GetLongestLinearDimensionLine( pLink, BestEndPoint, pPoints, PointCount, &pStartConnector );
	}
	else
	{
		PointCount = 0;
		pPoints = pLink->GetHull( PointCount );
		
		BestStartPoint = GetLongestHullDimensionLine( pLink, BestEndPoint, pPoints, PointCount, &pStartConnector );
	}

	if( BestStartPoint < 0 )
	{
		pRenderer->SelectObject( pOldPen );
		pRenderer->SetTextColor( OldFontColor );
		return CFArea();
	}

	/*
	 * Create line(s) for alignment and comparison of the measurement points.
	 */

	CFLine OrientationLine( pPoints[BestStartPoint], pPoints[BestEndPoint] );

	if( pLink->IsActuator() )
	{
		CFLine MeasurementLine = OrientationLine;

		POSITION Position = pLink->GetConnectorList()->GetHeadPosition();
		CConnector *pEndConnector = Position == 0 ? 0 : pLink->GetConnectorList()->GetNext( Position );
		if( pEndConnector == pStartConnector )
			pEndConnector = Position == 0 ? 0 : pLink->GetConnectorList()->GetNext( Position );
		if( pStartConnector == 0 || pEndConnector == 0 )
		{
			pRenderer->SelectObject( pOldPen );
			pRenderer->SetTextColor( OldFontColor );
			return CFArea();
		}

		CFLine LengthLine = OrientationLine;
		CFLine ThrowLine = OrientationLine;
		CFLine ExtensionLine;

		double StartDistance = Distance( pStartConnector->GetPoint(), pEndConnector->GetPoint() );

		LengthLine.SetLength( StartDistance );
		ThrowLine.SetLength( pLink->GetStroke() );
		ExtensionLine.SetLine( ThrowLine.GetEnd(), LengthLine.GetEnd() );

		double Offset = OFFSET_INCREMENT * -2;
		DimensionsArea += DrawMeasurementLine( pRenderer, LengthLine, LengthLine.GetEnd(), LengthLine.GetStart(), Offset, bDrawLines, bDrawText );

		Offset = -OFFSET_INCREMENT;
		DimensionsArea += DrawMeasurementLine( pRenderer, ThrowLine, ThrowLine.GetEnd(), ThrowLine.GetStart(), Offset, bDrawLines, bDrawText );
		DimensionsArea += DrawMeasurementLine( pRenderer, ExtensionLine, ExtensionLine.GetEnd(), ExtensionLine.GetStart(), Offset, bDrawLines, bDrawText );
	}
	else
	{
		CFLine PerpOrientationLine;
		CFLine Temp = OrientationLine;
		Temp.ReverseDirection();
		Temp.PerpendicularLine( PerpOrientationLine );

		/*
		 * Create lists of connectors that are sorted according to location along the
		 * two different orientation lines. The sorting is not necessary if there are
		 * only two connectors. Neither is the perpendicular orientation line.
		 */

		vector<ConnectorDistance> ConnectorReference( ConnectorCount );
		vector<ConnectorDistance> ConnectorReferencePerp( ConnectorCount );

		POSITION Position = pLink->GetConnectorList()->GetHeadPosition();
		int Counter = 0;
		while( Position != 0 )
		{
			CConnector *pConnector = pLink->GetConnectorList()->GetNext( Position );
			if( pConnector == 0 )
				continue;

			if( Counter < ConnectorCount )
			{
				ConnectorReference[Counter].m_pConnector = pConnector;
				ConnectorReference[Counter].m_Distance = DistanceAlongLine( OrientationLine, pConnector->GetPoint() );
				ConnectorReferencePerp[Counter].m_pConnector = pConnector;
				ConnectorReferencePerp[Counter].m_Distance = DistanceAlongLine( PerpOrientationLine, pConnector->GetPoint() );
				++Counter;
			}
		}

		/*
		 * Sort the connectors along the orientation lines.
		 */

		if( ConnectorCount > 2 )
		{
			sort( ConnectorReference.begin(), ConnectorReference.end(), CompareConnectorDistance() );
			sort( ConnectorReferencePerp.begin(), ConnectorReferencePerp.end(), CompareConnectorDistance() );
		}

		/*
		 * Draw Measurement Lines.
		 */

		double Offset = OFFSET_INCREMENT;
		double NextOffset = 0;
		int StartPointInList = 0;

		// Find start point in list.
		for( int Counter = 0; Counter < ConnectorCount; ++Counter )
		{
			if( ConnectorReference[Counter].m_pConnector == pStartConnector )
			{
				StartPointInList = Counter;
				break;
			}
		}

		NextOffset = /*(int)*/ConnectorReference[0].m_Distance;

		// Points before the start point on the orientation line.
		for( int Counter = StartPointInList - 1; Counter >= 0; --Counter )
		{
			if( fabs( ConnectorReference[Counter].m_Distance ) < 0.001 )
				continue;
			CFLine MeasurementLine = OrientationLine;
			MeasurementLine.SetLength( ConnectorReference[Counter].m_Distance );
			MeasurementLine.ReverseDirection();
			DimensionsArea += DrawMeasurementLine( pRenderer, MeasurementLine, pStartConnector->GetPoint(), ConnectorReference[Counter].m_pConnector->GetPoint(), Offset, bDrawLines, bDrawText );
			Offset -= OFFSET_INCREMENT;
		}

		Offset = -OFFSET_INCREMENT;

		// Points after the start point on the orientation line.
		for( int Counter = StartPointInList + 1; Counter < ConnectorCount; ++Counter )
		{
			if( ConnectorReference[Counter].m_pConnector == pStartConnector )
				break;
			if( fabs( ConnectorReference[Counter].m_Distance ) < 0.001 )
				continue;
			CFLine MeasurementLine = OrientationLine;
			MeasurementLine.SetLength(  ConnectorReference[Counter].m_Distance );
			DimensionsArea += DrawMeasurementLine( pRenderer, MeasurementLine, ConnectorReference[Counter].m_pConnector->GetPoint(), pStartConnector->GetPoint(), Offset, bDrawLines, bDrawText );
			Offset -= OFFSET_INCREMENT;
		}

		if( ConnectorCount > 2 && !bIsLine )
		{
			// Find start point in list.
			for( int Counter = 0; Counter < ConnectorCount; ++Counter )
			{
				if( ConnectorReferencePerp[Counter].m_pConnector == pStartConnector )
				{
					StartPointInList = Counter;
					break;
				}
			}

			Offset = NextOffset - OFFSET_INCREMENT;
			// Points before the start point on the perp orientation line.
			for( int Counter = StartPointInList - 1; Counter >= 0; --Counter )
			{
				if( fabs( ConnectorReferencePerp[Counter].m_Distance ) < 0.001 )
					continue;
				CFLine MeasurementLine = PerpOrientationLine;
				MeasurementLine.SetLength( ConnectorReferencePerp[Counter].m_Distance );
				MeasurementLine.ReverseDirection();
				DimensionsArea += DrawMeasurementLine( pRenderer, MeasurementLine, pStartConnector->GetPoint(), ConnectorReferencePerp[Counter].m_pConnector->GetPoint(), Offset, bDrawLines, bDrawText );
				Offset -= OFFSET_INCREMENT;
			}

			Offset = NextOffset - OFFSET_INCREMENT;
			// Points after the start point on the perp orientation line.
			for( int Counter = StartPointInList + 1; Counter < ConnectorCount; ++Counter )
			{
				if( fabs( ConnectorReferencePerp[Counter].m_Distance ) < 0.001 )
					continue;
				CFLine MeasurementLine = PerpOrientationLine;
				MeasurementLine.SetLength(  ConnectorReferencePerp[Counter].m_Distance );
				DimensionsArea += DrawMeasurementLine( pRenderer, MeasurementLine, ConnectorReferencePerp[Counter].m_pConnector->GetPoint(), pStartConnector->GetPoint(), Offset, bDrawLines, bDrawText );
				Offset -= OFFSET_INCREMENT;
			}
		}
	}

	pRenderer->SelectObject( pOldPen );
	pRenderer->SetTextColor( OldFontColor );

	return DimensionsArea;
}

CFArea CLinkageView::DrawDimensions( CRenderer* pRenderer, unsigned int OnLayers, CConnector *pConnector, bool bDrawLines, bool bDrawText )
{
	if( !m_bShowDimensions )
		return CFArea();

	if( ( pConnector->GetLayers() & OnLayers ) == 0 )
		return CFArea();

	if( ( pConnector->GetLayers() & CLinkageDoc::DRAWINGLAYER ) != 0 && !m_bShowDrawingLayerDimensions )
		return CFArea();

	CFArea DimensionsArea;

	CPen Pen;
	Pen.CreatePen( PS_SOLID, 1, RGB( 192, 192, 192 ) );
	CPen *pOldPen = pRenderer->SelectObject( &Pen );
	COLORREF OldFontColor = pRenderer->GetTextColor();
	pRenderer->SetTextColor( RGB( 96, 96, 96 ) );

	if( pConnector->GetDrawCircleRadius() > 0 )
	{
		std::list<double> RadiusList;
		RadiusList.push_back( pConnector->GetDrawCircleRadius() );
		DimensionsArea += DrawCirclesRadius( pRenderer, pConnector->GetPoint(), RadiusList, bDrawLines, bDrawText );
	}

	pRenderer->SelectObject( pOldPen );
	pRenderer->SetTextColor( OldFontColor );

	return DimensionsArea;
}

static void DrawChainLine( CRenderer* pRenderer, CFLine Line, double ShiftingFactor )
{
	const double Gapping = 6.0;

	double Offset = fmod( ShiftingFactor, Gapping * 2 );

	double Start = -Gapping + Offset;
	double End = Start + Gapping;

	CFLine DarkLine( Line );
	if( End > 0 )
	{
		DarkLine.MoveEndsFromStart( Start < 0 ? 0 : Start, End );
		pRenderer->DrawLine( DarkLine );
	}

	double Length = Line.GetLength();

	for( int Step = 1; ; ++Step )
	{
		Start = End;
		End = Start + Gapping;
		if( Step % 2 == 0 )
		{
			DarkLine.SetLine( Line );
			DarkLine.MoveEndsFromStart( Start < 0 ? 0 : Start, End > Length ? Length : End );
			pRenderer->DrawLine( DarkLine );
		}
		if( End >= Length )
			break;
	}
}

void CLinkageView::DrawChain( CRenderer* pRenderer, unsigned int OnLayers, CGearConnection *pGearConnection )
{
	if( pGearConnection == 0 || pGearConnection->m_pGear1 == 0 || pGearConnection->m_pGear2 == 0 )
		return;

	if( ( pGearConnection->m_pGear1->GetLayers() & OnLayers ) == 0 || ( pGearConnection->m_pGear2->GetLayers() & OnLayers ) == 0 )
		return;

	CConnector *pConnector1 = pGearConnection->m_pGear1->GetConnector( 0 );
	if( pConnector1 == 0 )
		return;

	CConnector *pConnector2 = pGearConnection->m_pGear2->GetConnector( 0 );
	if( pConnector2 == 0 )
		return;

	double Radius1 = 0;
	double Radius2 = 0;
	pGearConnection->m_pGear1->GetGearsRadius( pGearConnection, Radius1, Radius2 );

	CPen DotPen( PS_SOLID, 1, COLOR_CHAIN );
	CPen* pOldPen = pRenderer->SelectObject( &DotPen );

	CFLine Line1;
	CFLine Line2;
	if( !GetTangents( CFCircle( pConnector1->GetPoint(), Radius1 ), CFCircle( pConnector2->GetPoint(), Radius2 ), Line1, Line2 ) )
		return;

	// SOME CHAIN MOVEMENT TEST CODE...
	double GearAngle = pGearConnection->m_pGear1->GetRotationAngle();
	CLink *pConnectionLink = pGearConnection->m_pGear1->GetConnector( 0 )->GetSharingLink( pGearConnection->m_pGear2->GetConnector( 0 ) );
	double LinkAngle = pConnectionLink == 0 ? 0 : -pConnectionLink->GetTempRotationAngle();
	double UseAngle = GearAngle + LinkAngle;

	double DistanceAround = ( -UseAngle / 360.0 ) * Radius1 * 2 * PI_FOR_THIS_CODE;

	DrawChainLine( pRenderer, Scale( Line1 ), Scale( DistanceAround ) );
	Line2.ReverseDirection();
	DrawChainLine( pRenderer, Scale( Line2 ), Scale( DistanceAround ) );

	pRenderer->SelectObject( pOldPen );
}

void CLinkageView::DrawLock( CRenderer* pRenderer, CFPoint LockLocation )
{
	CPen LockPen;
	LockPen.CreatePen( PS_SOLID, 1, RGB( 0, 0, 0 ) );
	CPen *pOldPen = pRenderer->SelectObject( &LockPen );

	pRenderer->SelectObject( &LockPen );

	pRenderer->MoveTo( LockLocation.x, LockLocation.y );
	pRenderer->LineTo( LockLocation.x, LockLocation.y + AdjustYCoordinate( UnscaledUnits( 4 ) ) );
	pRenderer->LineTo( LockLocation.x + UnscaledUnits( 5 ), LockLocation.y + AdjustYCoordinate( UnscaledUnits( 4 ) ) );
	pRenderer->LineTo( LockLocation.x + UnscaledUnits( 5 ), LockLocation.y );
	pRenderer->LineTo( LockLocation.x, LockLocation.y );

	pRenderer->MoveTo( LockLocation.x + UnscaledUnits( 1 ), LockLocation.y + AdjustYCoordinate( UnscaledUnits( 4 ) ) );
	pRenderer->LineTo( LockLocation.x + UnscaledUnits( 1 ), LockLocation.y + AdjustYCoordinate( UnscaledUnits( 6 ) ) );
	pRenderer->LineTo( LockLocation.x + UnscaledUnits( 2 ), LockLocation.y + AdjustYCoordinate( UnscaledUnits( 7 ) ) );
	pRenderer->LineTo( LockLocation.x + UnscaledUnits( 3 ), LockLocation.y + AdjustYCoordinate( UnscaledUnits( 7 ) ) );
	pRenderer->LineTo( LockLocation.x + UnscaledUnits( 4 ), LockLocation.y + AdjustYCoordinate( UnscaledUnits( 6 ) ) );
	pRenderer->LineTo( LockLocation.x + UnscaledUnits( 4 ), LockLocation.y + AdjustYCoordinate( UnscaledUnits( 4 ) ) );

	pRenderer->SelectObject( pOldPen );
}

void CLinkageView::DrawLink( CRenderer* pRenderer, const GearConnectionList *pGearConnections, unsigned int OnLayers, CLink *pLink, bool bShowlabels, bool bDrawHighlight, bool bDrawFill )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if( ( pLink->GetLayers() & OnLayers ) == 0 )
		return;

	CPen* pOldPen = 0;

	int Count = (int)pLink->GetConnectorList()->GetCount();

	if( bDrawHighlight )
	{
		/*
		 * Draw only the highlight and then return.
		 * this allows the highlight to be behind all other objects.
		 */
		if( m_bShowSelection )
		{
			if( pLink->IsSelected( true ) )
			{
				CPen GrayPen;

				GrayPen.CreatePen( PS_SOLID, 1, pDoc->GetLastSelectedLink() == pLink ? COLOR_MAJORSELECTION : COLOR_MINORSELECTION ) ;
				pOldPen = pRenderer->SelectObject( &GrayPen );

				CFRect AreaRect;
				pLink->GetArea( *pGearConnections, AreaRect );

				if( pLink->GetConnectorList()->GetCount() > 1 || pLink->IsGear() )
				{
					AreaRect = Scale( AreaRect );
					AreaRect.InflateRect( 2 * m_ConnectorRadius, 2 * m_ConnectorRadius );
					pRenderer->DrawRect( AreaRect );
				}
				pRenderer->SelectObject( pOldPen );

				DrawFasteners( pRenderer, OnLayers, pLink );
			}
			return;
		}
	}

	bool bUseOffset = false;
	if( pLink->IsMeasurementElement( &bUseOffset ) )
	{
		COLORREF Color = pLink->GetColor();
		COLORREF LightColor = LightenColor( Color, 0.75 );
		CPen Pen;
		Pen.CreatePen( PS_SOLID, 1, Color );
		CPen LightPen;
		LightPen.CreatePen( PS_SOLID, 1, LightColor );
		CPen *pOldPen = pRenderer->SelectObject( &Pen );
		COLORREF OldFontColor = pRenderer->GetTextColor();

		pRenderer->SetTextColor( RGB( 96, 96, 96 ) );
		pRenderer->SetBkMode( TRANSPARENT );
		pRenderer->SetTextAlign( TA_LEFT | TA_TOP );
		//pRenderer->SetBkColor( RGB( 255, 255, 255 ) );

		int PointCount = 0;
		CFPoint* Points = pLink->GetHull( PointCount );
		// reverse the points because the -y change causes the hull to be in the wrong direction.
		for( int Index = 0; Index < PointCount - 1; ++Index )
		{
			CFLine Line( Points[Index], Points[Index+1] );
			if( bUseOffset )
			{
				pRenderer->SelectObject( &LightPen );
				pRenderer->DrawLine( Scale( Line ) );
			}
			pRenderer->SelectObject( &Pen );
			DrawMeasurementLine( pRenderer, Line, bUseOffset ? OFFSET_INCREMENT : 0, true, true );
		}
		pRenderer->SelectObject( pOldPen );
	}

	CPen Pen;
	COLORREF Color;

	if( pRenderer->GetColorCount() == 2 )
		Color = RGB( 0, 0, 0 );
	else
		Color = pLink->GetColor();

	if( ( m_SelectedEditLayers & CLinkageDoc::MECHANISMLAYERS ) == 0 )
		Color = LightenColor( DesaturateColor( Color, 0.5 ), 0.6 );

	CFPoint Average;

	if( bShowlabels && ( Count > 1 || pLink->IsGear() ) )
	{
		/*
		 * Don't draw generic ID values for drawing elements. Only
		 * mechanism elements can show an ID string when no name is
		 * assigned by the user.
		 */
		CString &Name = pLink->GetName();
		//if( !Name.IsEmpty() || ( pLink->GetLayers() & CLinkageDoc::MECHANISMLAYERS ) != 0 || m_bShowDebug )
		{
			pRenderer->SetBkMode( TRANSPARENT );
			CString Number = pLink->GetIdentifierString( m_bShowDebug );

			CConnector *pConnector1 = pLink->GetConnectorList()->GetHead();

			double LinkLabelOffset;
			if( pLink->IsGear() )
			{
				pRenderer->SetTextAlign( TA_BOTTOM | TA_RIGHT );
				double dx = m_ConnectorRadius - UnscaledUnits( 1 );
				double dy = m_ConnectorRadius - UnscaledUnits( 2 );

				Average = pConnector1->GetPoint();
				Average = Scale( Average );
				Average.x -= dx;
				Average.y += AdjustYCoordinate( dy );
				LinkLabelOffset = 0;
			}
			else
			{
				pRenderer->SetTextAlign( TA_BOTTOM | TA_LEFT );
				pLink->GetAveragePoint( *pGearConnections, Average );
				Average = Scale( Average );
				Average.y -= AdjustYCoordinate( UnscaledUnits( m_UsingFontHeight ) * 0.75 );
				LinkLabelOffset = UnscaledUnits( pLink->IsSolid() ? 14 : 6 );
			}

			if( Count == 2 || true ) // it seems ok to do this all of the time, which works better for some parts list information.
			{
				CConnector *pConnector2 = pLink->GetConnectorList()->GetTail();
				if( pConnector1 != 0 && pConnector2 != 0 && pConnector1 != pConnector2 )
				{
					// Note that this is based on the original point to keep the location from jumping while moving.
					double dx = fabs( pConnector1->GetOriginalPoint().x - pConnector2->GetOriginalPoint().x );
					double dy = fabs( pConnector1->GetOriginalPoint().y - pConnector2->GetOriginalPoint().y );
					if( dx > dy )
						Average.y += AdjustYCoordinate( LinkLabelOffset ) * 1.5;
					else
						Average.x += LinkLabelOffset;
				}
			}
			else
			{
				// A guess to keep the text from overlapping the average point of a link.
				//Average.y += AdjustYCoordinate( LinkLabelOffset ) * 1.5;
				Average.x += LinkLabelOffset;
			}

			double Spacing = 0;
			if( pLink->IsLocked() )
			{
				CFPoint LockLocation( Average );
				LockLocation.x += m_ConnectorRadius;
				LockLocation.y += AdjustYCoordinate( m_ConnectorRadius );
				DrawLock( pRenderer, LockLocation );
				Spacing += /*(int)*/UnscaledUnits( 12 );
			}

			pRenderer->TextOut( Average.x + Spacing, Average.y, Number );

			if( pLink->IsActuator() )
			{
				Average.y += AdjustYCoordinate( UnscaledUnits( m_UsingFontHeight ) );
				CString Speed;
				double CPM = pLink->GetCPM();
				if( fabs( CPM - floor( CPM ) ) < .0001 )
					Speed.Format( "%d", (int)CPM );
				else
					Speed.Format( "%.2lf", CPM );
				if( pLink->IsAlwaysManual() )
					Speed += "m";
				pRenderer->TextOut( Average.x - UnscaledUnits( 3 ) + Spacing, Average.y + UnscaledUnits( 4 ), Speed );
			}
		}
	}

	if( pLink->IsMeasurementElement() )
		return;

	Pen.CreatePen( PS_SOLID, pLink->GetLineSize(), Color );
	pOldPen = pRenderer->SelectObject( &Pen );

	if( pLink->IsActuator() )
		DrawActuator( pRenderer, OnLayers, pLink, bDrawFill );
	else
	{
		CConnector *pSlider = pLink->GetConnectedSlider( 0 );
		if( 0 && pLink->GetConnectorCount() == 2 && pLink->GetConnectedSliderCount() > 0 && !pLink->IsSolid() && pSlider != 0 && pSlider->GetSlideRadius() != 0 )
		{
			CFArc SliderArc;
			if( !pSlider->GetSliderArc( SliderArc ) )
				return;

			SliderArc = Scale( SliderArc );
			pRenderer->Arc( SliderArc );
		}
		else if( pLink->GetShapeType() == CLink::POLYLINE )
		{
			int PointCount = 0;
			CFPoint* Points = pLink->GetPoints( PointCount );
			if( Points != 0 && PointCount > 0 )
			{
				pRenderer->MoveTo( Scale( Points[0] ) );
				for( int Index = 1; Index < PointCount; ++Index )
					pRenderer->LineTo( Scale( Points[Index] ) );
			}
		}
		else
		{
			int PointCount = 0;
			CFPoint* Points = 0;
			if( pLink->GetShapeType() == CLink::POLYGON )
				Points = pLink->GetPoints( PointCount );
			else
				Points = pLink->GetHull( PointCount );
			CFPoint* PixelPoints = new CFPoint[PointCount];
			double *pHullRadii = new double[PointCount];
			// reverse the points because the -y change causes the hull to be in the wrong direction.
			for( int Index = 0; Index < PointCount; ++Index )
			{
				PixelPoints[PointCount-1-Index] = Scale( Points[Index] );
				pHullRadii[Index] = 0;
			}
			if( pLink->GetConnectedSliderCount() > 0 )
			{
				for( int Index = 0; Index < PointCount; ++Index )
				{
					for( int Counter = 0; Counter < pLink->GetConnectedSliderCount(); ++Counter )
					{
						CConnector *pSlider = pLink->GetConnectedSlider( Counter );
						if( pSlider == 0 || pSlider->GetSlideRadius() == 0 )
							continue;
						CFPoint Limit1;
						CFPoint Limit2;
						pSlider->GetSlideLimits( Limit1, Limit2 );
						Limit1 = Scale( Limit1 );
						Limit2 = Scale( Limit2 );
						CFPoint NextPixelPoint = Index == PointCount - 1 ? PixelPoints[0] : PixelPoints[Index+1];
						if( ( PixelPoints[Index] == Limit1 && NextPixelPoint == Limit2 ) 
						    || ( PixelPoints[Index] == Limit2 && NextPixelPoint == Limit1 ) )
						{
							pHullRadii[Index] = pSlider->GetSlideRadius();
							break;
						}
					}
				}
			}
			
			pRenderer->LinkageDrawExpandedPolygon( PixelPoints, pHullRadii, PointCount, Color, bDrawFill, pLink->IsSolid() ? ( m_ConnectorRadius + UnscaledUnits( 3 ) ) : UnscaledUnits( 0 ) );
			delete [] PixelPoints;
			delete [] pHullRadii;
			PixelPoints = 0;
		}

		if( pLink->IsGear() )
		{
			COLORREF GearColor = Color;
			if( ( m_SelectedEditLayers & CLinkageDoc::MECHANISMLAYERS ) == 0 )
				GearColor = LightenColor( DesaturateColor( Color, 0.5 ), 0.6 );

			CElementItem *pFastenedTo = pLink->GetFastenedTo();
			if( pFastenedTo != 0 && pFastenedTo->GetElement() != 0 )
				GearColor = pFastenedTo->GetElement()->GetColor();

			CConnector *pGearConnector = pLink->GetGearConnector();
			if( pGearConnector != 0 )
			{
				std::list<double> RadiusList;
				pLink->GetGearRadii( *pGearConnections, RadiusList );

				double LargestRadius = RadiusList.back();
				double SmallestRadius = RadiusList.front();

				if( bDrawFill )
				{
					double Red = GetRValue( GearColor ) / 255.0;
					double Green = GetGValue( GearColor ) / 255.0;
					double Blue = GetBValue( GearColor ) / 255.0;
					if( Red < 1.0 )
						Red += ( 1.0 - Red ) * .965;
					if( Green < 1.0 )
						Green += ( 1.0 - Green ) * .965;
					if( Blue < 1.0 )
						Blue += ( 1.0 - Blue ) * .965;

					COLORREF NewColor = RGB( (int)( Red * 255 ), (int)( Green * 255 ), (int)( Blue * 255 ) );
					CBrush Brush;
					Brush.CreateSolidBrush( NewColor );
					CBrush *pOldBrush = pRenderer->SelectObject( &Brush );
					pRenderer->SelectStockObject( NULL_PEN );

					CFCircle Circle( Scale( pGearConnector->GetPoint() ), Scale( LargestRadius ) );
					pRenderer->Circle( Circle );

					pRenderer->SelectObject( pOldBrush );
				}
				else
				{
					CPen AngleMarkerPen( PS_SOLID, 1, GearColor );
					//CPen GearPen( PS_DOT, 1, GearColor );
					CPen GearPen( PS_SOLID, pLink->GetLineSize(), GearColor );

					CBrush *pOldBrush = (CBrush*)pRenderer->SelectStockObject( NULL_BRUSH );

					if( pGearConnector != 0 )
					{
						pRenderer->SelectObject( &GearPen );
						for( std::list<double>::iterator it = RadiusList.begin(); it != RadiusList.end(); ++it )
						{
							CFArc Arc( Scale( pGearConnector->GetPoint() ), Scale( *it ), pGearConnector->GetPoint(), pGearConnector->GetPoint() );
							pRenderer->Arc( Arc );
						}
					}

					pRenderer->SelectObject( &AngleMarkerPen );
					CFPoint Point = pGearConnector->GetPoint();
					Point = Scale( Point );

					// Draw a small line to show rotation angle.
					CFPoint StartPoint( Point.x, Point.y + AdjustYCoordinate( Scale( LargestRadius ) ) );
					CFPoint EndPoint( Point.x, Point.y + AdjustYCoordinate( Scale( SmallestRadius / 2 ) ) );
					StartPoint.RotateAround( Point, -pLink->GetRotationAngle() );
					EndPoint.RotateAround( Point, -pLink->GetRotationAngle() );
					pRenderer->MoveTo( StartPoint );
					pRenderer->LineTo( EndPoint );

					pRenderer->SelectObject( pOldBrush );
					pRenderer->SelectObject( (CPen*)0 );

					if( !pLink->IsPositionValid() && bShowlabels )
						DrawFailureMarks( pRenderer, OnLayers, Point, Scale( LargestRadius ), pLink->GetSimulationError() );
				}
			}
		}
		else
		{
			if( !pLink->IsPositionValid() && bShowlabels )
				DrawFailureMarks( pRenderer, OnLayers, Average, m_ConnectorRadius, pLink->GetSimulationError() );
		}
	}

	pRenderer->SelectObject( pOldPen );
}

void CLinkageView::OnEditMakeparallelogram()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if( pDoc->GetSelectedConnectorCount() != 4 || pDoc->GetFirstSelectedConnector() == 0 )
		return;
}

#if 0
void CLinkageView::OnUpdateEditMakeparallelogram(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( !m_bSimulating && pDoc->GetSelectedConnectorCount() == 4 && pDoc->GetLastSelectedConnector() != 0 );
	pCmdUI->Enable( FALSE );
}

void CLinkageView::OnEditMakerighttriangle()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if( pDoc->GetSelectedConnectorCount() != 3 || pDoc->GetLastSelectedConnector() == 0 )
		return;

	CImageSelectDialog Dlg;
	Dlg.DoModal();
}

void CLinkageView::OnUpdateEditMakerighttriangle(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( pDoc->GetSelectedConnectorCount() == 3 && pDoc->GetLastSelectedConnector() != 0 );
//	pCmdUI->Enable( FALSE );
}
#endif

void CLinkageView::OnUpdateEditUndo(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( pDoc->CanUndo() && !m_bSimulating );
}

void CLinkageView::OnEditUndo()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->Undo();
	//if( m_bShowParts )
		//pDoc->SelectElement();
	UpdateForDocumentChange();
	SelectionChanged();
}

CRect CLinkageView::GetDocumentScrollingRect( void )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CFRect DocumentArea = GetDocumentArea();

	CFRect DocumentRect = Scale( DocumentArea );

	static const int EDITBORDER = 50;

	DocumentRect.InflateRect( EDITBORDER, EDITBORDER );

	return CRect( (int)DocumentRect.left, (int)DocumentRect.top, (int)DocumentRect.right, (int)DocumentRect.bottom );
}

void CLinkageView::SetScrollExtents( bool bEnableBars )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	m_ScrollOffsetAdjustment.SetPoint( 0, 0 );

	SCROLLINFO ScrollInfo;
	memset( &ScrollInfo, 0, sizeof( ScrollInfo ) );
	ScrollInfo.cbSize = sizeof( ScrollInfo );
	ScrollInfo.fMask = SIF_DISABLENOSCROLL | SIF_POS | SIF_PAGE | SIF_RANGE;

	if( pDoc->IsEmpty() || !bEnableBars || m_bSimulating )
	{
		SetScrollInfo( SB_HORZ, &ScrollInfo, TRUE );
		SetScrollInfo( SB_VERT, &ScrollInfo, TRUE );
		return;
	}

	CRect ClientRect = m_DrawingRect;

	m_Zoom = m_ScreenZoom;
	m_ScrollPosition = m_ScreenScrollPosition;

	CRect DocumentRect = GetDocumentScrollingRect();

	CRect ScrollDocumentRect( min( DocumentRect.left, ClientRect.left ),
				              min( DocumentRect.top, ClientRect.top ),
				              max( DocumentRect.right, ClientRect.right ) + 1,
				              max( DocumentRect.bottom, ClientRect.bottom ) + 1 );

	ScrollInfo.nMin = ScrollDocumentRect.left;
	ScrollInfo.nMax = ScrollDocumentRect.right - 1;
	ScrollInfo.nPage = ClientRect.Width() + 1;
	ScrollInfo.nPos = ClientRect.left;
	SetScrollInfo( SB_HORZ, &ScrollInfo, TRUE );

	ScrollInfo.nMin = ScrollDocumentRect.top;
	ScrollInfo.nMax = ScrollDocumentRect.bottom - 1;
	ScrollInfo.nPage = ClientRect.Height() + 1;
	ScrollInfo.nPos = ClientRect.top;
	SetScrollInfo( SB_VERT, &ScrollInfo, TRUE );
}

void CLinkageView::OnScroll( int WhichBar, UINT nSBCode, UINT nPos )
{
	int Minimum;
	int Maximum;
	GetScrollRange( WhichBar, &Minimum, &Maximum );
	int Current = GetScrollPos( WhichBar );
	int New = Current;
	int Move = 0;

	switch( nSBCode )
	{
		case SB_LEFT:
			New = Minimum;
			break;

		case SB_RIGHT:
			New = Maximum;
			break;

		case SB_ENDSCROLL:
			return;

		case SB_LINELEFT:
			New -= 10;
			if( New < Minimum )
				New = Minimum;
			break;

		case SB_LINERIGHT:
			New += 10;
			if( New > Maximum )
				New = Maximum;
			break;

		case SB_PAGELEFT:
		{
			// Get the page size.
			SCROLLINFO   info;
			GetScrollInfo( WhichBar, &info, SIF_ALL );

			if( info.nPage < 60 )
				info.nPage = 60;

			New -= info.nPage;
			if( New < Minimum )
				New = Minimum;
			break;
		}

		case SB_PAGERIGHT:
		{
			// Get the page size.
			SCROLLINFO   info;
			GetScrollInfo( WhichBar, &info, SIF_ALL );

			if( info.nPage < 60 )
				info.nPage = 60;

			New += info.nPage;
			if( New > Maximum - (int)info.nPage )
				New = Maximum - (int)info.nPage;
			break;
		}

		case SB_THUMBPOSITION:
			New = nPos;
			break;

		case SB_THUMBTRACK:
			New = nPos;
			break;
	}

	if( WhichBar == SB_VERT )
		m_ScreenScrollPosition.y += New - Current;
	else
		m_ScreenScrollPosition.x += New - Current;

	/*
	 * If thumb tracking, don't tweak the scroll bar size and position until after the scrolling is done.
	 */
	if( nSBCode == SB_THUMBTRACK )
		SetScrollPos( WhichBar, New );
	else
		SetScrollExtents();

	InvalidateRect( 0 );
}

void CLinkageView::OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
	OnScroll( SB_VERT, nSBCode, nPos );
}

void CLinkageView::OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
	OnScroll( SB_HORZ, nSBCode, nPos );
}

void CLinkageView::OnEditSlide()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pDoc->ConnectSliderLimits();

	UpdateForDocumentChange();
}

void CLinkageView::OnUpdateEditSlide(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pCmdUI->Enable( pDoc->IsSelectionSlideable() && !m_bSimulating && m_bAllowEdit );
}

bool CLinkageView::ConnectorProperties( CConnector *pConnector )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	double DocumentScale = pDoc->GetUnitScale();

	CConnectorPropertiesDialog Dialog;
	Dialog.m_RPM = pConnector->GetRPM();
	Dialog.m_LimitAngle = fabs( pConnector->GetLimitAngle() );
	Dialog.m_bAnchor = pConnector->IsAnchor();
	Dialog.m_bInput = pConnector->IsInput();
	Dialog.m_bDrawing = pConnector->IsDrawing();
	Dialog.m_bAlwaysManual = pConnector->IsAlwaysManual();
	Dialog.m_xPosition = pConnector->GetPoint().x * DocumentScale;
	Dialog.m_yPosition = pConnector->GetPoint().y * DocumentScale;
	Dialog.m_bIsSlider = pConnector->IsSlider();
	Dialog.m_SlideRadius = pConnector->GetSlideRadius() * DocumentScale;
	Dialog.m_StartOffset = fabs( pConnector->GetStartOffset() );
	Dialog.m_bLocked = pConnector->IsLocked();
	Dialog.m_Name = pConnector->GetName();
	Dialog.m_MinimumSlideRadius = 0;
	if( pConnector->GetFastenedTo() != 0 && pConnector->GetFastenedTo()->GetElement() != 0 )
		Dialog.m_FastenedTo = "Fastened to " + pConnector->GetFastenedTo()->GetElement()->GetIdentifierString( m_bShowDebug );
	Dialog.m_Color = pConnector->GetColor();

	if( pConnector->IsSlider() )
	{
		CFLine LimitsLine;
		CFPoint Point2;
		if( pConnector->GetSlideLimits( LimitsLine.m_Start, LimitsLine.m_End ) )
			Dialog.m_MinimumSlideRadius = ( ( LimitsLine.GetLength() / 2 )  * DocumentScale ) + 0.00009;
	}
	Dialog.m_Label = pConnector->GetIdentifierString( m_bShowDebug );
	Dialog.m_Label += " - Connector Properties";

	if( Dialog.DoModal() == IDOK )
	{
		pDoc->PushUndo();

		pConnector->SetRPM( Dialog.m_RPM );
		pConnector->SetLimitAngle( fabs( Dialog.m_LimitAngle ) );
		bool bNoLongerAnAnchor = pConnector->IsAnchor() && !Dialog.m_bAnchor;
		pConnector->SetAsAnchor( Dialog.m_bAnchor );
		pConnector->SetAsInput( Dialog.m_bInput );
		pConnector->SetAsDrawing( Dialog.m_bDrawing );
		pConnector->SetAlwaysManual( Dialog.m_bAlwaysManual );
		pConnector->SetName( Dialog.m_Name );
		pConnector->SetColor( Dialog.m_Color );
		if( Dialog.m_bColorIsSet )
			pConnector->SetUserColor( true );
		if( Dialog.m_StartOffset == 0 || pConnector->GetLimitAngle() == 0 )
			pConnector->SetStartOffset( Dialog.m_StartOffset );
		else
			pConnector->SetStartOffset( fabs( fmod( Dialog.m_StartOffset, pConnector->GetLimitAngle() * 2 ) ) );
		pConnector->SetLocked( Dialog.m_bLocked == TRUE );

		if( Dialog.m_xPosition != pConnector->GetPoint().x ||
		    Dialog.m_yPosition != pConnector->GetPoint().y )
		{
			pConnector->SetPoint( Dialog.m_xPosition / DocumentScale, Dialog.m_yPosition / DocumentScale );
		}
		double NewSlideRadius = Dialog.m_SlideRadius / DocumentScale;
		if( NewSlideRadius != pConnector->GetSlideRadius() )
		{
			pConnector->SetSlideRadius( NewSlideRadius );
			CFPoint Point = pConnector->GetPoint();
			CFArc TheArc;
			if( pConnector->GetSliderArc( TheArc ) )
			{
				Point.SnapToArc( TheArc );
				pConnector->SetPoint( Point );
			}
		}

		if( bNoLongerAnAnchor )
		{
			// Anchor-to-anchor gear connections break when the connector is no longer an anchor.
			pDoc->RemoveGearRatio( pConnector, 0 );
		}

		return true;
	}
	return false;
}

bool CLinkageView::PointProperties( CConnector *pConnector )
{
//	CTestDialog Test;
//	Test.DoModal();
//	return false;

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	double DocumentScale = pDoc->GetUnitScale();

	CPointPropertiesDialog Dialog;
	Dialog.m_RPM = pConnector->GetRPM();
	Dialog.m_xPosition = pConnector->GetPoint().x * DocumentScale;
	Dialog.m_yPosition = pConnector->GetPoint().y * DocumentScale;
	Dialog.m_Radius = pConnector->GetDrawCircleRadius() * DocumentScale;
	Dialog.m_bDrawCircle = pConnector->GetDrawCircleRadius() != 0 ? TRUE : FALSE;
	Dialog.m_Name = pConnector->GetName();
	Dialog.m_Label = pConnector->GetIdentifierString( m_bShowDebug );
	Dialog.m_Label += " - Point Properties";
	Dialog.m_LineSize = pConnector->GetLineSize();
	if( pConnector->GetFastenedTo() != 0 && pConnector->GetFastenedTo()->GetElement() != 0 )
		Dialog.m_FastenTo = "Fastened to " + pConnector->GetFastenedTo()->GetElement()->GetIdentifierString( m_bShowDebug );
	Dialog.m_Color = pConnector->GetColor();

	if( Dialog.DoModal() == IDOK )
	{
		pDoc->PushUndo();

		pConnector->SetRPM( Dialog.m_RPM );
		pConnector->SetName( Dialog.m_Name );
		pConnector->SetDrawCircleRadius( Dialog.m_bDrawCircle != FALSE ? Dialog.m_Radius / DocumentScale : 0.0 );
		pConnector->SetColor( Dialog.m_Color );
		pConnector->SetLineSize( Dialog.m_bDrawCircle != FALSE ? Dialog.m_LineSize : 1 );

		if( Dialog.m_xPosition != pConnector->GetPoint().x ||
		    Dialog.m_yPosition != pConnector->GetPoint().y )
		{
			pConnector->SetPoint( Dialog.m_xPosition / DocumentScale, Dialog.m_yPosition / DocumentScale );
		}
		pConnector->UpdateControlKnobs();

		return true;
	}
	return false;
}

bool CLinkageView::LinkProperties( CLink *pLink )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int OriginalLineSize = 1;
	bool OriginalIsSolid = false;
	CNullableColor OriginalColor;

	CLinkPropertiesDialog Dialog;
	if( pLink == 0 )
	{
		Dialog.m_LineSize = OriginalLineSize;
		Dialog.m_bSolid = OriginalIsSolid ? TRUE : FALSE;
		Dialog.m_bActuator = FALSE;
		Dialog.m_ConnectorCount = 2;
		Dialog.m_ActuatorCPM = 0;
		Dialog.m_bAlwaysManual = FALSE;
		Dialog.m_SelectedLinkCount = 0;
		Dialog.m_ThrowDistance = 0;
		Dialog.m_bIsGear = false;
		Dialog.m_Label = "Multiple Link Properties";
		Dialog.m_Color = OriginalColor;
		Dialog.m_bFastened = false;
		Dialog.m_bLocked = false;
		Dialog.m_StartOffset = 0;

		OriginalLineSize = 1;
		OriginalIsSolid = false;

		LinkList* pLinkList = pDoc->GetLinkList();
		POSITION Position = pLinkList->GetHeadPosition();
		while( Position != NULL )
		{
			CLink* pSelectedLink = pLinkList->GetNext( Position );
			if( pSelectedLink != 0 && pSelectedLink->IsSelected() )
			{
				Dialog.m_LineSize = max( Dialog.m_LineSize, pSelectedLink->GetLineSize() );
				Dialog.m_bSolid = Dialog.m_bSolid || pSelectedLink->IsSolid();
				// Dialog.m_bIsGear = Dialog.m_bIsGear || pSelectedLink->IsGear(); CANT CHANGE THIS SO DONT SHOW IT!
				++Dialog.m_SelectedLinkCount;
			}
		}
	}
	else
	{
		Dialog.m_LineSize = pLink->GetLineSize();
		Dialog.m_bSolid = pLink->IsSolid();
		Dialog.m_bActuator = pLink->IsActuator();
		Dialog.m_ConnectorCount = pLink->GetConnectorCount();
		Dialog.m_ActuatorCPM = pLink->GetCPM();
		Dialog.m_bAlwaysManual = pLink->IsAlwaysManual();
		Dialog.m_ThrowDistance = pLink->GetStroke() * pDoc->GetUnitScale();
		if( !pLink->IsActuator() )
			Dialog.m_ThrowDistance = pLink->GetLength() / 2 * pDoc->GetUnitScale();
		Dialog.m_SelectedLinkCount = 1;
		Dialog.m_bIsGear = pLink->IsGear();
		Dialog.m_bLocked = pLink->IsLocked();
		Dialog.m_Label = pLink->GetIdentifierString( m_bShowDebug );
		Dialog.m_Label += " - Link Properties";
		Dialog.m_Name = pLink->GetName();
		if( pLink->GetFastenedTo() != 0 && pLink->GetFastenedTo()->GetElement() != 0 )
			Dialog.m_FastenedTo = "Fastened to " + pLink->GetFastenedTo()->GetElement()->GetIdentifierString( m_bShowDebug );
		Dialog.m_bFastened = pLink->GetFastenedTo() != 0;
		Dialog.m_Color = pLink->GetColor();
		Dialog.m_StartOffset = pLink->GetStartOffset() * pDoc->GetUnitScale();
		if( !pLink->IsActuator() )
			Dialog.m_StartOffset = 0;
	}
	if( Dialog.DoModal() == IDOK )
	{
		pDoc->PushUndo();

		if( pLink == 0 )
		{
			LinkList* pLinkList = pDoc->GetLinkList();
			POSITION Position = pLinkList->GetHeadPosition();
			while( Position != NULL )
			{
				CLink* pSelectedLink = pLinkList->GetNext( Position );
				if( pSelectedLink != 0 && pSelectedLink->IsSelected() )
				{
					if( Dialog.m_LineSize != OriginalLineSize )
						pSelectedLink->SetLineSize( Dialog.m_LineSize );
					if( Dialog.m_bSolid != ( OriginalIsSolid ? TRUE : FALSE ) )
						pSelectedLink->SetSolid( Dialog.m_bSolid != 0 );
					if( Dialog.m_bColorIsSet )
						pSelectedLink->SetColor( Dialog.m_Color );
				}
			}
		}
		else
		{
			pLink->SetLineSize( Dialog.m_LineSize );
			pLink->SetSolid( Dialog.m_bSolid != 0 );
			pLink->SetLocked( Dialog.m_bLocked != 0 );
			pLink->SetActuator( Dialog.m_bActuator != 0 );
			pLink->SetCPM( Dialog.m_ActuatorCPM );
			pLink->SetAlwaysManual( Dialog.m_bAlwaysManual != 0 );
			pLink->SetName( Dialog.m_Name );
			pLink->SetStroke( Dialog.m_ThrowDistance / pDoc->GetUnitScale() );
			pLink->SetColor( Dialog.m_Color );
			if( Dialog.m_bColorIsSet )
				pLink->SetUserColor( true );

			pLink->SetStartOffset( fmod( fabs( Dialog.m_StartOffset ), pLink->GetStroke() * 2 ) / pDoc->GetUnitScale() );
		}

		return true;
	}
	return false;
}

bool CLinkageView::LineProperties( CLink *pLink )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CLinePropertiesDialog Dialog;
	if( pLink == 0 )
	{
		Dialog.m_LineSize = 1;
		Dialog.m_SelectedLinkCount = 0;
		Dialog.m_bMeasurementLine = FALSE;
		Dialog.m_bOffsetMeassurementLine = FALSE;
		Dialog.m_Label = "Multiple Line/Link Properties";
//		Dialog.m_Color = pLink->GetColor();
//		Dialog.m_bPolygon = !pLink->IsPolyline();

		LinkList* pLinkList = pDoc->GetLinkList();
		POSITION Position = pLinkList->GetHeadPosition();
		while( Position != NULL )
		{
			CLink* pSelectedLink = pLinkList->GetNext( Position );
			if( pSelectedLink != 0 && pSelectedLink->IsSelected() )
			{
				Dialog.m_LineSize = max( Dialog.m_LineSize, pSelectedLink->GetLineSize() );
				++Dialog.m_SelectedLinkCount;
			}
		}
	}
	else
	{
		Dialog.m_LineSize = pLink->GetLineSize();
		bool bUseLineOffset = false;
		Dialog.m_bMeasurementLine = pLink->IsMeasurementElement( &bUseLineOffset ) ? TRUE : FALSE;
		Dialog.m_bOffsetMeassurementLine = bUseLineOffset ? TRUE : FALSE;
		Dialog.m_SelectedLinkCount = 1;
		Dialog.m_Label = pLink->GetIdentifierString( m_bShowDebug );
		Dialog.m_Label += " - Line Properties";
		Dialog.m_Name = pLink->GetName();
		if( pLink->GetFastenedTo() != 0 && pLink->GetFastenedTo()->GetElement() != 0 )
			Dialog.m_FastenTo = "Fastened to " + pLink->GetFastenedTo()->GetElement()->GetIdentifierString( m_bShowDebug );
		Dialog.m_Color = pLink->GetColor();
		switch( pLink->GetShapeType() )
		{
			case CLink::POLYGON: Dialog.m_ShapeType = 1; break;
			case CLink::POLYLINE: Dialog.m_ShapeType = 2; break;
			case CLink::HULL: default: Dialog.m_ShapeType = 0; break;
		}
	}
	if( Dialog.DoModal() == IDOK )
	{
		pDoc->PushUndo();

		if( pLink == 0 )
		{
			LinkList* pLinkList = pDoc->GetLinkList();
			POSITION Position = pLinkList->GetHeadPosition();
			while( Position != NULL )
			{
				CLink* pSelectedLink = pLinkList->GetNext( Position );
				if( pSelectedLink != 0 && pSelectedLink->IsSelected() )
				{
					pSelectedLink->SetLineSize( Dialog.m_LineSize );
					pSelectedLink->SetColor( Dialog.m_Color );
				}
			}
		}
		else
		{
			pLink->SetLineSize( Dialog.m_LineSize );
			pLink->SetName( Dialog.m_Name );
			pLink->SetMeasurementElement( Dialog.m_bMeasurementLine != FALSE, Dialog.m_bOffsetMeassurementLine != FALSE );
			pLink->SetColor( Dialog.m_Color );
			switch( Dialog.m_ShapeType )
			{
				case 1: pLink->SetShapeType( CLink::POLYGON ); break;
				case 2: pLink->SetShapeType( CLink::POLYLINE ); break;
				case 0: default: pLink->SetShapeType( CLink::HULL ); break;
			}
		}

		return true;
	}
	return false;
}

void CLinkageView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
{
	if( pDC->IsPrinting() )
	{
		DEVMODE* pDevMode = pInfo->m_pPD->GetDevMode();
		pDevMode->dmOrientation = m_PrintOrientationMode;
		pDevMode->dmFields |= DM_ORIENTATION;
		pDC->ResetDC( pDevMode );
	}

	CView::OnPrepareDC(pDC, pInfo);
}

void CLinkageView::OnFileSaveVideo()
{
	if( !CanSimulate() )
		return;

	CFileDialog dlg( 0, "avi", 0, 0, "AVI Video Files|*.avi|All Files|*.*||" );
	dlg.m_ofn.lpstrTitle = "Save Video";
	dlg.m_ofn.Flags |= OFN_NOCHANGEDIR | OFN_NONETWORKBUTTON | OFN_OVERWRITEPROMPT;
	if( dlg.DoModal() != IDOK )
		return;

	CRecordDialog RecordDialog;
	if( RecordDialog.DoModal() != IDOK )
		return;

	if( !InitializeVideoFile( dlg.GetPathName(), RecordDialog.m_EncoderName ) )
		return;

	m_bRecordingVideo = true;
	m_RecordQuality = RecordDialog.m_QualitySelection;

	m_VideoStartFrames = (int)( RecordDialog.m_bUseStartTime ? ConvertToSeconds( RecordDialog.m_StartTime ) * FRAMES_PER_SECONDS : 0 );
	m_VideoRecordFrames = (int)( RecordDialog.m_bUseRecordTime ? ConvertToSeconds( RecordDialog.m_RecordTime ) * FRAMES_PER_SECONDS : 0 );
	m_VideoEndFrames = (int)( RecordDialog.m_bUseEndTime ? ConvertToSeconds( RecordDialog.m_EndTime ) * FRAMES_PER_SECONDS : 0 );

	if( RecordDialog.m_bUseStartTime || RecordDialog.m_bUseRecordTime || RecordDialog.m_bUseEndTime )
		m_bUseVideoCounters = true;
	else
		m_bUseVideoCounters = false;

	ConfigureControlWindow( AUTO );
	StartMechanismSimulate( AUTO );
}

void CLinkageView::OnFileSaveImage()
{
	CExportImageSettingsDialog SizeDialog;
	SizeDialog.m_pLinkageView = this;
	if( SizeDialog.DoModal() != IDOK )
		return;

	CFileDialog dlg( 0, "jpg", 0, 0, "JPEG Files (*.jpg)|*.jpg|PNG Files (*.png)|*.png|All Supported Image Files (*.jpg;*.png)|*.jpg;*.png|All Files (*.*)|*.*||" );
	dlg.m_ofn.lpstrTitle = "Save As JPEG or PNG Image";
	dlg.m_ofn.Flags |= OFN_NOCHANGEDIR | OFN_NONETWORKBUTTON | OFN_OVERWRITEPROMPT;
	if( dlg.DoModal() != IDOK )
		return;

	SaveAsImage( dlg.GetPathName(), SizeDialog.GetResolutionWidth(), SizeDialog.GetResolutionHeight(), SizeDialog.GetResolutionScale(), SizeDialog.GetMarginScale() );
}

void CLinkageView::OnFileSaveDXF()
{
	if( AfxMessageBox( "This feature is not complete. Solid links and dotted lines, as well as some other features, may not work properly.", MB_OKCANCEL | MB_ICONEXCLAMATION ) != IDOK )
		return;

	CFileDialog dlg( 0, "dxf", 0, 0, "DXF Files (*.dxf)|*.dxf|All Files (*.*)|*.*||" );
	dlg.m_ofn.lpstrTitle = "Export to DXF";
	dlg.m_ofn.Flags |= OFN_NOCHANGEDIR | OFN_NONETWORKBUTTON | OFN_OVERWRITEPROMPT;
	if( dlg.DoModal() != IDOK )
		return;

	SaveAsDXF( dlg.GetPathName() );
}

void CLinkageView::OnUpdateBackgroundTransparency( CCmdUI *pCmdUI )
{
	pCmdUI->Enable( m_pBackgroundBitmap != 0 );
}

void CLinkageView::OnBackgroundTransparency()
{
	CMFCRibbonBar *pRibbon = ((CFrameWndEx*) AfxGetMainWnd())->GetRibbonBar();
	if( pRibbon == 0 )
		return;
	CMFCRibbonSlider * pSlider = DYNAMIC_DOWNCAST( CMFCRibbonSlider, pRibbon->FindByID( ID_BACKGROUND_TRANSPARENCY ) );
	if( pSlider == 0 )
		return;
	double Value = (double)pSlider->GetPos() / (double)pSlider->GetRangeMax();

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pDoc->PushUndo();
	pDoc->SetBackgroundTransparency( Value );

	UpdateForDocumentChange();
}

void CLinkageView::OnDeleteBackground()
{
	if( m_pBackgroundBitmap != 0 )
	{
		delete m_pBackgroundBitmap;
		m_pBackgroundBitmap = 0;
	}
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->SetBackgroundImage( "" );

	UpdateForDocumentChange();
}

void CLinkageView::OnUpdateDeleteBackground( CCmdUI *pCmdUI )
{
	pCmdUI->Enable( m_pBackgroundBitmap != 0 );
}

void CLinkageView::OnUpdateFileSaveMotion( CCmdUI *pCmdUI )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Count = 0;

	ConnectorList* pConnectors = pDoc->GetConnectorList();
	POSITION Position = pConnectors->GetHeadPosition();
	while( Position != 0 )
	{
		CConnector *pConnector = pConnectors->GetNext( Position );
		if( pConnector == 0 )
			continue;
		if( !pConnector->IsDrawing() )
			continue;

		int DrawPoint = 0;
		int PointCount = 0;
		int MaxPoints = 0;
		pConnector->GetMotionPath( DrawPoint, PointCount, MaxPoints );
		Count += PointCount;
		if( Count > 0 )
			break;
	}

	pCmdUI->Enable( Count > 0 );
}

void CLinkageView::OnFileOpenBackground()
{
	#if defined( LINKAGE_USE_DIRECT2D )
		CFileDialog dlg( TRUE, "", 0, 0, "JPEG Files (*.jpg)|*.jpg|PNG Files (*.png)|*.png|All Supported Image Files (*.jpg;*.png)|*.jpg;*.png|All Files (*.*)|*.*||" );
		dlg.m_ofn.lpstrTitle = "Import Background Image";
		dlg.m_ofn.Flags |= OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_NONETWORKBUTTON;
		if( dlg.DoModal() != IDOK )
			return;

		CFile BackgroundFile;
		if( !BackgroundFile.Open( dlg.GetPathName(), CFile::modeRead ) )
		{
			AfxMessageBox( "The selected file cannot be opened.", MB_OKCANCEL | MB_ICONEXCLAMATION );
			return;
		}
		BackgroundFile.Close();

		if( m_pBackgroundBitmap != 0 )
		{
			delete m_pBackgroundBitmap;
			m_pBackgroundBitmap = 0;
		}

		std::ifstream file(  dlg.GetPathName(), std::ios::binary | std::ios::ate );
		std::streamsize size = file.tellg();
		file.seekg( 0, std::ios::beg );

		if( size > 0 )
		{
			std::vector<byte> buffer( (size_t)size );
			if( file.read( (char*)buffer.data(), (size_t)size ) )
			{
				m_pBackgroundBitmap = ImageBytesToRendererBitmap( buffer, (size_t)size );

				CLinkageDoc* pDoc = GetDocument();
				ASSERT_VALID(pDoc);
				std::string Base64Data = base64_encode( (const unsigned char*)buffer.data(), (unsigned int)size );
				pDoc->SetBackgroundImage( Base64Data );
			}
		}
		else
		{
			AfxMessageBox( "The selected file cannot be opened.", MB_OKCANCEL | MB_ICONEXCLAMATION );
			return;
		}

		file.close();

		UpdateForDocumentChange();
	#endif
}

CRendererBitmap* CLinkageView::ImageBytesToRendererBitmap( std::vector<byte> &Buffer, size_t Size )
{
	#if defined( LINKAGE_USE_DIRECT2D )
		CRenderer Renderer( CRenderer::WINDOWS_D2D );

		// Maybe slow and wrong to create a renderer but the D2D stuff is needed for this.
		CBitmap Bitmap;
		PrepareRenderer( Renderer, 0, 0, 0, 1.0, false, 0.0, 1.0, false, false, false, 0 );

		return Renderer.LoadRendererBitmapFromMemory( (const unsigned char*)Buffer.data(), Size );
	#else
		return 0;
	#endif
}

void CLinkageView::OnFileSaveMotion()
{
	CFileDialog dlg( 0, "txt", 0, 0, "Text Files (*.txt)|*.txt|All Files (*.*)|*.*||" );
	dlg.m_ofn.lpstrTitle = "Export Motion Paths";
	dlg.m_ofn.Flags |= OFN_NOCHANGEDIR | OFN_NONETWORKBUTTON | OFN_OVERWRITEPROMPT;
	if( dlg.DoModal() != IDOK )
		return;

	CFile OutputFile;
	if( !OutputFile.Open( dlg.GetPathName(), CFile::modeCreate | CFile::modeWrite ) )
	{
		AfxMessageBox( "The selected file cannot be opened.", MB_OKCANCEL | MB_ICONEXCLAMATION );
		return;
	}

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	double DocumentScale = pDoc->GetUnitScale();

	ConnectorList* pConnectors = pDoc->GetConnectorList();
	POSITION Position = pConnectors->GetHeadPosition();
	bool bFirst = true;
	while( Position != 0 )
	{
		CConnector *pConnector = pConnectors->GetNext( Position );
		if( pConnector == 0 )
			continue;
		if( !pConnector->IsDrawing() && !pConnector->IsAnchor() )
			continue;

		if( pConnector->IsAnchor() )
		{
			CFPoint Point = pConnector->GetOriginalPoint();
			CString Name = "Anchor ";
			Name += pConnector->GetIdentifierString( m_bShowDebug );
			Name += "\r\n";
			OutputFile.Write( Name, Name.GetLength() );
			CString Temp;
			Temp.Format( "%.6f,%.6f\r\n", Point.x * DocumentScale, Point.y * DocumentScale );
			OutputFile.Write( (const char*)Temp, Temp.GetLength() );
		}
		else
		{
			int DrawPoint = 0;
			int PointCount = 0;
			int MaxPoints = 0;
			CFPoint *pPoints = pConnector->GetMotionPath( DrawPoint, PointCount, MaxPoints );

			CString Name = "Connector ";
			Name += pConnector->GetIdentifierString( m_bShowDebug );
			Name += "\r\n";
			OutputFile.Write( Name, Name.GetLength() );

			CString Temp;
			Temp.Format( "%d\r\n", PointCount );
			OutputFile.Write( Temp, Temp.GetLength() );

			for( int Counter = 0; Counter < PointCount; ++Counter )
			{
				Temp.Format( "%.6f,%.6f\r\n", pPoints[Counter].x * DocumentScale, pPoints[Counter].y * DocumentScale );
				OutputFile.Write( (const char*)Temp, Temp.GetLength() );
			}
		}
	}
}

void CLinkageView::OnFilePrint()
{
}

class CMyPreviewView : public CPreviewView
{
	public:
	CDialogBar *GetToolBar( void ) { return m_pToolBar; }
};

void CLinkageView::OnFilePrintPreview()
{
	ShowPrintPreviewCategory();
	CView::OnFilePrintPreview();

	/*
	 * The code below is a bit of a klude that relies on the print preview working
	 * in a very specific way. There is a CDialogBar that is created by the print preview
	 * code and it sits on top of the windows title bar. This has the effect of disabling
	 * the close button, as well as the min and max buttons. But it fails and the close button
	 * malfunctions when it is pressed causing a short hang in the program (or longer).
	 *
	 * So this code uses a short child class of the CPreviewView class to allow us to
	 * get the dialog bar and hide it. There seem to be no controls in it at this time so
	 * it is safe to hide. The mainframe code is then allowed to handle the close message
	 * as it sees fit (usually closing the print preview and not the whole window).
	 * This code will fail if the pointers are to objects other than what is expected!
	 */

	CMainFrame *pFrame = (CMainFrame*)AfxGetMainWnd();
	if( pFrame == 0 )
		return;
	CView *pView = pFrame->GetActiveView();
	if( pView == 0 )
		return;
	CMyPreviewView *pPreview = (CMyPreviewView*)pView;
	CDialogBar *pPreviewToolBar = pPreview->GetToolBar();
	if( pPreviewToolBar == 0 )
		return;
	pPreviewToolBar->ShowWindow( SW_HIDE );
}

void CLinkageView::OnEndPrintPreview( CDC* pDC, CPrintInfo* pInfo, POINT point, CPreviewView* pView )
{
	CView::OnEndPrintPreview( pDC, pInfo, point, pView );
	ShowPrintPreviewCategory( false );
}

void CLinkageView::OnSimulateInteractive()
{
	if( !CanSimulate() )
		return;
	
	ConfigureControlWindow( INDIVIDUAL );

	StartMechanismSimulate( INDIVIDUAL );
}

void CLinkageView::OnSimulatePause()
{
	if( !CanSimulate() )
		return;

	if( m_bSimulating && ( m_SimulationControl == AUTO || m_SimulationControl == ONECYCLE ) )
		m_SimulationControl = STEP;
	else
	{
		ConfigureControlWindow( STEP );
		StartMechanismSimulate( STEP );
	}
}

void CLinkageView::OnSimulateOneCycle()
{
	if( !CanSimulate() )
		return;

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if( m_bSimulating )
	{
		if( m_SimulationSteps != m_PauseStep )
			return;

		// Assume that the simulation step absolute value is at zero or some number evenly divisible by the cycle step.
		m_PauseStep = m_SimulationSteps + m_Simulator.GetCycleSteps( pDoc, &m_ForceCPM );
	}
	else
	{
		m_SimulationSteps = 0;
		m_PauseStep = m_Simulator.GetCycleSteps( pDoc, &m_ForceCPM  );
		if( m_PauseStep == 0 )
			return;
		ConfigureControlWindow( ONECYCLE );
		StartMechanismSimulate( ONECYCLE );
	}
}

void CLinkageView::OnSimulateOneCycleX()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if( m_TimerID != 0 )
		timeKillEvent( m_TimerID );

	//pDoc->SelectElement();
	m_MouseAction = ACTION_NONE;

	if( m_bSimulating )
	{
		m_bSimulating = false;
		m_TimerID = 0;
		OnMechanismReset();
	}

	pDoc->Reset( true );
	m_Simulator.Reset();

	m_PauseStep = m_Simulator.GetCycleSteps( pDoc, &m_ForceCPM  );
	if( m_PauseStep == 0 )
		return;

	bool bGoodSimulation = true;
	int Counter = 0;
	for( ; Counter <= m_PauseStep && bGoodSimulation; ++Counter )
		bGoodSimulation = m_Simulator.SimulateStep( pDoc, Counter, true, 0, 0, 0, false, m_ForceCPM );

	DebugItemList.Clear();

	// Start the timer to get the normal sim code to keep running.
	m_SimulationControl = ONECYCLEX;
	m_SimulationSteps = m_PauseStep;
	if( m_TimerID != 0 )
		timeKillEvent( m_TimerID );
	SetScrollExtents( false );
	m_ControlWindow.ShowWindow( m_ControlWindow.GetControlCount() > 0 ? SW_SHOWNORMAL : SW_HIDE );
	m_bSimulating = true;
	m_TimerID = timeSetEvent( 33, 1, TimeProc, (DWORD_PTR)this, 0 );
}

void CLinkageView::OnEscape()
{
	if( m_bSimulating )
		StopSimulation();

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pDoc->ClearSelection();
	pDoc->Reset( true, true );

	UpdateForDocumentChange();
}

void CLinkageView::OnUpdateEscape(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( true );
}

void CLinkageView::StartTimer( void )
{
	timeBeginPeriod( TimerMinimumResolution );

	// Create the timer queue.
	m_hTimerQueue = CreateTimerQueue();

	CreateTimerQueueTimer( &m_hTimer, m_hTimerQueue, TimerQueueCallback, (PVOID)this , 5, 5, 0 );
}

void CLinkageView::EndTimer( void )
{
	DeleteTimerQueueTimer ( m_hTimerQueue, m_hTimer, 0 );
	DeleteTimerQueue( m_hTimerQueue );

	timeEndPeriod( TimerMinimumResolution );
}

void CLinkageView::OnSimulateStep( int Direction, bool bBig )
{
	m_SimulationSteps = ( Direction > 0 ? 1 : -1 ) * ( bBig ? 10 : 1 );
}

void CLinkageView::OnSimulateForward()
{
	OnSimulateStep( 1, false );
}

void CLinkageView::OnSimulateBackward()
{
	OnSimulateStep( -1, false );
}

void CLinkageView::OnSimulateForwardBig()
{
	OnSimulateStep( 1, true );
}

void CLinkageView::OnSimulateBackwardBig()
{
	OnSimulateStep( -1, true );
}

void CLinkageView::OnUpdateSimulatePause(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( ( !m_bSimulating || m_SimulationControl == AUTO ) && !pDoc->IsEmpty() && m_bAllowEdit );
}

void CLinkageView::OnUpdateSimulateOneCycle(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( !pDoc->IsEmpty() && m_Simulator.GetCycleSteps( pDoc, 0 ) != 0 && ( m_SimulationSteps == 0 || m_SimulationSteps == m_PauseStep ) );
}

void CLinkageView::OnUpdateSimulateForwardBackward(CCmdUI *pCmdUI)
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable( m_bSimulating && m_SimulationControl == STEP && !pDoc->IsEmpty() );
}

void CLinkageView::OnUpdateSimulateInteractive(CCmdUI *pCmdUI)
{
	pCmdUI->Enable( !m_bSimulating && m_bAllowEdit );
}

void CLinkageView::OnSimulateManual()
{
	if( !CanSimulate() )
		return;

	ConfigureControlWindow( GLOBAL );
	StartMechanismSimulate( GLOBAL );
}

void CLinkageView::OnUpdateSimulateManual(CCmdUI *pCmdUI)
{
	pCmdUI->Enable( !m_bSimulating && m_bAllowEdit );
}

void CLinkageView::OnInsertDouble()
{
	InsertDouble( 0 );
}

void CLinkageView::OnInsertAnchor()
{
	InsertAnchor( 0 );
}

void CLinkageView::OnInsertAnchorLink()
{
	InsertAnchorLink( 0 );
}

void CLinkageView::OnInsertRotatinganchor()
{
	InsertRotatinganchor( 0 );
}

void CLinkageView::OnInsertActuator()
{
	InsertActuator( 0 );
}

void CLinkageView::OnInsertConnector()
{
	InsertConnector( 0 );
}

void CLinkageView::OnInsertCircle()
{
	InsertCircle( 0 );
}

void CLinkageView::OnInsertTriple()
{
	InsertTriple( 0 );
}

void CLinkageView::OnInsertQuad()
{
	InsertQuad( 0 );
}

void CLinkageView::OnInsertPoint()
{
	InsertPoint( 0 );
}

void CLinkageView::OnInsertLine()
{
	InsertLine( 0 );
}

void CLinkageView::OnInsertMeasurement()
{
	InsertMeasurement( 0 );
}

void CLinkageView::OnInsertGear()
{
	InsertGear( 0 );
}

void CLinkageView::OnInsertLink()
{
	InsertLink( 0 );
}

void CLinkageView::OnPopupGallery()
{
	if( m_pPopupGallery == 0 )
		return;
	int Selection = m_pPopupGallery->GetSelection();

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CFPoint PopupPoint = Unscale( m_PopupPoint );

	switch( Selection )
	{
		case 0: InsertConnector( &PopupPoint ); break;
		case 1: InsertAnchor( &PopupPoint ); break;
		case 2: InsertDouble( &PopupPoint ); break;
		case 3: InsertAnchorLink( &PopupPoint ); break;
		case 4: InsertRotatinganchor( &PopupPoint ); break;
		case 5: InsertActuator( &PopupPoint ); break;
		case 6: InsertTriple( &PopupPoint ); break;
		case 7: InsertQuad( &PopupPoint ); break;
		case 8: InsertGear( &PopupPoint ); break;
		case 9: break; // Blank spot
		case 10: InsertPoint( &PopupPoint ); break;
		case 11: InsertLine( &PopupPoint ); break;
		case 12: InsertMeasurement( &PopupPoint ); break;
		case 13: InsertCircle( &PopupPoint ); break;
		default: return;
	}
}

void CLinkageView::InsertActuator( CFPoint *pPoint )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->InsertLink( CLinkageDoc::MECHANISMLAYER, Unscale( LINK_INSERT_PIXELS ), pPoint == 0 ? GetDocumentViewCenter() : *pPoint, pPoint != 0, 2, CLinkageDoc::ACTUATOR, m_bNewLinksSolid );
	m_bSuperHighlight = true;
	UpdateForDocumentChange();
	SelectionChanged();
}

int CLinkageView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	#if defined( LINKAGE_USE_DIRECT2D )
		CWindowDC DC( this );
		int PPI = DC.GetDeviceCaps( LOGPIXELSX );
		m_ScreenDPIScale = (double)PPI / 96.0;
	#else
		m_ScreenDPIScale = 1.0;
	#endif

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	m_ControlWindow.Create( 0, 0, WS_CHILD | WS_CLIPCHILDREN, CRect( 0, 0, 100, 100 ), this, ID_CONTROL_WINDOW );
	m_ControlWindow.ShowWindow( SW_HIDE );

    BOOL success = m_OleWndDropTarget.Register(this);

	return 0;
}

void CLinkageView::OnDestroy()
{
	m_bRequestAbort = true;

	m_ControlWindow.DestroyWindow();

	CView::OnDestroy();
}

COleDropWndTarget::COleDropWndTarget() {}

COleDropWndTarget::~COleDropWndTarget() {}

DROPEFFECT COleDropWndTarget::OnDragEnter(CWnd* pWnd, COleDataObject*
           pDataObject, DWORD dwKeyState, CPoint point )
{
	// We always copy the dropped item.
	return DROPEFFECT_COPY;
}

void COleDropWndTarget::OnDragLeave(CWnd* pWnd)
{
    COleDropTarget::OnDragLeave(pWnd);
}

DROPEFFECT COleDropWndTarget::OnDragOver(CWnd* pWnd, COleDataObject*
           pDataObject, DWORD dwKeyState, CPoint point )
{
	// We always copy the dropped item and allow a drop anywhere in the view window.
	return DROPEFFECT_COPY;
}

BOOL COleDropWndTarget::OnDrop(CWnd* pWnd, COleDataObject* pDataObject,
                 DROPEFFECT dropEffect, CPoint point )
{
    HGLOBAL  hGlobal;
    LPCSTR   pData;

    UINT CF_Linkage = RegisterClipboardFormat( "RECTORSQUID_Linkage_CLIPBOARD_XML_FORMAT" );

    // Get text data from COleDataObject
    hGlobal=pDataObject->GetGlobalData(CF_Linkage);

	if( hGlobal == 0 )
		return FALSE;

    // Get pointer to data
    pData=(LPCSTR)GlobalLock(hGlobal);

    ASSERT(pData!=NULL);

    // Unlock memory - Send dropped text into the "bit-bucket"
    GlobalUnlock(hGlobal);

    return TRUE;
}

void CLinkageView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}

static const int SHIFT_FLAG = 0x01;
static const int CONTROL_FLAG = 0x02;
static const int ALT_FLAG = 0x04;

bool CLinkageView::HandleShortcutKeys( UINT nChar, unsigned int MyFlags )
{
	static const unsigned long Shortcuts[][3] =
	{
		{ '-', ID_VIEW_ZOOMOUT, 0 },
		{ '_', ID_VIEW_ZOOMOUT, 0 },
		{ '+', ID_VIEW_ZOOMIN, 0 },
		{ '=', ID_VIEW_ZOOMIN, 0 },
		{ 'A', ID_EDIT_ADDCONNECTOR, 0 },
		{ 'A', ID_EDIT_SELECT_ALL, CONTROL_FLAG },
		{ 'B', ID_EDIT_COMBINE, 0 },
		{ 'C', ID_EDIT_CONNECT, 0 },
		{ 'C', ID_EDIT_COPY, CONTROL_FLAG },
		{ 'D', ID_VIEW_DIMENSIONS, 0 },
		{ 'E', ID_EDIT_SELECTLINK, 0 },
		{ 'F', ID_EDIT_FASTEN, 0},
		{ 'G', ID_EDIT_MAKEANCHOR, 0 },
		{ 'J', ID_EDIT_JOIN, 0 },
		{ 'K', ID_EDIT_LOCK, 0 },
		{ 'L', ID_EDIT_CONNECT, 0 }, // CHANGED FROM ID_EDIT_SLIDE 6/4/2018
		{ 'N', ID_FILE_NEW, CONTROL_FLAG },
		{ 'O', ID_FILE_OPEN, CONTROL_FLAG },
		{ 'P', ID_FILE_PRINT, CONTROL_FLAG },
		{ 'P', ID_PROPERTIES_PROPERTIES, 0 },
		{ 'R', ID_SIMULATION_RUN, 0 },
		{ 'S', ID_SIMULATION_STOP, 0 },
		{ 'S', ID_FILE_SAVE, CONTROL_FLAG },
		{ 'T', ID_EDIT_SPLIT, 0 },
		{ 'U', ID_EDIT_UNFASTEN, 0 },
		{ 'V', ID_EDIT_PASTE, CONTROL_FLAG },
		{ 'V', ID_VIEW_ANICROP, 0 },
		{ 'X', ID_EDIT_CUT, CONTROL_FLAG },
		{ 'Z', ID_EDIT_UNDO, CONTROL_FLAG },
		{ VK_INSERT, ID_EDIT_COPY, CONTROL_FLAG },
		{ VK_INSERT, ID_EDIT_PASTE, SHIFT_FLAG },
		{ VK_DELETE, ID_EDIT_CUT, SHIFT_FLAG },
		{ VK_DELETE, ID_EDIT_DELETESELECTED},
		{ VK_BACK, ID_EDIT_UNDO, ALT_FLAG },
		{ VK_TAB, ID_SELECT_NEXT, 0 },
		{ VK_TAB, ID_SELECT_PREVIOUS, SHIFT_FLAG },

		{ VK_LEFT, ID_NUDGE_LEFT, 0 },
		{ VK_RIGHT, ID_NUDGE_RIGHT, 0 },
		{ VK_UP, ID_NUDGE_UP, 0 },
		{ VK_DOWN, ID_NUDGE_DOWN, 0 },
		{ VK_LEFT, ID_NUDGE_BIGLEFT, SHIFT_FLAG },
		{ VK_RIGHT, ID_NUDGE_BIGRIGHT, SHIFT_FLAG },
		{ VK_UP, ID_NUDGE_BIGUP, SHIFT_FLAG },
		{ VK_DOWN, ID_NUDGE_BIGDOWN, SHIFT_FLAG },
		{ VK_LEFT, ID_NUDGE_BIGLEFT, CONTROL_FLAG },
		{ VK_RIGHT, ID_NUDGE_BIGRIGHT, CONTROL_FLAG },
		{ VK_UP, ID_NUDGE_BIGUP, CONTROL_FLAG },
		{ VK_DOWN, ID_NUDGE_BIGDOWN, CONTROL_FLAG },

		{ '<', ID_SIMULATE_BACKWARD, 0 },
		{ '[', ID_SIMULATE_BACKWARD, 0 },
		{ '>', ID_SIMULATE_FORWARD, 0 },
		{ ']', ID_SIMULATE_FORWARD, 0 },
		{ VK_ESCAPE, ID_ESCAPE, 0 },
		{ 0, 0, 0 }
	};

	for( int Index = 0; Shortcuts[Index][0] != 0; ++Index )
	{
		if( Shortcuts[Index][0] == nChar && Shortcuts[Index][2] == MyFlags )
		{
			this->SendMessage( WM_COMMAND, Shortcuts[Index][1] );
			return true;
		}
	}

	return false;
}

void CLinkageView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if( !HandleShortcutKeys( nChar, 0 ) )
		CView::OnChar(nChar, nRepCnt, nFlags);
	/*
	 * Handle a few special keys here because accelerators cannot be used
	 * for them. The accelerators keep the characters from being used in
	 * an edit control in the ribbon bar.
	 */

	/*switch( nChar )
	{
		case '-':
		case '_':
			OnViewZoomout();
			break;
		case '+':
		case '=':
			OnViewZoomin();
			break;
	}
	CView::OnChar(nChar, nRepCnt, nFlags);*/
}

void CLinkageView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	unsigned int MyFlags = 0;
	if( ( GetKeyState( VK_SHIFT ) & 0x8000 ) != 0 )
		MyFlags |= SHIFT_FLAG;
	else if( ( GetKeyState( VK_CONTROL ) & 0x8000 ) != 0)
		MyFlags |= CONTROL_FLAG;

	if( !HandleShortcutKeys( nChar, MyFlags ) )
		CView::OnKeyDown( nChar, nRepCnt, nFlags );
}

BOOL CLinkageView::PreTranslateMessage(MSG* pMsg)
{
	if( pMsg->message == WM_KEYDOWN && pMsg->wParam==VK_TAB )
	{
		OnKeyDown( VK_TAB, 0, 0 );
		return TRUE;
	}

	return CView::PreTranslateMessage(pMsg);
}

/*
 * Scale to get from document units to pixels. Document units are 96 DPI on a
 * typical screen. The document unit selection, inches or mm, are not used in
 * internal processes, just in showing measurement data to the user and
 * measurement input.
 *
 * Scale is also a document to pixel conversion function.
 *
 * Y document coordinates are negated to get from document to the screen
 * coordinate system and back.
 */

void CLinkageView::Scale( double &x, double &y )
{
	y *= m_YUpDirection;
	x *= m_Zoom;
	y *= m_Zoom;
	x -= m_ScrollPosition.x;
	y -= m_ScrollPosition.y;
	x += m_DrawingRect.Width() / m_DPIScale / 2;
	y += m_DrawingRect.Height() / m_DPIScale / 2;
}

void CLinkageView::Unscale( double &x, double &y )
{
	x -= m_DrawingRect.Width() / m_DPIScale / 2;
	y -= m_DrawingRect.Height() / m_DPIScale / 2;
	x += m_ScrollPosition.x;
	y += m_ScrollPosition.y;
	x /= m_Zoom;
	y /= m_Zoom;
	y *= m_YUpDirection;
}

CFPoint CLinkageView::Unscale( CFPoint Point )
{
	CFPoint NewPoint = Point;
	Unscale( NewPoint.x, NewPoint.y );
	return NewPoint;
}

double CLinkageView::Unscale( double Distance )
{
	return Distance / m_Zoom;
}

double CLinkageView::Scale( double Distance )
{
	return Distance * m_Zoom;
}

CFRect CLinkageView::Unscale( CFRect Rect )
{
	CFRect NewRect( Rect.left, Rect.top, Rect.right, Rect.bottom );
	Unscale( NewRect.left, NewRect.top );
	Unscale( NewRect.right, NewRect.bottom );
	NewRect.Normalize();
	return NewRect;
}

CFRect CLinkageView::Scale( CFRect Rect )
{
	Scale( Rect.left, Rect.top );
	Scale( Rect.right, Rect.bottom );
	CFRect NewRect( Rect.left, Rect.top, Rect.right, Rect.bottom );
	NewRect.Normalize();
	return NewRect;
}

CFArc CLinkageView::Unscale( CFArc TheArc )
{
	CFPoint Center = Unscale( CFPoint( TheArc.x, TheArc.y ) );
	CFLine TempLine = Unscale( CFLine( TheArc.m_Start, TheArc.m_End ) );
	CFArc NewArc( Center, TheArc.r / m_Zoom, TempLine.m_Start, TempLine.m_End );
	return NewArc;
}

CFArc CLinkageView::Scale( CFArc TheArc )
{
	CFPoint Center = Scale( CFPoint( TheArc.x, TheArc.y ) );
	CFLine TempLine = Scale( CFLine( TheArc.m_Start, TheArc.m_End ) );
	CFArc NewArc( Center, TheArc.r * m_Zoom, TempLine.m_Start, TempLine.m_End );
	return NewArc;
}

CFLine CLinkageView::Unscale( CFLine Line )
{
	CFLine NewLine = Line;
	Unscale( NewLine.m_Start.x, NewLine.m_Start.y );
	Unscale( NewLine.m_End.x, NewLine.m_End.y );
	return NewLine;
}

CFLine CLinkageView::Scale( CFLine Line )
{
	CFLine NewLine = Line;
	Scale( NewLine.m_Start.x, NewLine.m_Start.y );
	Scale( NewLine.m_End.x, NewLine.m_End.y );
	return NewLine;
}

CFPoint CLinkageView::Scale( CFPoint Point )
{
	Scale( Point.x, Point.y );
	return Point;
}

CFCircle CLinkageView::Unscale( CFCircle Circle )
{
	CFCircle NewCircle = Circle;
	Unscale( NewCircle.x, NewCircle.y );
	NewCircle.r /= m_Zoom;
	return NewCircle;
}

CFCircle CLinkageView::Scale( CFCircle Circle )
{
	CFCircle NewCircle = Circle;
	Scale( NewCircle.x, NewCircle.y );
	NewCircle.r *= m_Zoom;
	return NewCircle;
}

static int Selection = 0;

void CLinkageView::OnViewUnits()
{
	CMFCRibbonBar *pRibbon = ((CFrameWndEx*) AfxGetMainWnd())->GetRibbonBar();
	if( pRibbon == 0 )
		return;
	CMFCRibbonComboBox * pComboBox = DYNAMIC_DOWNCAST( CMFCRibbonComboBox, pRibbon->FindByID( ID_VIEW_UNITS ) );
	if( pComboBox == 0 )
		return;

	int nCurSel = pComboBox->GetCurSel();
	if( nCurSel < 0 )
		return;

	DWORD Data = (DWORD)pComboBox->GetItemData( nCurSel );

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if( pDoc->GetUnits() != (CLinkageDoc::_Units)Data )
	{
		pDoc->SetUnits( (CLinkageDoc::_Units)Data );
		pDoc->SetModifiedFlag();
	}

	ShowSelectedElementCoordinates();
	InvalidateRect( 0 );
}

void CLinkageView::OnUpdateViewUnits( CCmdUI *pCmdUI )
{
//	CLinkageDoc* pDoc = GetDocument();
//	ASSERT_VALID(pDoc);
//	CLinkageDoc::_Units Units = pDoc->GetUnits();

	pCmdUI->Enable( !m_bSimulating );
}

void CLinkageView::ShowSelectedElementCoordinates( void )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CMFCRibbonBar *pRibbon = ((CFrameWndEx*) AfxGetMainWnd())->GetRibbonBar();
	if( pRibbon == 0 )
		return;
	CMFCRibbonEdit * pEditBox = DYNAMIC_DOWNCAST( CMFCRibbonEdit, pRibbon->FindByID( ID_VIEW_COORDINATES ) );
	if( pEditBox == 0 )
		return;

	// The cast is acceptable since the CMyMFCRibbonLabel adds functionality but no variables.
	CMyMFCRibbonLabel * pLabel = (CMyMFCRibbonLabel*)pRibbon->FindByID( ID_VIEW_DIMENSIONSLABEL );
	if( pLabel == 0 )
		return;

	CString Hint = "";
	CString Text = pDoc->GetSelectedElementCoordinates( &Hint );

	pEditBox->SetEditText( Text );
	pLabel->SetTextAndResize( Hint );


	//CMFCRibbonLabel
}

void CLinkageView::OnViewCoordinates()
{
	CMFCRibbonBar *pRibbon = ((CFrameWndEx*) AfxGetMainWnd())->GetRibbonBar();
	if( pRibbon == 0 )
		return;
	CMFCRibbonEdit * pEditBox = DYNAMIC_DOWNCAST( CMFCRibbonEdit, pRibbon->FindByID( ID_VIEW_COORDINATES ) );
	if( pEditBox == 0 )
		return;

	CString Text = pEditBox->GetEditText();
	if( Text.GetLength() == 0 )
		return;

	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	bool bUpdateRotationCenter = true;

	if( pDoc->IsSelectionMeshableGears() )
	{
		double Size1 = 0;
		double Size2 = 0;
		int Count = sscanf_s( (const char*)Text, "%lf:%lf", &Size1, &Size2 );
		if( Count < 2 )
			return;

		CLink *pLink1 = pDoc->GetSelectedLink( 0, false );
		CLink *pLink2 = pDoc->GetSelectedLink( 1, false );

		CGearConnection *pConnection = pDoc->GetGearRatio( pLink1, pLink2 );

		if( !pDoc->SetGearRatio( pLink1, pLink2, Size1, Size2, pConnection == 0 ? false : pConnection->m_bUseSizeAsRadius, pConnection == 0 ? CGearConnection::GEARS : pConnection->m_ConnectionType ) )
			return;
	}
	else
	{
		CLinkageDoc::_CoordinateChange Result = pDoc->SetSelectedElementCoordinates( &m_SelectionRotatePoint, Text  );
		if( Result == CLinkageDoc::_CoordinateChange::NONE )
			return;

		if( Result == CLinkageDoc::_CoordinateChange::ROTATION )
			bUpdateRotationCenter = false;

	}

	ShowSelectedElementCoordinates();

	m_SelectionContainerRect = GetDocumentArea( false, true );
	m_SelectionAdjustmentRect = GetDocumentAdjustArea( true );

	UpdateForDocumentChange( bUpdateRotationCenter );
}

void CLinkageView::OnUpdateViewCoordinates( CCmdUI *pCmdUI )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int SelectedConnectors = pDoc->GetSelectedConnectorCount();
	int SelectedLinks = pDoc->GetSelectedLinkCount( true );

	//pCmdUI->Enable( !m_bSimulating && ( ( SelectedConnectors == 1 || SelectedConnectors == 2 || SelectedConnectors == 3 || pDoc->IsSelectionMeshableGears() ) ? 1 : 0 ) && m_bAllowEdit );
	pCmdUI->Enable( !m_bSimulating && m_bAllowEdit && ( SelectedConnectors + SelectedLinks > 0 || pDoc->IsSelectionMeshableGears() ) ? 1 : 0 );
}

static bool GetEncoderClsid(WCHAR *format, CLSID *pClsid)
{
	unsigned int num = 0,  size = 0;
	GetImageEncodersSize(&num, &size);
	if( size == 0 )
		return false;
	ImageCodecInfo *pImageCodecInfo = (ImageCodecInfo *)(malloc(size));
	if( pImageCodecInfo == NULL )
		return false;
	GetImageEncoders(num, size, pImageCodecInfo);
	for( unsigned int Index = 0; Index < num; ++Index )
	{
		if(wcscmp(pImageCodecInfo[Index].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[Index].Clsid;
			free(pImageCodecInfo);
			return true;
		}
	}
	free(pImageCodecInfo);
	return false;
}

bool CLinkageView::DisplayAsImage( CDC *pOutputDC, int xOut, int yOut, int OutWidth, int OutHeight, int RenderWidth, int RenderHeight, double ResolutionAdjustmentFactor, double MarginScale, COLORREF BackgroundColor, bool bAddBorder )
{
	CRenderer Renderer( CRenderer::WINDOWS_GDI );
	CBitmap MemoryBitmap;

	int Width = (int)( 1920 * ResolutionAdjustmentFactor );
	double Temp = 1920 * RenderHeight / RenderWidth;
	int Height = (int)( Temp * ResolutionAdjustmentFactor );

	CRect ImageRect( 0, 0, Width, Height );

	CDC *pDC = GetDC();

	double ScaleFactor = 4.0;

	double SaveDPIScale = m_DPIScale;
	m_DPIScale = 1.0;

	PrepareRenderer( Renderer, &ImageRect, &MemoryBitmap, pDC, ScaleFactor, true, MarginScale, 1.0, false, true, false, 0 );

	DoDraw( &Renderer );

	m_DPIScale = SaveDPIScale;

	CDC ShrunkDC;
	CBitmap ShrunkBitmap;

	ShrunkDC.CreateCompatibleDC( pDC );
	ShrunkBitmap.CreateCompatibleBitmap( pDC, RenderWidth, RenderHeight );
	ShrunkDC.SelectObject( &ShrunkBitmap );
	ShrunkDC.PatBlt( 0, 0, RenderWidth, RenderHeight, WHITENESS );

	StretchBltPlus( ShrunkDC.GetSafeHdc(),
		            0, 0, RenderWidth, RenderHeight,
		            Renderer.GetSafeHdc(),
					0, 0, (int)( Width * ScaleFactor ), (int)( Height * ScaleFactor ),
					SRCCOPY );

	double WidthScale = (double)OutWidth / (double)RenderWidth;
	double HeightScale = (double)OutHeight / (double)RenderHeight;
	double Scale = min( WidthScale, HeightScale );
	if( Scale > 1.0 )
		Scale = 1.0;
	int FinalWidth = (int)( RenderWidth * Scale );
	int FinalHeight = (int)( RenderHeight * Scale );
	int xOffset = ( OutWidth - FinalWidth ) / 2;
	int yOffset = ( OutHeight - FinalHeight ) / 2;

	CBrush BackgroundBrush( BackgroundColor );
	CRect AllRect( xOut, yOut, xOut + OutWidth, yOut + OutHeight );
	pOutputDC->FillRect( &AllRect, &BackgroundBrush );

	StretchBltPlus( pOutputDC->GetSafeHdc(),
		            xOut + xOffset, yOut + yOffset, FinalWidth, FinalHeight,
		            ShrunkDC.GetSafeHdc(),
					0, 0, RenderWidth, RenderHeight,
					SRCCOPY );

	if( bAddBorder )
	{
		CBrush Brush( RGB( 0, 0, 0 ) );
		CRect Rect( xOut + xOffset, yOut + yOffset, xOut + xOffset + FinalWidth, yOut + yOffset + FinalHeight );
		pOutputDC->FrameRect( &Rect, &Brush );
	}

	Renderer.EndDraw();

	return true;
}

bool CLinkageView::SaveAsImage( const char *pFileName, int RenderWidth, int RenderHeight, double ResolutionAdjustmentFactor, double MarginScale )
{
	CString SearchableFileName = pFileName;
	SearchableFileName.MakeUpper();
	bool bJPEG = false;
	if( SearchableFileName.Find( ".JPG" ) == SearchableFileName.GetLength() - 4 )
		bJPEG = true;
	else if( SearchableFileName.Find( ".PNG" )  != SearchableFileName.GetLength() - 4 )
	{
		AfxMessageBox( "Images can be saved as either JPEG or PNG files. Please enter or pick a file name with a .jpg or .png file extension." );
		return false;
	}

	CRenderer Renderer( CRenderer::WINDOWS_GDI );
	CBitmap MemoryBitmap;

	int Width = (int)( 1920 * ResolutionAdjustmentFactor );
	double Temp = 1920 * RenderHeight / RenderWidth;
	int Height = (int)( Temp * ResolutionAdjustmentFactor );

	CRect ImageRect( 0, 0, Width, Height );

	CDC *pDC = GetDC();

	double ScaleFactor = 4.0;

	//m_Zoom = m_ScreenZoom * m_DPIScale;
	double SaveDPIScale = m_DPIScale;
	m_DPIScale = 1.0;

	PrepareRenderer( Renderer, &ImageRect, &MemoryBitmap, pDC, ScaleFactor, true, MarginScale, 1.0, false, true, false, 0 );

	DoDraw( &Renderer );

	Renderer.EndDraw();

	m_DPIScale = SaveDPIScale;

	CDC ShrunkDC;
	CBitmap ShrunkBitmap;

	int FinalWidth = RenderWidth;
	int FinalHeight = RenderHeight;

	ShrunkDC.CreateCompatibleDC( pDC );
	ShrunkBitmap.CreateCompatibleBitmap( pDC, FinalWidth, FinalHeight );
	ShrunkDC.SelectObject( &ShrunkBitmap );
	ShrunkDC.PatBlt( 0, 0, FinalWidth, FinalHeight, WHITENESS );

	StretchBltPlus( ShrunkDC.GetSafeHdc(),
		            0, 0, FinalWidth, FinalHeight,
		            Renderer.GetSafeHdc(),
					0, 0, (int)( Width * ScaleFactor ), (int)( Height * ScaleFactor ),
					SRCCOPY );

	Gdiplus::Bitmap *pSaveBitmap = ::new Gdiplus::Bitmap( (HBITMAP)ShrunkBitmap.GetSafeHandle(), (HPALETTE)0 );
	EncoderParameters JPEGParams;
	JPEGParams.Count = 1;
	JPEGParams.Parameter[0].NumberOfValues = 1;
	JPEGParams.Parameter[0].Guid  = EncoderQuality;
	JPEGParams.Parameter[0].Type  = EncoderParameterValueTypeLong;
	ULONG Quality = 85;
	JPEGParams.Parameter[0].Value = &Quality;

	CLSID imageCLSID;
	if( !GetEncoderClsid( bJPEG ? L"image/png" : L"image/png", &imageCLSID ) )
	{
		AfxMessageBox( "There is an internal error with the encoder for your image file." );
		return false;
	}

	wchar_t *wFileName = ToUnicode( pFileName );
	bool bResult = pSaveBitmap->Save( wFileName, &imageCLSID, bJPEG ? &JPEGParams : 0 ) == Gdiplus::Ok;

	ReleaseDC( pDC );

	::delete pSaveBitmap;

	InvalidateRect( 0 );

	return bResult;
}

bool CLinkageView::SaveAsDXF( const char *pFileName )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	double UnitScale = pDoc->GetUnitScale();
	CFRect DocumentRect;

	DocumentRect = GetDocumentArea();

	double ConnectorSize = max( fabs( DocumentRect.Height() ), fabs( DocumentRect.Width() ) ) / 750;
	ConnectorSize *= pDoc->GetUnitScale();

	CRenderer Renderer( CRenderer::DXF_FILE );

	int Width = (int)( DocumentRect.Width() * pDoc->GetUnitScale() ) + 2;
	int Height = (int)( DocumentRect.Height() * pDoc->GetUnitScale() ) + 2;

	CRect ImageRect( 0, 0, Width, Height );

	PrepareRenderer( Renderer, &ImageRect, 0, 0, 1.0, false, 0.0, ConnectorSize, false, false, false, 0 );

	m_Zoom = pDoc->GetUnitScale();
	DoDraw( &Renderer );

	Renderer.SaveDXF( pFileName );

	Renderer.EndDraw();

	InvalidateRect( 0 );

	return true;
}

void CLinkageView::OnSelectSample (UINT ID )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CSampleGallery GalleryData;

	int Counter;
	CString ExampleData;
	CString ExampleName;
	int Count = GalleryData.GetCount();
	for( Counter = 0; Counter < Count; ++Counter )
	{
		if( ID == GalleryData.GetCommandID( Counter ) )
		{
			if( m_bSimulating )
				StopMechanismSimulate();
			pDoc->SelectSample( Counter );
			break;
		}
	}
}

void CLinkageView::UpdateForDocumentChange( bool bUpdateRotationCenter )
{
	SetScrollExtents();
	InvalidateRect( 0 );
	MarkSelection( true, bUpdateRotationCenter );
	ShowSelectedElementCoordinates();
}

void CLinkageView::SelectionChanged( void )
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if( pDoc->IsSelectionAdjustable() )
		SetStatusText( m_VisibleAdjustment == ADJUSTMENT_ROTATE ? "0.0000°" : ( m_VisibleAdjustment == ADJUSTMENT_STRETCH ? "100.000%" : "" ) );
}

#if 0

LRESULT CLinkageView::OnGesture(WPARAM wParam, LPARAM lParam)
{
    // Create a structure to populate and retrieve the extra message info.
    GESTUREINFO gi;

    ZeroMemory(&gi, sizeof(GESTUREINFO));

    gi.cbSize = sizeof(GESTUREINFO);

    BOOL bResult  = GetGestureInfo((HGESTUREINFO)lParam, &gi);
    BOOL bHandled = FALSE;

    if (bResult){
        // now interpret the gesture
        switch (gi.dwID){
           case GID_ZOOM:
               // Code for zooming goes here
               bHandled = TRUE;
               break;
           case GID_PAN:
               // Code for panning goes here
               bHandled = TRUE;
               break;
           case GID_ROTATE:
               // Code for rotation goes here
               bHandled = TRUE;
               break;
           case GID_TWOFINGERTAP:
               // Code for two-finger tap goes here
               bHandled = TRUE;
               break;
           case GID_PRESSANDTAP:
               // Code for roll over goes here
               bHandled = TRUE;
               break;
           default:
               // A gesture was not recognized
               break;
        }
    }else{
        DWORD dwErr = GetLastError();
        if (dwErr > 0){
            //MessageBoxW(hWnd, L"Error!", L"Could not retrieve a GESTUREINFO structure.", MB_OK);
        }
    }
    if (bHandled){
        return 0;
    }else{
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
  }
}

#endif


void CLinkageView::OnFileSave()
{
	CLinkageDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->DoFileSave();
	CLinkageApp *pApp = (CLinkageApp*)AfxGetApp();
	if( pApp != 0 )
		pApp->SaveStdProfileSettings();
}


void CLinkageView::OnSetFocus(CWnd* pOldWnd)
{
	CView::OnSetFocus(pOldWnd);

	// TODO: Add your message handler code here
}


void CLinkageView::OnKillFocus(CWnd* pNewWnd)
{
	CView::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here
}
