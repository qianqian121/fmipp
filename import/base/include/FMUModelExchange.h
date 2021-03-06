/* --------------------------------------------------------------
 * Copyright (c) 2013, AIT Austrian Institute of Technology GmbH.
 * All rights reserved. See file FMIPP_LICENSE for details.
 * --------------------------------------------------------------*/

#ifndef _FMIPP_FMU_MODELEXCHANGE_H
#define _FMIPP_FMU_MODELEXCHANGE_H

#include <cstdio>
#include <map>

#include "import/base/include/FMUModelExchangeBase.h"
#include "import/integrators/include/Integrator.h"


struct BareFMUModelExchange;


/**
 * \file FMUModelExchange.h 
 *
 * Defines the class FMUModelExchange which provides importing and integrating of FMUs for ModelExchange
 *
 * \class FMUModelExchange FMUModelExchange.h 
 * Implementation of abstract base class FMUModelExchangeBase.  
 *  
 * The FMI standard requires to define the macro MODEL_IDENTIFIER for each entity of FMU ME
 * seperately. This is not done here, because this class links dynamically during run-time.
 */


class __FMI_DLL FMUModelExchange : public FMUModelExchangeBase
{

public:

	/**
	 * Constructor.
	 *
	 * @param[in]  fmuPath  path to FMU (as URI)
	 * @param[in]  modelName  model name
	 * @param[in]  stopBeforeEvent  if true, integration stops immediately before an event
	 * @param[in]  eventSearchPrecision  numerical search precision for events during integration
	 * @param[in]  type  integrator type
	 */
	FMUModelExchange( const std::string& fmuPath,
			  const std::string& modelName,
			  const fmiBoolean stopBeforeEvent = fmiFalse,
			  const fmiReal eventSearchPrecision = 1e-4,
#ifdef USE_SUNDIALS
			  const IntegratorType type = IntegratorType::bdf
#else
			  const IntegratorType type = IntegratorType::dp
#endif
			  );

	/**
	 * Constructor.
	 *
	 * @param[in]  xmlPath  path to XML model description (as URI)
	 * @param[in]  dllPath  path to shared library (as URI)
	 * @param[in]  modelName  model name
	 * @param[in]  stopBeforeEvent  if true, integration stops immediately before an event
	 * @param[in]  eventSearchPrecision  numerical search precision for events during integration
	 * @param[in]  type  integrator type
	 */
	FMUModelExchange( const std::string& xmlPath,
			  const std::string& dllPath,
			  const std::string& modelName,
			  const fmiBoolean stopBeforeEvent = fmiFalse,
			  const fmiReal eventSearchPrecision = 1e-4,

#ifdef USE_SUNDIALS
			  const IntegratorType type = IntegratorType::bdf
#else
			  const IntegratorType type = IntegratorType::dp
#endif
			  );


	/// Copy constructor.
	FMUModelExchange( const FMUModelExchange& aFMU );

	/// Destructor.
	virtual ~FMUModelExchange();

	/// @copydoc FMUModelExchangeBase::instantiate
	virtual fmiStatus instantiate( const std::string& instanceName,
				       fmiBoolean loggingOn = fmiFalse );

	/// \copydoc FMUModelExchangeBase::initialize
	virtual fmiStatus initialize();

	/// \copydoc FMUModelExchangeBase::getContinuousStates 
	virtual fmiStatus getContinuousStates( fmiReal* val );

	/// \copydoc FMUModelExchangeBase::setContinuousStates
	virtual fmiStatus setContinuousStates( const fmiReal* val );

	/// \copydoc FMUModelExchangeBase::getDerivatives
	virtual fmiStatus getDerivatives( fmiReal* val );

	/// \copydoc FMUModelExchangeBase::getEventIndicators
	virtual fmiStatus getEventIndicators( fmiReal* eventsind );

	/// \copydoc FMUModelExchangeBase::integrate( fmiReal tend, unsigned int nsteps )
	virtual fmiReal integrate( fmiReal tend, unsigned int nsteps );

	/// \copydoc FMUModelExchangeBase::integrate( fmiReal tend, double deltaT = 1e-5 )
	virtual fmiReal integrate( fmiReal tend, double deltaT = 1e-5 );

	/// \copydoc FMUModelExchangeBase::stepOverEvent
	virtual fmiBoolean stepOverEvent();

	/// \copydoc FMUModelExchangeBase::completedIntegratorStep
	virtual fmiStatus completedIntegratorStep();

	/// \copydoc FMUModelExchangeBase::failedIntegratorStep
	virtual void failedIntegratorStep( fmiTime time );

	/// \copydoc FMUModelExchangeBase::getEventFlag
	virtual fmiBoolean getEventFlag();

	/// \copydoc FMUModelExchangeBase::setEventFlag
	virtual void setEventFlag( fmiBoolean flag );

	/// \copydoc FMUModelExchangeBase::getIntEvent
	virtual fmiBoolean getIntEvent();

	/// \copydoc FMUModelExchangeBase::getTimeEvent
	virtual fmiReal getTimeEvent();

