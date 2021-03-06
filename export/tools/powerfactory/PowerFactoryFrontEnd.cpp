/* --------------------------------------------------------------
 * Copyright (c) 2013, AIT Austrian Institute of Technology GmbH.
 * All rights reserved. See file FMIPP_LICENSE for details.
 * --------------------------------------------------------------*/

/// \file PowerFactoryFrontEnd.cpp

// Check for compilation with Visual Studio 2010 (required).
#if ( _MSC_VER == 1600 )
#include "windows.h"
#include <Lmcons.h>
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
//#include <boost/filesystem.hpp>

// Project file includes.
#include "PowerFactoryFrontEnd.h"
#include "PowerFactoryRealScalar.h"
#include "PowerFactoryTimeAdvance.h"
#include "PowerFactoryExtraOutput.h"
#include "export/include/HelperFunctions.h"
#include "import/base/include/ModelDescription.h"

// PFSim project includes (advanced PowerFactory wrapper)
#include "Types.h"
#include "PowerFactory.h"


using namespace std;


// Forward declaration
bool initializeScalar( PowerFactoryRealScalar* scalar,
		       const ModelDescription::Properties& description,
		       PowerFactoryFrontEnd* frontend,
		       PowerFactory* pf );


PowerFactoryFrontEnd::PowerFactoryFrontEnd() :
	pf_( 0 ), time_( 0 ), extraOutput_( 0 )
{}


PowerFactoryFrontEnd::~PowerFactoryFrontEnd()
{

	if ( 0 != pf_ ) {

		// Deactivate the project.
		if ( pf_->Ok != pf_->deactivateProject() )
			logger( fmiWarning, "WARNING", "deactivation of project failed" );

		// Delete the project.
		string executeCmd = string( "del " ) + target_ + string( "\\" ) + projectName_;
		if ( pf_->Ok != pf_->execute( executeCmd.c_str() ) )
			logger( fmiWarning, "WARNING", "could not delete project" );

		// Empty the recycle bin (delete the project once and forever).
		// Note: For PF 15.0.3 string( "\\Recycle Bin\\*" ) was used."
		executeCmd = string( "del " ) + target_ + string( "\\RecBin\\*" );
		if ( pf_->Ok != pf_->execute( executeCmd.c_str() ) )
			logger( fmiWarning, "WARNING", "could not empty recycle bin" );

		// Exit PowerFactory.
		if ( pf_->Ok != pf_->execute( "exit" ) )
			logger( fmiWarning, "WARNING", "exiting failed" );

		// Delete the wrappper-internal representation of the model variables.
		BOOST_FOREACH( RealMap::value_type& v, realScalarMap_ )
			delete v.second;

		/// \FIXME deallocation of object of type PowerFactory causes the program to halt
		//delete pf_;
	}

	if ( 0 != time_ ) delete time_;

	if ( 0 != extraOutput_ ) delete extraOutput_;
}


