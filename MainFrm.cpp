#include "stdafx.h"
#include "Linkage.h"
#include "io.h"
#include "MainFrm.h"
#include "mymfcribbongallery.h"
#include "examples_xml.h"
#include "samplegallery.h"
#include "AboutDialog.h"
#include "LinkageDoc.h"
#include "resource.h"
#include "MyMFCRibbonCategory.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	ON_WM_CREATE()
	//ON_COMMAND_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_OFF_2007_AQUA, &CMainFrame::OnApplicationLook)
	ON_COMMAND(ID_FILE_PRINT, &CMainFrame::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CMainFrame::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CMainFrame::OnFilePrintPreview)
	ON_UPDATE_COMMAND_UI(ID_FILE_PRINT_PREVIEW, &CMainFrame::OnUpdateFilePrintPreview)
	ON_COMMAND(ID_HELP_USERGUIDE, &CMainFrame::OnHelpUserguide)
	ON_COMMAND(ID_HELP_ABOUT, &CMainFrame::OnHelpAbout)
	ON_COMMAND(ID_FILE_HELPDOC, &CMainFrame::OnHelpUserguide)
	ON_COMMAND(ID_FILE_HELPABOUT, &CMainFrame::OnHelpAbout)
	ON_COMMAND(ID_RIBBON_SAMPLE_GALLERY, &CMainFrame::OnSelectSample)
	ON_WM_SETCURSOR()
	ON_WM_DROPFILES()
	ON_WM_TIMER()
	ON_WM_CLOSE()
END_MESSAGE_MAP()

// CMainFrame construction/destruction

class CValidatedString : public CString
{
	public: bool LoadString( UINT nID )
	{
		BOOL bNameValid = CString::LoadString( nID );
		ASSERT( bNameValid );
		return bNameValid != 0;
	}

	public: const CString& operator =( const CString& stringSrc)
	{
		return CString::operator=( stringSrc );
	}
};

