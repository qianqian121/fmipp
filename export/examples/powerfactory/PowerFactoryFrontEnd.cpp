/* --------------------------------------------------------------
 * Copyright (c) 2013, AIT Austrian Institute of Technology GmbH.
 * All rights reserved. See file FMIPP_LICENSE for details.
 * --------------------------------------------------------------*/

/// \file PowerFactoryFrontEnd.cpp

// Check for compilation with Visual Studio 2010 (required).
#if ( _MSC_VER == 1600 )
#include "Windows.h"
#else
#error This project requires Visual Studio 2010.
#endif

// Standard library includes.
#include <sstream>
#include <stdexcept>

// Boost library includes.
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>

// Project file includes.
#include "PowerFactoryFrontEnd.h"
#include "PowerFactoryRealScalar.h"
#include "export/include/HelperFunctions.h"

// PFSim project includes (advanced PowerFactory wrapper)
#include "Types.h"
#include "PowerFactory.h"


using namespace std;



PowerFactoryFrontEnd::PowerFactoryFrontEnd() {}


PowerFactoryFrontEnd::~PowerFactoryFrontEnd()
{
	// Deactivate the project.
	if ( pf_->Ok != pf_->deactivateProject() )
		logger( fmiWarning, "WARNING", "deactivation of project failed" );

	// Delete the project.
	string executeCmd = string( "del " ) + target_ + string( "\\" ) + projectName_;
	if ( pf_->Ok != pf_->execute( executeCmd.c_str() ) )
		logger( fmiWarning, "WARNING", "could not delete project" );


	// Empty the recycle bin (delete the project once and forever).
	executeCmd = string( "del " ) + target_ + string( "\\Recycle Bin\\*" );
	if ( pf_->Ok != pf_->execute( executeCmd.c_str() ) )
		logger( fmiWarning, "WARNING", "could not empty recycle bin" );

	// Exit PowerFactory.
	if ( pf_->Ok != pf_->execute( "exit" ) )
		logger( fmiWarning, "WARNING", "exiting failed" );

	// Delete the wrappper-internal representation of the model variables.
	BOOST_FOREACH( RealMap::value_type& v, realScalarMap_ )
		delete v.second;
}


fmiStatus
PowerFactoryFrontEnd::setReal( const fmiValueReference& ref, const fmiReal& val )
{
	// Search for value reference.
	RealMap::const_iterator itFind = realScalarMap_.find( ref );

	// Check if scalar according to the value reference exists.
	if ( itFind == realScalarMap_.end() )
	{
		logger( fmiWarning, "WARNING", "unknown value reference" );
		return fmiWarning;
	}

	const PowerFactoryRealScalar* scalar = itFind->second;
	// Check if scalar is defined as input.
	if ( scalar->causality_ != ScalarVariableAttributes::input )
	{
		logger( fmiWarning, "WARNING", "scalar is not an input variable" );
		return fmiWarning;
	}

	api::DataObject* dataObj = 0;
	// Search for PowerFactory object by class name and object name.
	if ( pf_->getCalcRelevantObject( scalar->className_, scalar->objectName_, dataObj ) == pf_->Ok )
	{
		// Set value of parameter of PowerFactory object using the parameter name.
		if ( ( 0 != dataObj ) &&
		     ( pf_->setAttributeDouble( dataObj, scalar->parameterName_.c_str(), val ) == pf_->Ok ) ) 
		{
			return fmiOK;
		}

		logger( fmiWarning, "WARNING", "not able to set data" );
		return fmiWarning;
	}

	logger( fmiWarning, "WARNING", "not able to retrieve oject from PowerFactory" );
	return fmiWarning;
}


fmiStatus
PowerFactoryFrontEnd::setInteger( const fmiValueReference& ref, const fmiInteger& val )
{
	return fmiFatal;
}


fmiStatus PowerFactoryFrontEnd::setBoolean( const fmiValueReference& ref, const fmiBoolean& val )
{
	return fmiFatal;
}


fmiStatus PowerFactoryFrontEnd::setString( const fmiValueReference& ref, const fmiString& val )
{
	return fmiFatal;
}