fmiStatus
PowerFactoryFrontEnd::setReal( const fmiValueReference& ref, const fmiReal& val )
{
	// Search for value reference.
	RealMap::const_iterator itFind = realScalarMap_.find( ref );

	// Check if scalar according to the value reference exists.
	if ( itFind == realScalarMap_.end() )
	{
		ostringstream err;
		err << "setReal -> unknown value reference = " << ref;
		logger( fmiWarning, "WARNING", err.str() );
		return fmiWarning;
	}

	const PowerFactoryRealScalar* scalar = itFind->second;
	// Check if scalar is defined as input.
	if ( scalar->causality_ != ScalarVariableAttributes::input )
	{
		ostringstream err;
		err << "setReal -> scalar is not an input variable, value reference = " << ref;
		logger( fmiWarning, "WARNING", err.str() );
		return fmiWarning;
	}

	// Set value of parameter of PowerFactory object using the parameter name.
	if (( 0 != scalar->apiDataObject_ ) &&
	    ( pf_->setAttributeDouble( scalar->apiDataObject_, scalar->parameterName_.c_str(), val ) == pf_->Ok )) 
	{
		return fmiOK;
	}

	string err = string( "setReal -> not able to set data: class name = " ) + 
		scalar->className_ + string( ", object name = " ) + scalar->objectName_ +
		string( ", parameter name = " ) + scalar->parameterName_;

	logger( fmiWarning, "WARNING", err );

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
		ostringstream err;
		err << "getReal -> unknown value reference = " << ref;
		logger( fmiWarning, "WARNING", err.str() );
		val = 0;
		return fmiWarning;
	}

	const PowerFactoryRealScalar* scalar = itFind->second;
	// Extract data from PowerFactory object using the parameter name.
	if (( 0 != scalar->apiDataObject_ ) &&
	    ( pf_->getAttributeDouble( scalar->apiDataObject_, scalar->parameterName_.c_str(), val ) == pf_->Ok ))
	{
		return fmiOK;
	}

	string err = string( "getReal -> not able to read data: class name = " ) + 
		scalar->className_ + string( ", object name = " ) + scalar->objectName_ +
		string( ", parameter name = " ) + scalar->parameterName_;

	logger( fmiWarning, "WARNING", err );
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
                ostringstream err;
		err << "invalid input URL for XML model description file: " << modelDescriptionUrl;
		logger( fmiFatal, "URL", err.str() );
		return fmiFatal;
	}

	// Parse the XML model description file.
	ModelDescription modelDescription( modelDescriptionPath );

	// Check if parsing was successfull.
	if ( false == modelDescription.isValid() ) {
                ostringstream err;
		err << "unable to parse XML model description file: " << modelDescriptionPath;
		logger( fmiFatal, "MODEL-DESCRIPTION", err.str() );
		return fmiFatal;
	}

	// Check if GUID matches.
	if ( modelDescription.getGUID() != fmuGUID ) { // Check if GUID is consistent.
		string err = string( "wrong GUID: " ) + fmuGUID +
			string(" --- expected: " ) + modelDescription.getGUID();
		logger( fmiFatal, "GUID", err );
		return fmiFatal;
	}

	// Check if MIME type is consistent.
	if ( modelDescription.getMIMEType() != mimeType ) {
		string warning = string( "Wrong MIME type: " ) + mimeType +
			string( " --- expected: " ) + modelDescription.getMIMEType();
		logger( fmiWarning, "MIME-TYPE", warning );
	}

	// Copy additional input files (specified in XML description elements
	// of type  "Implementation.CoSimulation_Tool.Model.File").
	if ( false == copyAdditionalInputFiles( &modelDescription, fmuLocationTrimmed ) ) {
		logger( fmiFatal, "FILE-COPY", "not able to copy additional input files" );
		return fmiFatal;
	}

	// The input file URI may start with "fmu://". In that case the
	// FMU's location has to be prepended to the URI accordingly.
	string inputFileUrl = modelDescription.getEntryPoint();
	string inputFilePath;
	processURI( inputFileUrl, fmuLocationTrimmed );
	if ( false == HelperFunctions::getPathFromUrl( inputFileUrl, inputFilePath ) ) {
                ostringstream err;
		err << "invalid URL for input file (entry point): " << inputFileUrl;
		logger( fmiFatal, "URL", err.str() );
		return fmiFatal;
	}

	// Extract PowerFactory project name.
	projectName_ = modelDescription.getModelAttributes().get<string>( "modelName" );
	// Extract PowerFactory target.
	if ( false == parseTarget( &modelDescription ) )
	{
		logger( fmiFatal, "ABORT", "could not parse target" );
		return fmiFatal;
	}

	// Parse number of model variables from model description.
	size_t nRealScalars;
	size_t nIntegerScalars;
	size_t nBooleanScalars;
	size_t nStringScalars;
	modelDescription.getNumberOfVariables( nRealScalars, nIntegerScalars, nBooleanScalars, nStringScalars );
	if ( ( 0 != nIntegerScalars ) && ( 0 != nBooleanScalars ) && ( 0 != nStringScalars ) ) {
		logger( fmiFatal, "ABORT", "only variables of type 'fmiReal' supported" );
		return fmiFatal;
	}

	// All preliminary checks done, create the actual wrapper now.
	pf_ = PowerFactory::create();
	if ( 0 == pf_ ) {
		logger( fmiFatal, "ABORT", "creation of PowerFactory API wrapper failed" );
		return fmiFatal;
	}


	// Set visibility of PowerFactory GUI.
	if ( pf_->Ok != pf_->showUI( visible ) ) {
		logger( fmiFatal, "ABORT", "could not set UI visibility" );
		return fmiFatal;
	}

	// In case there is already a project in PowerFactory's cash with the same name delete it.
	const string deleteCmd = string( "del " ) + target_ + string( "\\" ) + projectName_;
	pf_->execute( deleteCmd.c_str() );

	// Import project file into PowerFactory.
	const string importCmd = string( "pfdimport g_target=" ) + target_ + string( " g_file=" ) + inputFilePath;
	if ( pf_->Ok != pf_->execute( importCmd.c_str() ) )  {
		logger( fmiFatal, "ABORT", "could not import project" );
		return fmiFatal;
	}

	// Actiavte PowerFactory project.
	if ( pf_->Ok != pf_->activateProject( projectName_ ) ) {
		logger( fmiFatal, "ABORT", "could not activate project" );
		return fmiFatal;
	}

	// Instantiate the mechanism for time advance.
	if ( false == instantiateTimeAdvanceMechanism( &modelDescription ) ) {
		return fmiFatal;
	}

	// Initialize wrapper-internal representation of variables.
	if ( false == initializeVariables( &modelDescription ) ) {
		return fmiFatal;
	}

	// Initialize output of extra simulation results to file.
	extraOutput_ = new PowerFactoryExtraOutput( functions_ );
	if ( false == extraOutput_->initializeExtraOutput( pf_ ) ) {
		return fmiFatal;
	}

	return fmiOK;
}


