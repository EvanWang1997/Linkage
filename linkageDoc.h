// LinkageDoc.h : interface of the CLinkageDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_LinkageDOC_H__50E4E0FC_CCEC_41BE_A091_953276D378B2__INCLUDED_)
#define AFX_LinkageDOC_H__50E4E0FC_CCEC_41BE_A091_953276D378B2__INCLUDED_

#include "connector.h"
#include "link.h"
#include "bitarray.h"
#include "ControlWindow.h"
#include "ControlKnob.h"
#include <deque>

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMemorySaveRecord
{
public:
	CMemorySaveRecord( BYTE *pData );
	void ClearContent( BYTE **ppData );
	~CMemorySaveRecord();
	BYTE *m_pData;
};

class CLinkageDoc : public CDocument
{
	protected: // create from serialization only

	CLinkageDoc();
	DECLARE_DYNCREATE(CLinkageDoc)

	public:

	enum _Direction{ HORIZONTAL, VERTICAL, INLINE, INLINESPACED, FLIPHORIZONTAL, FLIPVERTICAL, DIAGONAL };
	enum _CoordinateChange{ NONE, DISTANCE, ROTATION, SCALE, OTHER };
	enum _InsertType{ LINK, CONNECTOR, CIRCLE, ANCHOR, INPUT_ANCHOR, ACTUATOR, MEASUREMENT, ANGLE_MEASUREMENT, GEAR_LINK };
	//bool bAnchor, bool bRotating, bool bSlider, bool bActuator, bool bMeasurement, bool bSolid, bool bGear
	
	static const unsigned int DRAWINGLAYER = 0x00000001;
	static const unsigned int MECHANISMLAYER = 0x00000010;

	static const unsigned int MECHANISMLAYERS = 0xFFFFFFF0;
	static const unsigned int ALLLAYERS = 0xFFFFFFFF;

	//CLinkageDoc *GetPartsDocument( bool bRecompute );
	//void CreatePartsFromDocument( CLinkageDoc *pOriginalDoc );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLinkageDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CLinkageDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	void Copy( bool bCut );
	void Paste( void );
	void CLinkageDoc::ClearDocument( void );

	LinkList* GetLinkList( void ) { return &m_Links; }
	ConnectorList* GetConnectorList( void ) { return &m_Connectors; }
	GearConnectionList * GetGearConnections( void ) { return &m_GearConnectionList; }
	GearConnectionList & GetGearConnectionsRef( void ) { return m_GearConnectionList; }
	void FillElementList( ElementList &Elements ); 
	
	void SetUnitScale( double Scale ) { m_UnitScaling = Scale; }
	double GetUnitScale( void ) { return m_UnitScaling; }

	void SetScaleFactor( double Scale ) { m_ScaleFactor = Scale; }
	double GetScaleFactor( void ) { return m_ScaleFactor; }

	enum _Units { MM, INCH };

	void SetUnits( CLinkageDoc::_Units Units );
	CLinkageDoc::_Units GetUnits( void );

	static double GetBaseUnitsPerInch( void ) { return 96.0; }

	void SetViewLayers( unsigned int Layers );
	void SetEditLayers( unsigned int Layers );
	void KeepLayerSelections( unsigned int Layers );


