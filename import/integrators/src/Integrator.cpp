/* --------------------------------------------------------------
 * Copyright (c) 2013, AIT Austrian Institute of Technology GmbH.
 * All rights reserved. See file FMIPP_LICENSE for details.
 * --------------------------------------------------------------*/

/**
 * \file Integrator.cpp
 * The Integrator serves as an interface between the IntegratorSteppers and FMUModelExchange
 */ 

#include <cstdio>
#include <cassert>
#include <limits>

#include "common/fmi_v1.0/fmiModelTypes.h"

#include "import/base/include/FMUModelExchangeBase.h"

#include "import/integrators/include/Integrator.h"
#include "import/integrators/include/IntegratorStepper.h"

#include <iostream>

using namespace std;


Integrator::Integrator( DynamicalSystem* fmu ) :
	fmu_( fmu ),
	stepper_( 0 ),
	is_copy_( false )
{}


Integrator::Integrator( const Integrator& other ) :
	fmu_( other.fmu_ ),
	stepper_( other.stepper_ ),
	states_( other.states_ ),
	time_( other.time_ ),
	is_copy_( true )
{}


Integrator::~Integrator()
{
	// Internally, ODEINT makes copies of this class (when using
	// boost::fusion::for_each). This copies can however all use
	// the same stepper. Only when the destructor of the original
	// instance is called, the stepper is deleted.
	if ( false == is_copy_ || stepper_ )
		delete stepper_;
}


Integrator* Integrator::clone() const
{
	Integrator* integrator = new Integrator( this->fmu_ );
	integrator->initialize();
	Integrator::Properties props = properties_;
	integrator->setProperties( props );
	return integrator;
}

void Integrator::initialize(){
	states_      = state_type( fmu_->nStates(), std::numeric_limits<fmiReal>::quiet_NaN() );
	time_        = std::numeric_limits<fmiReal>::quiet_NaN();
}


void Integrator::setType( IntegratorType type )
{
	if ( 0 != stepper_ )
		delete stepper_;
	properties_.type  = type;
	stepper_ = IntegratorStepper::createStepper( properties_, fmu_ );
}


void Integrator::setProperties( Integrator::Properties& properties )
{
	if ( 0 != stepper_ )
		delete stepper_;
	stepper_ = IntegratorStepper::createStepper( properties, fmu_ );
	properties_ = properties;
}

Integrator::Properties Integrator::getProperties() const
{
	return properties_;
}


Integrator::EventInfo Integrator::integrate( fmiTime step_size, fmiTime dt, fmiTime eventSearchPrecision )
{
	// Get current time.
	time_ = fmu_->getTime();

	// Get current continuous states.
	fmu_->getContinuousStates( &states_.front() );

	// Invoke integration method.
	stepper_->invokeMethod( eventInfo_, states_, time_, step_size, dt, eventSearchPrecision );

	// if no event happened, return
	if ( !eventInfo_.stateEvent ){
		return eventInfo_;
	} // else, use a binary search to locate the event upt to the eventSearchPrecision_
	else{
		/* an event happend. locate it using an event-search loop ( binary search )
		 *    * tLower     last time where the stepper did not detect an event
		 *                 this variable gets written by invokeMethod
		 *    * tUpper     first time where the stepper detected an event
		 *                 this variable gets written by invokeMethod
		 */
		if ( eventInfo_.tUpper > time_ + step_size ){
			// in case the stepper adapted the step size, make sure you only search
			// for an event within the integration limits

			fmiTime currentTime = fmu_->getTime();
			fmiTime stepSize =  time_ + step_size - fmu_->getTime();
			stepper_->do_step_const( eventInfo_, states_, currentTime,
						 stepSize
						 );
			fmu_->setContinuousStates( &states_[0] );
			fmu_->setTime( time_ + step_size );
			if ( !fmu_->checkStateEvent() ){
				eventInfo_.stateEvent = false;
				return eventInfo_;
			}
			eventInfo_.tUpper = time_ + step_size;
		}
		while ( eventInfo_.tUpper - eventInfo_.tLower > eventSearchPrecision/2.0 ){
			// create backup states
			state_type states_bak = states_;

			// let the stepper integrate the left half of the Interval [tLower_,tUpper_]
			//stepper_->reset();
			fmiTime dt = ( eventInfo_.tUpper - eventInfo_.tLower )/2.0;
			fmiTime time = eventInfo_.tLower;
			stepper_->do_step_const( eventInfo_, states_, time, dt );

			// write the result in the model and check for int events
			fmu_->setContinuousStates( &states_[0] );
			fmu_->setTime( time );

			// update the event horizon according to the flag eventHappened_
			if ( !fmu_->checkStateEvent() ){
				// update the event horizon
				eventInfo_.tLower = ( eventInfo_.tUpper + eventInfo_.tLower )/2.0;
				eventInfo_.tUpper = eventInfo_.tUpper;
			} else{
				// use the backup variables
				fmu_->setContinuousStates( &states_bak[0] );
				fmu_->setTime( eventInfo_.tLower );
				states_ = states_bak;

				// reset the stepper
				stepper_->reset();

				// update the event horizon
				eventInfo_.tUpper = ( eventInfo_.tUpper + eventInfo_.tLower )/2.0;
			}
		}
		// make sure the event is *strictly* inside the interval [tLower_, tUpper_]
		eventInfo_.tUpper += eventSearchPrecision/8.0;
		time_              = eventInfo_.tLower;
		return eventInfo_;
	}
}


// get time horizon for the event
void Integrator::getEventHorizon( fmiTime& tLower, fmiTime& tUpper ){
	tLower = eventInfo_.tLower;
	tUpper = eventInfo_.tUpper;
}
