/* --------------------------------------------------------------
 * Copyright (c) 2013, AIT Austrian Institute of Technology GmbH.
 * All rights reserved. See file FMIPP_LICENSE for details.
 * --------------------------------------------------------------*/

#ifndef _POWER_FACRORY_TIME_ADVANCE_H
#define _POWER_FACRORY_TIME_ADVANCE_H


/**
 * \file PowerFactoryTimeAdvance.h
 * \class PowerFactoryTimeAdvance PowerFactoryTimeAdvance.h
 * This is the base class for implementing mechanisms that advance time in a PowerFactory simulation.
 *
 * PowerFactory provides several possibilities to advance time in a simulation run, for
 * instance with the help of triggers or via DPL scripts. Class PowerFactoryTimeAdvance
 * is the base class for implementing such mechanisms, which is then used by the FMI
 * function doStep(...).
 */ 

#include "import/base/include/ModelDescription.h"

class PowerFactory;
class PowerFactoryFrontEnd;


class PowerFactoryTimeAdvance
{

public:

	PowerFactoryTimeAdvance( PowerFactoryFrontEnd* fe,
				 PowerFactory* pf ) : fe_( fe ), pf_( pf ) {}

	virtual ~PowerFactoryTimeAdvance() {}

	virtual fmiStatus instantiate( const ModelDescription::Properties& vendorAnnotations ) = 0;

	virtual fmiStatus initialize( fmiReal tStart, fmiBoolean stopTimeDefined, fmiReal tStop ) = 0;

	virtual fmiStatus advanceTime( fmiReal comPoint, fmiReal stepSize ) = 0;

protected:

	PowerFactoryFrontEnd* fe_;
	PowerFactory* pf_;
};



/**
 * \class TriggerTimeAdvance PowerFactoryTimeAdvance.h
 * This class implements a mechanism that advances time in a PowerFactory simulation withthe help of triggers.
 */ 

class TriggerTimeAdvance : public PowerFactoryTimeAdvance
{

public:

	TriggerTimeAdvance( PowerFactoryFrontEnd* fe,
			    PowerFactory* pf );

	virtual ~TriggerTimeAdvance();

	/** For the PowerFactory wrapper an extra node called "digpf" is expected in the vendor
	 *  annotations of the model description. This extra node is expected as input for this
	 *  function. For every trigger, an individual node of the form
	 *  <Trigger name="trigger-name" scale="60"/> is expected.
	 */
	virtual fmiStatus instantiate( const ModelDescription::Properties& vendorAnnotations );

	/** Initialize all triggers. For each trigger the individual scale (according to the
	 *  instantiation) is applied, e.g., the start time is initialized with the value "tStart/scale".
	 */
	virtual fmiStatus initialize( fmiReal tStart, fmiBoolean stopTimeDefined, fmiReal tStop );

	/** Advance time for all triggers. For each trigger the individual scale (according to the
	 * instantiation) is applied, e.g., the communication point time is passed to PowerFactory
	 * with the value "( comPoint + stepsize )/scale".
	 */
	virtual fmiStatus advanceTime( fmiReal comPoint, fmiReal stepSize );

private:

	/// Define collection for triggers (plus their individual time-scale).
	typedef std::vector< std::pair<api::DataObject*, fmiReal> > TriggerCollection;

	/// List of all available triggers.
	TriggerCollection triggers_;

	/// Time of last communication point.
	fmiReal lastComPoint_;
};



/**
 * \class DPLScriptTimeAdvance PowerFactoryTimeAdvance.h
 * This class implements a mechanism that advances time in a PowerFactory simulation 
 * with the help of a DPL script.
 *
 * It is assumed that there is only one script responsible for advacing the time of
 * the whole simulation. This script is supposed to have only one input parameters,
 * i.e., the communication point time. 
 */ 

class DPLScriptTimeAdvance : public PowerFactoryTimeAdvance
{

public:

	DPLScriptTimeAdvance( PowerFactoryFrontEnd* fe,
			      PowerFactory* pf );

	virtual ~DPLScriptTimeAdvance();

	/** For the PowerFactory wrapper an extra node called "digpf" is expected in the
	 *  vendor annotations of the model description. This extra node is expected as
	 *  input for this function. For every trigger, an individual node of the form
	 *  <DPLScript name="script-name" scale="0.001" offset="10000"/> is expected.
	 */
	virtual fmiStatus instantiate( const ModelDescription::Properties& vendorAnnotations );

	/** Initialize the simulation time. The offset and scale according to the instantiation
	 *  are applied, e.g., the start time is initialized with the value "offest + tStart/scale".
	 */
	virtual fmiStatus initialize( fmiReal tStart, fmiBoolean stopTimeDefined, fmiReal tStop );

	/** Advance time with the help of the DPL script. The scale and offest according
	 *  to the instantiation are applied, e.g., the communication point time is passed
	 *  to PowerFactory with the value "offest + ( comPoint + stepsize )/scale".
	 */
	virtual fmiStatus advanceTime( fmiReal comPoint, fmiReal stepSize );

private:

	/// Name of DPL script.
	std::string dplScriptName_;

	/// Time offset.
	fmiReal offset_;

	/// Time scale.
	fmiReal scale_;

	/// Time of last communication point.
	fmiReal lastComPoint_;
};





#endif // _POWER_FACRORY_TIME_ADVANCE_H
