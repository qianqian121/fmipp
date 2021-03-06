# --------------------------------------------------------------
# Copyright (c) 2013, AIT Austrian Institute of Technology GmbH.
# All rights reserved. See file FMIPP_LICENSE for details.
# --------------------------------------------------------------

cmake_minimum_required( VERSION 2.8.12 )

project( fmippim_wrap )

find_package( SWIG )
if ( SWIG_FOUND )
   include( UseSWIG )

   set_source_files_properties( libfmippim.i PROPERTIES CPLUSPLUS ON )
   if ( INCLUDE_SUNDIALS )
      set_source_files_properties( libfmippim.i PROPERTIES SWIG_FLAGS "-DUSE_SUNDIALS" )
   endif()

   # swig does not yet support matlab, so the following was tested with swig based on https://github.com/jaeandersson/swig
   if ( BUILD_SWIG_MATLAB )
      find_package( Matlab REQUIRED COMPONENTS MX_LIBRARY )

      message( "foo: ${Matlab_ROOT_DIR}/bin/mex" )
      
      include_directories( ${Matlab_INCLUDE_DIRS} )

      set( FMIPP_WRAP_MATLAB_MODULE "fmippim_wrap_matlab" )
      # the output name of the matlab mex-file has to be the same as the module with MEX added, since the swig implementation i'm using wants it that way...
      
      swig_add_module( ${FMIPP_WRAP_MATLAB_MODULE} matlab libfmippim.i )

      if ( WIN32 )
      	 if ( CMAKE_SIZEOF_VOID_P EQUAL 8 )
	    set_target_properties( ${SWIG_MODULE_${FMIPP_WRAP_MATLAB_MODULE}_REAL_NAME} PROPERTIES OUTPUT_NAME "fmippimMEX" PREFIX "" SUFFIX ".mexw64" )
	 else ()
	    set_target_properties( ${SWIG_MODULE_${FMIPP_WRAP_MATLAB_MODULE}_REAL_NAME} PROPERTIES OUTPUT_NAME "fmippimMEX" PREFIX "" SUFFIX ".mexw32" )
	 endif ()
      elseif( APPLE )
      	 set_target_properties( ${SWIG_MODULE_${FMIPP_WRAP_MATLAB_MODULE}_REAL_NAME} PROPERTIES OUTPUT_NAME "fmippimMEX" PREFIX "" SUFFIX ".mexmaci64" )
      elseif( UNIX )
         set_target_properties( ${SWIG_MODULE_${FMIPP_WRAP_MATLAB_MODULE}_REAL_NAME} PROPERTIES OUTPUT_NAME "fmippimMEX" PREFIX "" SUFFIX ".mexa64" )
      endif ()

      swig_link_libraries( ${FMIPP_WRAP_MATLAB_MODULE} ${Matlab_LIBRARIES} )
      swig_link_libraries( ${FMIPP_WRAP_MATLAB_MODULE} fmippim )
   endif ()

   if ( BUILD_SWIG_OCTAVE )
      find_package( HDF5 REQUIRED )

      if ( NOT DEFINED OCTAVE_INCLUDE_DIRS )
         message( FATAL_ERROR "Variable OCTAVE_INCLUDE_DIRS is not defined!" )
      endif()
      
      include_directories( ${OCTAVE_INCLUDE_DIRS} )
      include_directories( ${OCTAVE_INCLUDE_DIRS}/octave )
      include_directories( ${HDF5_INCLUDE_DIRS} )

      set( FMIPP_WRAP_OCTAVE_MODULE "fmippim_wrap_octave" )
      # the output name of the octave .oct-file has to be the same as the module name, otherwise octave does not find it when importing...

      swig_add_module( ${FMIPP_WRAP_OCTAVE_MODULE} octave libfmippim.i )
      set_target_properties( ${SWIG_MODULE_${FMIPP_WRAP_OCTAVE_MODULE}_REAL_NAME} PROPERTIES OUTPUT_NAME "fmippim" )
      swig_link_libraries( fmippim_wrap_octave fmippim )
      if( APPLE )
            swig_link_libraries( fmippim_wrap_octave ${OCTAVE_LIBRARIES}/liboctave.dylib )
      	    swig_link_libraries( fmippim_wrap_octave ${OCTAVE_LIBRARIES}/liboctinterp.dylib )
      endif ()
   endif()

   if ( BUILD_SWIG_PYTHON )
      find_package( PythonInterp REQUIRED )
      find_package( PythonLibs REQUIRED )
      include_directories( ${PYTHON_INCLUDE_DIRS} )
	  set(CMAKE_SWIG_OUTDIR ${CMAKE_CURRENT_BINARY_DIR}/fmipp)

      set( FMIPP_WRAP_PYTHON_MODULE "fmippim_wrap_python" )
      # change the output name of the python dynamic library to _fmippim, otherwise pyhton does not find it when importing...

      swig_add_module( ${FMIPP_WRAP_PYTHON_MODULE} python libfmippim.i )
	  set_target_properties( ${SWIG_MODULE_${FMIPP_WRAP_PYTHON_MODULE}_REAL_NAME} PROPERTIES OUTPUT_NAME "fmipp/_fmippim" )
      swig_link_libraries( ${FMIPP_WRAP_PYTHON_MODULE} fmippim ${PYTHON_LIBRARIES} )


	  set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
	  set(SETUP_PY    "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
	  set(DEPS        "${CMAKE_CURRENT_SOURCE_DIR}/__init__.py")
	  set(OUTPUT   	  "${CMAKE_CURRENT_BINARY_DIR}/fmipp/__init__.py")

	  configure_file(${SETUP_PY_IN} ${SETUP_PY})

	  add_custom_command(OUTPUT ${OUTPUT}
			  COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} build
			  COMMAND ${CMAKE_COMMAND} -E touch ${OUTPUT}
			  DEPENDS ${DEPS})
	  add_custom_target(target ALL DEPENDS ${OUTPUT})


	  install(CODE "execute_process(COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} install)")
      
   endif()

   if ( BUILD_SWIG_JAVA )
      find_package( Java REQUIRED )
      include( UseJava )
      find_package( JNI REQUIRED )
      include_directories( ${JAVA_INCLUDE_PATH} )
      include_directories( ${JAVA_INCLUDE_PATH2} )

      # ATTENTION: The following lines are needed to place a generated java wrapper into an appropriate package.
      
      # However, the according flag causes a swig error when generating a Python wrapper (unrecognized option).
      # Therefore, the following command has to issued only AFTER the python wrapper module has been declared!
      set_property( SOURCE libfmippim.i PROPERTY SWIG_FLAGS "-package" "at.ait.fmipp" )

      swig_add_module( fmippim_wrap_java java libfmippim.i )
      swig_link_libraries( fmippim_wrap_java fmippim )
   
      if ( WIN32 )
         if ( MINGW )
            # add aliases because otherwise JNI doesn't find the functions in the dll
            set_target_properties( fmippim_wrap_java PROPERTIES LINK_FLAGS "-Wl,--add-stdcall-alias" )
	 endif ()
      endif ()

      set( FMIPPIM_JAR_FILENAME "fmipp.jar" )

      add_custom_command( OUTPUT ${FMIPPIM_JAR_FILENAME} DEPENDS fmippim_wrap_java
        COMMAND ${CMAKE_COMMAND} -E echo "Compiling Java files..."
	COMMAND ${Java_JAVAC_EXECUTABLE} at/ait/fmipp/*.java
	COMMAND ${CMAKE_COMMAND} -E echo "Creating jar file..."
	COMMAND ${Java_JAR_EXECUTABLE} -cf ${FMIPPIM_JAR_FILENAME} at/ait/fmipp/*.class )

      add_custom_target( fmipp_jar ALL DEPENDS ${FMIPPIM_JAR_FILENAME} )

      add_custom_command( TARGET fmippim_wrap_java POST_BUILD
	COMMAND ${Java_JAR_EXECUTABLE} -cf temp.zip *.java
	COMMAND ${CMAKE_COMMAND} -E make_directory at/ait/fmipp
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )
			      
      add_custom_command( TARGET fmippim_wrap_java POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_CURRENT_BINARY_DIR}/temp.zip temp.zip
	COMMAND ${Java_JAR_EXECUTABLE} -xf temp.zip
	COMMAND ${CMAKE_COMMAND} -E remove temp.zip
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/at/ait/fmipp )

      install( TARGETS fmippim_wrap_java DESTINATION lib )
      install( FILES ${CMAKE_CURRENT_BINARY_DIR}/${FMIPPIM_JAR_FILENAME} DESTINATION lib )

      set( FMIPPIM_JAR ${CMAKE_CURRENT_BINARY_DIR}/${FMIPPIM_JAR_FILENAME} CACHE INTERNAL "find the created jar. necessary because add_jar and wildcards don't work out here and so it's complicated with dependencies." )
      set( FMIPPIM_WRAP_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE INTERNAL "have the directory because we need the path where to look for the dynamic library." )

   endif()

endif ( SWIG_FOUND )