	bool ClearSelection( void );
	bool SelectElement( CElement *pElement );
	bool SelectElement( CConnector *pConnector );
	bool SelectElement( CLink *pLink );
	bool SelectElement( CFPoint Point, double GrabDistance, double SolidLinkExpansion, bool bMultiSelect, int SelectionDepth, bool &bSelectionChanged );
	bool SelectElement( CFRect Rect, double SolidLinkExpansion, bool bMultiSelect, bool &bSelectionChanged );
	bool SelectElement( void );
	bool DeSelectElement( CElement *pElement );
	bool DeSelectElement( CLink *pLink );
	bool DeSelectElement( CConnector *pConnector );
	bool SelectLinkFromConnectors( void );
	bool FindElement( CFPoint Point, double GrabDistance, double SolidLinkExpansion, CLink *&pFoundLink, CConnector *&pFoundConnector );
	CFPoint CheckForSnap( ConnectorList &SelectedConnectors, double SnapDistance, bool bElementSnap, bool bGridSnap, double xGrid, double yGrid, CFPoint &ReferencePoint, bool bSnapToSelectionPoint );
	bool CheckForGridSnap( CConnector *pConnector, double SnapDistance, double xGrid, double yGrid, CFPoint &ReferencePoint, CFPoint &Adjustment );
	bool CheckForElementSnap( CConnector *pConnector, double SnapDistance, CFPoint &ReferencePoint, CFPoint &Adjustment, bool bSnapToSelectionPoint );
	bool CheckForSliderElementSnap( CConnector *pConnector, double SnapDistance, CFPoint &ReferencePoint, CFPoint &Adjustment, bool bSnapToSelectionPoint );
	bool CheckForMotionPathSnap( CConnector *pConnector, double SnapDistance, CFPoint &ReferencePoint, CFPoint &Adjustment );
	bool MoveSelected( CFPoint Point, bool bElementSnap, bool bGridSnap, double xGrid, double yGrid, double SnapDistance, CFPoint &ReferencePoint, bool bSnapToSelectionPoint );
	bool MoveSelected( CFPoint Offset );
	CString GetSelectedElementCoordinates( CString *pHintText );
	CString GetSelectedElementAngle( void );
	bool RotateSelected( CFPoint CenterPoint, double Angle );
	bool StretchSelected( CFRect OriginalRect, CFRect NewRect, _Direction Direction );
	bool StretchSelected( double Percentage );
	bool MeetSelected( void );
	bool MoveCapturedController( CFPoint Point );
	bool FixupSliderLocations( void );
	bool FixupGearConnections( void );
	bool FixupSliderLocation( CConnector *pConnector, CFPoint &NewPoint );
	bool FixupSliderLocation( CConnector *pConnector );
	bool FinishChangeSelected( void );
	bool StartChangeSelected( void );
	bool DeleteConnector( CConnector *pConnector, CLink *pDeletingLink = 0 );
	void RemoveLink( CLink *pLink );
	bool DeleteLink( CLink *pLink, CConnector *pDeletingConnector = 0 );
	int GetSelectedConnectorCount( void );
	int GetSelectedLinkCount( bool bOnlyWithMultipleConnectors );
	CLink *GetSelectedLink( int Index, bool bOnlyWithMultipleConnectors );
	bool IsAnySelected( void );
	_CoordinateChange SetSelectedElementCoordinates( CFPoint *pRotationCenterPoint, const char *pCoordinateString );
	void SetBackgroundImage( std::string Base64Data ) { m_BackgroundImageData = Base64Data; }
	std::string& GetBackgroundImage( void ) { return m_BackgroundImageData; }
	void SetBackgroundTransparency( double Transparency ) { m_BackgroundTransparency = Transparency; }
	double GetBackgroundTransparency( void ) { return m_BackgroundTransparency; }
	bool CanSimulate( CString &ErrorIfFalse );

	enum _SelectDirection { NEXT, PREVIOUS };
	bool SelectNext( _SelectDirection SelectDirection );
	void GetNextAutoSelectable( _SelectDirection SelectDirection, POSITION &Position, bool &bIsConnector );
	
	bool IsEmpty( void );

	bool ConnectGears( CLink *pLink1, CLink *pLink2, double Size1, double Size2 );
	bool DisconnectGear( CLink *pLink1 );
	
	CLink* InsertLink( unsigned int Layers, double ScaleFactor, CFPoint DesiredPoint, bool bForceToPoint, int ConnectorCount, _InsertType Type, bool bSolid );
	
