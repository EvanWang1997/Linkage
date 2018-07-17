#pragma once
#include "linkageDoc.h"

#if 0
typedef enum  { None, Bind, Flop } SimulatorErrorType; 

class CSimulatorError
{
	public:
	CSimulatorError( CConnector *pBadConnector, SimulatorErrorType Error )
	{
		m_BadConectors.AddTail( pBadConnector );
		m_Error = Error;
	}
	CSimulatorError( CLink *pBadLink, SimulatorErrorType Error )
	{
		m_BadLinks.AddTail( pBadLink );
		m_Error = Error;
	}
	ConnectorList m_BadConectors;
	LinkList m_BadLinks;
	SimulatorErrorType m_Error;
};

#endif

class CSimulator
{
public:

	CSimulator();
	~CSimulator();

	bool SimulateStep( CLinkageDoc *pDoc, int StepNumber, bool bAbsoluteStepNumber, int* pInputID, double *pInputPositions, int InputCount, bool bNoMultiStep, double ForceAllCPM );
	bool Reset( void );
	int GetSimulationSteps( CLinkageDoc *pDoc );
	int GetCycleSteps( CLinkageDoc *pDoc, double *pNewCPM = 0 );
	CElement * GetCycleElement( CLinkageDoc *pDoc );
	bool IsSimulationValid( void );
	void Options( bool bUseIncreasedMomentum );
	static double GetStepsPerMinute( void ) { return 3600.0; }

private:
	class CSimulatorImplementation *m_pImplementation;
};