	/// \copydoc FMUModelExchangeBase::raiseEvent
	virtual void raiseEvent();

	/// \copydoc FMUModelExchangeBase::checkEvents
	virtual fmiBoolean checkEvents();
	
	/// \copydoc FMUModelExchangeBase::checkStateEvent
	virtual fmiBoolean checkStateEvent();

	/// \copydoc FMUModelExchangeBase::checkTimeEvent
	virtual fmiBoolean checkTimeEvent();

	/// \copydoc FMUModelExchangeBase::handleEvents
	virtual void handleEvents( fmiTime tstop );

	/// \copydoc FMUModelExchangeBase::setTime
	virtual void setTime( fmiReal time );

	/// \copydoc FMUModelExchangeBase::rewindTime
	virtual void rewindTime( fmiReal deltaRewindTime );

	/// \copydoc FMUBase::getTime()
	virtual fmiReal getTime() const;

	/// \copydoc FMUBase::getType()
	virtual FMIType getType( const std::string& variableName ) const;

	/// \copydoc FMUBase::getValueRef
	virtual fmiValueReference getValueRef( const std::string& name ) const;

	/// \copydoc FMUBase::getLastStatus
	virtual fmiStatus getLastStatus() const;

	/// \copydoc FMUBase::getValue( fmiValueReference valref, fmiReal& val )
	virtual fmiStatus getValue( fmiValueReference valref, fmiReal& val );

	/// \copydoc FMUBase::getValue( fmiValueReference valref, fmiInteger& val )
	virtual fmiStatus getValue( fmiValueReference valref, fmiInteger& val );

	/// \copydoc FMUBase::getValue( fmiValueReference valref, fmiBoolean& val )
	virtual fmiStatus getValue( fmiValueReference valref, fmiBoolean& val );

	/// \copydoc FMUBase::getValue( fmiValueReference valref, std::string& val )
	virtual fmiStatus getValue( fmiValueReference valref, std::string& val );

	/// \copydoc FMUBase::getValue( fmiValueReference* valref, fmiReal* val, std::size_t ival )
	virtual fmiStatus getValue( fmiValueReference* valref, fmiReal* val, std::size_t ival );

	/// \copydoc FMUBase::getValue( fmiValueReference* valref, fmiInteger* val, std::size_t ival )
	virtual fmiStatus getValue( fmiValueReference* valref, fmiInteger* val, std::size_t ival );

	/// \copydoc FMUBase::getValue( fmiValueReference* valref, fmiBoolean* val, std::size_t ival )
	virtual fmiStatus getValue( fmiValueReference* valref, fmiBoolean* val, std::size_t ival );

	/// \copydoc FMUBase::getValue( fmiValueReference* valref, std::string* val, std::size_t ival )
	virtual fmiStatus getValue( fmiValueReference* valref, std::string* val, std::size_t ival );

	/// \copydoc FMUBase::getValue( const std::string& name,  fmiReal& val )
	virtual fmiStatus getValue( const std::string& name, fmiReal& val );

	/// \copydoc FMUBase::getValue( const std::string& name,  fmiInteger& val )
	virtual fmiStatus getValue( const std::string& name, fmiInteger& val );

	/// \copydoc FMUBase::getValue( const std::string& name,  fmiBoolean& val )
	virtual fmiStatus getValue( const std::string& name, fmiBoolean& val );

	/// \copydoc FMUBase::getValue( const std::string& name,  std::string& val )
	virtual fmiStatus getValue( const std::string& name, std::string& val );

	/// \copydoc FMUBase::getRealValue( const std::string& name )
	virtual fmiReal getRealValue( const std::string& name );

	/// \copydoc FMUBase::getIntegerValue( const std::string& name )
	virtual fmiInteger getIntegerValue( const std::string& name );

	/// \copydoc FMUBase::getBooleanValue( const std::string& name )
	virtual fmiBoolean getBooleanValue( const std::string& name );

	/// \copydoc FMUBase::getStringValue( const std::string& name )
	virtual fmiString getStringValue( const std::string& name );

	/// \copydoc FMUBase::setValue( fmiValueReference valref, fmiReal& val ) 
	virtual fmiStatus setValue( fmiValueReference valref, fmiReal& val );

	/// \copydoc FMUBase::setValue( fmiValueReference valref, fmiInteger& val ) 
	virtual fmiStatus setValue( fmiValueReference valref, fmiInteger& val );

	/// \copydoc FMUBase::setValue( fmiValueReference valref, fmiBoolean& val ) 
	virtual fmiStatus setValue( fmiValueReference valref, fmiBoolean& val );

	/// \copydoc FMUBase::setValue( fmiValueReference valref, std::string& val ) 
	virtual fmiStatus setValue( fmiValueReference valref, std::string& val );

	/// \copydoc FMUBase::setValue( fmiValueReference* valref, fmiReal* val, std::size_t ival )
	virtual fmiStatus setValue( fmiValueReference* valref, fmiReal* val, std::size_t ival );

