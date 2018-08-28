#include "stdafx.h"
#include "Simulator.h"
#include "linkageDoc.h"
#include "DebugItem.h"

static const double NO_MOMENTUM = 1.0;
static const double MOMENTUM = 2.5;

extern CDebugItemList DebugItemList;

#define SWAP(x,y) do \
   { unsigned char swap_temp[sizeof(x) == sizeof(y) ? (signed)sizeof(x) : -1]; \
     memcpy(swap_temp,&y,sizeof(x)); \
     memcpy(&y,&x,       sizeof(x)); \
     memcpy(&x,swap_temp,sizeof(x)); \
    } while(0)

class CSimulatorImplementation
{
	public:

	const int INTERMEDIATE_STEPS = 2;
	long m_SimulationStep;
//	int m_DesiredSimulationStep;
	bool m_bSimulationValid;
	bool m_bUseIncreasedMomentum = false;

	CSimulatorImplementation()
	{
		m_bSimulationValid = false;
		Reset();
	}

	~CSimulatorImplementation()
	{
	}

	void Options( bool bUseIncreasedMomentum )
	{
		m_bUseIncreasedMomentum = bUseIncreasedMomentum;
	}

	bool IsSimulationValid( void ) { return m_bSimulationValid; }

	bool Reset( void )
	{
		m_SimulationStep = 0;
		return true;
	}

	#pragma optimize( "agt", on )
	int CommonDivisor( int *Values, int Count )
	{
		for(;;)
		{
			/*
			 * If any value is one then simply return 1 since there can be
			 * no smaller value.
			 */
			for( int Counter = 0; Counter < Count; ++Counter )
			{
				if( Values[Counter] == 1 )
					return 1;
			}

			/*
			 * Find out if all values except for one of them are zero. Return
			 * the one non-zero value.
			 */
			int NonzeroValue = 0;
			int NonzeroCount = 0;
			for( int Counter = 0; Counter < Count; ++Counter )
			{
				if( Values[Counter] == 0 )
					continue;
				NonzeroValue = Values[Counter];
				++NonzeroCount;
			}
			if( NonzeroCount == 1 )
				return NonzeroValue;

			/*
			 * Find the lowest value and then change all of the other values
			 * to be the remainder of the value divided by the lowest value.
			 */
			int LowestLocation = -1;
			int LowestValue = 999999999;
			for( int Counter = 0; Counter < Count; ++Counter )
			{
				if( Values[Counter] < LowestValue )
				{
					LowestLocation = Counter;
					LowestValue = Values[Counter];
				}
			}
			for( int Counter = 0; Counter < Count; ++Counter )
			{
				if( Counter == LowestLocation )
					continue;
				Values[Counter] %= LowestValue;
	//			if( Values[Counter] == 0 )
	//				Values[Counter] = 1;
			}

			/*
			 * The new values will get processed at the top of the loop. This
			 * computation will continue until one of the conditions up there
			 * is met and we return something. The loop does not have a maximum
			 * loop count because the compuations are guanranteed to have an
			 * effect, to the best of my knowledge.
			 */
		}
	}
	#pragma optimize( "", on )

	CElement * GetCycleElement( CLinkageDoc *pDoc )
	{
		int InputCount = 0;
		POSITION Position = pDoc->GetConnectorList()->GetHeadPosition();
		CElement *pLastInput = 0;
		while( Position != NULL )
		{
			CConnector* pConnector = pDoc->GetConnectorList()->GetNext( Position );
			if( pConnector == 0 )
				continue;
			if( pConnector->IsInput() )
			{
				pLastInput = pConnector;
				++InputCount;
			}
		}

		Position = pDoc->GetLinkList()->GetHeadPosition();
		while( Position != NULL )
		{
			CLink* pLink = pDoc->GetLinkList()->GetNext( Position );
			if( pLink == 0 )
				continue;
			if( pLink->IsActuator() )
			{
				pLastInput = pLink;
				++InputCount;
			}
		}
		return InputCount == 1 ? pLastInput : 0;
	}

	int GetCycleSteps( CLinkageDoc *pDoc, double *pNewCPM )
	{
		CElement *pInputElement = GetCycleElement( pDoc );
		if( pInputElement == 0 )
			return 0;

		double Cycle = 0;
		int Adjustment = 1;
		if( pInputElement->IsConnector() )
			Cycle = fabs( ((CConnector*)pInputElement)->GetRPM() );
		else
		{
			Cycle = fabs( ((CLink*)pInputElement)->GetCPM() );
			Adjustment = 2;

			if( pNewCPM != 0 )
			{
				// Calculate a new CPM for the actuator that moves it just a bit faster so it gets to exactly the end
				// within the given number of simulation steps.
				int Temp = (int)( 1800.0 / Cycle / Adjustment );
				*pNewCPM = 1800.0 / Temp / Adjustment;
			}
		}

		return ( int)( 1800.0 / Cycle / Adjustment );
	}

	int GetSimulationSteps( CLinkageDoc *pDoc )
	{
		int InputCount = 0;
		POSITION Position = pDoc->GetConnectorList()->GetHeadPosition();
		int Count = 0;
		CConnector *pLastInput = 0;
		while( Position != NULL )
		{
			CConnector* pConnector = pDoc->GetConnectorList()->GetNext( Position );
			if( pConnector == 0 )
				continue;
			if( pConnector->IsInput() )
				++InputCount;
		}

		Position = pDoc->GetLinkList()->GetHeadPosition();
		while( Position != NULL )
		{
			CLink* pLink = pDoc->GetLinkList()->GetNext( Position );
			if( pLink == 0 )
				continue;
			if( pLink->IsActuator() )
				++InputCount;
		}

		if( InputCount == 0 )
			return 1;

		int *Values = new int [InputCount];
		if( Values == 0 )
			return 0;

		Position = pDoc->GetConnectorList()->GetHeadPosition();
		int Index = 0;
		while( Position != NULL )
		{
			CConnector* pConnector = pDoc->GetConnectorList()->GetNext( Position );
			if( pConnector == 0 )
				continue;
			if( pConnector->IsInput() )
				Values[Index++] = (int)( fabs( pConnector->GetRPM() ) + .9999 );
		}

		Position = pDoc->GetLinkList()->GetHeadPosition();
		while( Position != NULL )
		{
			CLink* pLink = pDoc->GetLinkList()->GetNext( Position );
			if( pLink == 0 )
				continue;
			if( pLink->IsActuator() )
				Values[Index++] = (int)( fabs( pLink->GetCPM() ) + .9999 );
		}

		/*
		 * Convert RPM values to simulation steps to get a single rotation.
		 */

		//for( Index = 0; Index < InputCount; ++Index )
		//	Values[Index] = ( 30 * INTERMEDIATE_STEPS * 60 ) / Values[Index];

		/*
		 * Figure out a common divisor that can be used
		 * to reduce the necessary rotations to do a complete
		 * cycle of the entire mechanism.
		 */

		if( InputCount == 1 )
			return 1800 / Values[0];

		int Divisor = CommonDivisor( Values, InputCount );

		/*
		 * We now assume all RPM values are integers and divide one minute
		 * of simulation by the common divisor. This gives the smallest
		 * number of simulation steps needed to get all inputs back to their
		 * starting locations at the same time. Fractional RPM will not
		 * work since more than one minute would be needed and the steps might
		 * be way too large to simulate in a reasonable amount of time.
		 */
		return 1800 / Divisor;
	}

	bool SimulateStep( CLinkageDoc *pDoc, int StepNumber, bool bAbsoluteStepNumber, int* pInputID, double *pInputPositions, int InputCount, bool bNoMultiStep, double ForceAllCPM )
	{
		if( pDoc == 0 )
			return false;

		m_bSimulationValid = true;

		ConnectorList *pConnectors = pDoc->GetConnectorList();
		LinkList *pLinks = pDoc->GetLinkList();

		if( pConnectors == 0 || pLinks == 0 )
			return false;

		if( pConnectors->GetCount() == 0 || pLinks->GetCount() == 0 )
			return false;

		int StepsToMove = StepNumber;
		if( bAbsoluteStepNumber )
			StepsToMove = StepNumber - m_SimulationStep;
		else if( InputCount > 0 && !bNoMultiStep )
			StepsToMove = 150;

		DWORD TickCount = GetTickCount();
		static const int MAX_TIME_TO_SIMULATE = 250;

		int Direction = StepsToMove > 0 ? +1 : -1;

		bool bResult = true;

		int StartStep = m_SimulationStep;

		bool IgnoreTime = false;
		#if defined( _DEBUG )
			IgnoreTime = true;
		#endif

		while( StepsToMove != 0 && ( GetTickCount() < TickCount + MAX_TIME_TO_SIMULATE || IgnoreTime ) )
		{
			m_SimulationStep += Direction;
			StepsToMove -= Direction;

			double TotalManualControlDistance = 0;

			int Counter = 0;

			/*
			 * The m_SimulationStep value has already been changed to the next step. Do calculations and simulation
			 * for all of the partial steps up to and including the m_SimulationStep step number.
			 */
			for( int IntermediateStep = INTERMEDIATE_STEPS - 1; IntermediateStep >= 0 && bResult; --IntermediateStep )
			{
				DebugItemList.RemoveAll(); // Delete these before the last step.
				POSITION Position = pConnectors->GetHeadPosition();
				while( Position != 0 )
				{
					CConnector* pConnector = pConnectors->GetNext( Position );
					if( pConnector == 0 )
						continue;
					pConnector->SetTempFixed( false );
					// If this is a rotation input connector then rotate it now (the temp rotation value).
					if( !pConnector->IsInput() )
						continue;

					// Find the input control for this connector.
					for( Counter = 0; Counter < InputCount && pInputID[Counter] != 10000 + pConnector->GetIdentifier(); )
						++Counter;
					if( Counter == InputCount )
					{
						/*
						 * There is no manual input control so rotate this connector
						 * automatically.
						 */

						if( !pConnector->IsAlwaysManual() )
						{
							double RPM = ForceAllCPM == 0 ? pConnector->GetRPM() : ForceAllCPM;
							double RotationAngle = ( ( m_SimulationStep * INTERMEDIATE_STEPS ) - ( IntermediateStep * Direction ) ) * ( -( pConnector->GetRPM() * 0.2 ) / INTERMEDIATE_STEPS );
							if( pConnector->GetLimitAngle() > 0 )
							{
								RotationAngle = OscillatedAngle( RotationAngle + ( pConnector->GetStartOffset() * pConnector->GetDirection() ), pConnector->GetLimitAngle() );
								RotationAngle -= pConnector->GetStartOffset() * pConnector->GetDirection();
							}
							pConnector->SetRotationAngle( RotationAngle );
							pConnector->MakeAnglePermenant();
						}
					}
					else
					{
						double DesiredAngle = 0;
						if( pInputPositions[Counter] != 0 )
							DesiredAngle = 360 - ( 360.0 * pInputPositions[Counter] );
						double Difference = DesiredAngle - pConnector->GetRotationAngle();
						if( Difference > 180.0 )
							Difference -= 360.0;
						if( Difference > 2 )
							Difference = 2;
						else if( Difference < -2 )
							Difference = -2;
						Difference /= INTERMEDIATE_STEPS;
						if( Difference != 0 )
						{
							pConnector->IncrementRotationAngle( Difference );
							TotalManualControlDistance += Difference;
						}
					}
				}

				Position = pLinks->GetHeadPosition();
				while( Position != 0 )
				{
					CLink *pLink = pLinks->GetNext( Position );
					if( pLink == 0  )
						continue;
					pLink->SetTempFixed( false );
					pLink->InitializeForMove();

					if( !pLink->IsActuator() )
						continue;

					// Find the input control for this connector.
					for( Counter = 0; Counter < InputCount && pInputID[Counter] != pLink->GetIdentifier(); )
						++Counter;
					if( Counter == InputCount )
					{
						/*
						 * There is no input control so extend this link automatically.
						 */
						if( !pLink->IsAlwaysManual() )
						{
							double CPM = ForceAllCPM == 0 ? pLink->GetCPM() : ForceAllCPM;
							double Distance = pLink->GetStroke() * fabs( CPM ) / 900.0;
							double Extension = ( ( m_SimulationStep * INTERMEDIATE_STEPS ) - ( IntermediateStep * Direction ) ) * ( Distance / INTERMEDIATE_STEPS );
							//Extension = OscillatedDistance( Extension + pLink->GetStartOffset(), pLink->GetStroke() );
							//Extension -= pLink->GetStartOffset();
							pLink->SetExtension( Extension );
						}
					}
					else
					{
						// This is a bit of a kludge. The extension distance will be
						// positive or nagative depending on the actuator being a push
						// or a pull at the start of its movement. On the other hand,
						// the increment should be positive in either case and a positive
						// increment actually causes the extension distance to move
						// negative when the CPM is negative. There is no simple fix
						// and something else would need to be changed to alter this
						// need for a kludge.

						double DesiredExtension = 0;
						if( pInputPositions[Counter] != 0 )
							DesiredExtension = pLink->GetStroke() * pInputPositions[Counter];
						double RealExtension = pLink->GetExtendedDistance();
						if( pLink->GetCPM() < 0 )
							RealExtension = pLink->GetStroke() + RealExtension;
						//RealExtension += pLink->GetStartOffset();
						double ActuatorDifference = DesiredExtension - RealExtension;
						double MaxDistance = pLink->GetStroke() * fabs( pLink->GetCPM() ) / 900.0;
						if( ActuatorDifference > MaxDistance )
							ActuatorDifference = MaxDistance;
						else if( ActuatorDifference < -MaxDistance )
							ActuatorDifference = -MaxDistance;
						if( pLink->GetCPM() < 0 )
							ActuatorDifference = -ActuatorDifference;
						ActuatorDifference /= INTERMEDIATE_STEPS;
						if( ActuatorDifference != 0 )
						{
							pLink->IncrementExtension( ActuatorDifference );
							TotalManualControlDistance += ActuatorDifference;
						}
					}

					if( pLink->GetFixedConnectorCount() > 1 )
					{
						pLink->GetConnector( 1 )->SetPositionValid( false );
						pLink->SetSimulationError( CElement::MANGLE );
					}
				}

				bResult = MoveSimulation( pDoc );
			}

			// Undo the step if the simulation failed.
			if( !bResult )
			{
				m_SimulationStep = StartStep;
				break;
			}

			if( InputCount > 1 && TotalManualControlDistance == 0 )
				break;
		}

		return bResult;
	}