	void InsertConnector( unsigned int Layers, double ScaleFactor, CFPoint DesiredPoint, bool bForceToPoint )  { InsertLink( Layers, ScaleFactor, DesiredPoint, bForceToPoint, 1, CONNECTOR, false ); }
	void InsertCircle( unsigned int Layers, double ScaleFactor, CFPoint DesiredPoint, bool bForceToPoint )  { InsertLink( Layers, ScaleFactor, DesiredPoint, bForceToPoint, 1, CIRCLE, false ); }
	void InsertAnchor( unsigned int Layers, double ScaleFactor, CFPoint DesiredPoint, bool bForceToPoint, bool bSolid )  { InsertLink( Layers, ScaleFactor, DesiredPoint, bForceToPoint, 1, ANCHOR, bSolid ); }
	void InsertAnchorLink( unsigned int Layers, double ScaleFactor, CFPoint DesiredPoint, bool bForceToPoint, bool bSolid )  { InsertLink( Layers, ScaleFactor, DesiredPoint, bForceToPoint, 2, ANCHOR, bSolid ); }
	void InsertRotateAnchor( unsigned int Layers, double ScaleFactor, CFPoint DesiredPoint, bool bForceToPoint, bool bSolid )  { InsertLink( Layers, ScaleFactor, DesiredPoint, bForceToPoint, 2, INPUT_ANCHOR, bSolid ); }
	void InsertLink( unsigned int Layers, double ScaleFactor, CFPoint DesiredPoint, bool bForceToPoint, int ConnectorCount, bool bSolid ) { InsertLink( Layers, ScaleFactor, DesiredPoint, bForceToPoint, ConnectorCount, LINK, bSolid ); }
	void InsertActuator( unsigned int Layers, double ScaleFactor, CFPoint DesiredPoint, bool bForceToPoint, bool bSolid )  { InsertLink( Layers, ScaleFactor, DesiredPoint, bForceToPoint, 2, ACTUATOR, bSolid ); }
	CLink* InsertMeasurement( unsigned int Layers, double ScaleFactor, CFPoint DesiredPoint, bool bForceToPoint )  { return InsertLink( Layers, ScaleFactor, DesiredPoint, bForceToPoint, 2, MEASUREMENT, false ); }
	void InsertAngleMeasurement( unsigned int Layers, double ScaleFactor, CFPoint DesiredPoint, bool bForceToPoint )  { InsertLink( Layers, ScaleFactor, DesiredPoint, bForceToPoint, 3, ANGLE_MEASUREMENT, false ); }
	void InsertGear( unsigned int Layers, double ScaleFactor, CFPoint DesiredPoint, bool bForceToPoint )   { InsertLink( Layers, ScaleFactor, DesiredPoint, bForceToPoint, 1, GEAR_LINK, false ); }
	//bool GetExampleName( int Index, CString &Name );
	//bool GetExampleText( int Index, CString &Text );

	bool CanAddConnector( void );
	
	void SelectAll( void );
	
	void Reset( bool bClearMotionPath = true, bool KeepCurrentPositions = false );
	
	CLink* ConnectSelected( void );
	bool FastenSelected( void );
	void FastenThese( CConnector *pFastenThis, CLink *pFastenToThis );
	void FastenThese( CLink *pFastenThis, CLink *pFastenToThis );
	void FastenThese( CLink *pFastenThis, CConnector *pFastenToThis );
	void FastenThese( CConnector *pFastenThis, CConnector *pFastenToThis );
	bool UnfastenSelected( void );
	bool JoinSelected( bool bSaveUndoState );
	bool LockSelected( void );
	void SplitSelected( void );
	void MakeRightAngleSelected( void );
	CLinkageDoc::_CoordinateChange MakeSelectedAtAngle( double Angle );
	void MakeParallelogramSelected( bool bMakeRectangle );
	void CombineSelected( void );
	void MakeAnchorSelected( void );
	bool AddConnectorToSelected( double Offset );

	void AlignSelected( _Direction Direction );

	void DeleteSelected( void );
	bool ConnectSliderLimits( bool bTestOnly = false );
	
	void GetDocumentArea( CFRect &BoundingRect, bool bSelectedOnly = false );
	void GetDocumentAdjustArea( CFRect &BoundingRect, bool bSelectedOnly = false );

	//static void ConfigureExampleMenu( CMenu *pMenu );

	bool IsSelectionAdjustable( void );

	bool SetGearRatio( CLink *pGear1, CLink *pGear2, double Size1, double Size2, bool bUseSizeAsSize, bool bSizeIsDiameter, CGearConnection::ConnectionType ConnectionType );
	CGearConnection *GetGearRatio( CLink *pGear1, CLink *pGear2, double *pSize1 = 0, double *pSize2 = 0 );
	
	void* GetLastSelectedConnector( void ) 
	{
		return m_SelectedConnectors.GetHeadPosition() == 0 ? 0 : m_SelectedConnectors.GetHead(); 
	}
	void* GetFirstSelectedConnector( void ) 
	{
		return m_SelectedConnectors.GetHeadPosition() == 0 ? 0 : m_SelectedConnectors.GetTail(); 
	}
	CConnector* GetSelectedConnector( int Index );