fmiStatus
PowerFactoryFrontEnd::getReal( const fmiValueReference& ref, fmiReal& val )
{
	// Search for value reference.
	RealMap::const_iterator itFind = realScalarMap_.find( ref );

	// Check if scalar according to the value reference exists.
	if ( itFind == realScalarMap_.end() )
	{
		logger( fmiWarning, "WARNING", "unknown value reference" );
		val = 0;
		return fmiWarning;
	}

	const PowerFactoryRealScalar* scalar = itFind->second;
	api::DataObject* dataObj = 0;
	// Search for PowerFactory object by class name and object name.
	if ( pf_->getCalcRelevantObject( scalar->className_, scalar->objectName_, dataObj ) == pf_->Ok )
	{
		// Extract data from PowerFactory object using the parameter name.
		if ( ( 0 != dataObj ) &&
		     ( pf_->getAttributeDouble( dataObj, scalar->parameterName_.c_str(), val ) == pf_->Ok ) )
		{
			return fmiOK;
		}

		logger( fmiWarning, "WARNING", "not able to read data" );
		return fmiWarning;
	}

	logger( fmiWarning, "WARNING", "not able to retrieve oject from PowerFactory" );
	return fmiWarning;
}


fmiStatus
PowerFactoryFrontEnd::getInteger( const fmiValueReference& ref, fmiInteger& val )
{
	return fmiFatal;
}


fmiStatus PowerFactoryFrontEnd::getBoolean( const fmiValueReference& ref, fmiBoolean& val )
{
	return fmiFatal;
}


fmiStatus PowerFactoryFrontEnd::getString( const fmiValueReference& ref, fmiString& val )
{
	return fmiFatal;
}



fmiStatus
PowerFactoryFrontEnd::instantiateSlave( const string& instanceName, const string& fmuGUID,
					const string& fmuLocation, const string& mimeType,
					fmiReal timeout, fmiBoolean visible )
{
	instanceName_ = instanceName;

	const string seperator( "/" );
	// Trim FMU location path (just to be sure).
	const string fmuLocationTrimmed = boost::trim_copy( fmuLocation );
	// Construct URI of XML model description file.
	const string modelDescriptionUrl = fmuLocationTrimmed + seperator + string( "modelDescription.xml" );

	// Get the path of the XML model description file.
	string modelDescriptionPath;
	if ( false == HelperFunctions::getPathFromUrl( modelDescriptionUrl, modelDescriptionPath ) ) {
                stringstream err;
		err << "invalid input URL for XML model description file: " << modelDescriptionUrl;
		logger( fmiFatal, "ABORT", err.str() );
		return fmiFatal;
	}

	// Parse the XML model description file.
	ModelDescription modelDescription( modelDescriptionPath );

	// Check if parsing was successfull.
	if ( false == modelDescription.isValid() ) {
                stringstream err;
		err << "unable to parse XML model description file: " << modelDescriptionPath;
		logger( fmiFatal, "ABORT", err.str() );
		return fmiFatal;
	}


	// Check if GUID matches.
	if ( modelDescription.getGUID() != fmuGUID ) { // Check if GUID is consistent.
		logger( fmiFatal, "ABORT", "wrong GUID" );
		return fmiFatal;
	}

	// Check if MIME type matches.
	if ( modelDescription.getMIMEType() != mimeType ) { // Check if MIME type is consistent.
		string err = string( "Wrong MIME type: " ) + mimeType +
			string( " --- expected: " ) + modelDescription.getMIMEType();
		logger( fmiFatal, "ABORT", err );
		return fmiFatal;
	}

	// Copy additional input files (specified in XML description elements
	// of type  "Implementation.CoSimulation_Tool.Model.File").
	if ( false == copyAdditionalInputFiles( modelDescription, fmuLocationTrimmed ) ) {
		logger( fmiFatal, "ABORT", "not able to copy additional input files" );
		return fmiFatal;
	}

	// Create wrapper.
	pf_ = PowerFactory::create();
	if ( 0 == pf_ ) {
		logger( fmiFatal, "ABORT", "creation of PowerFactory API wrapper failed" );
		return fmiFatal;
	}

	// The input file URI may start with "fmu://". In that case the
	// FMU's location has to be prepended to the URI accordingly.
	string inputFileUrl = modelDescription.getEntryPoint();
	string inputFilePath;
	processURI( inputFileUrl, fmuLocationTrimmed );
	if ( false == HelperFunctions::getPathFromUrl( inputFileUrl, inputFilePath ) ) {
                stringstream err;
		err << "invalid URL for input file (entry point): " << inputFileUrl;
		logger( fmiFatal, "ABORT", err.str() );
		return fmiFatal;
	}

	// Extract PowerFactory project name.
	projectName_ = modelDescription.getModelAttributes().get<string>( "modelName" );
	// Extract PowerFactory target.
	if ( false == parseTarget( modelDescription, target_ ) ) {
		logger( fmiFatal, "ABORT", "could not parse project target" );
		return fmiFatal;
	}

	// Import project file into PowerFactory.
	const string executeCmd = string( "pfdimport g_target=" ) + target_ + string( " g_file=" ) + inputFilePath;
	if ( pf_->Ok != pf_->execute( executeCmd.c_str() ) )  {
		logger( fmiFatal, "ABORT", "could not import project" );
		return fmiFatal;
	}

	// Actiavte PowerFactory project.
	if ( pf_->Ok != pf_->activateProject( projectName_ ) ) {
		logger( fmiFatal, "ABORT", "could not activate project" );
		return fmiFatal;
	}

	// Set visibility of PowerFactory GUI.
	if ( pf_->Ok != pf_->showUI( visible ) ) {
		logger( fmiFatal, "ABORT", "could not set UI visibility" );
		return fmiFatal;
	}

	// Initialize triggers.
	if ( false == initializeTriggers( modelDescription ) ) return fmiFatal;
	     
	size_t nRealScalars;
	size_t nIntegerScalars;
	size_t nBooleanScalars;
	size_t nStringScalars;
	     
	// Parse number of model variables from model description.
	modelDescription.getNumberOfVariables( nRealScalars, nIntegerScalars, nBooleanScalars, nStringScalars );

	if ( ( 0 != nIntegerScalars ) && ( 0 != nBooleanScalars ) && ( 0 != nStringScalars ) ) {
		logger( fmiFatal, "ABORT", "only variables of type 'fmiReal' supported" );
		return fmiFatal;
	}

	// Initialize wrapper-internal representation of variables.
	if ( false == initializeVariables( modelDescription ) ) {
		return fmiFatal;
	}

	return fmiOK;
}