fmiStatus
PowerFactoryFrontEnd::initializeSlave( fmiReal tStart, fmiBoolean stopTimeDefined, fmiReal tStop )
{
	// Initialize starting time.
	fmiStatus status = time_->initialize( tStart, stopTimeDefined, tStop );
	if ( fmiOK != status ) return status;

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

	// Write extra simulation results.
	if ( false == extraOutput_->writeExtraOutput( tStart, pf_ ) ) {
		string err( "not able to write extra simulation results" );
		logger( fmiWarning, "WARNING", err );
		return fmiWarning;
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
	// Advance time in simulation.
	fmiStatus status = time_->advanceTime( comPoint, stepSize );
	if ( fmiOK != status ) return status;

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

	// Write extra simulation results.
	if ( false == extraOutput_->writeExtraOutput( comPoint + stepSize, pf_ ) ) {
		string err( "not able to write extra simulation results" );
		logger( fmiWarning, "WARNING", err );
		return fmiWarning;
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
PowerFactoryFrontEnd::instantiateTimeAdvanceMechanism( const ModelDescription* modelDescription )
{
	// Check if vendor annotations are available.
	using namespace ModelDescriptionUtilities;
	if ( modelDescription->hasVendorAnnotations() )
	{
		// Extract current application name from MIME type.
		string applicationName = modelDescription->getMIMEType().substr( 14 );
		// Extract vendor annotations.
		const Properties& vendorAnnotations = modelDescription->getVendorAnnotations();

		// Check if vendor annotations according to current application are available.
		if ( hasChild( vendorAnnotations, applicationName ) )
		{
			const Properties& annotations = vendorAnnotations.get_child( applicationName );

			// Count numbers of Trigger and DPLScript nodes.
			unsigned int numTriggerNodes = annotations.count( "Trigger" );
			unsigned int numDPLScriptNodes = annotations.count( "DPLScript" );

			// Choose time advance mechanism.
			if ( ( numTriggerNodes > 0 ) && ( numDPLScriptNodes == 0 ) ) {
				// Initialize trigger mechanism.
				time_ = new TriggerTimeAdvance( this, pf_ );
				logger( fmiOK, "TIME-ADVANCE", "use triggers" );
			} else if ( ( numTriggerNodes == 0 ) && ( numDPLScriptNodes > 0 ) ) {
				// Initialize DPL script mechanism.
				time_ = new DPLScriptTimeAdvance( this, pf_ );
				logger( fmiOK, "TIME-ADVANCE", "use DPL script" );
			} else if ( ( numTriggerNodes == 0 ) && ( numDPLScriptNodes == 0 ) ) {
				// Neither triggers nor DPL scripts defined, issue message and abort.
				logger( fmiFatal, "TIME-ADVANCE", "no trigger and no DPL script defined" );
				return false;
			} else if ( ( numTriggerNodes > 0 ) && ( numDPLScriptNodes > 0 ) ) {
				// Both triggers and DPL scripts defined, issue message and abort.
				ostringstream err;
				err << "both triggers (" << numTriggerNodes
				    << ") and DPL scripts (" << numDPLScriptNodes << ") defined";
				logger( fmiFatal, "TIME-ADVANCE", err.str() );
				return false;
			}

			// Instantiate time advance mechanism.
			if ( fmiOK != time_->instantiate( annotations ) ) return false;

		} else {
			string err( "vendor annotations contain no node called '" );
			err += applicationName + string( "'");
			logger( fmiFatal, "ABORT", err );
			return false;
		}

	} else {
		logger( fmiFatal, "ABORT", "no vendor annotations found in model description" );
		return false;
	}

	return true;
}


bool
PowerFactoryFrontEnd::initializeVariables( const ModelDescription* modelDescription )
{
	// Check if model description is available.
	if ( false == modelDescription->hasModelVariables() ) {
		logger( fmiWarning, "WARNING", "model variable description missing" );
		return false;
	}

	// Get variable description.
	const ModelDescription::Properties& modelVariables = modelDescription->getModelVariables();

	PowerFactoryRealScalar* scalar;

	// Iterate through variable decriptions.
	BOOST_FOREACH( const ModelDescription::Properties::value_type &v, modelVariables )
	{
		// Create new scalar for internal representation of variables.
		scalar = new PowerFactoryRealScalar;
		// Initialize scalar according to variable description.
		if ( false == initializeScalar( scalar, v.second, this, pf_ ) ) {
			delete scalar;
			return false;
		}
		// Add scalar to internal map.
		realScalarMap_[scalar->valueReference_] = scalar;
	}

	return true;
}


bool
PowerFactoryFrontEnd::parseTarget( const ModelDescription* modelDescription )
{
	using namespace ModelDescriptionUtilities;

	// Check if vendor annotations are available.
	if ( modelDescription->hasVendorAnnotations() )
	{
		// Extract current application name from MIME type.
		string applicationName = modelDescription->getMIMEType().substr( 14 );
		// Extract vendor annotations.
		const Properties& vendorAnnotations = modelDescription->getVendorAnnotations();

		// Check if vendor annotations according to current application are available.
		if ( hasChild( vendorAnnotations, applicationName ) )
		{
			if ( hasChildAttributes( vendorAnnotations, applicationName ) )
			{
				// Extract target from XML description.
				const Properties& attributes =
					getChildAttributes( vendorAnnotations, applicationName );

				if ( hasChild( attributes, "target" ) ) {
					target_ = attributes.get<string>( "target" );
					return true;
				}
			}

			// Alternatively, get current user name via WIN32 API and use it as target.
			char username[UNLEN+1];
			DWORD username_len = UNLEN+1;
			GetUserName( username, &username_len );
			target_ = string( "\\" ) + username;

			ostringstream log;
			log << "no project target defined in vendor annotations, "
			    << "will use current user name: " << target_;
			logger( fmiOK, "TARGET", log.str() );

			return true;
		} else {
			ostringstream err;
			err << "vendor annotations do not contain information specific to PowerFactory "
			    << "(XML node '" << applicationName << "' is missing)";
			logger( fmiFatal, "XML", err.str() );
		}
	} else {
		string err( "no vendor annotations found in model description" );
		logger( fmiFatal, "XML", err );
	}

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


bool
initializeScalar( PowerFactoryRealScalar* scalar,
		  const ModelDescription::Properties& description,
		  PowerFactoryFrontEnd* frontend,
		  PowerFactory* pf )
{
	using namespace ScalarVariableAttributes;
	using namespace ModelDescriptionUtilities;

	// Get XML attributes from scalar description.
	const Properties& attributes = getAttributes( description );

	// Parse class name, object name and parameter name from description.
	bool parseStatus =
		PowerFactoryRealScalar::parseFMIVariableName( attributes.get<string>( "name" ),
							      scalar->className_,
							      scalar->objectName_,
							      scalar->parameterName_ );

	if ( false == parseStatus ) {
		ostringstream err;
		err << "bad variable name: " << attributes.get<string>( "name" );
		frontend->logger( fmiWarning, "WARNING", err.str() );
		return false;
	}

	// Extract information regarding value reference, causality and variability.
	scalar->valueReference_ = attributes.get<int>( "valueReference" );
	scalar->causality_ = getCausality( attributes.get<string>( "causality" ) );
	scalar->variability_ = getVariability( attributes.get<string>( "variability" ) );

	api::DataObject* dataObj = 0;
	int check = -1;
	// Search for PowerFactory object by class name and object name.
	check = pf->getCalcRelevantObject( scalar->className_, scalar->objectName_, dataObj );
	if ( check != pf->Ok )
	{
		ostringstream err;
		err << "unable to get object: " << scalar->objectName_
		    << " (type " << scalar->className_ << ")";
		frontend->logger( fmiWarning, "WARNING", err.str() );
		return false;
	} else if ( 0 != dataObj ) {
		scalar->apiDataObject_ = dataObj;
	}

	if ( hasChildAttributes( description, "Real" ) )
	{
		// This wrapper handles only variables of type 'fmiReal'!
		const Properties& properties = getChildAttributes( description, "Real" );

		// Check if a start value has been defined.
		if ( properties.find( "start" ) != properties.not_found() ) {

			// // Check if scalar is defined as input.
			// if ( scalar->causality_ != ScalarVariableAttributes::input ) {
			// 	ostringstream err;
			// 	err << "not an input: " << attributes.get<string>( "name" );
			// 	frontend->logger( fmiWarning, "WARNING", err.str() );
			// 	return false;
			// }
 
			fmiReal start = properties.get<fmiReal>( "start" );

			// Set value of parameter of PowerFactory object using the parameter name.
			check = pf->setAttributeDouble( dataObj, scalar->parameterName_.c_str(), start );
			if ( check != pf->Ok ) 
			{
				ostringstream err;
				err << "unable to set attribute: " << attributes.get<string>( "name" );
				frontend->logger( fmiWarning, "WARNING", err.str() );
				return false;
			}
		}
	}

	/// \FIXME What about the remaining properties?

	return true;
}
