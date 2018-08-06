// LinkPropertiesDialog.cpp : implementation file
//

#include "stdafx.h"
#include "Linkage.h"
#include "LinePropertiesDialog.h"

// CLinePropertiesDialog dialog

IMPLEMENT_DYNAMIC(CLinePropertiesDialog, CMyDialog)

CLinePropertiesDialog::CLinePropertiesDialog(CWnd* pParent /*=NULL*/)
	: CMyDialog( pParent, CLinePropertiesDialog::IDD )
	, m_LineSize(1)
	, m_SelectedLinkCount(0)
	, m_Name(_T(""))
	, m_bMeasurementLine(FALSE)
	, m_FastenTo( _T( "" ) )
	, m_bOffsetMeassurementLine(FALSE)
	, m_ShapeType(0)
	, m_LengthsAngles(0)
{
	m_bColorIsSet = false;
	m_Color = RGB( 200, 200, 200 );
}

CLinePropertiesDialog::~CLinePropertiesDialog()
{
}

void CLinePropertiesDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_LineSize);
	DDV_MinMaxInt(pDX, m_LineSize, 1, 4);
	DDX_Control(pDX, IDC_SPIN1, m_SpinControl);
	DDX_Text(pDX, IDC_EDIT3, m_Name);
	DDX_Control(pDX, IDC_NAMEPROMPT, m_NamePromptControl);
	DDX_Control(pDX, IDC_EDIT3, m_NameControl);
	DDX_Check(pDX, IDC_CHECK1, m_bMeasurementLine);
	DDX_Control(pDX, IDC_CHECK1, m_MeasurementLineControl);
	DDX_Control(pDX, IDC_RPMPROMPT, m_LineSizeControl);
	DDX_Control(pDX, IDC_EDIT1, m_LineSizeInputControl);
	DDX_Control(pDX, IDC_FASTENEDTO, m_FastenToControl);
	DDX_Text(pDX, IDC_FASTENEDTO, m_FastenTo);
	DDX_Control(pDX, IDC_COLOR, m_ColorControl);
	DDX_Radio(pDX, IDC_RADIO3, m_ShapeType);
	DDX_Control(pDX, IDC_RADIO1, m_TypeControl1);
	DDX_Control(pDX, IDC_RADIO2, m_TypeControl2);
	DDX_Control(pDX, IDC_RADIO3, m_TypeControl3);
	DDX_Control(pDX, IDC_RADIO8, m_ShowLengthsControl);
	DDX_Control(pDX, IDC_RADIO10, m_ShowAnglesControl);
	DDX_Control(pDX, IDC_CHECK7, OffsetLineControl);
	DDX_Check(pDX, IDC_CHECK7, m_bOffsetMeassurementLine);
	DDX_Radio(pDX, IDC_RADIO8, m_LengthsAngles);

	m_SpinControl.SetRange(1, 4);

	if (m_SelectedLinkCount > 1)
	{
		m_NamePromptControl.EnableWindow(FALSE);
		m_NameControl.EnableWindow(FALSE);
		m_MeasurementLineControl.EnableWindow(FALSE);
	}

	if (pDX->m_bSaveAndValidate)
		m_Color = m_ColorControl.GetColor();
	else
		m_ColorControl.SetColor(m_Color);

	OnBnClickedCheck1();
	OnBnClickedLengthsAngles();
}

BEGIN_MESSAGE_MAP(CLinePropertiesDialog, CMyDialog)
	ON_BN_CLICKED(IDC_CHECK1, &CLinePropertiesDialog::OnBnClickedCheck1)
	ON_STN_CLICKED( IDC_COLOR, &CLinePropertiesDialog::OnStnClickedColor )
	ON_BN_CLICKED(IDC_RADIO8, &CLinePropertiesDialog::OnBnClickedLengthsAngles)
	ON_BN_CLICKED(IDC_RADIO10, &CLinePropertiesDialog::OnBnClickedLengthsAngles)
END_MESSAGE_MAP()

// CLinePropertiesDialog message handlers

void CLinePropertiesDialog::OnBnClickedCheck1()
{
	bool bUnchecked = m_MeasurementLineControl.GetCheck() == BST_UNCHECKED;
	OffsetLineControl.EnableWindow( !bUnchecked );
	m_LineSizeControl.EnableWindow( bUnchecked );
	m_LineSizeInputControl.EnableWindow( bUnchecked );
	m_SpinControl.EnableWindow( bUnchecked );
	m_TypeControl1.EnableWindow( bUnchecked );
	m_TypeControl2.EnableWindow( bUnchecked );
	m_TypeControl3.EnableWindow( bUnchecked );
	m_ShowLengthsControl.EnableWindow( !bUnchecked );
	m_ShowAnglesControl.EnableWindow( !bUnchecked );
}

void CLinePropertiesDialog::OnStnClickedColor()
{
	CColorDialog dlg;
	if( dlg.DoModal() == IDOK )
	{
		m_Color = dlg.GetColor();
		m_ColorControl.SetColor( m_Color );
		m_bColorIsSet = true;
	}
}


void CLinePropertiesDialog::OnBnClickedLengthsAngles()
{
	bool bShowinglengths = m_ShowLengthsControl.GetCheck() == BST_CHECKED;
	OffsetLineControl.EnableWindow( bShowinglengths );
}