fmiStatus
PowerFactoryFrontEnd::initializeSlave( fmiReal tStart, fmiBoolean StopTimeDefined, fmiReal tStop )
{
	// Iterate through all available triggers.
	TriggerCollection::iterator itTrigger;
	for ( itTrigger = triggers_.begin(); itTrigger != triggers_.end(); ++itTrigger )
	{
		// The trigger value will be set to the start time (scaled).
		fmiReal value =  tStart / itTrigger->second;

		// Set the trigger value.
		if ( pf_->Ok != pf_->setAttributeDouble( itTrigger->first, "ftrigger", value ) )
		{
			logger( fmiFatal, "ABORT", "only variables of type 'fmiReal' supported" );
			return fmiFatal;
		}
	}

	// Set the start time as the first communication point.
	lastComPoint_ = tStart;

	// Make a power flow calculation (triggers calculation of "flexible data").
	if ( pf_->calculatePowerFlow() != pf_->Ok ) {
		logger( fmiFatal, "ABORT", "power flow calculation failed" );
		return fmiFatal;
	}

	// Check if power flow is valid.
	if ( pf_->isPowerFlowValid() != pf_->Ok ) {
		logger( fmiDiscard, "DISCARD", "power flow calculation not valid" );
		return fmiDiscard;
	}

	return fmiOK;
}


fmiStatus
PowerFactoryFrontEnd::resetSlave()
{
	return fmiFatal; /// \FIXME Replace dummy implementation.
}


fmiStatus
PowerFactoryFrontEnd::setRealInputDerivatives( const fmiValueReference vr[], size_t nvr,
					       const fmiInteger order[], const fmiReal value[])
{
	return fmiFatal; /// \FIXME Replace dummy implementation.
}


fmiStatus
PowerFactoryFrontEnd::getRealOutputDerivatives( const fmiValueReference vr[], size_t nvr,
						const fmiInteger order[], fmiReal value[])
{
	return fmiFatal; /// \FIXME Replace dummy implementation.
}


fmiStatus
PowerFactoryFrontEnd::doStep( fmiReal comPoint, fmiReal stepSize, fmiBoolean newStep )
{
	// Sanity check for step size.
	if ( stepSize < 0. ) {
		logger( fmiDiscard, "DISCARD", "step size has to be greater equal zero" );
		return fmiDiscard;
	}

	// Sanity check for the current communication point.
	if ( fabs( comPoint - lastComPoint_ ) > 1e-9 ) {
		logger( fmiDiscard, "DISCARD", "wrong communication point" );
		return fmiDiscard;
	}

	// The internal simulation time is set to the communication point plus the step size.
	fmiReal time = comPoint + stepSize;

	// Iterate through all available triggers.
	TriggerCollection::iterator itTrigger;
	for ( itTrigger = triggers_.begin(); itTrigger != triggers_.end(); ++itTrigger )
	{
		// The trigger value will be set to the current simulation time (scaled).
		fmiReal value =  time/itTrigger->second;

		// Set the trigger value.
		if ( pf_->Ok != pf_->setAttributeDouble( itTrigger->first, "ftrigger", value ) ) {
			logger( fmiFatal, "ABORT", "could not set trigger value" );
			return fmiFatal;
		}
	}

	// Set the current simulation time as the next communication point.
	lastComPoint_ = time;

	// Make a power flow calculation (triggers calculation of "flexible data").
	if ( pf_->calculatePowerFlow() != pf_->Ok ) {
		logger( fmiFatal, "ABORT", "power flow calculation failed" );
		return fmiFatal;
	}

	// Check if power flow is valid.
	if ( pf_->isPowerFlowValid() != pf_->Ok ) {
		logger( fmiDiscard, "DISCARD", "power flow calculation not valid" );
		return fmiDiscard;
	}

	return fmiOK;
}