	void* GetLastSelectedLink( void ) 
	{
		return m_SelectedLinks.GetHeadPosition() == 0 ? 0 : m_SelectedLinks.GetHead(); 
	}
	void* GetFirstSelectedLink( void ) 
	{
		return m_SelectedLinks.GetHeadPosition() == 0 ? 0 : m_SelectedLinks.GetTail(); 
	}
	
	bool CanUndo( void ) { return m_UndoStack.size() > 0; }
	bool CanRedo( void ) { return m_RedoStack.size() > 0; }

	void SelectSample( int Index );

	void PushUndo( void );
	void PushRedo( void );
	void PopUndo( void );
	void PopRedo( void );

	bool AutoJoinSelected( void );
	
	bool IsSelectionConnectable( void ) { return m_bSelectionConnectable; }
	bool IsSelectionCombinable( void ) { return m_bSelectionCombinable; }
	bool IsSelectionJoinable( void ) { return m_bSelectionJoinable; }
	bool IsSelectionSlideable( void ) { return m_bSelectionSlideable; }
	bool IsSelectionSplittable( void ) { return m_bSelectionSplittable; }
	bool IsSelectionRectangle( void ) { return m_bSelectionRectangle; } 
	bool IsSelectionLineable( void ) { return m_bSelectionLineable; } 
	int GetAlignConnectorCount( void ) { return m_AlignConnectorCount; }
	bool IsSelectionFastenable( void ) { return m_bSelectionFastenable; }
	bool IsSelectionUnfastenable( void ) { return m_bSelectionUnfastenable; }
	bool IsSelectionMeshableGears( void ) { return m_bSelectionMeshable; }
	bool IsSelectionLockable( void ) { return m_bSelectionLockable; }
	bool IsSelectionMakeAnchor( void ) { return m_bSelectionMakeAnchor; }
	bool IsSelectionMeetable( void ) { return m_bSelectionMeetable; }

	bool IsSelectionTriangle( void ) { return m_bSelectionTriangle; }
	bool IsSelectionAngleable( void ) { return m_bSelectionAngleable; }

	bool IsSelectionPositionable( void ) { return m_bSelectionPositionable; }
	bool IsSelectionLengthable( void ) { return m_bSelectionLengthable; }
	bool IsSelectionRotatable( void ) { return m_bSelectionRotatable; }
	bool IsSelectionScalable( void ) { return m_bSelectionScalable; }

	void GetSnapLines( CFLine &Line1, CFLine &Line2 ) { Line1 = m_SnapLine[0]; Line2 = m_SnapLine[1]; }
	
	void SetSelectedModifiableCondition( void );

	static CString GetUnitsString( CLinkageDoc::_Units Units, bool bShortVersion = false );
	static CLinkageDoc::_Units GetUnitsValue( const char *pUnits );

	void RemoveGearRatio( CConnector *pGearConnector, CLink *pGearLink );

	void SetGrid( CFPoint GridSize ) { m_UserGrid = GridSize; m_bUseGrid = true; }
	void SetGrid( void ) { m_bUseGrid = false; }
	bool GetGrid( CFPoint &GridSize ) { GridSize = m_bUseGrid ? m_UserGrid : GridSize; return m_bUseGrid; }

	virtual void OnDocumentEvent(DocumentEvent deEvent);

private:
	LinkList m_Links;
	ConnectorList m_Connectors;
	ConnectorList m_SelectedConnectors;
	LinkList m_SelectedLinks;
	GearConnectionList m_GearConnectionList;

	int m_AlignConnectorCount;

	double m_UnitScaling;
	enum _Units m_Units;
	double m_ScaleFactor;
	CFPoint m_UserGrid;
	bool m_bUseGrid;

	std::deque<class CMemorySaveRecord *> m_UndoStack;
	std::deque<class CMemorySaveRecord *> m_RedoStack;

	double m_BackgroundTransparency;
	std::string m_BackgroundImageData;

	unsigned int m_SelectedLayers;
	
	CConnector *m_pCapturedConnector;
	CControlKnob *m_pCapturedConrolKnob;
	CFPoint m_CaptureOffset;
	
	void PushUndoRedo( std::deque<class CMemorySaveRecord *> &TheStack );
	
	bool PopUndoRedo( std::deque<CMemorySaveRecord*> &TheStack );