	bool ValidateMovement( CLinkageDoc *pDoc )
	{
		if( pDoc == 0 )
			return false;

		ConnectorList *pConnectors = pDoc->GetConnectorList();
		LinkList *pLinks = pDoc->GetLinkList();

		if( pConnectors == 0 || pLinks == 0 )
			return false;

		if( pConnectors->GetCount() == 0 || pLinks->GetCount() == 0 )
			return false;

		POSITION Position = pConnectors->GetHeadPosition();
		while( Position != NULL )
		{
			CConnector* pCheckConnector = pConnectors->GetNext( Position );
			if( pCheckConnector == 0 || !pCheckConnector->IsOnLayers( CLinkageDoc::MECHANISMLAYERS ) )
				continue;
			if( !pCheckConnector->IsPositionValid() )
				return false;
		}

		bool bResult = true;
		Position = pConnectors->GetHeadPosition();
		while( Position != NULL )
		{
			CConnector* pCheckConnector = pConnectors->GetNext( Position );
			if( pCheckConnector == 0 || !pCheckConnector->IsOnLayers( CLinkageDoc::MECHANISMLAYERS ) )
				continue;

			if( !pCheckConnector->IsFixed() )
			{
				pCheckConnector->SetPositionValid( false );
				pCheckConnector->SetSimulationError( CElement::UNDEFINED );
				bResult = false;
			}
		}
		Position = pLinks->GetHeadPosition();
		while( Position != 0 )
		{
			CLink *pLink = pLinks->GetNext( Position );
			if( pLink == 0 || !pLink->IsOnLayers( CLinkageDoc::MECHANISMLAYERS ) || pLink->GetConnectorCount() <= 1 ) //|| !pLink->IsGear() )
				continue;

			if( !pLink->IsTempFixed() )
			{
				pLink->SetPositionValid( false );
				pLink->SetSimulationError( CElement::MANGLE );
				bResult = false;
			}
		}

		return bResult;
	}

	bool FindGearsToMatch( CLink *pLink, GearConnectionList &GearConnections )
	{
		if( !pLink->IsFixed() && !pLink->IsTempFixed() )
			return false;

		int FoundCount = 0;

		POSITION Position = GearConnections.GetHeadPosition();
		while( Position != 0 )
		{
			CGearConnection *pConnection = GearConnections.GetNext( Position );
			if( pConnection == 0 || pConnection->m_pGear1 == 0 || pConnection->m_pGear2 == 0 )
				continue;

			if( pConnection->m_pGear1 == pLink || pConnection->m_pGear2 == pLink )
			{
				if( pConnection->m_pGear1 == pLink && pConnection->m_pGear2->IsFixed() )
					continue;
				else if( pConnection->m_pGear2 == pLink && pConnection->m_pGear1->IsFixed() )
					continue;

				// The other gear can be rotated if it's connector is fixed.
				CLink *pOtherGear = pConnection->m_pGear1 == pLink ? pConnection->m_pGear2 : pConnection->m_pGear1;
				CConnector *pConnector = pLink->GetConnector( 0 );
				CConnector *pOtherConnector = pOtherGear->GetConnector( 0 );
				if( pConnector == 0 || pOtherConnector == 0 || pOtherGear->IsFixed() || !pOtherConnector->IsFixed() )
					continue;

				// Find the link that connects these two gears. It may be the ground if they both reside on anchors.
				CLink *pConnectionLink = pConnector->GetSharingLink( pOtherConnector );
				if( pConnectionLink == 0 )
				{
					if( !pConnector->IsAnchor() || !pOtherConnector->IsAnchor() )
						continue;
				}

				pConnection->m_pDriveGear = pLink;

				double GearAngle = pLink->GetTempRotationAngle();
				double LinkAngle = pConnectionLink == 0 ? 0 : -pConnectionLink->GetTempRotationAngle();
				double UseAngle = GearAngle + LinkAngle;

				if( pConnection->m_ConnectionType == pConnection->CHAIN )
					UseAngle = -UseAngle;

				if( pOtherGear == pConnection->m_pGear1 )
					UseAngle *= pConnection->Ratio();
				else
					UseAngle /= pConnection->Ratio();

				UseAngle += LinkAngle;
				pOtherConnector->SetRotationAngle( -UseAngle );

				pOtherGear->RotateAround( pOtherConnector );
				++FoundCount;
				pLink->IncrementMoveCount();

				CElementItem *pFastenedTo = pOtherGear->GetFastenedTo();
				if( pFastenedTo != 0 )
				{
					// Gears are unusual because they can move the element that they are fastened to, not just be moved by that element.
					// But it only happens when a gear is fastened to a link, not to a connector.
					if( pFastenedTo->GetLink() != 0 )
						pFastenedTo->GetLink()->RotateAround( pOtherConnector );
				}
			}
		}

		return FoundCount > 0;
	}

	bool CheckForMovement( CLinkageDoc *pDoc, CLink *pLink )
	{
		GearConnectionList *pGearConnections = pDoc->GetGearConnections();

		/*
		 * Check for movement for A SPECIFIC LINK.
		 *
		 * The function will move Links that have an input connector. It
		 * will also find links that can be moved for some reason or another
		 * and move them in to their preoper new location if possible.
		 */

		int FixedCount = pLink->GetFixedConnectorCount();

		bool bResult = true;

		if( FixedCount != pLink->GetConnectorCount() || !pLink->IsFixed() || pLink->IsGear() )
		{
			CConnector *pFixed = pLink->GetFixedConnector();
			if( pFixed != 0 && pFixed->IsInput() )
			{
				if( !pLink->IsFixed() )
				{
					if( pLink->IsGearAnchored() )
						bResult = pLink->FixAll();
					else
						bResult = pLink->RotateAround( pFixed );
				}

				if( pLink->IsGear() )
					bResult = FindGearsToMatch( pLink, *pGearConnections );
			}
			else if( pLink->IsGear() )
			{
				if( pLink->IsGearAnchored() && !pLink->IsFixed() )
					bResult = pLink->FixAll();
				bResult = FindGearsToMatch( pLink, *pGearConnections );
			}
			else if( FixedCount <= 1 )
			{
				bResult = FindLinksToMatch( pLink );
				if( !bResult )
					bResult = FindSlideLinksToMatch( pLink, pDoc->GetLinkList() );
				if( !bResult )
					bResult = FindLinkTriangleMatch( pLink );
			}
			else
			{
				bResult = FixAllIfPossible( pLink );
			}
		}

		return bResult;
	}

	bool FixAllIfPossible( CLink *pLink )
	{
		/* 
		 * Check the link to make sure it is not stretched. Stretching happens
		 * if some other link is rotated into position and one of the rotated connectors
		 * tries to move another link that has another fixed connector. I'm not sure
		 * if this is a recent problem or if the code always needed this test. but it is a bug
		 * to need this test because one link should not be able to change the position of a
		 * connector on another if the second link already has a fixed connector. Unless of course
		 * the new connector position that screws things up is actually in a valid position for
		 * both links.
		 */

		CConnector *pFixedConnector = pLink->GetFixedConnector( 0 );
		if( pFixedConnector == 0 )
			return false;

		CFPoint OriginalFixedPoint = pFixedConnector->GetOriginalPoint();
		CFPoint FixedPoint = pFixedConnector->GetPoint();

		ConnectorList *pConnectors = pLink->GetConnectorList();
		if( pConnectors == 0 )
			return false;

		POSITION Position = pConnectors->GetHeadPosition();
		while( Position != 0 )
		{
			CConnector *pConnector = pConnectors->GetNext( Position );
			if( pConnector == 0 || !pConnector->IsFixed() || pConnector == pFixedConnector )
				continue;

			double OriginalDistance = Distance( OriginalFixedPoint, pConnector->GetOriginalPoint() );
			double distance = Distance( FixedPoint, pConnector->GetPoint() );
			if( fabs( OriginalDistance - distance ) > 0.0001 )
				return false;
		}

		return pLink->FixAll();
	}

	bool MoveSimulation( CLinkageDoc *pDoc )
	{
		if( pDoc == 0 )
			return false;

		ConnectorList *pConnectors = pDoc->GetConnectorList();
		LinkList *pLinks = pDoc->GetLinkList();

		if( pConnectors == 0 || pLinks == 0 )
			return false;

		if( pConnectors->GetCount() == 0 || pLinks->GetCount() == 0 )
			return false;

		POSITION Position;
		for(;;)
		{
			int MoveCount = 0;

			Position = pLinks->GetHeadPosition();
			while( Position != 0 )
			{
				CLink *pLink = pLinks->GetNext( Position );
				if( pLink == 0 || !pLink->IsOnLayers( CLinkageDoc::MECHANISMLAYERS ) )
					continue;
				pLink->ResetMoveCount();
				CheckForMovement( pDoc, pLink );
				MoveCount += pLink->GetMoveCount();
			}
			if( MoveCount == 0 )
				break;
		}

		bool bValid = ValidateMovement( pDoc );
		if( !bValid )
			return false;

		/*if( !ValidateMovement( pDoc ) )
		{
			// Add motion points up to the current position.
			Position = pConnectors->GetHeadPosition();
			while( Position != 0 )
			{
				CConnector* pConnector = pConnectors->GetNext( Position );
				if( pConnector == 0 )
					continue;
				pConnector->AddMotionPoint();
			}

			m_bSimulationValid = false;
			return false;
		}*/

		Position = pConnectors->GetHeadPosition();
		while( Position != 0 )
		{
			CConnector* pConnector = pConnectors->GetNext( Position );
			if( pConnector == 0 )
				continue;
			pConnector->AddMotionPoint();
			pConnector->MakePermanent();
		}

		Position = pLinks->GetHeadPosition();
		while( Position != 0 )
		{
			CLink *pLink = pLinks->GetNext( Position );
			if( pLink == 0 )
				continue;
			pLink->MakePermanent();
			pLink->UpdateControlKnobs();
		}

		return true;
	}

