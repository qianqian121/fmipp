// --------------------------------------------------------------
// Copyright (c) 2013, AIT Austrian Institute of Technology GmbH.
// All rights reserved. See file FMIPP_LICENSE for details.
// --------------------------------------------------------------
#include <iostream>
#include <stdlib.h>
#include <common/fmi_v1.0/fmiModelTypes.h>
#include <common/FMIPPConfig.h>
#include <import/base/include/ModelManager.h>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE testModelDescription
#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_CASE( test_model_manager_me )
{
	std::string modelName( "zigzag" );
	std::string fmuUrl = std::string( FMU_URI_PRE ) + modelName;

	ModelManager& manager = ModelManager::getModelManager();

	BareFMUModelExchange* bareFMU1 = manager.getModel( fmuUrl, modelName, fmiTrue );
	BareFMUModelExchange* bareFMU2 = manager.getModel( fmuUrl, modelName, fmiTrue );

	BOOST_REQUIRE_MESSAGE( bareFMU1 == bareFMU2,
			       "Bare FMUs are not equal." );
}


BOOST_AUTO_TEST_CASE( test_model_manager_me_no_file )
{
	std::string modelName( "idontexist" );
	std::string fmuUrl = std::string( FMU_URI_PRE ) + modelName;

	ModelManager& manager = ModelManager::getModelManager();

	BareFMUModelExchange* bareFMU = manager.getModel( fmuUrl, modelName, fmiTrue );
	BOOST_REQUIRE( 0 == bareFMU );
}


BOOST_AUTO_TEST_CASE( test_model_manager_me_no_v1_0 )
{
	std::string modelName( "v2_0" );
	std::string fmuUrl = std::string( FMU_URI_PRE ) + modelName;

	ModelManager& manager = ModelManager::getModelManager();

	BareFMUModelExchange* bareFMU = manager.getModel( fmuUrl, modelName, fmiTrue );
	BOOST_REQUIRE( 0 == bareFMU );
}


BOOST_AUTO_TEST_CASE( test_model_manager_cs )
{
	std::string modelName( "sine_standalone" );
	std::string fmuUrl = std::string( FMU_URI_PRE ) + modelName;

	ModelManager& manager = ModelManager::getModelManager();

	BareFMUCoSimulation* bareFMU1 = manager.getSlave( fmuUrl, modelName, fmiTrue );
	BareFMUCoSimulation* bareFMU2 = manager.getSlave( fmuUrl, modelName, fmiTrue );

	BOOST_REQUIRE_MESSAGE( bareFMU1 == bareFMU2,
			       "Bare FMUs are not equal." );
}