fmiStatus
PowerFactoryFrontEnd::cancelStep()
{
	return fmiFatal; /// \FIXME Replace dummy implementation.
}


fmiStatus
PowerFactoryFrontEnd::getStatus( const fmiStatusKind s, fmiStatus* value )
{
	return fmiFatal; /// \FIXME Replace dummy implementation.
}


fmiStatus
PowerFactoryFrontEnd::getRealStatus( const fmiStatusKind s, fmiReal* value )
{
	return fmiFatal; /// \FIXME Replace dummy implementation.
}


fmiStatus
PowerFactoryFrontEnd::getIntegerStatus( const fmiStatusKind s, fmiInteger* value )
{
	return fmiFatal; /// \FIXME Replace dummy implementation.
}


fmiStatus
PowerFactoryFrontEnd::getBooleanStatus( const fmiStatusKind s, fmiBoolean* value )
{
	return fmiFatal; /// \FIXME Replace dummy implementation.
}


fmiStatus
PowerFactoryFrontEnd::getStringStatus( const fmiStatusKind s, fmiString* value )
{
	return fmiFatal; /// \FIXME Replace dummy implementation.
}


bool
PowerFactoryFrontEnd::initializeVariables( const ModelDescription& modelDescription )
{
	// Check if model description is available.
	if ( false == modelDescription.hasModelVariables() ) {
		logger( fmiWarning, "WARNING", "model variable description missing" );
		return false;
	}

	// Get variable description.
	const ModelDescription::Properties& modelVariables = modelDescription.getModelVariables();

	PowerFactoryRealScalar* scalar;

	// Iterate through variable decriptions.
	BOOST_FOREACH( const ModelDescription::Properties::value_type &v, modelVariables )
	{
		// Create new scalar for internal representation of variables.
		scalar = new PowerFactoryRealScalar;
		// Initialize scalar according to variable description.
		if ( false == initializeScalar( scalar, v.second ) ) {
			delete scalar;
			return false;
		}
		// Add scalar to internal map.
		realScalarMap_[scalar->valueReference_] = scalar;
	}

	return true;
}


bool
PowerFactoryFrontEnd::initializeTriggers( const ModelDescription& modelDescription )
{
	using namespace ModelDescriptionUtilities;

	// Check if vendor annotations are available.
	if ( modelDescription.hasVendorAnnotations() )
	{
		// Extract current application name from MIME type.
		string applicationName = modelDescription.getMIMEType().substr( 14 );
		// Extract vendor annotations.
		const Properties& vendorAnnotations = modelDescription.getVendorAnnotations();

		// Check if vendor annotations according to current application are available.
		if ( hasChild( vendorAnnotations, applicationName ) )
		{
			const Properties& annotations = vendorAnnotations.get_child( applicationName );
			BOOST_FOREACH( const Properties::value_type &v, annotations )
			{
				// Check if XML element describes a trigger.
				if ( v.first != "Trigger" ) continue;

				// Extract object name and time scale for trigger.
				const Properties& attributes = getAttributes( v.second );
				string name = attributes.get<string>( "name" );
				fmiReal scale = attributes.get<fmiReal>( "scale" );

				api::DataObject* trigger;

				// Search for trigger object by class name (SetTrigger) and object name.
				if ( pf_->Ok !=
				     pf_->getActiveStudyCaseObject( "SetTrigger", name, false, trigger ) )
				{
					string err = "[PowerFactoryFrontEnd] trigger not found: ";
					err += name;
					logger( fmiWarning, "WARNING", err );
					return false;
				}

				// Activate trigger.
				if ( pf_->Ok != pf_->setAttributeDouble( trigger, "outserv", 0 ) )
				{
					string err = "[PowerFactoryFrontEnd] failed activating the trigger: ";
					err += name;
					logger( fmiWarning, "WARNING", err );
					return false;
				}

				// Add trigger to internal list of all available triggers.
				triggers_.push_back( make_pair( trigger, scale ) );
			}
		}
	}

	return true;
}