	bool JoinToSlideLinkSpecial( CLink *pLink, CConnector *pFixedConnector, CConnector *pCommonConnector, CLink *pOtherLink )
	{
		/*
		 * Rotate the "this" Link and slide the other link so that they are still
		 * connected. The "this" Link has only one connection that is in a new
		 * temp location, pFixedConnector, and the other Link needs to have two
		 * fixed sliding connectors that it must slide through.
		 *
		 * The "this" Link will not have proper temp locations for all
		 * connectors yet, only the fixed one. It is in an odd screwed up state
		 * at the moment but will be fixed shortly.
		 *
		 * SPECIAL CASE: If this link IS pOtherLink link then we are only trying
		 * to slide this link to a new position based on slider pCommonConnector
		 * ( which is a slider but not one of the sliders that we are sliding ON).
		 * The common connector has limits that are fixed and we can move this
		 * link to a new position without changing any other link.
		 *
		 * This code is for the special case where the other link slides on a
		 * curved slider path. The sliders must have the exact same slide radius
		 * for this to work properly.
		 */

		if( pCommonConnector == 0 )
			return false;

		bool bOtherLinkOnly = ( pLink == pOtherLink && pFixedConnector == 0 );

		CConnector *pLimit1 = 0;
		CConnector *pLimit2 = 0;
		CConnector *pSlider1 = 0;
		CConnector *pSlider2 = 0;
		CConnector *pActualSlider1 = 0;
		CConnector *pActualSlider2 = 0;
		bool bOtherLinkHasSliders;

		// Double check that it can slide and get the limits for that slide.
		if( !pOtherLink->CanOnlySlide( &pLimit1, &pLimit2, &pSlider1, &pSlider2, &bOtherLinkHasSliders ) )
			return false;

		pActualSlider1 = pSlider1;
		pActualSlider2 = pSlider2;

		// The sliding connectors MUST use the exact same slide radius.
		if( pSlider1->GetSlideRadius() != pSlider2->GetSlideRadius() )
			return false;

		/*
		 * Check to make sure that we are not trying to position this link using a
		 * common connector that is also one of the sliders that is on the "track"
		 * because the slider is then limiting the movement, not defining it.
		 */
		if( pCommonConnector == pSlider1 || pCommonConnector == pSlider2 )
			return false;

		CFArc LinkArc;
		if( !pSlider1->GetOriginalSliderArc( LinkArc ) )
			return false;


		if( bOtherLinkHasSliders )
		{
			SWAP( pSlider1, pLimit1 );
			SWAP( pSlider2, pLimit2 );
		}

		/*
		 * Create a circle that represents the valid positions of the common connector
		 * based on the sliding connectors.
		 */

		CFCircle CommonCircle = LinkArc;
		CommonCircle.r = Distance( CommonCircle.GetCenter(), pCommonConnector->GetPoint() );

 		CFPoint Intersect;
 		CFPoint Intersect2;
 		bool bHit = false;
 		bool bHit2 = false;

		if( bOtherLinkOnly )
		{
			CConnector *pTempLimit1 = 0;
			CConnector *pTempLimit2 = 0;
			if( !pCommonConnector->GetSlideLimits( pTempLimit1, pTempLimit2 ) )
				return false;

			if( pCommonConnector->GetSlideRadius() == 0 )
			{
				bool bOnSegments = false;
				Intersects( CFLine( pTempLimit1->GetTempPoint(), pTempLimit2->GetTempPoint() ),
								 CommonCircle, Intersect, Intersect2, bHit, bHit2, false, false );
			}
			else
			{
				// This is the arc that the common connector slides on for this link, not for the sliding pOtherLink.
				CFArc TheArc;
				if( !pCommonConnector->GetTempSliderArc( TheArc ) )
					return false;

				bHit = bHit2 = TheArc.ArcIntersection( CommonCircle, &Intersect, &Intersect2 );
				//DebugItemList.AddTail( new CDebugItem( CommonCircle, RGB( 255, 0, 0 ) ) );
				//DebugItemList.AddTail( new CDebugItem( TheArc, RGB( 0, 255, 0 ) ) );
				//DebugItemList.AddTail( new CDebugItem( Intersect, RGB( 0, 0, 255 ) ) );
				//DebugItemList.AddTail( new CDebugItem( Intersect2, RGB( 0, 0, 255 ) ) );
			}
		}
		else
		{
			if( pFixedConnector == 0 )
				return false;

			double r = Distance( pFixedConnector->GetOriginalPoint(), pCommonConnector->GetOriginalPoint() );
			r = pLink->GetLinkLength( pFixedConnector, pCommonConnector );
 			CFCircle Circle( pFixedConnector->GetTempPoint(), r );

			bHit = bHit2 = CommonCircle.CircleIntersection( Circle, &Intersect, &Intersect2 );
		}

		if( !bHit && !bHit2 )
			return false;

		if( !bHit2 )
			Intersect2 = Intersect;
		else if( !bHit )
			Intersect = Intersect2;

		double d1 = Distance( pCommonConnector->GetTempPoint(), Intersect );
		double d2 = Distance( pCommonConnector->GetTempPoint(), Intersect2 );

		if( d2 < d1 )
			Intersect = Intersect2;

		if( !bOtherLinkOnly )
		{
			double TempAngle = GetAngle( pFixedConnector->GetTempPoint(), Intersect, pFixedConnector->GetOriginalPoint(), pCommonConnector->GetOriginalPoint() );

			TempAngle = GetClosestAngle( pFixedConnector->GetRotationAngle(), TempAngle );
			pFixedConnector->SetRotationAngle( TempAngle );
			if( !pLink->RotateAround( pFixedConnector ) )
				return false;

			pLink->IncrementMoveCount( -1 ); // Don't count the common connector twice.
		}

		// Calculate the difference between the original center of the slider arc and the new center of the slider arc.
		CFArc OriginalArc;
		if( !pSlider1->GetOriginalSliderArc( OriginalArc ) )
			return false;

		CFPoint Offset = LinkArc.GetCenter() - OriginalArc.GetCenter();

		// Caluclate the new unrotated location of the common connector and get the angle to the new location.

		CFPoint OrignalPoint = pCommonConnector->GetOriginalPoint();
		OrignalPoint += Offset;
		double Angle = GetAngle( LinkArc.GetCenter(), Intersect, OrignalPoint );

		// Cheat and move the common connector to the exact intersection location
		// in case the math caused it to be off a tiny bit. It shouldn't happen
		// but it looks like it when running some simulations.

		pCommonConnector->MovePoint( Intersect );

		/*
		 * Now move and rotate the other link so that it passes through its
		 * sliders and meets with the new common connector location. The angle
		 * of rotation is the original angle we figured out earlier in the process
		 * and the rotation happens around the common connector.
		 */

		pCommonConnector->SetRotationAngle( -Angle );
		if( !pOtherLink->RotateAround( pCommonConnector ) )
			return false;

		/*
		 * Do bounds checking of the sliding connectors on the sliding arc.
		 * This is easier to do now than earlier. Use the new arc location.
		 */

		if( !pActualSlider1->GetTempSliderArc( LinkArc ) )
		{
			pLink->ResetMoveCount();
			return false;
		}
		CFArc LinkArc2;
		if( !pActualSlider2->GetTempSliderArc( LinkArc2 ) )
		{
			pLink->ResetMoveCount();
			return false;
		}

		if( !LinkArc.PointOnArc( pActualSlider1->GetPoint() ) )
		{
			pSlider1->SetPositionValid( false );
			pSlider1->SetSimulationError( CElement::RANGE );
			pLink->ResetMoveCount();
			return false;
		}

		if( !LinkArc2.PointOnArc( pActualSlider2->GetPoint() ) )
		{
			pSlider2->SetPositionValid( false );
			pSlider2->SetSimulationError( CElement::RANGE );
			pLink->ResetMoveCount();
			return false;
		}

		return true;
	}