CMainFrame::CMainFrame()
{
	m_pSampleGallery = 0;
	m_RibbonInitializingCounter = 0;

	// TODO: add member initialization code here
	//theApp.m_nAppLook = theApp.GetInt(_T("ApplicationLook"), ID_VIEW_APPLOOK_OFF_2007_SILVER);
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	DragAcceptFiles( TRUE );

	// set the visual manager and style based on persisted value
	OnApplicationLook( theApp.m_nAppLook );

	m_wndRibbonBar.Create(this);

	// if I provide the right sizes, will this work to stop the built-in scaling and use the images as I provide them?...
	// GetGlobalData()->EnableRibbonImageScale(FALSE);

	InitializeRibbon();

	HACCEL ha = this->GetDefaultAccelerator();

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	EnableDocking( CBRS_ALIGN_ANY );

	m_wndStatusBar.AddElement( new CMFCRibbonStatusBarPane( ID_STATUSBAR_PANE1, "", TRUE, 0, "This is a sample of the type on this machine. This is filler to ensure that the pane is large enough for my text!" ), "" );

	// enable Visual Studio 2005 style docking window behavior
	CDockingManager::SetDockingMode(DT_SMART);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWndEx::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

static CMFCRibbonPanel* AddPanel( CMFCRibbonCategory *pCategory, int NameID, HICON hIcon = (HICON)0 )
{
	CValidatedString strTemp;
	if( NameID > 0 )
		strTemp.LoadString( NameID );
	strTemp.Remove( '&' );
	return pCategory->AddPanel( strTemp, hIcon );
}

enum _ButtonDisplayMode { LARGE, LARGEONLY, SMALL, TEXT };

// Getting a number that looks good with a 1.0 DPI scale and a 2.5 DPI scale. Not sure how others look.
#define AFX_CHECK_BOX_DEFAULT_SIZE ( (int)( 16.0 * ( 1 + ( ( GetGlobalData()->GetRibbonImageScale() - 1 ) * 0.7 ) ) ) )
const int nTextMarginLeft = 4;
const int nTextMarginRight = 6;

class CMyMFCRibbonCheckBox : public CMFCRibbonCheckBox
{
	public:
	CMyMFCRibbonCheckBox(UINT nID, LPCTSTR lpszText) : CMFCRibbonCheckBox(nID, lpszText)
	{
	}
	
	CSize GetIntermediateSize(CDC* /*pDC*/)
	{
		ASSERT_VALID(this);
		m_szMargin = CSize( 2, 3);

		const CSize sizeCheckBox = CSize(AFX_CHECK_BOX_DEFAULT_SIZE, AFX_CHECK_BOX_DEFAULT_SIZE);

		int cx = sizeCheckBox.cx + m_sizeTextRight.cx + nTextMarginLeft + nTextMarginRight + m_szMargin.cx;
		int cy = max(sizeCheckBox.cy, m_sizeTextRight.cy) + 2 * m_szMargin.cy;

		return CSize(cx, cy);
	}
	
	void OnDraw(CDC* pDC)
	{
		ASSERT_VALID(this);
		ASSERT_VALID(pDC);

		if (m_rect.IsRectEmpty())
		{
			return;
		}

		const CSize sizeCheckBox = CSize(AFX_CHECK_BOX_DEFAULT_SIZE, AFX_CHECK_BOX_DEFAULT_SIZE);

		// Draw check box:
		CRect rectCheck = m_rect;
		rectCheck.top += (int)( GetGlobalData()->GetRibbonImageScale() );
		rectCheck.bottom += (int)( GetGlobalData()->GetRibbonImageScale() );
		rectCheck.DeflateRect(m_szMargin);
		//rectCheck.left++;
		rectCheck.right = rectCheck.left + sizeCheckBox.cx;
		rectCheck.top = rectCheck.CenterPoint().y - sizeCheckBox.cx / 2;

		rectCheck.bottom = rectCheck.top + sizeCheckBox.cy;

		const BOOL bIsHighlighted = (IsHighlighted() || IsFocused()) && !IsDisabled();
		int nState = 0;

		if (m_bIsChecked == 2)
		{
			nState = 2;
		}
		else if (IsChecked() || (IsPressed() && bIsHighlighted))
		{
			nState = 1;
		}

		CMFCVisualManager::GetInstance()->OnDrawCheckBoxEx(pDC, rectCheck, nState,
			bIsHighlighted, IsPressed() && bIsHighlighted, !IsDisabled());

		// Draw text:
		COLORREF clrTextOld = (COLORREF)-1;

		if (m_bIsDisabled)
		{
			if (m_bQuickAccessMode)
			{
				clrTextOld = pDC->SetTextColor(CMFCVisualManager::GetInstance()->GetRibbonQuickAccessToolBarTextColor(TRUE));
			}
			else
			{
				clrTextOld = pDC->SetTextColor(CMFCVisualManager::GetInstance()->GetToolbarDisabledTextColor());
			}
		}

		CRect rectText = m_rect;
		rectText.left = rectCheck.right + nTextMarginLeft;

		DrawRibbonText(pDC, m_strText, rectText, DT_SINGLELINE | DT_VCENTER);

		if (clrTextOld != (COLORREF)-1)
		{
			pDC->SetTextColor(clrTextOld);
		}

		if (IsFocused())
		{
			CRect rectFocus = rectText;
			rectFocus.OffsetRect(-nTextMarginLeft / 2, 0);
			rectFocus.DeflateRect(0, 2);

			pDC->DrawFocusRect(rectFocus);
		}
	}
};

static void AddRibbonButton( CMFCRibbonPanel *pPanel, int NameID, UINT CommandID, int ImageOffset = -1, enum _ButtonDisplayMode Mode = TEXT )
{
	CValidatedString strTemp;
	if( NameID > 0 )
		strTemp.LoadString( NameID );
	CMFCRibbonButton *pTemp = new CMFCRibbonButton( CommandID, strTemp, Mode == TEXT || Mode == LARGEONLY ? -1 : ImageOffset, Mode == TEXT || Mode == SMALL ? -1 : ImageOffset);
	pPanel->Add( pTemp );
}

static void AddRibbonText( CMFCRibbonPanel *pPanel, int NameID )
{
	CValidatedString strTemp;
	if( NameID > 0 )
		strTemp.LoadString( NameID );
	pPanel->Add( new CMFCRibbonLabel( strTemp ) );
}

static void AddRibbonSlider( CMFCRibbonPanel *pPanel, int NameID, UINT CommandID )
{
	CValidatedString strTemp;
	if( NameID > 0 )
		strTemp.LoadString( NameID );
	CMFCRibbonSlider *pSlider = new CMFCRibbonSlider( CommandID );
	pSlider->SetRange( 0, 10 );
	pSlider->SetZoomButtons( TRUE );
	pSlider->SetZoomIncrement( 1 );
	pSlider->SetDescription( strTemp );
	pPanel->Add( pSlider );
}

static void AddRibbonCheckbox( CMFCRibbonPanel *pPanel, int NameID, UINT CommandID )
{
	CString strTemp;
	if( NameID > 0 )
		strTemp.LoadString( NameID );
	pPanel->Add( new CMyMFCRibbonCheckBox( CommandID, strTemp ) );
}

static CMFCRibbonComboBox *AddRibbonCombobox( CMFCRibbonPanel *pPanel, int NameID, UINT CommandID, int Image )
{
	CString strTemp;
	if( NameID > 0 )
		strTemp.LoadString( NameID );
	CMFCRibbonComboBox *pNewCombobox = new CMFCRibbonComboBox( CommandID, 0, -1, strTemp, Image );
	pNewCombobox->EnableDropDownListResize( FALSE );
	pNewCombobox->SetWidth( 90 );
	pPanel->Add( pNewCombobox );
	return pNewCombobox;
}

/*static CMFCRibbonGallery* AddRibbonSampleGallery( CMFCRibbonCategory* pCategory, HICON hIcon = 0 )
{
	CMFCRibbonPanel* pPanelSamples = AddPanel( pCategory, IDS_RIBBON_OPENSAMPLE, hIcon );

	CString strTemp;
	strTemp.LoadString( IDS_RIBBON_SAMPLE_GALLERY );
	CMFCRibbonGallery *pSampleGallery = new CMFCRibbonGallery( ID_RIBBON_SAMPLE_GALLERY, strTemp, -1, 41, IDB_SAMPLE_GALLERY, 96 );
	pSampleGallery->SetIconsInRow( 6 );
	pSampleGallery->SetButtonMode();
	pSampleGallery->SelectElement( -1 );
	pPanelSamples->Add( pSampleGallery );

	CSampleGallery GalleryData;

	int Count = GalleryData.GetCount();
	for( int Counter = 0; Counter < Count; ++Counter )
	{
		CValidatedString strTemp;
		strTemp.LoadString( GalleryData.GetStringID( Counter ) );
		int EOL = strTemp.Find( '\n' );
		if( EOL >= 0 )
			strTemp.Truncate( EOL );
		pSampleGallery->SetItemToolTip( Counter, strTemp );
	}

	return pSampleGallery;
}*/

static void AddPrintButton( CMFCRibbonPanel *pPanel )
{
	CValidatedString strTemp;
	strTemp.LoadString(IDS_RIBBON_PRINT);
	CMFCRibbonButton* pBtnPrint = new CMFCRibbonButton(ID_FILE_PRINT, strTemp, 6, 6);
	pBtnPrint->SetKeys(_T("p"), _T("w"));
	strTemp.LoadString(IDS_RIBBON_PRINT_LABEL);
	pBtnPrint->AddSubItem(new CMFCRibbonLabel(strTemp));
	strTemp.LoadString(IDS_RIBBON_PRINT_QUICK);
	pBtnPrint->AddSubItem(new CMFCRibbonButton(ID_FILE_PRINT_DIRECT, strTemp, 7, 7, TRUE));
	strTemp.LoadString(IDS_RIBBON_PRINT_PREVIEW);
	pBtnPrint->AddSubItem(new CMFCRibbonButton(ID_FILE_PRINT_PREVIEW, strTemp, 8, 8, TRUE));
	strTemp.LoadString(IDS_RIBBON_PRINT_SETUP);
	pBtnPrint->AddSubItem(new CMFCRibbonButton(ID_FILE_PRINT_SETUP, strTemp, 11, 11, TRUE));
	pPanel->Add(pBtnPrint);
}

static void AddExportButton( CMFCRibbonPanel *pPanel )
{
	CValidatedString strTemp;
	strTemp.LoadString(IDS_RIBBON_EXPORT);

	static CMenu Dummy;

	CMFCRibbonButton* pButtonExport = new CMFCRibbonButton( 0, strTemp, 76, 76 );

	Dummy.CreatePopupMenu();
	pButtonExport->SetMenu( Dummy.GetSafeHmenu() );

	//AddRibbonButton( pMainPanel, IDS_RIBBON_SAVEVIDEO, ID_FILE_SAVEVIDEO, 5, LARGE );
	//AddRibbonButton( pMainPanel, IDS_RIBBON_SAVEIMAGE, ID_FILE_SAVEIMAGE, 37, LARGE );

	strTemp.LoadString(IDS_RIBBON_EXPORT_LABEL);
	pButtonExport->AddSubItem(new CMFCRibbonLabel(strTemp));

	strTemp.LoadString(IDS_RIBBON_SAVEVIDEO);
	pButtonExport->AddSubItem(new CMFCRibbonButton(ID_FILE_SAVEVIDEO, strTemp, 5, 5, TRUE));
	strTemp.LoadString(IDS_RIBBON_SAVEIMAGE);
	pButtonExport->AddSubItem(new CMFCRibbonButton(ID_FILE_SAVEIMAGE, strTemp, 37, 37, TRUE));
	strTemp.LoadString(IDS_RIBBON_SAVEDXF);
	pButtonExport->AddSubItem(new CMFCRibbonButton(ID_FILE_SAVEDXF, strTemp, 77, 77, TRUE));
	strTemp.LoadString(IDS_RIBBON_SAVEMOTION);
	pButtonExport->AddSubItem(new CMFCRibbonButton( ID_FILE_SAVEMOTION, strTemp, 83, 83, TRUE));
	pPanel->Add(pButtonExport);
}

//void CMainFrame::CreateSamplePanel( CMFCRibbonCategory* pCategory )
//{
//	m_pSampleGallery = AddRibbonSampleGallery( pCategory, m_PanelImages.ExtractIcon(29) );
//}

void CMainFrame::CreateClipboardPanel( CMFCRibbonCategory* pCategory )
{
	CMFCRibbonPanel* pPanelClipboard = AddPanel( pCategory, IDS_RIBBON_CLIPBOARD, m_PanelImages.ExtractIcon(30) );

	AddRibbonButton( pPanelClipboard, IDS_RIBBON_PASTE, ID_EDIT_PASTE, 14, LARGE );
	AddRibbonButton( pPanelClipboard, IDS_RIBBON_CUT, ID_EDIT_CUT, 21, SMALL );
	AddRibbonButton( pPanelClipboard, IDS_RIBBON_COPY, ID_EDIT_COPY, 22, SMALL );
	AddRibbonButton( pPanelClipboard, IDS_RIBBON_SELECTALL, ID_EDIT_SELECT_ALL, 44, SMALL );
	AddRibbonButton( pPanelClipboard, IDS_RIBBON_SELECTELEMENTS, ID_EDIT_SELECT_ELEMENTS, 90, LARGE );
}

void CMainFrame::CreateViewPanel( CMFCRibbonCategory* pCategory )
{
	CMFCRibbonPanel* pPanelView = AddPanel( pCategory, IDS_RIBBON_VIEW, m_PanelImages.ExtractIcon(31) );
	AddRibbonButton( pPanelView, IDS_RIBBON_ZOOMFIT, ID_VIEW_ZOOMFIT, 20, SMALL );
	AddRibbonButton( pPanelView, IDS_RIBBON_VIEW_ZOOMIN, ID_VIEW_ZOOMIN, 16, SMALL );
	AddRibbonButton( pPanelView, IDS_RIBBON_VIEW_ZOOMOUT, ID_VIEW_ZOOMOUT, 17, SMALL );

	AddRibbonCheckbox( pPanelView, IDS_RIBBON_VIEWDRAWING, ID_VIEW_DRAWING );
	AddRibbonCheckbox( pPanelView, IDS_RIBBON_VIEWMECHANISM, ID_VIEW_MECHANISM );

	//AddRibbonCheckbox( pPanelView, IDS_RIBBON_LABELS, ID_VIEW_LABELS );
	//AddRibbonCheckbox( pPanelView, IDS_RIBBON_ANGLES, ID_VIEW_ANGLES );
	//AddRibbonCheckbox( pPanelView, IDS_RIBBON_VIEW_ANICROP, ID_VIEW_ANICROP );
	//AddRibbonCheckbox( pPanelView, IDS_RIBBON_DIMENSIONS, ID_VIEW_DIMENSIONS );

	CMFCRibbonButton* pDetailsButton = new CMFCRibbonButton( ID_ALIGN_DETAILSBUTTON, "Details", 69 );

	AppendMenuItem( pDetailsButton, IDS_RIBBON_LABELS, ID_VIEW_LABELS, 10 );
	//AppendMenuItem( pDetailsButton, IDS_RIBBON_ANGLES, ID_VIEW_ANGLES, 12 );
	AppendMenuItem( pDetailsButton, IDS_RIBBON_VIEW_ANICROP, ID_VIEW_ANICROP, 13 );
	AppendMenuItem( pDetailsButton, IDS_RIBBON_DIMENSIONS, ID_VIEW_DIMENSIONS, 15 );
	//AppendMenuItem( pDetailsButton, IDS_RIBBON_GROUNDDIMENSIONS, ID_VIEW_GROUNDDIMENSIONS, 88 );
	//AppendMenuItem( pDetailsButton, IDS_RIBBON_DRAWINGLAYERDIMENSIONS, ID_VIEW_DRAWINGLAYERDIMENSIONS, 88 );
	//AppendMenuItem( pDetailsButton, IDS_RIBBON_USEDIAMETER, ID_VIEW_USEDIAMETER, 108 );
	//AppendMenuItem( pDetailsButton, IDS_RIBBON_VIEW_LARGEFONT, ID_VIEW_LARGEFONT, 78 );
	//AppendMenuItem( pDetailsButton, IDS_RIBBON_SOLIDLINKS, ID_VIEW_SOLIDLINKS, 75 );
	AppendMenuItem( pDetailsButton, IDS_RIBBON_VIEW_AUTOGRID, ID_VIEW_SHOWGRID, 85 );
	AppendMenuItem( pDetailsButton, IDS_RIBBON_VIEW_USERGRID, ID_VIEW_EDITGRID, 97 );
	//AppendMenuItem( pDetailsButton, IDS_RIBBON_MOMENTUM, ID_EDIT_MOMENTUM, 94 );
	//AppendMenuItem( pDetailsButton, IDS_RIBBON_VIEW_DEBUG, ID_VIEW_DEBUG, 81 );

	// The menu is only needed in order to make the button work properly.
	// Without the menu, clicking on the icon in the button will not display
	// the menu but clicking the text will.
	static CMenu Menu;
	Menu.CreatePopupMenu();
	pDetailsButton->SetMenu( Menu.GetSafeHmenu() );

	pPanelView->Add( pDetailsButton );

	AddRibbonButton( pPanelView, IDS_RIBBON_VIEW_PARTS, ID_VIEW_PARTS, 86, LARGE );
}

void CMainFrame::CreateDimensionsPanel( CMFCRibbonCategory* pCategory )
{
	CMFCRibbonPanel* pPanelDimensions = AddPanel( pCategory, IDS_RIBBON_DIM, m_PanelImages.ExtractIcon(32) );
	CMFCRibbonComboBox *pUnitsBox = AddRibbonCombobox( pPanelDimensions, 0, ID_VIEW_UNITS, 52 );
	if( pUnitsBox != 0 )
	{
		pUnitsBox->AddItem( CLinkageDoc::GetUnitsString( CLinkageDoc::MM ), CLinkageDoc::MM );
		pUnitsBox->AddItem( CLinkageDoc::GetUnitsString( CLinkageDoc::INCH ), CLinkageDoc::INCH );
	}

	// Useful number? ... int Temp = pUnitsBox->GetWidth();

	CMFCRibbonEdit *pEdit = new CMFCRibbonEdit( ID_VIEW_COORDINATES, 96, 0, 53 );
	pEdit->SetWidth( 104 );
	pPanelDimensions->Add( pEdit );

	CMFCRibbonLabel *pLabel = new CMFCRibbonLabel( "     " );
	pLabel->SetID( ID_VIEW_DIMENSIONSLABEL );
	pPanelDimensions->Add( pLabel );

	//AddRibbonButton( pPanelDimensions, IDS_RIBBON_SET_RATIO, ID_EDIT_SET_RATIO, 79, SMALL );
	
		//CMFCRibbonButton* pSetButton = new CMFCRibbonButton( ID_DIMENSION_SET, "Edit", 53 );


	//AppendMenuItem( pSetButton, IDS_DIMENSION_SETLOCATION, ID_DIMENSION_SETLOCATION, 79 );
	//AppendMenuItem( pSetButton, IDS_DIMENSION_SETLENGTH, ID_DIMENSION_SETLENGTH, 79 );
	//AppendMenuItem( pSetButton, IDS_DIMENSION_ANGLE, ID_DIMENSION_ANGLE, 79 );
	//AppendMenuItem( pSetButton, IDS_RIBBON_SET_RATIO, ID_EDIT_SET_RATIO, 79 );
	//AppendMenuItem( pSetButton, IDS_DIMENSION_ROTATE, ID_DIMENSION_ROTATE, 79 );
	//AppendMenuItem( pSetButton, IDS_DIMENSION_SCALE, ID_DIMENSION_SCALE, 79 );








	// The menu is only needed in order to make the button work properly.
	// Without the menu, clicking on the icon in the button will not display
	// the menu but clicking the text will.
	//static CMenu Menu;
	//Menu.CreatePopupMenu();
	//pSetButton->SetMenu( Menu.GetSafeHmenu() );

	//pPanelDimensions->Add( pSetButton );




	//AddRibbonButton( pPanelDimensions, IDS_RIBBON_SET_RATIO, ID_EDIT_SET_RATIO, 79, SMALL );
}

void CMainFrame::CreatePrintPanel( CMFCRibbonCategory* pCategory )
{
	CMFCRibbonPanel* pPanePrint = AddPanel( pCategory, IDS_RIBBON_PRINT, m_PanelImages.ExtractIcon(33) );

	AddRibbonButton( pPanePrint, IDS_RIBBON_PRINT, ID_FILE_PRINT, 6, LARGE );
	AddRibbonButton( pPanePrint, IDS_RIBBON_PRINT_QUICK, ID_FILE_PRINT_DIRECT, 7, LARGE );
	AddRibbonButton( pPanePrint, IDS_RIBBON_PRINT_PREVIEW, ID_FILE_PRINT_PREVIEW, 8, LARGE );
}

void CMainFrame::CreateHelpPanel( CMFCRibbonCategory* pCategory )
{
	CMFCRibbonPanel* pPanelHelp = AddPanel( pCategory, IDS_RIBBON_HELP, m_PanelImages.ExtractIcon(33) );

	AddRibbonButton( pPanelHelp, IDS_RIBBON_HELPABOUT, ID_FILE_HELPABOUT, 91, LARGE );
	AddRibbonButton( pPanelHelp, IDS_RIBBON_HELPDOC, ID_FILE_HELPDOC, 92, LARGE );
}

void CMainFrame::CreateBackgroundPanel( CMFCRibbonCategory* pCategory )
{
	CMFCRibbonPanel* pPanelBackground = AddPanel( pCategory, IDS_RIBBON_IMAGE, m_PanelImages.ExtractIcon(33) );

	AddRibbonButton( pPanelBackground, IDS_RIBBON_OPEN, ID_BACKGROUND_OPEN, 1, LARGE );
	AddRibbonButton( pPanelBackground, IDS_RIBBON_BACKGROUND_DELETE, ID_BACKGROUND_DELETE, 95, LARGE );
}

void CMainFrame::CreateOptionsPanel( CMFCRibbonCategory* pCategory )
{
	CMFCRibbonPanel* pPanelOptions = AddPanel( pCategory, IDS_RIBBON_OPTIONS, m_PanelImages.ExtractIcon(33) );

	//AddRibbonCheckbox( pPanelOptions, IDS_RIBBON_LABELS, ID_VIEW_LABELS );
	AddRibbonCheckbox( pPanelOptions, IDS_RIBBON_ANGLES, ID_VIEW_ANGLES );
	//AddRibbonCheckbox( pPanelOptions, IDS_RIBBON_VIEW_ANICROP, ID_VIEW_ANICROP );
	//AddRibbonCheckbox( pPanelOptions, IDS_RIBBON_DIMENSIONS, ID_VIEW_DIMENSIONS );
	AddRibbonCheckbox( pPanelOptions, IDS_RIBBON_GROUNDDIMENSIONS, ID_VIEW_GROUNDDIMENSIONS );
	AddRibbonCheckbox( pPanelOptions, IDS_RIBBON_DRAWINGLAYERDIMENSIONS, ID_VIEW_DRAWINGLAYERDIMENSIONS );
	AddRibbonCheckbox( pPanelOptions, IDS_RIBBON_USEDIAMETER, ID_VIEW_USEDIAMETER );
	AddRibbonCheckbox( pPanelOptions, IDS_RIBBON_VIEW_LARGEFONT, ID_VIEW_LARGEFONT );
	AddRibbonCheckbox( pPanelOptions, IDS_RIBBON_SOLIDLINKS, ID_VIEW_SOLIDLINKS );
	//AddRibbonCheckbox( pPanelOptions, IDS_RIBBON_VIEW_AUTOGRID, ID_VIEW_SHOWGRID );
	//AddRibbonCheckbox( pPanelOptions, IDS_RIBBON_VIEW_USERGRID, ID_VIEW_EDITGRID );
	//AddRibbonCheckbox( pPanelOptions, IDS_RIBBON_VIEW_PARTS, ID_VIEW_PARTS );
	AddRibbonCheckbox( pPanelOptions, IDS_RIBBON_MOMENTUM, ID_EDIT_MOMENTUM );
	AddRibbonCheckbox( pPanelOptions, IDS_RIBBON_VIEW_DEBUG, ID_VIEW_DEBUG );
}

void CMainFrame::CreateBackgroundViewPanel( CMFCRibbonCategory* pCategory )
{
	CMFCRibbonPanel* pPanelView = AddPanel( pCategory, IDS_RIBBON_APPEARANCE, m_PanelImages.ExtractIcon(33) );
	AddRibbonText( pPanelView, IDS_RIBBON_TRANSPARENCY );
	AddRibbonSlider( pPanelView, 0, ID_BACKGROUND_TRANSPARENCY );
}

void CMainFrame::CreateSettingsPanel( CMFCRibbonCategory* pCategory )
{
	CMFCRibbonPanel* pPaneSettings = AddPanel( pCategory, IDS_RIBBON_SETTINGS, m_PanelImages.ExtractIcon(33) );

//	AddRibbonCheckbox( pPaneSettings, IDS_RIBBON_LABELS, ID_VIEW_LABELS );
//	AddRibbonCheckbox( pPaneSettings, IDS_RIBBON_ANGLES, ID_VIEW_ANGLES );
//	AddRibbonCheckbox( pPaneSettings, IDS_RIBBON_VIEW_ANICROP, ID_VIEW_ANICROP );
//	AddRibbonCheckbox( pPaneSettings, IDS_RIBBON_DIMENSIONS, ID_VIEW_DIMENSIONS );
	AddRibbonCheckbox( pPaneSettings, IDS_RIBBON_SOLIDLINKS, ID_VIEW_SOLIDLINKS );
}

void CMainFrame::CreatePrintOptionsPanel( CMFCRibbonCategory* pCategory )
{
	CMFCRibbonPanel* pPanePrintOptions = AddPanel( pCategory, IDS_RIBBON_PRINTOPTIONS, m_PanelImages.ExtractIcon(33) );

	AddRibbonButton( pPanePrintOptions, IDS_RIBBON_PRINT_SETUP, ID_FILE_PRINT_SETUP, 11, LARGE );
	AddRibbonCheckbox( pPanePrintOptions, IDS_RIBBON_PRINTFULL, ID_FILE_PRINTFULL );
}

void CMainFrame::CreateInsertPanel( CMFCRibbonCategory* pCategory )
{
	CMFCRibbonPanel* pPaneInsert = AddPanel( pCategory, IDS_RIBBON_INSERT, m_PanelImages.ExtractIcon(33) );

	CValidatedString strTemp;
	strTemp.LoadString( IDS_RIBBON_INSERTBUTTON );

	CMFCRibbonButton* pInsertButton = new CMFCRibbonButton( ID_EDIT_INSERTBUTTON, strTemp, 28, 28 );

	AppendMenuItem( pInsertButton, IDS_RIBBON_INSERT_CONNECTOR, ID_INSERT_CONNECTOR, 29 );
	AppendMenuItem( pInsertButton, IDS_RIBBON_INSERT_ANCHOR, ID_INSERT_ANCHOR, 89 );
	AppendMenuItem( pInsertButton, IDS_RIBBON_INSERT_LINK2, ID_INSERT_LINK2, 28 );
	AppendMenuItem( pInsertButton, IDS_RIBBON_INSERT_ANCHORLINK, ID_INSERT_ANCHORLINK, 27 );
	AppendMenuItem( pInsertButton, IDS_RIBBON_INSERT_INPUT, ID_INSERT_INPUT, 26 );
	AppendMenuItem( pInsertButton, IDS_RIBBON_INSERT_ACTUATOR, ID_INSERT_ACTUATOR, 49 );
	AppendMenuItem( pInsertButton, IDS_RIBBON_INSERT_LINK3, ID_INSERT_LINK3, 31 );
	AppendMenuItem( pInsertButton, IDS_RIBBON_INSERT_LINK4, ID_INSERT_LINK4, 43 );

	AppendMenuItem( pInsertButton, IDS_RIBBON_INSERT_POINT, ID_INSERT_POINT, 42 );
	AppendMenuItem( pInsertButton, IDS_RIBBON_INSERT_LINE, ID_INSERT_LINE, 45 );
	AppendMenuItem( pInsertButton, IDS_RIBBON_INSERTMEASUREMENT, ID_INSERT_MEASUREMENT, 74 );
	AppendMenuItem( pInsertButton, IDS_RIBBON_INSERTGEAR, ID_INSERT_GEAR, 82 );

	// The menu is only needed in order to make the button work properly.
	// Without the menu, clicking on the icon in the button will not display
	// the menu but clicking tyhe text will.
	static CMenu Menu;
	Menu.CreatePopupMenu();
	pInsertButton->SetMenu( Menu.GetSafeHmenu() );

	pInsertButton->SetDescription( "" );

	pPaneInsert->Add( pInsertButton );
}

void CMainFrame::CreateElementPanel( CMFCRibbonCategory* pCategory )
{
	CMFCRibbonPanel* pPaneAction = AddPanel( pCategory, IDS_RIBBON_ACTION, m_PanelImages.ExtractIcon(34) );
	AddRibbonButton( pPaneAction, IDS_RIBBON_ADDCONNECTOR, ID_EDIT_ADDCONNECTOR, 73, SMALL );
	AddRibbonButton( pPaneAction, IDS_RIBBON_CONNECT, ID_EDIT_CONNECT, 32, SMALL );
	AddRibbonButton( pPaneAction, IDS_RIBBON_JOIN, ID_EDIT_JOIN, 34, SMALL );
	AddRibbonButton( pPaneAction, IDS_RIBBON_COMBINE, ID_EDIT_COMBINE, 35, SMALL );
	AddRibbonButton( pPaneAction, IDS_RIBBON_SLIDE, ID_EDIT_SLIDE, 36, SMALL );
	AddRibbonButton( pPaneAction, IDS_RIBBON_SPLIT, ID_EDIT_SPLIT, 38, SMALL );
	AddRibbonButton( pPaneAction, IDS_RIBBON_FASTEN, ID_EDIT_FASTEN, 33, SMALL );
	AddRibbonButton( pPaneAction, IDS_RIBBON_UNFASTEN, ID_EDIT_UNFASTEN, 80, SMALL );
	AddRibbonButton( pPaneAction, IDS_RIBBON_LOCK, ID_EDIT_LOCK, 84, SMALL );
	AddRibbonButton( pPaneAction, IDS_RIBBON_PROPERTIES_PROPERTIES, ID_PROPERTIES_PROPERTIES, 4, LARGE );
}

void CMainFrame::AppendMenuItem( CMenu &Menu, UINT StringID, UINT ID, int BitmapIndex )
{
	CValidatedString strTemp;
	strTemp.LoadStringA( StringID );
	Menu.AppendMenu( MF_STRING, ID, strTemp );
}

void CMainFrame::AppendMenuItem( CMFCRibbonButton *pButton, UINT StringID, UINT ID, int BitmapIndex )
{
	CValidatedString strTemp;
	strTemp.LoadString( StringID );

	pButton->AddSubItem( new CMFCRibbonButton( ID, strTemp, BitmapIndex ) );
}

void CMainFrame::CreateAlignPanel( CMFCRibbonCategory* pCategory )
{
	CMFCRibbonPanel* pPaneAlign = AddPanel( pCategory, IDS_RIBBON_ALIGN, m_PanelImages.ExtractIcon(35) );

	AddRibbonButton( pPaneAlign, IDS_RIBBON_UNDO, ID_EDIT_UNDO, 19, LARGE );
	AddRibbonButton( pPaneAlign, IDS_RIBBON_REDO, ID_EDIT_REDO, 107, LARGE );

	AddRibbonCheckbox( pPaneAlign, IDS_RIBBON_EDITDRAWING, ID_EDIT_DRAWING );
	AddRibbonCheckbox( pPaneAlign, IDS_RIBBON_EDITMECHANISM, ID_EDIT_MECHANISM );

	CMFCRibbonButton* pSnapButton = new CMFCRibbonButton( ID_ALIGN_SNAPBUTTON, "Snap   ", 68 );

	AppendMenuItem( pSnapButton, IDS_RIBBON_SNAP, ID_EDIT_SNAP, 47 );
	AppendMenuItem( pSnapButton, IDS_RIBBON_GRIDSNAP, ID_EDIT_GRIDSNAP, 46 );
	AppendMenuItem( pSnapButton, IDS_RIBBON_AUTOJOIN, ID_EDIT_AUTOJOIN, 48 );

	// The menu is only needed in order to make the button work properly.
	// Without the menu, clicking on the icon in the button will not display
	// the menu but clicking the text will.
	static CMenu Menu;
	Menu.CreatePopupMenu();
	pSnapButton->SetMenu( Menu.GetSafeHmenu() );

	pPaneAlign->Add( pSnapButton );

	CValidatedString strTemp;
	strTemp.LoadString( IDS_RIBBON_ALIGNBUTTON );

	CMFCRibbonButton* pAlignButton = new CMFCRibbonButton( ID_ALIGN_ALIGNBUTTON, strTemp, 58, 58 );

	AppendMenuItem( pAlignButton, IDS_DIMENSION_SETLOCATION, ID_DIMENSION_SETLOCATION, 103 );
	AppendMenuItem( pAlignButton, IDS_DIMENSION_SETLENGTH, ID_DIMENSION_SETLENGTH, 102 );
	AppendMenuItem( pAlignButton, IDS_RIBBON_SETANGLE, ID_ALIGN_SETANGLE, 66 );
	AppendMenuItem( pAlignButton, IDS_RIBBON_RIGHTANGLE, ID_ALIGN_RIGHTANGLE, 54 );
	AppendMenuItem( pAlignButton, IDS_RIBBON_RECTANGLE, ID_ALIGN_RECTANGLE, 55 );
	AppendMenuItem( pAlignButton, IDS_RIBBON_PARALLELOGRAM, ID_ALIGN_PARALLELOGRAM, 56 );
	AppendMenuItem( pAlignButton, IDS_RIBBON_HORIZONTAL, ID_ALIGN_HORIZONTAL, 57 );
	AppendMenuItem( pAlignButton, IDS_RIBBON_VERTICAL, ID_ALIGN_VERTICAL, 58 );
	AppendMenuItem( pAlignButton, IDS_RIBBON_LINEUP, ID_ALIGN_LINEUP, 59 );
	AppendMenuItem( pAlignButton, IDS_RIBBON_EVENSPACE, ID_ALIGN_EVENSPACE, 65 );
	AppendMenuItem( pAlignButton, IDS_RIBBON_FLIPH, ID_ALIGN_FLIPH, 60 );
	AppendMenuItem( pAlignButton, IDS_RIBBON_FLIPV, ID_ALIGN_FLIPV, 61 );
	AppendMenuItem( pAlignButton, IDS_RIBBON_MEET, ID_ALIGN_MEET, 93 );
	AppendMenuItem( pAlignButton, IDS_ALIGN_SET_RATIO, ID_EDIT_SET_RATIO, 101 );
	AppendMenuItem( pAlignButton, IDS_DIMENSION_ROTATE, ID_DIMENSION_ROTATE, 100 );
	AppendMenuItem( pAlignButton, IDS_DIMENSION_SCALE, ID_DIMENSION_SCALE, 99 );

	// The menu is only needed in order to make the button work properly.
	// Without the menu, clicking on the icon in the button will not display
	// the menu but clicking the text will.
	static CMenu Menu2;
	Menu2.CreatePopupMenu();
	pAlignButton->SetMenu( Menu2.GetSafeHmenu() );

	pPaneAlign->Add( pAlignButton );
}


class MyCMFCRibbonButtonsGroup : public CMFCRibbonButtonsGroup
{
	public:

	MyCMFCRibbonButtonsGroup( int yOffset = 0 ) : CMFCRibbonButtonsGroup()
	{
		m_SpecialOffsetForMe = yOffset;
	}

	virtual BOOL IsAlignByColumn() const { return TRUE; }

	int m_SpecialOffsetForMe;

	virtual void OnAfterChangeRect( CDC* pDC )
	{
		/*
		 * After calling the parent class function, adjust the rectangles
		 * of the buttons to get them to display better. This is a kludge
		 * and there may be code in the ribbon classes that doesn't deal
		 * wwell with this adjustment. A parent window might not know the
		 * right location of these buttons after this change it made.
		 */

		CMFCRibbonButtonsGroup::OnAfterChangeRect( pDC );

		for (int i = 0; i < m_arButtons.GetSize(); i++)
		{
			CMFCRibbonBaseElement* pButton = m_arButtons [i];
			ASSERT_VALID(pButton);

			CRect rect = pButton->GetRect();

			if (rect.IsRectEmpty())
				continue;

			rect.OffsetRect( 0, m_SpecialOffsetForMe );

			pButton->SetRect( rect );
		}

		// The overall rect for this button group needs to get changed too.
		CRect rect;
		rect = GetRect();
		rect.OffsetRect( 0, m_SpecialOffsetForMe );
		SetRect( rect );
	}
};

void CMainFrame::CreateSimulationPanel( CMFCRibbonCategory* pCategory )
{
	CMFCRibbonPanel* pPanelMechanism = AddPanel( pCategory, IDS_RIBBON_SIMULATION, m_PanelImages.ExtractIcon(36) );
	AddRibbonButton( pPanelMechanism, IDS_RIBBON_RUN, ID_SIMULATION_RUN, 23, LARGE );
	AddRibbonButton( pPanelMechanism, IDS_RIBBON_STOP, ID_SIMULATION_STOP, 24, LARGE );
	AddRibbonButton( pPanelMechanism, IDS_RIBBON_PIN, ID_SIMULATION_PIN, 63, LARGE );

	MyCMFCRibbonButtonsGroup *pGroup = new MyCMFCRibbonButtonsGroup( 6 );
	pGroup->AddButton( new CMFCRibbonButton( ID_SIMULATION_SIMULATE, "", 25, -1 ) );
	pGroup->AddButton( new CMFCRibbonButton( ID_SIMULATE_INTERACTIVE, "", 50, -1 ) );
	pGroup->AddButton( new CMFCRibbonButton( ID_SIMULATE_MANUAL, "", 51, -1 ) );
	pPanelMechanism->Add( pGroup );

	MyCMFCRibbonButtonsGroup *pGroup2 = new MyCMFCRibbonButtonsGroup( 10 );
	pGroup2->AddButton( new CMFCRibbonButton( ID_SIMULATION_ONECYCLE, "", 104, -1 ) );
	pGroup2->AddButton( new CMFCRibbonButton( ID_SIMULATION_ONECYCLEX, "", 106, -1 ) );
	pGroup2->AddButton( new CMFCRibbonButton( ID_SIMULATION_RUNFAST, "", 109, -1 ) );
	//pGroup2->AddButton( new CMFCRibbonButton( ID_SIMULATION_PAUSE, "", 70, -1 ) );
	//pGroup2->AddButton( new CMFCRibbonButton( ID_SIMULATE_FORWARD, "", 72, -1 ) );
	pPanelMechanism->Add( pGroup2 );

	MyCMFCRibbonButtonsGroup *pGroup3 = new MyCMFCRibbonButtonsGroup( 10 );
	pGroup3->AddButton( new CMFCRibbonButton( ID_SIMULATE_BACKWARD, "", 71, -1 ) );
	pGroup3->AddButton( new CMFCRibbonButton( ID_SIMULATION_PAUSE, "", 70, -1 ) );
	pGroup3->AddButton( new CMFCRibbonButton( ID_SIMULATE_FORWARD, "", 72, -1 ) );
	pPanelMechanism->Add( pGroup3 );
}

void CMainFrame::CreateMainCategory( void )
{
	CValidatedString strTemp;

	m_wndRibbonBar.SetApplicationButton (&m_MainButton, CSize (50, 32) );

	CMFCRibbonMainPanel* pMainPanel = m_wndRibbonBar.AddMainCategory(strTemp, IDB_FILESMALL, IDB_FILELARGE, CSize( 16, 16 ), CSize( 32, 32 ) );

	AddRibbonButton( pMainPanel, IDS_RIBBON_NEW, ID_FILE_NEW, 0, LARGE );
	AddRibbonButton( pMainPanel, IDS_RIBBON_OPEN, ID_FILE_OPEN, 1, LARGE );

	strTemp.LoadString( IDS_MENU_SAMPLE_GALLERY );
	CMFCRibbonGallery *pSampleGallery = new CMFCRibbonGallery( ID_RIBBON_SAMPLE_GALLERY, strTemp, -1, 41, IDB_SAMPLE_GALLERY, 96 );
	m_pSampleGallery = pSampleGallery;

	m_pSampleGallery->SetIconsInRow( 6 );
	m_pSampleGallery->SetButtonMode();
	m_pSampleGallery->SelectItem( -1 );

	pMainPanel->Add( m_pSampleGallery );

	CSampleGallery GalleryData;
	int Count = GalleryData.GetCount();
	for( int Counter = 0; Counter < Count; ++Counter )
	{
		CValidatedString strTemp;
		strTemp.LoadString( GalleryData.GetStringID( Counter ) );
		int EOL = strTemp.Find( '\n' );
		if( EOL >= 0 )
			strTemp.Truncate( EOL );
		m_pSampleGallery->SetItemToolTip( Counter, strTemp );
	}

	AddRibbonButton( pMainPanel, IDS_RIBBON_SAVE, ID_FILE_SAVE, 2, LARGE );
	AddRibbonButton( pMainPanel, IDS_RIBBON_SAVEAS, ID_FILE_SAVE_AS, 3, LARGE );

	AddExportButton( pMainPanel );

	pMainPanel->Add(new CMFCRibbonSeparator(TRUE));
	AddPrintButton( pMainPanel );

	pMainPanel->Add(new CMFCRibbonSeparator(TRUE));

	AddRibbonButton( pMainPanel, IDS_RIBBON_CLOSE, ID_FILE_CLOSE, 9, LARGE );

	strTemp.LoadString(IDS_RIBBON_RECENT_DOCS);
	pMainPanel->AddRecentFilesList(strTemp);
}

void CMainFrame::CreateHelpButtons( void )
{
	CValidatedString strTemp;

	// ABout button.
	CMFCRibbonButton* pAboutButton = new CMFCRibbonButton( ID_HELP_ABOUT, 0, m_PanelImages.ExtractIcon(1), FALSE );
	strTemp.LoadString( ID_HELP_ABOUT_TIP );
	pAboutButton->SetToolTipText(strTemp);
	strTemp.LoadString( ID_HELP_ABOUT_DESCRIPTION );
	pAboutButton->SetDescription(strTemp);
	m_wndRibbonBar.AddToTabs( pAboutButton );

	// Help button.
	CMFCRibbonButton* pHelpButton = new CMFCRibbonButton( ID_HELP_USERGUIDE, 0, m_PanelImages.ExtractIcon(0), FALSE );
	strTemp.LoadString( ID_HELP_USERGUIDE_TIP );
	pHelpButton->SetToolTipText(strTemp);
	strTemp.LoadString( ID_HELP_USERGUIDE_DESCRIPTION );
	pHelpButton->SetDescription(strTemp);
	m_wndRibbonBar.AddToTabs( pHelpButton );
}

CMFCRibbonCategory* CMainFrame::CreateCategory( CMyMFCRibbonBar *pRibbonBar, UINT StringId )
{
	static CMyMFCRibbonCategory *pFirstCategory = 0;
	CValidatedString strTemp;
	strTemp.LoadString(StringId);
	CMFCRibbonCategory* pCat = pRibbonBar->AddCategory(strTemp, 0, 0, CSize( 0, 0 ), CSize( 0, 0 ));
	if( pCat != 0 )
	{
		CMyMFCRibbonCategory *pMainCat = (CMyMFCRibbonCategory*)m_wndRibbonBar.GetMainCategory();
		if( pMainCat != 0 )
			((CMyMFCRibbonCategory*)pCat)->SetImageResources( pMainCat );
		else if( pFirstCategory != 0 )
			((CMyMFCRibbonCategory*)pCat)->SetImageResources( pFirstCategory );
		else
		{
			((CMyMFCRibbonCategory*)pCat)->SetImageResources( IDB_FILESMALL, IDB_FILELARGE, CSize( 16, 16 ), CSize( 32, 32 ) );
			pFirstCategory = (CMyMFCRibbonCategory*)pCat;
		}
	}
	return pCat;
}

void CMainFrame::CreateQuickAccessCommands( void )
{
	CList<UINT, UINT> lstQATCmds;

	lstQATCmds.AddTail(ID_FILE_NEW);
	lstQATCmds.AddTail(ID_FILE_OPEN);
	lstQATCmds.AddTail(ID_FILE_SAVE);
	lstQATCmds.AddTail(ID_FILE_PRINT_DIRECT);

	//m_wndRibbonBar.SetQuickAccessCommands(lstQATCmds);

	// All this to get rid of the Quick Access toolbar. It draws WRONG on UHD displays for some unknown reason.
	// The drawing bug is related to the size of the customization button and how it has a x2 multiplier on its height somewhere for some unknown reason.
	CList<UINT, UINT> Derp;
	m_wndRibbonBar.SetQuickAccessCommands( Derp );
	CMFCRibbonQuickAccessToolBar *pQAToolBar = m_wndRibbonBar.GetQuickAccessToolbar();
	pQAToolBar->RemoveAll();
	pQAToolBar->Redraw();

	//CMFCRibbonQuickAccessToolBar *pQAToolBar = m_wndRibbonBar.GetQuickAccessToolbar();
	//CMFCRibbonBaseElement *pLastButton = pQAToolBar->GetButton( pQAToolBar->GetCount() - 2 );
	//pLastButton->SetCompactMode( 1 );
	//pLastButton->SetDescription( "" );
	//CRect ButtonRect = pLastButton->GetRect();
	//ButtonRect.bottom /= 2;
	//pLastButton->SetRect( ButtonRect );
	//pLastButton->SetVisible( 0 );
}

void CMainFrame::CreatePrintingCategory( void )
{
	CMFCRibbonCategory* pCategoryPrinting = CreateCategory( &m_wndRibbonBar, IDS_RIBBON_PRINTING );
	CreatePrintPanel( pCategoryPrinting );
	CreatePrintOptionsPanel( pCategoryPrinting );
}

void CMainFrame::CreateHelpCategory( void )
{
	CMFCRibbonCategory* pCategoryHelp = CreateCategory( &m_wndRibbonBar, IDS_RIBBON_HELPCAT );
	CreateHelpPanel( pCategoryHelp );
}

void CMainFrame::CreateBackgroundCategory( void )
{
	CMFCRibbonCategory* pCategoryBackground = CreateCategory( &m_wndRibbonBar, IDS_RIBBON_BACKGROUNDCAT );
	CreateBackgroundPanel( pCategoryBackground );
	CreateBackgroundViewPanel( pCategoryBackground );
}

void CMainFrame::CreateOptionsCategory( void )
{
	CMFCRibbonCategory* pCategoryOptions = CreateCategory( &m_wndRibbonBar, IDS_RIBBON_OPTIONS );
	CreateOptionsPanel( pCategoryOptions );
}

void CMainFrame::CreateHomeCategory( void )
{
	CMFCRibbonCategory* pCategoryHome = CreateCategory( &m_wndRibbonBar, IDS_RIBBON_HOME );
	CreateClipboardPanel( pCategoryHome );
	CreateViewPanel( pCategoryHome );
	CreateDimensionsPanel( pCategoryHome );
	//CreateInsertPanel( pCategoryHome );
	CreateElementPanel( pCategoryHome );
	CreateAlignPanel( pCategoryHome );
	CreateSimulationPanel( pCategoryHome );
}

void CMainFrame::InitializeRibbon()
{
	m_wndRibbonBar.SetWindows7Look( TRUE );
	m_wndRibbonBar.EnablePrintPreview( 1 );

#if defined( LINKAGE_USE_DIRECT2D ) // only use the system DPI setting, the one the user can control for scaling, if DPI awareness is built into the program.
	CWindowDC DC( 0 );
	int PPI = DC.GetDeviceCaps( LOGPIXELSX );
	double DPIScale = (double)PPI / 96.0; // The assumption that all scaling is done from 96 DPI seems correct.
#else
	double DPIScale = 1.0;
#endif

	/*
	 * The MFC ribbon and tooltip code does not account for high DPI displays. The tooltips are unusually narrow on a 2.5x
	 * display density. This code attempts to set the max width, used by tooltips that have images, as well as the fixed widths
	 * that are used as the width for tips with no images. The MFC code is a bit crappy since it doesn't just take the max width
	 * and then reduce it by the image size then shrink to fit if there is a single line of text. Instead, it has up to three
	 * different "maximum" values all used in different situations.
	 */

	int MaxTooltipWidth = (int)( 200 * DPIScale );
	CWinAppEx *pAppEx = (CWinAppEx *)AfxGetApp();
	if( pAppEx != 0 )
	{
		CTooltipManager *pTTM = pAppEx->GetTooltipManager(); // if the app class is not derived from CWinAppEx then this might cause a crash.
		if( pTTM != 0 )
		{
			CMFCToolTipInfo Params;

			// This number will override the tooltip width information set elsewhere. Just take it and adjust it to deal with the text scaling DPI 
			MaxTooltipWidth = (int)( Params.m_nMaxDescrWidth * DPIScale );
			Params.m_nMaxDescrWidth = MaxTooltipWidth;
			pTTM->SetTooltipParams( AFX_TOOLTIP_TYPE_ALL, RUNTIME_CLASS (CMFCToolTipCtrl), &Params );

		}
	}

	m_wndRibbonBar.SetTooltipFixedWidth( MaxTooltipWidth, MaxTooltipWidth );

	// Load panel images:
	m_PanelImages.SetImageSize(CSize(16, 16));
	m_PanelImages.Load(IDB_BUTTONS);
	m_RibbonInitializingCounter = 0;

	SetTimer( 0, 1, 0 );
	return;
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWndEx::Dump(dc);
}
#endif //_DEBUG

#define TESTCOLOR RGB( 255, 0, 0 )

class CMFCVisualManagerWindows7Dave : public CMFCVisualManagerWindows7
{
	DECLARE_DYNCREATE( CMFCVisualManagerWindows7Dave )
public:
	CMFCVisualManagerWindows7Dave() {}
	virtual ~CMFCVisualManagerWindows7Dave() {}

	void SetButtonTheme( HTHEME htheme ) { m_hThemeButton = htheme; }
	HTHEME GetButtonTheme( void ) { return m_hThemeButton; }
	
	void OnDrawCheckBoxEx(CDC *pDC, CRect rect, int nState, BOOL bHighlighted, BOOL bPressed, BOOL bEnabled)
	{
		/*
		if (CMFCToolBarImages::m_bIsDrawOnGlass)
		{
			CDrawingManager dm(*pDC);

			rect.DeflateRect(1, 1);

			dm.DrawRect(rect, bEnabled ? GetGlobalData()->clrWindow : GetGlobalData()->clrBarFace, GetGlobalData()->clrBarShadow);

			if (nState == 1)
			{
				CMenuImages::Draw(pDC, CMenuImages::IdCheck, rect, CMenuImages::ImageBlack);
			}

			return;
		}*/

		if (bHighlighted)
		{
			pDC->DrawFocusRect(rect);
		}

		rect.DeflateRect(1, 1);
		pDC->FillSolidRect(&rect, bEnabled ? GetGlobalData()->clrWindow : GetGlobalData()->clrBarFace);
		//pDC->Draw3dRect(&rect, GetGlobalData()->clrBarDkShadow, GetGlobalData()->clrBarHilite);
		pDC->Draw3dRect(&rect, GetGlobalData()->clrBarDkShadow, GetGlobalData()->clrBarShadow);

		rect.DeflateRect(1, 1);
		//pDC->Draw3dRect(&rect, GetGlobalData()->clrBarShadow, GetGlobalData()->clrBarLight);
		pDC->Draw3dRect(&rect, GetGlobalData()->clrBarShadow, GetGlobalData()->clrBarLight);

		if (nState == 1)
		{
			// Do a small adjustment to better center the checkmark.
			rect.top -= (int)( GetGlobalData()->GetRibbonImageScale() * 1 );
			rect.bottom -= (int)( GetGlobalData()->GetRibbonImageScale() * 1 );
			CMenuImages::Draw(pDC, CMenuImages::IdCheck, rect, CMenuImages::ImageBlack);
		}
		else if (nState == 2)
		{
			rect.DeflateRect(1, 1);

			CBrush br;
			br.CreateHatchBrush(HS_DIAGCROSS, GetGlobalData()->clrBtnText);

			pDC->FillRect(rect, &br);
		}
	}

	virtual BOOL DrawTextOnGlass(
		CDC* pDC,
		CString strText,
		CRect rect,
		DWORD dwFlags,
		int nGlowSize = 0,
		COLORREF clrText = (COLORREF)-1);

		virtual COLORREF GetHighlightedMenuItemTextColor(CMFCToolBarMenuButton* pButton) { return 0; }
		virtual COLORREF GetCaptionBarTextColor(CMFCCaptionBar* pBar) { return 0; }
		virtual COLORREF GetMenuItemTextColor(
			CMFCToolBarMenuButton* pButton,  
			BOOL bHighlighted,  
			BOOL bDisabled) { return 0; }
		virtual COLORREF GetRibbonHyperlinkTextColor(CMFCRibbonLinkCtrl* pHyperLink) { return 0; }
		virtual COLORREF GetRibbonQuickAccessToolBarTextColor(BOOL bDisabled = FALSE) { return 0; }
		virtual COLORREF GetRibbonStatusBarTextColor(CMFCRibbonStatusBar* pStatusBar) { return 0; }
		virtual COLORREF GetStatusBarPaneTextColor(
			CMFCStatusBar* pStatusBar,  
			CMFCStatusBarPaneInfo* pPane) { return 0; }
		virtual COLORREF GetTabTextColor(
			const CMFCBaseTabCtrl* pTabWnd,  
			int iTab,  
			BOOL bIsActive) { return 0; }
		virtual COLORREF GetToolbarButtonTextColor(
			CMFCToolBarButton* pButton,  
			CMFCVisualManager::AFX_BUTTON_STATE state) { return 0; }

		// The following function only seems to get called for the Align menu button, not even for other non-menu buttons!
		// And since it's disabled, it is commented out since disabled colors are not adjusted by this code.
		//virtual COLORREF GetToolbarDisabledTextColor() { return TESTCOLOR; }

		virtual COLORREF OnDrawRibbonButtonsGroup(
			CDC* pDC,  
			CMFCRibbonButtonsGroup* pGroup,  
			CRect rectGroup) { return 0; }

		COLORREF OnFillRibbonButton(CDC* pDC, CMFCRibbonButton* pButton)
		{
			ASSERT_VALID(pDC);
			ASSERT_VALID(pButton);

			if (!CanDrawImage())
			{
				return CMFCVisualManagerWindows::OnFillRibbonButton(pDC, pButton);
			}

			BOOL bIsMenuMode = pButton->IsMenuMode();

			CRect rect(pButton->GetRect());

			CMFCControlRenderer* pRenderer = NULL;
			CMFCVisualManagerBitmapCache* pCache = NULL;
			int index = 0;

			BOOL bDisabled    = pButton->IsDisabled();
			BOOL bWasDisabled = bDisabled;
			BOOL bFocused     = pButton->IsFocused();
			BOOL bDroppedDown = pButton->IsDroppedDown();
			BOOL bPressed     = pButton->IsPressed() && !bIsMenuMode;
			BOOL bChecked     = pButton->IsChecked();
			BOOL bHighlighted = pButton->IsHighlighted() || bFocused;

			BOOL bDefaultPanelButton = pButton->IsDefaultPanelButton() && !pButton->IsQATMode();
			if (bFocused)
			{
				bDisabled = FALSE;
			}

			if (pButton->IsDroppedDown() && !bIsMenuMode)
			{
				bChecked     = TRUE;
				bPressed     = FALSE;
				bHighlighted = FALSE;
			}

			CMFCRibbonBaseElement::RibbonElementLocation location = pButton->GetLocationInGroup();

			if (pButton->IsKindOf(RUNTIME_CLASS(CMFCRibbonEdit)))
			{
				COLORREF color1 = m_clrRibbonEdit;
				if (bDisabled)
				{
					color1 = m_clrRibbonEditDisabled;
				}
				else if (bChecked || bHighlighted)
				{
					color1 = m_clrRibbonEditHighlighted;
				}

				COLORREF color2 = color1;

				rect.left = pButton->GetCommandRect().left;

				{
					CDrawingManager dm(*pDC);
					dm.FillGradient(rect, color1, color2, TRUE);
				}

				return (COLORREF)-1;
			}

			if (bChecked && bIsMenuMode && !pButton->IsGalleryIcon())
			{
				bChecked = FALSE;
			}

			if (location != CMFCRibbonBaseElement::RibbonElementNotInGroup && pButton->IsShowGroupBorder())
			{
				if (!pButton->GetMenuRect().IsRectEmpty())
				{
					CRect rectC = pButton->GetCommandRect();
					CRect rectM = pButton->GetMenuRect();

					CMFCControlRenderer* pRendererC = NULL;
					CMFCControlRenderer* pRendererM = NULL;

					CMFCVisualManagerBitmapCache* pCacheC = NULL;
					CMFCVisualManagerBitmapCache* pCacheM = NULL;

					if (location == CMFCRibbonBaseElement::RibbonElementSingleInGroup)
					{
						pRendererC = &m_ctrlRibbonBtnGroupMenu_F[0];
						pRendererM = &m_ctrlRibbonBtnGroupMenu_L[1];

						pCacheC = &m_cacheRibbonBtnGroupMenu_F[0];
						pCacheM = &m_cacheRibbonBtnGroupMenu_L[1];
					}
					else if (location == CMFCRibbonBaseElement::RibbonElementFirstInGroup)
					{
						pRendererC = &m_ctrlRibbonBtnGroupMenu_F[0];
						pRendererM = &m_ctrlRibbonBtnGroupMenu_F[1];

						pCacheC = &m_cacheRibbonBtnGroupMenu_F[0];
						pCacheM = &m_cacheRibbonBtnGroupMenu_F[1];
					}
					else if (location == CMFCRibbonBaseElement::RibbonElementLastInGroup)
					{
						pRendererC = &m_ctrlRibbonBtnGroupMenu_L[0];
						pRendererM = &m_ctrlRibbonBtnGroupMenu_L[1];

						pCacheC = &m_cacheRibbonBtnGroupMenu_L[0];
						pCacheM = &m_cacheRibbonBtnGroupMenu_L[1];
					}
					else
					{
						pRendererC = &m_ctrlRibbonBtnGroupMenu_M[0];
						pRendererM = &m_ctrlRibbonBtnGroupMenu_M[1];

						pCacheC = &m_cacheRibbonBtnGroupMenu_M[0];
						pCacheM = &m_cacheRibbonBtnGroupMenu_M[1];
					}

					int indexC = 0;
					int indexM = 0;

					BOOL bHighlightedC = pButton->IsCommandAreaHighlighted();
					BOOL bHighlightedM = pButton->IsMenuAreaHighlighted();

					if (bChecked)
					{
						indexC = 3;

						if (bHighlighted)
						{
							indexM = 5;
						}
					}

					if (bDisabled)
					{
						if (bChecked)
						{
							indexC = 5;
							indexM = 4;
						}
					}
					else
					{
						if (pButton->IsDroppedDown() && !bIsMenuMode)
						{
							indexC = pButton->IsChecked() ? 3 : 6;
							indexM = 3;
						}
						else
						{
							if (bFocused)
							{
								indexC = 6;
								indexM = 5;
							}

							if (bHighlightedC || bHighlightedM)
							{
								if (bChecked)
								{
									indexC = bHighlightedC ? 4 : 3;
								}
								else
								{
									indexC = bHighlightedC ? 1 : 6;
								}

								indexM = bHighlightedM ? 1 : 5;
							}

							if (bPressed)
							{
								if (bHighlightedC)
								{
									indexC = 2;
								}
							}
						}
					}

					if (indexC != -1 && indexM != -1)
					{
						int nCacheIndex = -1;
						if (pCacheC != NULL)
						{
							CSize size(rectC.Size());
							nCacheIndex = pCacheC->FindIndex(size);
							if (nCacheIndex == -1)
							{
								nCacheIndex = pCacheC->Cache(size, *pRendererC);
							}
						}

						if (nCacheIndex != -1)
						{
							pCacheC->Get(nCacheIndex)->Draw(pDC, rectC, indexC);
						}
						else
						{
							pRendererC->Draw(pDC, rectC, indexC);
						}

						nCacheIndex = -1;
						if (pCacheM != NULL)
						{
							CSize size(rectM.Size());
							nCacheIndex = pCacheM->FindIndex(size);
							if (nCacheIndex == -1)
							{
								nCacheIndex = pCacheM->Cache(size, *pRendererM);
							}
						}

						if (nCacheIndex != -1)
						{
							pCacheM->Get(nCacheIndex)->Draw(pDC, rectM, indexM);
						}
						else
						{
							pRendererM->Draw(pDC, rectM, indexM);
						}
					}

					return(COLORREF)-1;
				}
				else
				{
					if (location == CMFCRibbonBaseElement::RibbonElementSingleInGroup)
					{
						pRenderer = &m_ctrlRibbonBtnGroup_S;
						pCache    = &m_cacheRibbonBtnGroup_S;
					}
					else if (location == CMFCRibbonBaseElement::RibbonElementFirstInGroup)
					{
						pRenderer = &m_ctrlRibbonBtnGroup_F;
						pCache    = &m_cacheRibbonBtnGroup_F;
					}
					else if (location == CMFCRibbonBaseElement::RibbonElementLastInGroup)
					{
						pRenderer = &m_ctrlRibbonBtnGroup_L;
						pCache    = &m_cacheRibbonBtnGroup_L;
					}
					else
					{
						pRenderer = &m_ctrlRibbonBtnGroup_M;
						pCache    = &m_cacheRibbonBtnGroup_M;
					}

					if (bChecked)
					{
						index = 3;
					}

					if (bDisabled && !bFocused)
					{
						index = 0;
					}
					else
					{
						if (bPressed)
						{
							if (bHighlighted)
							{
								index = 2;
							}
						}
						else if (bHighlighted)
						{
							index++;
						}
					}
				}
			}
			else if (bDefaultPanelButton)
			{
				if (bPressed)
				{
					if (bHighlighted)
					{
						index = 2;
					}
				}
				else if (bHighlighted)
				{
					index = 1;
				}
				else if (bChecked)
				{
					index = 2;
				}

				if (bFocused && !bDroppedDown && m_ctrlRibbonBtnDefault.GetImageCount () > 3)
				{
					index = 3;
				}

				if (index != -1)
				{
					pRenderer = &m_ctrlRibbonBtnDefault;
					CMFCVisualManagerBitmapCache* pCache = &m_cacheRibbonBtnDefault;

					const CMFCControlRendererInfo& params = pRenderer->GetParams();

					int nCacheIndex = -1;
					if (pCache != NULL)
					{
						CSize size(params.m_rectImage.Width(), rect.Height());
						nCacheIndex = pCache->FindIndex(size);
						if (nCacheIndex == -1)
						{
							nCacheIndex = pCache->CacheY(size.cy, *pRenderer);
						}
					}

					if (nCacheIndex != -1)
					{
						pCache->Get(nCacheIndex)->DrawY(pDC, rect, CSize(params.m_rectInter.left, params.m_rectImage.right - params.m_rectInter.right), index);

						return GetGlobalData()->clrBtnText;
					}
				}
			}
			else if ((!bDisabled &&(bPressed || bChecked || bHighlighted)) || (bDisabled && bFocused))
			{
				if (!pButton->GetMenuRect().IsRectEmpty()/* &&
														 (pButton->IsHighlighted() || bChecked)*/)
				{
					CRect rectC = pButton->GetCommandRect();
					CRect rectM = pButton->GetMenuRect();

					CMFCControlRenderer* pRendererC = pButton->IsMenuOnBottom() ? &m_ctrlRibbonBtnMenuV[0] : &m_ctrlRibbonBtnMenuH[0];
					CMFCControlRenderer* pRendererM = pButton->IsMenuOnBottom() ? &m_ctrlRibbonBtnMenuV[1] : &m_ctrlRibbonBtnMenuH[1];

					int indexC = -1;
					int indexM = -1;

					BOOL bDropped      = pButton->IsDroppedDown();
					BOOL bHighlightedC = pButton->IsCommandAreaHighlighted();
					BOOL bHighlightedM = pButton->IsMenuAreaHighlighted();

					if (bDisabled)
					{
						if (bHighlightedC || bHighlightedM)
						{
							indexC = 4;
							indexM = 4;

							if (bHighlightedM)
							{
								indexM = 0;

								if (bDropped && !bIsMenuMode)
								{
									indexC = 5;
									indexM = 2;
								}
								else if (bPressed)
								{
									indexM = 1;
								}
							}
						}
					}
					else
					{
						if (bDropped && !bIsMenuMode)
						{
							indexC = 5;
							indexM = 2;
						}
						else
						{
							if (bFocused)
							{
								indexC = 5;
								indexM = 4;
							}

							if (bChecked)
							{
								indexC = 2;
								indexM = 2;
							}

							if (bHighlightedC || bHighlightedM)
							{
								indexM = 4;

								if (bPressed)
								{
									if (bHighlightedC)
									{
										indexC = 1;
									}
									else if (bHighlightedM)
									{
										indexC = bChecked ? 3 : 5;
									}
								}
								else
								{
									indexC = bChecked ? 3 : 0;

									if (bHighlightedM)
									{
										indexC = bChecked ? 3 : 5;
										indexM = 0;
									}
								}
							}
						}
					}

					if (indexC != -1)
					{
						pRendererC->Draw(pDC, rectC, indexC);
					}

					if (indexM != -1)
					{
						pRendererM->Draw(pDC, rectM, indexM);
					}

					return(COLORREF)-1;
				}
				else
				{
					index = -1;

					pRenderer = &m_ctrlRibbonBtn[0];
					if (rect.Height() > pRenderer->GetParams().m_rectImage.Height() * 1.5 && m_ctrlRibbonBtn[1].IsValid())
					{
						pRenderer = &m_ctrlRibbonBtn[1];
					}

					if (bDisabled && bFocused)
					{
						if (pRenderer->GetImageCount() > 4)
						{
							index = 4;
						}
						else
						{
							index = 0;
						}
					}

					if (!bDisabled)
					{
						if (bChecked)
						{
							index = 2;
						}

						if (bPressed)
						{
							if (bHighlighted)
							{
								index = 1;
							}
						}
						else if (bHighlighted)
						{
							index++;
						}
					}
				}
			}

			COLORREF clrText = bWasDisabled ? GetGlobalData()->clrGrayedText : COLORREF(-1);

			if (pRenderer != NULL)
			{
				if (index != -1)
				{
					int nCacheIndex = -1;
					if (pCache != NULL)
					{
						CSize size(rect.Size());
						nCacheIndex = pCache->FindIndex(size);
						if (nCacheIndex == -1)
						{
							nCacheIndex = pCache->Cache(size, *pRenderer);
						}
					}

					if (nCacheIndex != -1)
					{
						pCache->Get(nCacheIndex)->Draw(pDC, rect, index);
					}
					else
					{
						pRenderer->Draw(pDC, rect, index);
					}

					if (!bWasDisabled)
					{
						clrText = GetGlobalData()->clrBtnText;
					}
				}
			}

			return clrText;
		}
};

IMPLEMENT_DYNCREATE( CMFCVisualManagerWindows7Dave, CMFCVisualManagerWindows7 )

BOOL CMFCVisualManagerWindows7Dave::DrawTextOnGlass(
	CDC* pDC,
	CString strText,
	CRect rect,
	DWORD dwFlags,
	int nGlowSize,
	COLORREF clrText)
{
	return CMFCVisualManagerWindows7::DrawTextOnGlass( pDC, strText, rect, dwFlags, 10, (COLORREF)-1 );
}

// CMainFrame message handlers

void CMainFrame::OnApplicationLook(UINT id)
{
	CWaitCursor wait;

#if 0

	theApp.m_nAppLook = id;

	switch (theApp.m_nAppLook)
	{
	case ID_VIEW_APPLOOK_WIN_2000:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManager));
		break;

	case ID_VIEW_APPLOOK_OFF_XP:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOfficeXP));
		break;

	case ID_VIEW_APPLOOK_WIN_XP:
		CMFCVisualManagerWindows::m_b3DTabsXPTheme = TRUE;
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
		break;

	case ID_VIEW_APPLOOK_OFF_2003:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2003));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case ID_VIEW_APPLOOK_VS_2005:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2005));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	default:
		switch (theApp.m_nAppLook)
		{
		case ID_VIEW_APPLOOK_OFF_2007_BLUE:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_LunaBlue);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_BLACK:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_SILVER:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Silver);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_AQUA:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Aqua);
			break;
		}

		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
		CDockingManager::SetDockingMode(DT_SMART);
	}