	bool FindRoomFor( CFRect NeedRect, CFPoint &PlaceHere );

	void SetLinkConnector( CLink* pLink, CConnector* pConnector );

	void NormalizeConnectorLinks( void );
	
	bool ReadIn( CArchive& ar, bool bSelect, bool bObeyUnscaleOffset, bool bUseSavedSelection, bool bResetColors, bool bUseBackground );
	bool WriteOut( CArchive& ar, bool bUseBackground, bool bSelectedOnly = false );
	
	void RawAngleSelected( double Angle );


	// void MovePartsLinkToOrigin( CFPoint Origin, CLink *pPartsLink );
 	
	void DeleteContents( bool bDeleteUndoInfo );

	bool m_bSelectionConnectable;
	bool m_bSelectionCombinable;
	bool m_bSelectionJoinable;
	bool m_bSelectionSlideable;
	bool m_bSelectionSplittable;
	bool m_bSelectionTriangle;
	bool m_bSelectionAngleable;
	bool m_bSelectionRectangle;
	bool m_bSelectionLineable;
	bool m_bSelectionFastenable;
	bool m_bSelectionUnfastenable;
	bool m_bSelectionMeshable;
	bool m_bSelectionLockable;
	bool m_bSelectionMakeAnchor;
	bool m_bSelectionMeetable;
	bool m_bSelectionPositionable;
	bool m_bSelectionLengthable;
	bool m_bSelectionRotatable;
	bool m_bSelectionScalable;

	bool CheckMeshableGears( void );
	bool CheckMeshableGears( CLink *pGear1, CLink *pGear2 );

	unsigned int m_ViewLayers;
	unsigned int m_EditLayers;
	unsigned int m_UsableLayers; // Combination of view and edit layers
	
	CBitArray m_IdentifiedConnectors;
	CBitArray m_IdentifiedLinks;
	int m_HighestConnectorID;
	int m_HighestLinkID;

	CFPoint m_SelectionPoint;
	bool m_bSelectionPoint;

	CFLine m_SnapLine[2];

	void RemoveConnector( CConnector *pConnector );
	CConnector *FindConnector( int ID );
	CLink *CLinkageDoc::FindLink( int ID );

	unsigned int GetViewLayers( void ) { return m_ViewLayers; }
	unsigned int GetEditLayers( void ) { return m_EditLayers; }

	// void DeleteConnector( POSITION Position, CConnector *pConnector );
	// CGearConnection * GetSelectedGearLinks( CLink *&pLink1, CLink *&pLink2 );

	//bool Unfasten( CLink *pFastenedTo );
	bool Unfasten( CElement *pElement );
	bool SetGearRatio( CLink *pGear1, CLink *pGear2, double Size1, double Size2, bool bUseSizeAsSize, bool bSizeIsDiameter, CGearConnection::ConnectionType ConnectionType, bool bFromUI );
	int BuildSelectedLockGroup( ConnectorList *pLockGroup );
	bool IsLinkLocked( CConnector *pConnector );
	bool ChangeLinkLength( CLink *pLink, double Value, bool bPercentage );

	//afx_msg void OnSelectSample( UINT nID );

// Generated message map functions
protected:
	//{{AFX_MSG(CLinkageDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	virtual void DeleteContents();

	#if 0
	afx_msg void OnUpdateSelectSample(CCmdUI *pCmdUI);
	void OnSelectSampleSimple( void );
	void OnSelectSampleSmile( void );
	void OnSelectSampleJansen( void );
	void OnSelectSampleKlann( void );
	void OnSelectSampleJoules( void );
	void OnSelectSampleHoeken( void );
	void OnSelectSampleParallel( void );
	void OnSelectSampleFoldingTop( void );
	void OnSelectSampleSliding1( void );
	void OnSelectSampleSliding2( void );
	void OnSelectSampleLink1( void );
	void OnSelectSampleLink2( void );
	void OnSelectSampleLink3( void );
	void OnSelectSampleLink4( void );
	void OnSelectSampleActuator( void );
	void OnSelectSampleActuator2( void );
	void OnSelectSampleBucket( void );
	void OnSelectSampleComplicated( void );
	#endif
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LinkageDOC_H__50E4E0FC_CCEC_41BE_A091_953276D378B2__INCLUDED_)