	bool JoinToSlideLink( CLink *pLink, CConnector *pFixedConnector, CConnector *pCommonConnector, CLink *pOtherLink )
	{
		/*
		 * Rotate the "this" Link and slide the other link so that they are still
		 * connected. The "this" Link has only one connection that is in a new
		 * temp location, pFixedConnector, and the other Link needs to have two
		 * fixed sliding connectors that it must slide through.
		 *
		 * The "this" Link will not have proper temp locations for all
		 * connectors yet, only the fixed one. It is in an odd screwed up state
		 * at the moment but will be fixed shortly.
		 *
		 * SPECIAL CASE: If this link IS pOtherLink link then we are only trying
		 * to slide this link to a new position based on slider pCommonConnector
		 * ( which is a slider but not one of the sliders that we are sliding ON).
		 * The common connector has limits that are fixed and we can move this
		 * link to a new position without changing any other link.
		 */

		if( pCommonConnector == 0 )
			return false;

		bool bOtherLinkOnly = ( pLink == pOtherLink && pFixedConnector == 0 );

		CConnector *pLimit1 = 0;
		CConnector *pLimit2 = 0;
		CConnector *pSlider1 = 0;
		CConnector *pSlider2 = 0;
		bool bOtherLinkHasSliders;

		// Double check that it can slide and get the limits for that slide.
		if( !pOtherLink->CanOnlySlide( &pLimit1, &pLimit2, &pSlider1, &pSlider2, &bOtherLinkHasSliders ) )
			return false;

		if( pSlider1->GetSlideRadius() != pSlider2->GetSlideRadius() )
			return false;

		/*
		 * Test for the case where the path of the slide is not linear and handle
		 * it in another function.
		 */

		if( pSlider1->GetSlideRadius() != 0 )
			return JoinToSlideLinkSpecial( pLink, pFixedConnector, pCommonConnector, pOtherLink );

		/*
		 * IMPORTANT...
		 * bOtherLinkHasSliders tells us if the other link is the link with the
		 * sliders or if the other link is the link with the limits. This changes
		 * how the valid range of motion for the common connector is calculated.
		 */

		/*
		 * Check to make sure that we are not trying to position this link using a
		 * common connector that is also one of the sliders that is on the "track"
		 * because the slider is then limiting the movement, not defining it.
		 */
		if( pCommonConnector == pSlider1 || pCommonConnector == pSlider2 )
			return false;

		if( bOtherLinkHasSliders )
		{
			SWAP( pSlider1, pLimit1 );
			SWAP( pSlider2, pLimit2 );
		}

		/*
		 * Take the angle and distance from pLimit1 to the common connector. Add
		 * the slider change angle. Get a point that is offset from the slider
		 * originally closest to pLimit2 (yes, pLimit2) using the new angle and
		 * distance. This is one end of a segment that limits the location of the
		 * common connector.
		 *
		 * When the sliders are on the other link, the sliders and limits are swapped
		 * but the code works the same otherwise.
		 */
		bool bSlidersAreMoving = pSlider1->HasLink( pLink );

		double Angle = GetAngle( pSlider1->GetPoint(), pSlider2->GetPoint(), pSlider1->GetOriginalPoint(), pSlider2->GetOriginalPoint() );
		double TempAngle = GetAngle( pLimit1->GetOriginalPoint(), pCommonConnector->GetOriginalPoint() ) + Angle;
		double TempDistance = Distance( pLimit1->GetOriginalPoint(), pCommonConnector->GetOriginalPoint() );
		CFPoint EndPoint1; EndPoint1.SetPoint( pSlider1->GetPoint(), TempDistance, TempAngle );
		TempAngle = GetAngle( pLimit2->GetOriginalPoint(), pCommonConnector->GetOriginalPoint() ) + Angle;
		TempDistance = Distance( pLimit2->GetOriginalPoint(), pCommonConnector->GetOriginalPoint() );
		CFPoint EndPoint2; EndPoint2.SetPoint( pSlider2->GetPoint(), TempDistance, TempAngle );

		/*
		 * There is now a segment that is the limit for the common connector.
		 * Find where we can rotate the current link so that it intersects
		 * that segment. Abort if it misses the segment.
		 *
		 * Move the current link and the other link to meet at the new common
		 * connector. The change in angle is the same angle change needed here
		 * to rotate the other link before offsetting it to the new common
		 * connector location.
		 *
		 * SPECIAL CASE: If the limits of the slider pCommonConnector are fixed
		 * then the common connector location will be defined by a line
		 * intersection and not a circle intersection.
		 */

 		CFPoint Intersect;
 		CFPoint Intersect2;
 		bool bHit = false;
 		bool bHit2 = false;

 		CFLine LimitLine( EndPoint1, EndPoint2 );

		if( bOtherLinkOnly )
		{
			CConnector *pTempLimit1 = 0;
			CConnector *pTempLimit2 = 0;
			if( !pCommonConnector->GetSlideLimits( pTempLimit1, pTempLimit2 ) )
				return false;

			if( pCommonConnector->GetSlideRadius() == 0 )
			{
				bool bOnSegments = false;
				if( !Intersects( pTempLimit1->GetTempPoint(), pTempLimit2->GetTempPoint(),
								 EndPoint1, EndPoint2, Intersect, 0, &bOnSegments )
					|| !bOnSegments )
				{
					return false;
				}
 				bHit = true;
			}
			else
			{
				CFArc TheArc;
				if( !pCommonConnector->GetTempSliderArc( TheArc ) )
					return false;

				Intersects( LimitLine, TheArc, Intersect, Intersect2, bHit, bHit2, false, false );

				if( !bHit && !bHit2 )
					return false;

				if( !TheArc.PointOnArc( Intersect2 ) )
				{
					Intersect2 = Intersect;
					if( !TheArc.PointOnArc( Intersect ) )
						return false;
				}
			}
		}
		else
		{
			if( pFixedConnector == 0 )
				return false;

			double r = Distance( pFixedConnector->GetOriginalPoint(), pCommonConnector->GetOriginalPoint() );
			r = pLink->GetLinkLength( pFixedConnector, pCommonConnector );
 			CFCircle Circle( pFixedConnector->GetTempPoint(), r );
			Intersects( LimitLine, Circle, Intersect, Intersect2, bHit, bHit2, false, false );
 		}

		if( !bHit && !bHit2 )
			return false;

		if( !bHit2 )
			Intersect2 = Intersect;
		else if( !bHit )
			Intersect = Intersect2;

		double d1 = Distance( pCommonConnector->GetTempPoint(), Intersect );
		double d2 = Distance( pCommonConnector->GetTempPoint(), Intersect2 );

		if( d2 < d1 )
			Intersect = Intersect2;

		if( !bOtherLinkOnly )
		{
			double TempAngle = GetAngle( pFixedConnector->GetTempPoint(), Intersect, pFixedConnector->GetOriginalPoint(), pCommonConnector->GetOriginalPoint() );

			TempAngle = GetClosestAngle( pFixedConnector->GetRotationAngle(), TempAngle );
			pFixedConnector->SetRotationAngle( TempAngle );
			if( !pLink->RotateAround( pFixedConnector ) )
				return false;

			pLink->IncrementMoveCount( -1 ); // Don't count the common connector twice.
		}

		// Cheat and move the common connector to the exact intersection location
		// in case the math caused it to be off a tiny bit. It shouldn't happen
		// but it looks like it when running some simulations.

		pCommonConnector->MovePoint( Intersect );

		/*
		 * Now move and rotate the other link so that it passes through its
		 * sliders and meets with the new common connector location. The angle
		 * of rotation is the original angle we figured out earlier in the process
		 * and the rotation happens around the common connector.
		 */

		Angle = GetClosestAngle( pCommonConnector->GetRotationAngle(), Angle );
		pCommonConnector->SetRotationAngle( Angle );
		if( !pOtherLink->RotateAround( pCommonConnector ) )
			return false;

		return true;
	}

	bool SlideToLink( CLink* pLink, CConnector *pFixedConnector, CConnector *pSlider, CConnector *pLimit1, CConnector *pLimit2 )
	{
		/*
		 * Rotate the "this" Link so that the slider is on the segment between
		 * the limit connectors. The other link is left unmodified AT THIS TIME.
		 * The slide path may be an arc so move and rotate the slider to be on the arc.
		 *
		 * The "this" Link will not have proper temp locations for all
		 * connectors yet, only the fixed one. It is in an odd screwed up state
		 * at the moment but will be fixed shortly.
		 */

		double r;   // = Distance( pFixedConnector->GetOriginalPoint(), pSlider->GetOriginalPoint() );
 		r = pLink->GetLinkLength( pFixedConnector, pSlider );
 		CFCircle Circle( pFixedConnector->GetTempPoint(), r );

		CFPoint Intersect;
		CFPoint Intersect2;

		if( pSlider->GetSlideRadius() == 0 )
		{
 			CFLine Line( pLimit1->GetTempPoint(), pLimit2->GetTempPoint() );

 			bool bHit = false;
 			bool bHit2 = false;
			Intersects( Line, Circle, Intersect, Intersect2, bHit, bHit2, false, false );

			if( !bHit && !bHit2 )
			{
				return false;
			}

			if( !bHit2 )
				Intersect2 = Intersect;
			else if( !bHit )
				Intersect = Intersect2;
		}
		else
		{
			CFArc TheArc;
			if( !pSlider->GetTempSliderArc( TheArc ) )
				return false;

			if( !TheArc.ArcIntersection( Circle, &Intersect, &Intersect2 ) )
				return false;
		}

		// Try to give the point momentum by determining where it would be if
		// it moved from a previous point through it's current point to some
		// new point. Use that new point as the basis for selecting the new
		// location.

		CFLine TestLine( pSlider->GetPreviousPoint(), pSlider->GetPoint() );
		TestLine.SetLength( TestLine.GetLength() * ( m_bUseIncreasedMomentum ? MOMENTUM : NO_MOMENTUM ) );
		CFPoint SuggestedPoint = TestLine.GetEnd();


		double d1 = Distance( SuggestedPoint /*pCommonConnector->GetPoint()*/, Intersect );
		double d2 = Distance( SuggestedPoint /*pCommonConnector->GetPoint()*/, Intersect2 );

		if( d2 < d1 )
			Intersect = Intersect2;
		
		double Angle = GetAngle( pFixedConnector->GetTempPoint(), Intersect, pFixedConnector->GetOriginalPoint(), pSlider->GetOriginalPoint() );
		Angle = GetClosestAngle( pFixedConnector->GetRotationAngle(), Angle );
		pFixedConnector->SetRotationAngle( Angle );
		if( !pLink->RotateAround( pFixedConnector ) )
			return false;

		// Stretch or compress the link just a tiny bit to ensure that the end of it
		// is actually at the intersection point. Otherwise there are some numeric
		// issues that cause the picture of the mechanism to look a tiny bit off.

		pSlider->MovePoint( Intersect );
		DebugItemList.AddTail( new CDebugItem(pSlider->GetTempPoint(), RGB( 0, 255, 0 ) ) );

		return true;
	}

	bool SlideToSlider( CLink *pLink, CConnector *pTargetConnector, CConnector *pTargetLimit1, CConnector *pTargetLimit2 )
	{
		if( pLink->IsFixed() )
			return false;

		if( !pTargetConnector->IsFixed() )
			return false;
		
		if( !pTargetConnector->IsSlider() )
			return false;

		/*
		 * Slide the link so that it's slide limits line up through the fixed sliding connector.
		 */

		CConnector *pOther1 = 0;
		CConnector *pOther2 = 0;
		CConnector *pThis1 = 0;
		CConnector *pThis2 = 0;
		CConnector *pActualSlider1 = 0;
		CConnector *pActualSlider2 = 0;
		bool bOtherLinkHasSliders;

		// Double check that it can slide and get the limits for that slide.
		// Note that "pOther_" and "pThis_" pointers are not correct right after this call and
		// they might refer to the wrong link until swapped later.
		if( !pLink->CanOnlySlide( &pOther1, &pOther2, &pThis1, &pThis2, &bOtherLinkHasSliders ) )
			return false;

		// Save actual sliders and limits for error checking later.
		CConnector *pLimit1 = pOther1;
		CConnector *pLimit2 = pOther2;
		CConnector *pSlider1 = pThis1;
		CConnector *pSlider2 = pThis2;

		if( !bOtherLinkHasSliders )
		{
			// Swap so that the connector pointers point to connectors on the other link
			// and on this link are refering to the correct link. it does not matter which ones are sliders
			// and which ones are limits.
			SWAP( pThis1, pOther1 );
			SWAP( pThis2, pOther2 );
		}

		// Check to see if the slider that positions the link is part of the link. This is handled by other
		// code elsewhere so leave this function.
		if( pTargetConnector->IsSharingLink( pOther1 ) )
			return false;

		// Get a line through the other sliders or limits in their original location.
		// This is used to figure out the relationship between the slide control path of this
		// link and the one connector that is moving this link. The start of this line is moved
		// further out from the line segment to ensure that the intersection of the two line (done below)
		// is always on the segment now and later when the lines are being used for positioning.
		CFLine SlideLine( pOther1->GetOriginalPoint(), pOther2->GetOriginalPoint() );
		// Get the new version of this line.
		CFLine NewSlideLine( pOther1->GetTempPoint(), pOther2->GetTempPoint() );

		// Get the change in angle from the original to the new slide line to allow
		// the new connection line angle to be adjusted.
		double ChangeAngle = NewSlideLine.GetAngle() - SlideLine.GetAngle();
		ChangeAngle = GetClosestAngle( SlideLine.GetAngle(), ChangeAngle );

		// Get a line through the connector slider limits in their original location. This is
		// the line that controls the location of the link.
		CFLine ConnectedLine( pTargetLimit1->GetOriginalPoint(), pTargetLimit2->GetOriginalPoint() );
		// Get a new version of this line using the change angle of the slide path.
		CFLine NewConnectedLine = ConnectedLine;
		NewConnectedLine -= ConnectedLine.GetStart();
		NewConnectedLine += pTargetConnector->GetPoint();
		NewConnectedLine.m_End.RotateAround( NewConnectedLine.m_Start, ChangeAngle );


		// Get the intersection of the connected line and the slide path line.
		CFPoint Intersect;
		if( !Intersects( ConnectedLine, SlideLine, Intersect ) )
			return false;

		//DebugItemList.AddTail( new CDebugItem(NewConnectedLine, RGB( 0, 255, 0 ) ) );
		//DebugItemList.AddTail( new CDebugItem(Intersect, RGB( 0, 0, 255 ) ) );

		// Get the new intersection of the new connected line and the new slide path line.
		CFPoint NewIntersect;
		if( !Intersects( NewConnectedLine, NewSlideLine, NewIntersect ) )
			return false;

		/*
		 * Create a line from the slide line but make it start a long ways off. Then,
		 * get the distance along it to the intersect point. Do the same for the new
		 * slide line. Then determine the diffent distances along that line to the pThis1
		 * connector location. Use the change to determine the new location of that connector
		 * and finish the calculations by moving and rotating this link based on that new
		 * pThis12 connector location. 
		 */

		static const double StartAdjustment = -1000000.0;
		CFLine Temp = SlideLine;
		Temp.MoveEnds( StartAdjustment, 0 );
		CFLine NewTemp = NewSlideLine;
		NewTemp.MoveEnds( StartAdjustment, 0 );

		double DistanceToIntersect = Distance( Temp.GetStart(), Intersect );
		double DistanceTopThis1 = Distance( Temp.GetStart(), pThis1->GetOriginalPoint() );
		double Change = DistanceToIntersect - DistanceTopThis1;
		double DistanceToNewIntersect = Distance( NewTemp.GetStart(), NewIntersect );
		DistanceToNewIntersect -= Change;
		NewTemp.SetLength( DistanceToNewIntersect );

		// NewTemp now ends at the expected location of the pThis1 connector.


		pThis1->SetRotationAngle( ChangeAngle );
		pThis1->MovePoint( NewTemp.m_End );
		pLink->RotateAround( pThis1 );

		// Error checking.
		CFLine SlideLimitLine( pLimit1->GetTempPoint(), pLimit2->GetTempPoint() );
		CFPoint TestPoint = pSlider1->GetTempPoint();
		if( !TestPoint.SnapToLine( SlideLimitLine, true ) )
		{
			pSlider1->SetPositionValid( false );
			pSlider1->SetSimulationError( CElement::RANGE );
			return false;
		}
		TestPoint = pSlider2->GetTempPoint();
		if( !TestPoint.SnapToLine( SlideLimitLine, true ) )
		{
			pSlider2->SetPositionValid( false );
			pSlider2->SetSimulationError( CElement::RANGE );
			return false;
		}




		/////// JUST FOR DEBUGGING
		//pLink->SetTempFixed( true );

		/*

		If not on the line, return false

		Hmmm, figure out how far to slide the link to get it to the sliding connector

		*/

		return true;
	}