#endif

	CMFCVisualManager::SetDefaultManager( RUNTIME_CLASS( CMFCVisualManagerWindows7Dave ) );

	RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);

	theApp.WriteInt(_T("ApplicationLook"), theApp.m_nAppLook);
}

void CMainFrame::OnFilePrint()
{
	if (IsPrintPreview())
	{
		PostMessage(WM_COMMAND, AFX_ID_PREVIEW_PRINT);
	}
}

void CMainFrame::OnFilePrintPreview()
{
	if (IsPrintPreview())
	{
		//PostMessage(WM_COMMAND, AFX_ID_PREVIEW_CLOSE);  // force Print Preview mode closed
	}
}

void CMainFrame::OnUpdateFilePrintPreview(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(IsPrintPreview());
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnHelpUserguide()
{
	HINSTANCE hResult = 0;
	static const char *DOC_FILE = ".\\Linkage.pdf";
	if( _access( DOC_FILE, 0 ) == 0 )
		hResult = ShellExecute( this->GetSafeHwnd(), "open", DOC_FILE, 0, 0, SW_SHOWNORMAL );

	if( hResult == 0 || hResult == INVALID_HANDLE_VALUE )
		AfxMessageBox( "Unable to open the documentation file.", MB_ICONEXCLAMATION | MB_OK );
}

void CMainFrame::OnHelpAbout()
{
	CAboutDialog dlg;
	dlg.DoModal();
}

void CMainFrame::ConfigureDocumentationMenu( CMenu *pMenu )
{
	if( pMenu == 0 || _access( ".\\Linkage.pdf", 0 ) == 0 )
		return;

	// Remove the documentation menu item since there is no document.

	int Index;
	for( Index = pMenu->GetMenuItemCount() - 1; Index >= 0; --Index )
	{
		if( pMenu->GetMenuItemID( Index ) == ID_HELP_USERGUIDE )
			break;
	}
	if( Index < 0 )
		return; // never found the placeholder.

	pMenu->DeleteMenu( Index, MF_BYPOSITION );
	if( pMenu->GetMenuItemID( Index ) == 0 ) // Separator?
		pMenu->DeleteMenu( Index, MF_BYPOSITION );
}

void SetStatusText( const char *pText )
{
	CMainFrame* pFrame= (CMainFrame*)AfxGetMainWnd();
	pFrame->m_wndStatusBar.GetElement( 0 )->SetText( pText == 0 ? "" : pText );
	pFrame->m_wndStatusBar.Invalidate( true );
}

void SetStatusText( UINT ResourceID)
{
	CString String;
	BOOL bNameValid = String.LoadString( ResourceID );
	SetStatusText( String );
}

void ShowPrintPreviewCategory( bool bShow )
{
	CMainFrame* pFrame= (CMainFrame*)AfxGetMainWnd();
	pFrame->m_wndRibbonBar.SetPrintPreviewMode( bShow ? 1 : 0 );
}

void CMainFrame::OnSelectSample()
{
	int Selection = m_pSampleGallery->GetLastSelectedItem( ID_RIBBON_SAMPLE_GALLERY );

	m_pSampleGallery->SelectItem( -1 );

	CSampleGallery GalleryData;
	int Count = GalleryData.GetCount();
	if( Selection >= Count )
		return;

	PostMessage( WM_COMMAND, GalleryData.GetCommandID( Selection ) );
}

void CMainFrame::OnDropFiles(HDROP hDropInfo)
{
	CFrameWndEx::OnDropFiles(hDropInfo);
}


void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
	switch( m_RibbonInitializingCounter )
	{
		case 0: CreateMainCategory(); break;
		case 1: CreateHomeCategory(); break;
		case 2: CreatePrintingCategory(); break;
		case 3: 
			#if defined( LINKAGE_USE_DIRECT2D )
				CreateBackgroundCategory();
			#endif
			break;
		case 4: CreateOptionsCategory(); break;
		case 5: CreateHelpCategory(); break;
		case 6: CreateHelpButtons(); break;
		case 7: CreateQuickAccessCommands(); break;
		case 8: 
		{
			KillTimer( nIDEvent ); 
			SetCursor( AfxGetApp()->LoadStandardCursor( IDC_ARROW ) ); 
			CDocument *pDoc = GetActiveDocument();
			if( pDoc != 0 )
				pDoc->UpdateAllViews( 0 );
			break;
		}
	}
	++m_RibbonInitializingCounter;
}

BOOL CMainFrame::OnSetCursor( CWnd* pWnd, UINT nHitTest, UINT message )
{
	if( m_RibbonInitializingCounter < 6 )
	{
		SetCursor( AfxGetApp()->LoadStandardCursor( IDC_WAIT ) );
		return TRUE;
	}

	return CWnd::OnSetCursor( pWnd, nHitTest, message );
}


void CMainFrame::OnClose()
{
	if( IsPrintPreview() )
	{
		PostMessage(WM_COMMAND, AFX_ID_PREVIEW_CLOSE);
		return;
	}

	CFrameWndEx::OnClose();
}