bool
PowerFactoryFrontEnd::initializeScalar( PowerFactoryRealScalar* scalar,
					const ModelDescription::Properties& description )
{
	using namespace ScalarVariableAttributes;
	using namespace ModelDescriptionUtilities;

	// Get XML attributes from scalar description.
	const Properties& attributes = getAttributes( description );

	// Parse class name, object name and parameter name from description.
	bool parseStatus = parseFMIVariableName( attributes.get<string>( "name" ),
						 scalar->className_, scalar->objectName_, scalar->parameterName_ );

	if ( false == parseStatus ) {
		stringstream err;
		err << "bad variable name: " << attributes.get<string>( "name" );
		logger( fmiWarning, "WARNING", err.str() );
		return false;
	}

	// Extract information regarding value reference, causality and variability.
	scalar->valueReference_ = attributes.get<int>( "valueReference" );
	scalar->causality_ = getCausality( attributes.get<string>( "causality" ) );
	scalar->variability_ = getVariability( attributes.get<string>( "variability" ) );

	if ( hasChildAttributes( description, "Real" ) )
	{
		// This wrapper handles only variables of type 'fmiReal'!
		const Properties& properties = getChildAttributes( description, "Real" );

		// Check if a start value has been defined.
		if ( properties.find( "start" ) != properties.not_found() ) {

			// // Check if scalar is defined as input.
			// if ( scalar->causality_ != ScalarVariableAttributes::input ) {
			// 	stringstream err;
			// 	err << "not an input: " << attributes.get<string>( "name" );
			// 	logger( fmiWarning, "WARNING", err.str() );
			// 	return false;
			// }
 
			api::DataObject* dataObj = 0;
			int check = -1;
			// Search for PowerFactory object by class name and object name.
			check = pf_->getCalcRelevantObject( scalar->className_, scalar->objectName_, dataObj );
			if ( check != pf_->Ok )
			{
				stringstream err;
				err << "unable to get object: " << attributes.get<string>( "name" );
				logger( fmiWarning, "WARNING", err.str() );
				return false;
			} else if ( 0 != dataObj ) {
				// Extract start value from description.
				fmiReal start = properties.get<fmiReal>( "start" );

				// Set value of parameter of PowerFactory object using the parameter name.
				check = pf_->setAttributeDouble( dataObj, scalar->parameterName_.c_str(), start );
				if ( check != pf_->Ok ) 
				{
					stringstream err;
					err << "unable to set attribute: " << attributes.get<string>( "name" );
					logger( fmiWarning, "WARNING", err.str() );
					return false;
				}
			}
		}

	}

	/// \FIXME What about the remaining properties?

	return true;
}


bool
PowerFactoryFrontEnd::parseTarget( const ModelDescription& modelDescription,
				   std::string& target )
{
	using namespace ModelDescriptionUtilities;

	// Check if vendor annotations are available.
	if ( modelDescription.hasVendorAnnotations() )
	{
		// Extract current application name from MIME type.
		string applicationName = modelDescription.getMIMEType().substr( 14 );
		// Extract vendor annotations.
		const Properties& vendorAnnotations = modelDescription.getVendorAnnotations();

		// Check if vendor annotations according to current application are available.
		if ( hasChild( vendorAnnotations, applicationName ) )
		{
			// Extract target from XML description.
			const Properties& attributes = getChildAttributes( vendorAnnotations, applicationName );
			target = attributes.get<string>( "target" );
			return true;
		}
	}

	return false;
}


bool
PowerFactoryFrontEnd::parseFMIVariableName( const string& name,
					    string& className,
					    string& objectName,
					    string& parameterName )
{
	using namespace boost;

	// Split string, use "."-character as delimiter.
	vector<string> strs;
	boost::split( strs, name, is_any_of(".") );

	// Require three resulting strings (class name, object name, parameter name).
	if ( 3 == strs.size() )
	{
		className = algorithm::trim_copy( strs[0] );
		objectName = algorithm::trim_copy( strs[1] );
		parameterName = algorithm::trim_copy( strs[2] );
		return true;
	}

	stringstream err;
	err << "invalid variable name: " << name;
	logger( fmiWarning, "WARNING", err.str() );
	return false;
}


void
PowerFactoryFrontEnd::logger( fmiStatus status, const string& category, const string& msg )
{
	if ( ( status == fmiOK ) && ( fmiFalse == loggingOn_ ) ) return;

	functions_->logger( static_cast<fmiComponent>( this ),
			    instanceName_.c_str(), status,
			    category.c_str(), msg.c_str() );
}