	bool MoveToSlider( CLink *pLink, CConnector *pFixedConnector, CConnector *pSlider )
	{
		/*
		 * Rotate the "this" Link so that the slider it on the segment between
		 * the limit connectors. The other link is fixed and is not changed.
		 *
		 * The "this" Link will not have proper temp locations for all
		 * connectors yet, only the fixed one. It is in an odd screwed up state
		 * at the moment but will be fixed shortly.
		 */

 		CFCircle Circle( pFixedConnector->GetTempPoint(), pSlider->GetTempPoint() );
		Circle.r = Distance( pSlider->GetTempPoint(), pFixedConnector->GetTempPoint() );
		//DebugItemList.AddTail( new CDebugItem( CFArc( Circle ), RGB( 255, 0, 0 ) ) );

		CFPoint Offset = pFixedConnector->GetTempPoint() - pFixedConnector->GetOriginalPoint();

 		CFPoint Intersect;
 		CFPoint Intersect2;

		if(  pSlider->GetSlideRadius() == 0 )
		{
 			CConnector *pLimit1 = 0;
 			CConnector *pLimit2 = 0;
 			pSlider->GetSlideLimits( pLimit1, pLimit2 );

 			// The limits need to be offset by the offset of the fixed connector
 			// before they can be used. Offset the limit line to accomplish this.

 			CFLine LimitLine( pLimit1->GetOriginalPoint(), pLimit2->GetOriginalPoint() );
 			LimitLine += Offset;

 			bool bHit = false;
 			bool bHit2 = false;

			Intersects( LimitLine, Circle, Intersect, Intersect2, bHit, bHit2, false, false );

			if( !bHit && !bHit2 )
				return false;

			if( !bHit2 )
				Intersect2 = Intersect;
			else if( !bHit )
				Intersect = Intersect2;
		}
		else
		{
			CFArc TheArc;
			if( !pSlider->GetOriginalSliderArc( TheArc ) )
				return false;

			//DebugItemList.AddTail( new CDebugItem( TheArc, RGB( 0, 0, 255 ) ) );

			TheArc.x += Offset.x;
			TheArc.y += Offset.y;

			if( !TheArc.ArcIntersection( Circle, &Intersect, &Intersect2 ) )
				return false;

			//DebugItemList.AddTail( new CDebugItem( Intersect, RGB( 255, 0, 0 ) ) );
			//DebugItemList.AddTail( new CDebugItem( Intersect2, RGB( 0, 128, 128 ) ) );
		}

		// No momentum for this calculation.
		// The intersection point is the point of intersection if the slider were
		// in it's original location. it is not the new slider point.

		double d1 = Distance( pSlider->GetTempPoint(), Intersect );
		double d2 = Distance( pSlider->GetTempPoint(), Intersect2 );

		if( d2 < d1 )
			Intersect = Intersect2;

		double Angle = GetAngle( pFixedConnector->GetTempPoint(), Intersect, pSlider->GetTempPoint() );
		Angle = GetSmallestAngle( Angle );
		pFixedConnector->SetRotationAngle( Angle );
		if( !pLink->RotateAround( pFixedConnector ) )
			return false;

		return true;
	}

	bool RotateLink( CLink *pLink, CConnector *pFixedConnector, CConnector *pCommonConnector, CLink *pOtherToRotate, double Angle )
	{
		pFixedConnector->SetRotationAngle( Angle );
		if( !pLink->RotateAround( pFixedConnector ) )
			return false;

		// Test both links to see if they cause any other links to get mangled. Start with the pLink link then switch to the pOtherLink link.
		CLink *pTestLink = pLink;
		// for( int Link = 0; Link < 2; ++Link )
		{
			for( POSITION Position = pLink->GetConnectorList()->GetHeadPosition() ; Position != 0; )
			{
				CConnector* pTestConnector = pLink->GetConnectorList()->GetNext( Position );
				// Skip this connector if it is the fixed one or the common one.
				// We are just looking for the other connectors of the link
				if( pTestConnector == 0 || pTestConnector == pFixedConnector )
					continue;

				for( POSITION Position2 = pTestConnector->GetLinkList()->GetHeadPosition(); Position2 != 0; )
				{
					CLink *pTestLink = pTestConnector->GetLinkList()->GetNext( Position2 );
					if( pTestLink == 0 || pTestLink == pLink || pTestLink == pOtherToRotate )
						continue;

					// Check to see if this link is going to get mangled by the movement.
					CConnector *pCheckConnector = pTestLink->GetConnector( 0 );
					if( pCheckConnector == pTestConnector )
						CConnector *pCheckConnector = pTestLink->GetConnector( 1 );
					if( pCheckConnector == 0 )
						continue;

					if( !pCheckConnector->IsTempFixed() && !pCheckConnector->IsFixed() )
						continue;

					if( pTestLink->IsActuator() )
					{
						; // Not sure how to test to see if the actuator is getting stretched!
					}
					else
					{
						double d1 = Distance( pTestConnector->GetOriginalPoint(), pCheckConnector->GetOriginalPoint() );
						double d2 = Distance( pTestConnector->GetTempPoint(), pCheckConnector->GetTempPoint() );
						if( fabs( d1 - d2 ) > 0.00000001 )
						{
							pTestConnector->SetPositionValid( false );
							pLink->SetPositionValid( false );
							pLink->SetSimulationError( CElement::MANGLE );

							//DebugItemList.AddTail( new CDebugItem( pFixedConnector->GetTempPoint(), RGB( 0, 255, 0 ) ) );
							//DebugItemList.AddTail( new CDebugItem( pCommonConnector->GetTempPoint(), RGB( 0, 0, 255 ) ) );

							//DebugItemList.AddTail( new CDebugItem( pTestConnector->GetTempPoint() ) );
							//DebugItemList.AddTail( new CDebugItem( CFLine( pTestConnector->GetTempPoint(), pCheckConnector->GetTempPoint() ) ) );
							return false;
						}
					}
				}
			}
			//pTestLink = pOtherToRotate;
		}
			
		/*
		 * The movement of the current link could cause other links to "follow". They might be attached to this
		 * link and are not this link and are not the "other" link that is moving, or being moved by this one.
		 * Find those other links and mover and rotate them into position.
		 */

		for( POSITION Position = pLink->GetConnectorList()->GetHeadPosition() ; Position != 0; )
		{
			CConnector* pTestConnector = pLink->GetConnectorList()->GetNext( Position );
			if( pTestConnector == 0 || pTestConnector == pFixedConnector || pTestConnector == pCommonConnector )
				continue;
			for( POSITION Position2 = pTestConnector->GetLinkList()->GetHeadPosition(); Position2 != 0; )
			{
				CLink *pTestLink = pTestConnector->GetLinkList()->GetNext( Position2 );
				if( pTestLink == 0 || pTestLink == pLink || pTestLink == pOtherToRotate )
					continue;
				if( pTestLink->GetFixedConnectorCount() != 1 )
				{
					// The link should have two fixed connectors, the one that was fixed before the rotation that just happened
					// and the one that just got rotated. So if that link can be fully rotated to match the new location of
					// the connector that was just moved.
					if( pTestLink->GetFixedConnectorCount() > 2 )
					{
						// DO THIS LATER. MAYBE IT'S NOT NEEDED?
					}
					else
					{
						POSITION Position3 = pTestLink->GetConnectorList()->GetHeadPosition();
						while( Position3 != 0 )
						{
							CConnector *pConnector = pTestLink->GetConnectorList()->GetNext( Position3 );
							if( pConnector == 0 || pConnector == pTestConnector || !pConnector->IsFixed() )
								continue;

							double Diff = fabs( Distance( pConnector->GetOriginalPoint(), pTestConnector->GetOriginalPoint() ) - Distance( pConnector->GetTempPoint(), pTestConnector->GetTempPoint() ) );
							if( Diff < 0.0001 )
							{
								double Angle = GetAngle( pConnector->GetTempPoint(), pTestConnector->GetTempPoint(), pConnector->GetOriginalPoint(), pTestConnector->GetOriginalPoint() );
								Angle = GetClosestAngle( pConnector->GetRotationAngle(), Angle );
								if( !RotateLink( pTestLink, pConnector, pTestConnector, 0, Angle ) )
									return false;
								// It seems like this is the only place where the position of this connector is
								// being validated - so validate it.
								pTestConnector->SetPositionValid( true );
							}
							else
							{
								//pTestConnector->SetPositionValid( false );
								//return false;
							}
						}
					}
				}
			}
		}
		return true;
	}

