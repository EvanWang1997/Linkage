#pragma once
#include "afxcmn.h"
#include "afxwin.h"
#include "MyDialog.h"
#include "ColorStatic.h"
#include "NullableColor.h"

// CLinePropertiesDialog dialog

class CLinePropertiesDialog : public CMyDialog
{
	DECLARE_DYNAMIC(CLinePropertiesDialog)

public:
	CLinePropertiesDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CLinePropertiesDialog();

// Dialog Data
	enum { IDD = IDD_DRAWINGLINK_PROPERTIES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
	int m_LineSize;
	CSpinButtonCtrl m_SpinControl;

protected:

public:
	int m_SelectedLinkCount;
	CString m_Name;
	CStatic m_NamePromptControl;
	CEdit m_NameControl;
	BOOL m_bMeasurementLine;
	CButton m_MeasurementLineControl;
	CStatic m_LineSizeControl;
	CEdit m_LineSizeInputControl;
	afx_msg void OnBnClickedCheck1();
	CStatic m_FastenToControl;
	CString m_FastenTo;
	afx_msg void OnStnClickedColor();
	CColorStatic m_ColorControl;
	CNullableColor m_Color;
	//CButton m_PolygonControl;
	//CButton m_PolylineButton;
	int m_Polygon;
	bool m_bColorIsSet;
	CButton OffsetLineControl;
	BOOL m_bOffsetMeassurementLine;
	int m_ShapeType;
	CButton m_TypeControl1;
	CButton m_TypeControl2;
	CButton m_TypeControl3;
	CButton m_ShowLengthsControl;
	CButton m_ShowAnglesControl;
	afx_msg void OnBnClickedLengthsAngles();
	int m_LengthsAngles;
};