	/// \copydoc FMUBase::setValue( fmiValueReference* valref, fmiInteger* val, std::size_t ival )
	virtual fmiStatus setValue( fmiValueReference* valref, fmiInteger* val, std::size_t ival );

	/// \copydoc FMUBase::setValue( fmiValueReference* valref, fmiBoolean* val, std::size_t ival )
	virtual fmiStatus setValue( fmiValueReference* valref, fmiBoolean* val, std::size_t ival );

	/// \copydoc FMUBase::setValue( fmiValueReference* valref, std::string* val, std::size_t ival )
	virtual fmiStatus setValue( fmiValueReference* valref, std::string* val, std::size_t ival );

	/// \copydoc FMUBase::setValue( const std::string& name,  fmiReal val )
	virtual fmiStatus setValue( const std::string& name, fmiReal val );

	/// \copydoc FMUBase::setValue( const std::string& name,  fmiInteger val )
	virtual fmiStatus setValue( const std::string& name, fmiInteger val );

	/// \copydoc FMUBase::setValue( const std::string& name,  fmiBoolean val )
	virtual fmiStatus setValue( const std::string& name, fmiBoolean val );

	/// \copydoc FMUBase::setValue( const std::string& name,  std::string val )
	virtual fmiStatus setValue( const std::string& name, std::string val );

        /// \copydoc FMUBase::nStates
	virtual std::size_t nStates() const;

	/// \copydoc FMUBase::nEventInds
	virtual std::size_t nEventInds() const;

	/// \copydoc FMUBase::nValueRefs 
	virtual std::size_t nValueRefs() const;

	/// @copydoc FMUModelExchangeBase::setCallbacks
	virtual fmiStatus setCallbacks( me::fmiCallbackLogger logger,
					me::fmiCallbackAllocateMemory allocateMemory,
					me::fmiCallbackFreeMemory freeMemory );

        /// Send message to FMU logger.
	void logger( fmiStatus status, const char* category, const char* msg ) const;

	/// Send message to FMU logger.
	void logger( fmiStatus status, const std::string& category, const std::string& msg ) const;

	fmiReal getEventSearchPrecision(){
		return eventSearchPrecision_;
	}

	int integratorOrder(){
		return integrator_->stepperOrder();
	}

private:

	FMUModelExchange();		///< Prevent calling the default constructor.

	std::string instanceName_;	///< name of the instantiated FMU

	fmiComponent instance_;		///< Internal FMU instance.

	BareFMUModelExchange *fmu_;	///< Internal pointer to bare FMU ME functionalities and model description.

	std::size_t nStateVars_;	///< Number of state variables.
	std::size_t nEventInds_;	///< Number of event indivators.
	std::size_t nValueRefs_;	///< Number of value references.

	/// \FIXME Maps should be handled via ModelManager, to avoid duplication 
	///        of this (potentially large) map with every instance.
	std::map<std::string,fmiValueReference> varMap_;	/// Maps variable names and value references.
	std::map<std::string,FMIType> varTypeMap_;		/// Maps variable names and their types.

	fmiBoolean stopBeforeEvent_;			///< Flag determining internal event handling.

	fmiReal eventSearchPrecision_;			///< Search precision for events.

	Integrator* integrator_;			///< Integrator instance.

	fmiReal* intStates_;				///< Internal vector used for integration.
	fmiReal* intDerivatives_;			///< Internal vector used for integration.

	fmiReal time_;					///< Internal time.
	fmiReal tnextevent_;				///< Time of next scheduled event.
	fmiReal lastEventTime_;				///< Time of last event.
	fmiReal lastCompletedIntegratorStepTime_;	///< Time of last completed integrator step.
	fmiReal firstFailedIntegratorStepTime_;	///< Time of first integrator step error due to an event.

	fmiReal tstart_;			///< for determining event times and handling events
	fmiReal tlaststop_;			///< for determining event times and handling events

	fmiEventInfo* eventinfo_;		///< Internal event info.
	fmiReal*      eventsind_;		///< Current event indicators (internally used for event detection).
	fmiReal*      preeventsind_;		///< Previous event indicators (internally used for event detection).

	fmiBoolean stateEvent_; 		///< Internal flag indicationg that a state event has occured.
	fmiBoolean timeEvent_; 			///< Internal flag indicationg that a time event has occured.
	fmiBoolean raisedEvent_;		///< Internal flag indicationg that an event might have occured.
	fmiBoolean eventFlag_;
	fmiBoolean intEventFlag_; 		///< Internal flag indicationg that the integrator has found an event.

	fmiStatus lastStatus_;		        ///< Last status returned from an FMI function.

	/**
	 *  Update eventsind_ and preeventsind_ with event indicators from FMU according to
	 *  the current continuous states. Needed to "reset" internal event indicators.
	 */
	fmiStatus resetEventIndicators();

	void readModelDescription();			///< Extract specific information from the mode description.

	static const unsigned int maxEventIterations_ = 5; ///< Maximum number of internal event iterations.

};

#endif // _FMIPP_FMU_MODELEXCHANGE_H