	bool JoinToLink( CLink *pLink, CConnector *pFixedConnector, CConnector *pCommonConnector, CLink *pOtherToRotate )
	{
		/*
		 * Rotate the pLink and the other Link so that they are still
		 * connected. The pLink has only one connection that is in a new
		 * temp location, pFixedConnector, and the other Link needs to have a
		 * fixed connector of some sort too (that is not an input!).
		 *
		 * The pLink will not have proper temp locations for all
		 * connectors yet, only the fixed one. It is in an odd screwed up state
		 * at the moment but will be fixed shortly.
		 */

		CConnector *pOtherFixedConnector = pOtherToRotate->GetFixedConnector();
		if( pOtherFixedConnector == 0 )
		{
			if( pOtherToRotate->GetFixedConnectorCount() == 0 )
				return true; // Maybe it can be dealt with later in processing by another chain of Links.
			else
				return false;
		}
		if( pOtherFixedConnector->IsInput() )
			return false;

	//	CConnector* pCommonConnector = GetCommonConnector( this, pOtherToRotate );
	//	if( pCommonConnector == 0 )
	//		return false;

		// "this" link could be connected to something that can't move. Check to make
		// sure but don't otherwise move anything else. Check all Links connected to
		// this other than the pOtherToRotate Link and if any of them have fixed connectors,
		// return immediately.
		for( POSITION Position = pLink->GetConnectorList()->GetHeadPosition(); Position != 0 && 0; )
		{
			CConnector* pTestConnector = pLink->GetConnectorList()->GetNext( Position );
			if( pTestConnector == 0 || pTestConnector == pFixedConnector || pTestConnector == pCommonConnector )
				continue;
			for( POSITION Position2 = pTestConnector->GetLinkList()->GetHeadPosition(); Position2 != 0; )
			{
				CLink *pTestLink = pTestConnector->GetLinkList()->GetNext( Position2 );
				if( pTestLink == 0 || pTestLink == pLink || pTestLink == pOtherToRotate )
					continue;
				if( pTestLink->GetFixedConnectorCount() != 0 )
				{
					pCommonConnector->SetPositionValid( false );
					pTestLink->SetSimulationError( CElement::MANGLE );
					pLink->SetSimulationError( CElement::MANGLE );
					return false;
				}
			}
		}

		// pOtherToRotate could be connected to something that can't move. Check to make
		// sure but don't otherwise move anything else. Check all Links connected to
		// pOtherToRotate other than this Link and if any of them have fixed connectors,
		// return immediately.
		for( POSITION Position = pOtherToRotate->GetConnectorList()->GetHeadPosition(); Position != 0 && 0; )
		{
			CConnector* pTestConnector =  pOtherToRotate->GetConnectorList()->GetNext( Position );
			if( pTestConnector == 0 || pTestConnector == pOtherFixedConnector || pTestConnector == pFixedConnector )
				continue;
			for( POSITION Position2 = pTestConnector->GetLinkList()->GetHeadPosition(); Position2 != 0; )
			{
				CLink *pTestLink = pTestConnector->GetLinkList()->GetNext( Position2 );
				if( pTestLink == 0 || pTestLink == pLink || pTestLink == pOtherToRotate )
					continue;
				if( pTestLink->GetFixedConnectorCount() != 0 )
				{
					pCommonConnector->SetPositionValid( false );
					pTestLink->SetSimulationError( CElement::MANGLE );

					pOtherToRotate->SetSimulationError( CElement::MANGLE );
					return false;
				}
			}
		}

		double r1 = pLink->GetLinkLength( pFixedConnector, pCommonConnector );
		double r2 = pOtherToRotate->GetLinkLength( pOtherFixedConnector, pCommonConnector );

		CFCircle Circle1( pFixedConnector->GetTempPoint(), r1 );
		CFCircle Circle2( pOtherFixedConnector->GetTempPoint(), r2 );

		CFPoint Intersect;
		CFPoint Intersect2;

		if( !Circle1.CircleIntersection( Circle2, &Intersect, &Intersect2 ) )
		{
			pLink->SetPositionValid( false );
			pLink->SetSimulationError( CElement::MANGLE );
			pOtherToRotate->SetPositionValid( false );
			pOtherToRotate->SetSimulationError( CElement::MANGLE );
			return false;
		}

		// Try to give the point momentum by determining where it would be if
		// it moved from a previous point through it's current point to some
		// new point. Use that new point as the basis for selecting the new
		// location.

		CFLine Line( pCommonConnector->GetPreviousPoint(), pCommonConnector->GetPoint() );
		Line.SetLength( Line.GetLength() * ( m_bUseIncreasedMomentum ? MOMENTUM : NO_MOMENTUM ) );
		CFPoint SuggestedPoint = Line.GetEnd();

		double d1 = Distance( SuggestedPoint /*pCommonConnector->GetPoint()*/, Intersect );
		double d2 = Distance( SuggestedPoint /*pCommonConnector->GetPoint()*/, Intersect2 );

		if( d2 < d1 )
			Intersect = Intersect2;

		double Angle = GetAngle( pFixedConnector->GetTempPoint(), Intersect, pFixedConnector->GetOriginalPoint(), pCommonConnector->GetOriginalPoint() );
		Angle = GetSmallestAngle( Angle );
		pFixedConnector->SetRotationAngle( Angle );


		if( !RotateLink( pLink, pFixedConnector, pCommonConnector, pOtherToRotate, Angle ) )
			return false;



		//if( !pLink->RotateAround( pFixedConnector ) )
		//	return false;
		pCommonConnector->SetTempFixed( false ); // needed so it can be rotated again.
		pLink->IncrementMoveCount( -1 ); // Don't count that one twice.
		Angle = GetAngle( pOtherFixedConnector->GetTempPoint(), Intersect, pOtherFixedConnector->GetOriginalPoint(), pCommonConnector->GetOriginalPoint() );
		Angle = GetClosestAngle( pOtherToRotate->GetRotationAngle(), Angle );
		pOtherFixedConnector->SetRotationAngle( Angle );

		
		if( !RotateLink( pOtherToRotate, pOtherFixedConnector, pCommonConnector, pLink, Angle ) )
			return false;


		//if( !pOtherToRotate->RotateAround( pOtherFixedConnector ) )
		//	return false;

		return true;
	}


	int GetLinkTriangles( CLink *pLink, CList<LinkList*> &Triangles )
	{
		// return a list of link triangles that contain the pLink parameter.
		int Count = 0;

		CConnector *pBaseConnector = pLink->GetFixedConnector();

		/*
		 * Examine every combination of two links that are connected to pLink. See if any of
		 * them connect to each other. If so, that is a link triangle.
		 */

		// get a list of all links connected to pLink.
		LinkList ConnectedLinks;
		POSITION Position = pLink->GetConnectorList()->GetHeadPosition();
		while( Position != 0 )
		{
			CConnector *pConnector = pLink->GetConnectorList()->GetNext( Position );
			if( pConnector == 0 || pConnector->GetLinkCount() <= 1 )
				continue;
			POSITION Position2 = pConnector->GetLinkList()->GetHeadPosition();
			while( Position2 != 0 )
			{
				CLink *pSaveLink = pConnector->GetLinkList()->GetNext( Position2 );
				if( pSaveLink == 0 || pSaveLink == pLink )
					continue;
				ConnectedLinks.AddTail( pSaveLink );
			}
		}

		// Look at each pair of links to see if they connect to each other.
		Position = ConnectedLinks.GetHeadPosition();
		while( Position != 0 )
		{
			CLink *pFirstLink = ConnectedLinks.GetNext( Position );
			if( pFirstLink == 0 || pFirstLink == pLink )
				continue;
			POSITION Position2 = Position;
			while( Position2 != 0 )
			{
				CLink *pSecondLink = ConnectedLinks.GetNext( Position2 );
				if( pSecondLink == 0 )
					continue;
				CConnector *pNewConnector = CLink::GetCommonConnector( pFirstLink, pSecondLink );
				CConnector *pTestConnector = CLink::GetCommonConnector( pLink, pFirstLink );
				if( pNewConnector != 0 && pNewConnector != pTestConnector )
				{
					if( pNewConnector->IsFixed() )
						continue;
					// Found a triangle!
					LinkList *pLinkList = new LinkList;
					pLinkList->AddTail( pLink );
					pLinkList->AddTail( pFirstLink );
					pLinkList->AddTail( pSecondLink );
					Triangles.AddTail( pLinkList );
					++Count;
				}
			}
		}

		return Count;
	}

	bool SimulateLinkTriangle( LinkList *pLinkList, ConnectorList *pConnectors, CConnector *pFixedConnector, CConnector *pCommonConnector, double *pResultDistance, CConnector *ThreeConnectors[3], double &ThreeConectorsAngle )
	{
		if( pResultDistance == 0 )
			return false;

		// If none of the links are actuators, just find the orignal distance from the fixed connector to the common connector.
		POSITION Position = pLinkList->GetHeadPosition();
		int ActuatorCount = 0;
		while( Position != 0 )
		{
			CLink *pLink = pLinkList->GetNext( Position );
			if( pLink == 0 )
				return false;
			if( pLink->IsActuator() )
				++ActuatorCount; 
		} 
		if( ActuatorCount == 0 )
		{
			*pResultDistance = Distance( pFixedConnector->GetOriginalPoint(), pCommonConnector->GetOriginalPoint() );
			return true;
		}

		/*
		 * All link triangles are made from three links. The triangle has two meaningful connections to the
		 * rest of the mechanism: one connection is where two of the links connect to each other and the
		 * other conection is to another link that helps determine the link triangle position. At this point,
		 * the triangle is simulated to find the distance between those two meaningful connections. The simulation
		 * of the triangle is discarded once that distance is found.
		 *
		 * the first step is to find the two meaningful connections and find out which one of them is connected to
		 * just one triangle link, with the other connected to two of the links. Then the link triangle can be simulated.
		 *
		 * The calling function already tell us about the two meaninful connections. Just figure out which one to use
		 * to start the simulation of the link triangle.
		 */

		if( pLinkList->GetCount() != 3 )
			return false;

		CLink *pLink1 = 0;
		CConnector *pStartConnector = 0;

		int CountAtFixed = 0;
		Position = pLinkList->GetHeadPosition();
		while( Position != 0 )
		{
			CLink *pLink = pLinkList->GetNext( Position );
			if( pLink == 0 )
				return false;
			if( pLink->GetConnectorList()->Find( pFixedConnector ) != 0 )
			{
				++CountAtFixed;
				pLink1 = pLink;
				pStartConnector = pFixedConnector;
				if( CountAtFixed == 2 )
					break;
			}
		}
		if( CountAtFixed != 1 )
		{
			// The fixed connection has more than one of the triangle links connected there.

			int CountAtCommon = 0;
			Position = pLinkList->GetHeadPosition();
			while( Position != 0 )
			{
				CLink *pLink = pLinkList->GetNext( Position );
				if( pLink == 0 )
					return false;
				if( pLink->GetConnectorList()->Find( pCommonConnector ) != 0 )
				{
					++CountAtFixed;
					pLink1 = pLink;
					pStartConnector = pCommonConnector;
					if( CountAtCommon == 2 )
						return false; // Link triangle is not a link triangle? Weird.
				}
			}
		}

		/*
		 * pStartLink is now pointing to the link that is the only link at one of the two meaningful connections.
		 * Do the simuation arund it after getting pointers to the rest of the link triangle parts.
		 */

		CConnector *pEndConnector = pStartConnector == pFixedConnector ? pCommonConnector : pFixedConnector;

		CLink *pLink2 = 0;
		CLink *pLink3 = 0;

		Position = pLinkList->GetHeadPosition();
		while( Position != 0 )
		{
			CLink *pLink = pLinkList->GetNext( Position );
			if( pLink == 0 )
				return false;
			if( pLink == pLink1 )
				continue;
			if( pLink2 == 0 )
				pLink2 = pLink;
			else if( pLink3 == 0 )
				pLink3 = pLink;
		}

		CConnector *pConnector2 = CLink::GetCommonConnector( pLink1, pLink2 );
		CConnector *pConnector3 = CLink::GetCommonConnector( pLink1, pLink3 );
		if( pConnector2 == 0 || pConnector3 == 0 || pConnector2 == pCommonConnector || pConnector3 == pCommonConnector || pConnector2 == pFixedConnector || pConnector3 == pFixedConnector)
			return false;

		double Distance1 = pLink2->IsActuator() ? pLink2->GetActuatedConnectorDistance( pConnector2, pEndConnector ) : Distance( pConnector2->GetOriginalPoint(), pEndConnector->GetOriginalPoint() );
		double Distance2 = pLink3->IsActuator() ? pLink3->GetActuatedConnectorDistance( pConnector3, pEndConnector ) : Distance( pConnector3->GetOriginalPoint(), pEndConnector->GetOriginalPoint() );

		// Do the circle intersection thing. Note that there is no momentum for this because the previous simulation points are probably not appropriate for this.
		CFCircle Circle1( pConnector2->GetOriginalPoint(), Distance1 );
		CFCircle Circle2( pConnector3->GetOriginalPoint(), Distance2 );

		CFPoint Intersect;
		CFPoint Intersect2;

		if( !Circle1.CircleIntersection( Circle2, &Intersect, &Intersect2 ) )
			return false;

		// Pick the intersection closest to the original point of the given connector - not sure if this will always work.
		double d1 = Distance( pEndConnector->GetOriginalPoint(), Intersect );
		double d2 = Distance( pEndConnector->GetOriginalPoint(), Intersect2 );

		if( d2 < d1 )
			Intersect = Intersect2;

		*pResultDistance = Distance( pStartConnector->GetOriginalPoint(), Intersect );

		ThreeConnectors[0] = pStartConnector;
		ThreeConnectors[1] = pConnector2;
		ThreeConnectors[2] = pEndConnector;
		ThreeConectorsAngle = GetAngle( pConnector2->GetOriginalPoint(), pStartConnector->GetOriginalPoint(), Intersect );

		return true;
	}

	bool JoinToLink( LinkList *pLinkList, ConnectorList *pConnectors, CConnector *pFixedConnector, CConnector *pCommonConnector, CLink *pOtherToRotate )
	{
		/* 
		 * Do a little bit of simulation on the link triangle if it has one or two actuators.
		 * Find the new common connector location and then adjust the link triangle and the
		 * other link to put them in their new positions and change the triangle shape if needed.
		 */

		CConnector *pOtherFixedConnector = pOtherToRotate->GetFixedConnector();
		if( pOtherFixedConnector == 0 )
		{
			if( pOtherToRotate->GetFixedConnectorCount() == 0 )
				return true; // Maybe it can be dealt with later in processing by another chain of Links.
			else
				return false;
		}
		if( pOtherFixedConnector->IsInput() )
			return false;

		// "this" link could be connected to something that can't move. Check to make
		// sure but don't otherwise move anything else. Check all Links connected to
		// this other than the pOtherToRotate Link and if any of them have fixed connectors,
		// return immediately.
		for( POSITION Position = pConnectors->GetHeadPosition(); Position != 0; )
		{
			CConnector* pTestConnector = pConnectors->GetNext( Position );
			if( pTestConnector == 0 || pTestConnector == pFixedConnector || pTestConnector == pFixedConnector )
				continue;
			for( POSITION Position2 = pTestConnector->GetLinkList()->GetHeadPosition(); Position2 != 0; )
			{
				CLink *pTestLink = pTestConnector->GetLinkList()->GetNext( Position2 );
				if( pTestLink == 0 || pTestLink == pOtherToRotate || pLinkList->Contains( pTestLink ) )
					continue;
				if( pTestLink->GetFixedConnectorCount() != 0 )
				{
					pCommonConnector->SetPositionValid( false );
					pTestLink->SetSimulationError( CElement::MANGLE );
					return false;
				}
			}
		}

		// pOtherToRotate could be connected to something that can't move. Check to make
		// sure but don't otherwise move anything else. Check all Links connected to
		// pOtherToRotate other than this Link and if any of them have fixed connectors,
		// return immediately.
		for( POSITION Position = pOtherToRotate->GetConnectorList()->GetHeadPosition(); Position != 0; )
		{
			CConnector* pTestConnector =  pOtherToRotate->GetConnectorList()->GetNext( Position );
			if( pTestConnector == 0 || pTestConnector == pOtherFixedConnector || pTestConnector == pFixedConnector )
				continue;
			for( POSITION Position2 = pTestConnector->GetLinkList()->GetHeadPosition(); Position2 != 0; )
			{
				CLink *pTestLink = pTestConnector->GetLinkList()->GetNext( Position2 );
				if( pTestLink == 0 || pTestLink == pOtherToRotate || pLinkList->Contains( pTestLink ) )
					continue;
				if( pTestLink->GetFixedConnectorCount() != 0 )
				{
					pCommonConnector->SetPositionValid( false );
					pOtherToRotate->SetSimulationError( CElement::MANGLE );
					return false;
				}
			}
		}

		/* 
		 * Check for the actuators in the triangle. If there are any, take some extra steps to find the
		 * distance from the fixed conector to the new end of the triangle.
		 
		   MAYBE is it possible to just move the common connector and let the other simulator code see it later
		   and finish the simulation of the triangle?
		 
		 */ 

		double DistanceToCommon = 0;
		CConnector *ThreeConnectors[3];
		double ThreeConnectorsAngle = 0;
		if( !SimulateLinkTriangle( pLinkList,  pConnectors, pFixedConnector, pCommonConnector, &DistanceToCommon, ThreeConnectors, ThreeConnectorsAngle ) )
			return false;

		double r1 = DistanceToCommon;
		double r2 = pOtherToRotate->GetActuatedConnectorDistance( pOtherFixedConnector, pCommonConnector );

		CFCircle Circle1( pFixedConnector->GetTempPoint(), r1 );
		CFCircle Circle2( pOtherFixedConnector->GetTempPoint(), r2 );

		CFPoint Intersect;
		CFPoint Intersect2;

		if( !Circle1.CircleIntersection( Circle2, &Intersect, &Intersect2 ) )
			return false;

		// Try to give the point momentum by determining where it would be if
		// it moved from a previous point through it's current point to some
		// new point. Use that new point as the basis for selecting the new
		// location.

		CFLine Line( pCommonConnector->GetPreviousPoint(), pCommonConnector->GetPoint() );
		Line.SetLength( Line.GetLength() * ( m_bUseIncreasedMomentum ? MOMENTUM : NO_MOMENTUM ) );
		CFPoint SuggestedPoint = Line.GetEnd();

		double d1 = Distance( SuggestedPoint /*pCommonConnector->GetPoint()*/, Intersect );
		double d2 = Distance( SuggestedPoint /*pCommonConnector->GetPoint()*/, Intersect2 );

		if( d2 < d1 )
			Intersect = Intersect2;

		//pCommonConnector->SetTempFixed( false ); // needed so it can be rotated again.
		double Angle = GetAngle( pOtherFixedConnector->GetTempPoint(), Intersect, pOtherFixedConnector->GetOriginalPoint(), pCommonConnector->GetOriginalPoint() );
		Angle = GetClosestAngle( pOtherFixedConnector->GetRotationAngle(), Angle );
		pOtherFixedConnector->SetRotationAngle( Angle );
		if( !pOtherToRotate->RotateAround( pOtherFixedConnector ) )
			return false;

		/*
		 * Now that the end of the link triangle has been determined and fixed, part of the link triangle itself must be simulated. This cannot
		 * be done by the next pass of the simulation code because there is a choice about the location of one of the intermediate connectors
		 * that can often be made wrong by the other code. Use the information from the triangle simulation to outright position that intermediate 
		 * connector.
		 */


		// Find the link that contains the start connector from the triangle simulation.
		// Find the link that contains the end connector from the triangle simulation.
		// Simulate and move those two links into position.

		CLink *pXLink = 0;
		CLink *pXOtherToRotate = 0;
		POSITION Position = pLinkList->GetHeadPosition();
		while( Position != 0 )
		{
			CLink *pTestLink = pLinkList->GetNext( Position );
			if( pTestLink->GetConnectorList()->Find( ThreeConnectors[0] ) != 0 && pTestLink->GetConnectorList()->Find( ThreeConnectors[1] ) != 0 )
				pXLink = pTestLink;
			else if( pTestLink->GetConnectorList()->Find( ThreeConnectors[2] ) != 0 && pTestLink->GetConnectorList()->Find( ThreeConnectors[1] ) != 0 )
				pXOtherToRotate = pTestLink;
		}

		if( pXLink == 0 || pXOtherToRotate == 0 )
			return true;






		CConnector *pXFixedConnector = ThreeConnectors[0];
		CConnector *pXOtherFixedConnector = ThreeConnectors[2];
		pCommonConnector = ThreeConnectors[1];
		if( !pXFixedConnector->IsFixed() || !pXOtherFixedConnector->IsFixed() )
			return true;

		if( pXOtherFixedConnector->IsInput() )
			return false;

		r1 = pXLink->GetActuatedConnectorDistance( pXFixedConnector, pCommonConnector );
		r2 = pXOtherToRotate->GetActuatedConnectorDistance( pXOtherFixedConnector, pCommonConnector );

		Circle1.SetCircle( pXFixedConnector->GetTempPoint(), r1 );
		Circle2.SetCircle( pXOtherFixedConnector->GetTempPoint(), r2 );

		if( !Circle1.CircleIntersection( Circle2, &Intersect, &Intersect2 ) )
			return false;

		// Unlike the normal simulation, the angle is alreaqdy known from the triangle simulation. Check for the expected angle.
		double Angle1 = GetAngle( Intersect, pXFixedConnector->GetTempPoint(), pXOtherFixedConnector->GetTempPoint() );
		double Angle2 = GetAngle( Intersect2, pXFixedConnector->GetTempPoint(), pXOtherFixedConnector->GetTempPoint() );

		double Diff1 = fabs( Angle1 - ThreeConnectorsAngle );
		double Diff2 = fabs( Angle2 - ThreeConnectorsAngle );

		if( Diff2 < Diff1 )
			Intersect = Intersect2;

		Angle = GetAngle( pXFixedConnector->GetTempPoint(), Intersect, pXFixedConnector->GetOriginalPoint(), pCommonConnector->GetOriginalPoint() );
		Angle = GetClosestAngle( pXFixedConnector->GetRotationAngle(), Angle );
		pXFixedConnector->SetRotationAngle( Angle );
		if( !pXLink->RotateAround( pXFixedConnector ) )
			return false;
		pCommonConnector->SetTempFixed( false ); // needed so it can be rotated again.
		pXLink->IncrementMoveCount( -1 ); // Don't count that one twice.
		Angle = GetAngle( pXOtherFixedConnector->GetTempPoint(), Intersect, pXOtherFixedConnector->GetOriginalPoint(), pCommonConnector->GetOriginalPoint() );
		Angle = GetClosestAngle( pXOtherToRotate->GetRotationAngle(), Angle );
		pXOtherFixedConnector->SetRotationAngle( Angle );
		if( !pXOtherToRotate->RotateAround( pXOtherFixedConnector ) )
			return false;

		return true;
	}

	int GetLinkTriangleConnectors( LinkList *pLinkList, ConnectorList &Connectors, int MaxFixedCount )
	{
		Connectors.RemoveAll();
		POSITION Position = pLinkList->GetHeadPosition();
		while( Position != 0 )
		{
			CLink *pLink = pLinkList->GetNext( Position );
			if( pLink == 0 )
				continue;
			POSITION Position2 = pLink->GetConnectorList()->GetHeadPosition();
			while( Position2 != 0 )
			{
				CConnector *pConnector = pLink->GetConnectorList()->GetNext( Position2 );
				if( pConnector == 0 )
					continue;
				POSITION Position3 = Connectors.GetHeadPosition();
				while( Position3 != 0 )
				{
					CConnector *pExistingConnector = Connectors.GetNext( Position3 );
					if( pConnector == pExistingConnector )
					{
						pConnector = 0;
						break;
					}
				}
				if( pConnector == 0 )
					continue;
				Connectors.AddTail( pConnector );
			}
		}
		return (int)Connectors.GetCount();
	}

	bool FindLinkTriangleMatch( CLink *pLink )
	{
		//return false;
		/*
		 * A link triangle is a set of three links that can be treated like a single link.
		 * there is also the possibility of any number of the links being an actuator, which is tricky,
		 * but the concept is the same. If the current link is not part of a link triangle,
		 * nothing else is done here.
		 *
		 * Find another link that is connected to the current link triangle and can be
		 * used as a basis for moving the link and teh triangle into their new postitions.
		 *
		 * The possible configurations are (AND SOME MIGHT NOT BE HANDLED HERE BUT ARE LISTED FOR REFERNCE WHILE WRITING THE CODE)
		 *   A. Other link has a fixed connector and the triangle and link can be
		 *      moved and rotated to connect to each other.
		 *   B. Other link has a sliding connector that is now fixed and the
		 *      current link can be moved and rotated to match the slider.
		 *   C. Other link has limits for one of the current link sliders and the
		 *      current link can be moved and rotated to match those limits.
		 *   !D. Other link has two sliders that are not fixed and it and the
		 *      current link can both be moved and rotated to connect properly.
		 *   E. Other link can slide through two fixed sliders and the
		 *      current both links can be moved and the current
		 *      link rotated to meet properly.
		 *   F. Found a link connected to this one. The other link has a slider
		 *		onto this link and the other link also slides on something. That
		 *		other link has at least three sliders and it needs to be slid into
		 *		place based on it's slider connection to this link.
		 *   G. connected to other link with a slider but it has no limits. The
		 *		other link should be able to rotate into position as if it was
		 *		not a link with a slider.
		 *
		 */

		CConnector *pBaseConnector = pLink->GetFixedConnector();

		CList<LinkList*> Triangles;
		int Count = GetLinkTriangles( pLink, Triangles );

		if( Count == 0 )
			return false;

		POSITION TrianglePosition = Triangles.GetHeadPosition();
		while( TrianglePosition != 0 )
		{
			LinkList *pLinks = Triangles.GetNext( TrianglePosition );
			if( pLinks == 0 )
				continue;
			ConnectorList Connectors;
			int ConnectorCount = GetLinkTriangleConnectors( pLinks, Connectors, 1 );

			if( ConnectorCount < 3 )
				continue;

			CConnector *pLimit1 = 0;
			CConnector *pLimit2 = 0;
			CConnector *pSlider1 = 0;
			CConnector *pSlider2 = 0;

			int SliderCount = 0;

			POSITION Position = Connectors.GetHeadPosition();
			while( Position != 0 )
			{
				CConnector* pConnector = Connectors.GetNext( Position );
				if( pConnector == 0 || pConnector->IsFixed() )
					continue;

				if( pConnector->IsSlider() )
				{
					// Got a slider. See if it is limited by fixed limit connectors
					// of some other link. Move and rotate to fit betwen the limits.

					CConnector *pLimit1;
					CConnector *pLimit2;
					if( !pConnector->GetSlideLimits( pLimit1, pLimit2 ) )
						continue;

					if( pLimit1->IsFixed() && pLimit2->IsFixed() )
					{
						if( pBaseConnector == 0 )
						{
//							if( pLink->CanOnlySlide() )
//							{
//								// Case F.
//								return JoinToSlideLink( pLink, 0, pConnector, pLink );
//							}
						}
						else
						{
							// Case C.
//							return SlideToLink( pLink, pBaseConnector, pConnector, pLimit1, pLimit2 );
						}

						// The slider is limited so don't do additional tests
						// below this.
						continue;
					}
				}

				// Check all Links connected to this connector until we find one
				// that has a single fixed connector. Move and rotate both to
				// meet with each other properly. Check the connected links for
				// one that can slide but not rotate to meet the current link.

				POSITION Position2 = pConnector->GetLinkList()->GetHeadPosition();
				while( Position2 != 0 && pBaseConnector != 0 )
				{
					CLink *pOtherLink = pConnector->GetLinkList()->GetNext( Position2 );
					if( pOtherLink == 0 || pOtherLink == pLink )
						continue;

					CConnector *pOtherFixedConnector = pOtherLink->GetFixedConnector();
					if( pOtherFixedConnector != 0 && !pOtherFixedConnector->IsInput() && pBaseConnector != pOtherFixedConnector )
					{
						// Case A.
						return JoinToLink( pLinks, &Connectors, pBaseConnector, pConnector, pOtherLink );
					}

					if( pOtherLink->CanOnlySlide() )
					{
						// Case E.
//						return JoinToSlideLink( pLink, pBaseConnector, pConnector, pOtherLink );
					}
				}
			}

			// Look at the sliders whose limits are part of the current link. If
			// the slider is fixed then the current link can be moved and rotated
			// to meet it.

			//pBaseConnector = pLink->GetFixedConnector();
			//if( pBaseConnector != 0 )
			//{
				for( int Index = 0; Index < pLink->GetConnectedSliderCount(); ++Index )
				{
					pLink->GetConnectedSlider( Index );
					CConnector* pSlider = pLink->GetConnectedSlider( Index );
					if( pSlider == 0 )
						continue;

					if( pSlider->IsFixed() )
					{
						// Case B.
						// Got a fixed slider. Make the current link align with it.
//						return MoveToSlider( pLink, pBaseConnector, pSlider );
					}
				}
			//}
		}

		if( Triangles.GetCount() > 0 )
		{
			POSITION Position = Triangles.GetHeadPosition();
			while( Position != 0 )
			{
				LinkList* pLinks = Triangles.GetNext( Position );
				if( pLinks != 0 )
					delete pLinks;
			}
		}

		return false;
	}

	bool FindLinksToMatch( CLink *pLink )
	{
		/*
		 * Find another link that is connected to the current link and can be
		 * used as a basis for moving the two links into their new postitions.
		 *
		 * The possible configurations are:
		 *   A. Other link has a fixed connector and the two links can be moved
		 *      and rotated to connect to each other.
		 *   B. Other link has a sliding connector that is now fixed and the
		 *      current link can be moved and rotated to match the slider.
		 *   C. Other link has limits for one of the current link sliders and the
		 *      current link can be moved and rotated to match those limits.
		 *   !D. Other link has two sliders that are not fixed and it and the
		 *      current link can both be moved and rotated to connect properly.
		 *   E. Other link can slide through two fixed sliders and the
		 *      current both links can be moved and the current
		 *      link rotated to meet properly.
		 *   F. Found a link connected to this one. The other link has a slider
		 *		onto this link and the other link also slides on something. That
		 *		other link has at least three sliders and it needs to be slid into
		 *		place based on it's slider connection to this link.
		 *   G. connected to other link with a slider but it has no limits. The
		 *		other link should be able to rotate into position as if it was
		 *		not a link with a slider.
		 *
		 */

		CConnector *pLimit1 = 0;
		CConnector *pLimit2 = 0;
		CConnector *pSlider1 = 0;
		CConnector *pSlider2 = 0;

		int SliderCount = 0;

		ConnectorList *pConnectors = pLink->GetConnectorList();

		CConnector *pBaseConnector = pLink->GetFixedConnector();

		POSITION Position = pConnectors->GetHeadPosition();
		while( Position != 0 )
		{
			CConnector* pConnector = pConnectors->GetNext( Position );
			if( pConnector == 0 || pConnector->IsFixed() )
				continue;

			if( pConnector->IsSlider() )
			{
				// Got a slider. See if it is limited by fixed limit connectors
				// of some other link. Move and rotate to fit betwen the limits.

				CConnector *pLimit1;
				CConnector *pLimit2;
				if( !pConnector->GetSlideLimits( pLimit1, pLimit2 ) )
					continue;

				if( pLimit1->IsFixed() && pLimit2->IsFixed() )
				{
					if( pBaseConnector == 0 )
					{
						if( pLink->CanOnlySlide() )
						{
							// Case F.
							return JoinToSlideLink( pLink, 0, pConnector, pLink );
						}
					}
					else
					{
						// Case C.
						return SlideToLink( pLink, pBaseConnector, pConnector, pLimit1, pLimit2 );
					}

					// The slider is limited so don't do additional tests
					// below this.
					continue;
				}
			}

			// Check all Links connected to this connector until we find one
			// that has a single fixed connector. Move and rotate both to
			// meet with each other properly. Check the connected links for
			// one that can slide but not rotate to meet the current link.

			POSITION Position2 = pConnector->GetLinkList()->GetHeadPosition();
			while( Position2 != 0 && pBaseConnector != 0 )
			{
				CLink *pOtherLink = pConnector->GetLinkList()->GetNext( Position2 );
				if( pOtherLink == 0 || pOtherLink == pLink )
					continue;

				CConnector *pOtherFixedConnector = pOtherLink->GetFixedConnector();
				if( pOtherFixedConnector != 0 && !pOtherFixedConnector->IsInput() && pBaseConnector != pOtherFixedConnector )
				{
					// Case A.
					return JoinToLink( pLink, pBaseConnector, pConnector, pOtherLink );
				}

				if( pOtherLink->CanOnlySlide() )
				{
					// Case E.
					return JoinToSlideLink( pLink, pBaseConnector, pConnector, pOtherLink );
				}
			}
		}

		// Look at the sliders whose limits are part of the current link. If
		// the slider is fixed then the current link can be moved and rotated
		// to meet it.

		//pBaseConnector = pLink->GetFixedConnector();
		if( pBaseConnector != 0 )
		{
			for( int Index = 0; Index < pLink->GetConnectedSliderCount(); ++Index )
			{
				pLink->GetConnectedSlider( Index );
				CConnector* pSlider = pLink->GetConnectedSlider( Index );
				if( pSlider == 0 )
					continue;

				if( pSlider->IsFixed() )
				{
					// Case B.
					// Got a fixed slider. Make the current link align with it.
					return MoveToSlider( pLink, pBaseConnector, pSlider );
				}
			}
		}

		return false;
	}

	bool FindSlideLinksToMatch( CLink *pLink, LinkList *pLinkList )
	{
		/*
		 * Look for another link that has a sliding connection to the current
		 * link and the current link can only slide.
		 */

		CConnector *pLimit1 = 0;
		CConnector *pLimit2 = 0;
		CConnector *pSlider1 = 0;
		CConnector *pSlider2 = 0;

		if( !pLink->CanOnlySlide( &pLimit1, &pLimit2, &pSlider1, &pSlider2, 0 ) )
			return false;

		int SliderCount = 0;

		POSITION Position = pLinkList->GetHeadPosition();
		while( Position != 0 )
		{
			CLink *pOtherLink = pLinkList->GetNext( Position );
			if( pOtherLink == 0 || pOtherLink == pLink )
				continue;

			POSITION Position2 = pOtherLink->GetConnectorList()->GetHeadPosition();
			while( Position2 != 0 )
			{
				CConnector* pConnector = pOtherLink->GetConnectorList()->GetNext( Position2 );
				if( pConnector == 0 || !pConnector->IsFixed() || !pConnector->IsSlider() )
					continue;

				// If the tested sliding connector is the same one as this link slides on, this is not the right connector.
				if( pConnector == pSlider1 || pConnector == pSlider2 )
					continue;

				CConnector *pLimitA;
				CConnector *pLimitB;
				if( !pConnector->GetSlideLimits( pLimitA, pLimitB ) )
					continue;

				if( !pLink->IsConnected( pLimitA ) )
					continue;

				return SlideToSlider( pLink, pConnector, pLimitA, pLimitB );
			}
		}

		return false;
	}
};

CSimulator::CSimulator()
{
	m_pImplementation = new CSimulatorImplementation;
}

CSimulator::~CSimulator()
{
	if( m_pImplementation != 0 )
		delete m_pImplementation;
	m_pImplementation = 0;
}

bool CSimulator::Reset( void )
{
	if( m_pImplementation == 0 )
		return false;
	return m_pImplementation->Reset();
}

bool CSimulator::SimulateStep( CLinkageDoc *pDoc, int StepNumber, bool bAbsoluteStepNumber, int* pInputID, double *pInputPositions,
                               int InputCount, bool bNoMultiStep, double ForceCPM )
{
	if( m_pImplementation == 0 )
		return false;
	return m_pImplementation->SimulateStep( pDoc, StepNumber, bAbsoluteStepNumber, pInputID, pInputPositions, InputCount, bNoMultiStep, ForceCPM );
}

int CSimulator::GetSimulationSteps( CLinkageDoc *pDoc )
{
	if( m_pImplementation == 0 )
		return false;
	return m_pImplementation->GetSimulationSteps( pDoc );
}

int CSimulator::GetCycleSteps( CLinkageDoc *pDoc, double *pNewCPM )
{
	if( m_pImplementation == 0 )
		return false;
	return m_pImplementation->GetCycleSteps( pDoc, pNewCPM );
}

CElement * CSimulator::GetCycleElement( CLinkageDoc *pDoc )
{
	if( m_pImplementation == 0 )
		return false;
	return m_pImplementation->GetCycleElement( pDoc );
}

bool CSimulator::IsSimulationValid( void )
{
	if( m_pImplementation == 0 )
		return false;
	return m_pImplementation->IsSimulationValid();
}

void CSimulator::Options( bool bUseIncreasedMomentum )
{
	if( m_pImplementation == 0 )
		return;
	m_pImplementation->Options( bUseIncreasedMomentum );
}

