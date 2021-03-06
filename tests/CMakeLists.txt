cmake_minimum_required(VERSION 2.8)
if("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}" VERSION_GREATER 2.8.3)
  # Explicitly use new include policy to avoid globally shadowing included modules
  # http://www.cmake.org/cmake/help/cmake-2-8-docs.html#policy:CMP0017
  cmake_policy(SET CMP0017 NEW)
endif()
project(tests)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")

# Build debug build as default
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
endif()

#
# Windows
#

if(WIN32)
  set(WIN32_STATIC_BUILD "True")
endif()

if(WIN32_STATIC_BUILD)
  if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(EMSG "\nTo build Win32 STATIC OpenSCAD tests you must run")
    set(EMSG "${EMSG} \ncmake .. -DCMAKE_BUILD_TYPE=Release")
    set(EMSG "${EMSG} \nthen replace /MD with /MT in CMakeCache.txt")
    set(EMSG "${EMSG} \ni.e. sed -i s/\\/MD/\\/MT/ CMakeCache.txt") 
    set(EMSG "${EMSG} \nthen re-run cmake ..") 
    message(FATAL_ERROR ${EMSG})
  endif()
endif()

# Disable warnings
if(WIN32)
  # too long decorated names
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4503")
  # int cast to bool in CGAL
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4800")
  # unreferenced parameters in CGAL
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4100")
  # fopen_s advertisement
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_DEPRECATE")
  # lexer uses strdup & other POSIX stuff
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_NONSTDC_NO_DEPRECATE")
  # M_PI
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_USE_MATH_DEFINES")
endif()

# Debugging - if you uncomment, use nmake -f Makefile > log.txt (the log is big)
if(WIN32)
  # Linker debugging
  #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -VERBOSE")

  # Compiler debugging
  # you have to pass -DCMAKE_VERBOSE_MAKEFILE=ON to cmake when you run it. 
endif()


#
# Build test apps
#

# Boost
#
# usually it's found automatically, but some systems may need a custom install.
# in that case, run cmake with -DBOOST_ROOT=/path/to/boost/install
# (being the same path you passed to boost's --prefix when you built it)
if (NOT $ENV{MACOSX_DEPLOY_DIR} STREQUAL "")
  set(BOOST_ROOT "$ENV{MACOSX_DEPLOY_DIR}")
endif()

if(BOOST_ROOT)
  #set(Boost_DEBUG TRUE)
  set(Boost_NO_SYSTEM_PATHS TRUE)
  set(Boost_ADDITIONAL_VERSIONS "1.47.0")
  find_package( Boost 1.35.0 COMPONENTS thread program_options )
  if(Boost_FOUND)
    message(STATUS "Boost includes found: " ${Boost_INCLUDE_DIRS})
    message(STATUS "Boost libraries found:")
    foreach(boostlib ${Boost_LIBRARIES})
      message(STATUS "  " ${boostlib})
    endforeach()
    include_directories(${Boost_INCLUDE_DIRS})
  else()
    message(STATUS "BOOST_ROOT:" ${BOOST_ROOT})
    message(FATAL_ERROR "BOOST_ROOT specified but no boost found")
  endif()
else()
  message(STATUS "BOOST_ROOT unset. Assuming it will be found automatically.")
endif()

# Mac OS X
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  FIND_LIBRARY(COCOA_LIBRARY Cocoa)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

# Qt4
find_package(OpenGL)
find_package(Qt4 COMPONENTS QtCore QtGui QtOpenGL REQUIRED)
include(${QT_USE_FILE})

# Eigen2

# Turn off Eigen SIMD optimization
if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DEIGEN_DONT_ALIGN")
  endif()
endif()

if (NOT EIGEN2_INCLUDE_DIR)
  find_path(EIGEN2_INCLUDE_DIR
            Eigen/Core 
            PATHS ENV EIGEN2DIR /opt/local/include/eigen2 /usr/include/eigen2)
  if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD") 
    find_path(EIGEN2_INCLUDE_DIR 
              Eigen/Core 
              PATHS ENV EIGEN2DIR /usr/local/include/eigen2 )
  endif()
  if (NOT EIGEN2_INCLUDE_DIR)
    message(FATAL_ERROR "Eigen2 not found")
  else()
    message(STATUS "Eigen2 found in " ${EIGEN2_INCLUDE_DIR})
  endif()
endif()
include_directories(${EIGEN2_INCLUDE_DIR})

# OpenCSG
if (NOT $ENV{OPENCSG_DIR} STREQUAL "")
  set(OPENCSG_DIR "$ENV{OPENCSG_DIR}")
elseif (NOT $ENV{MACOSX_DEPLOY_DIR} STREQUAL "")
  set(OPENCSG_DIR "$ENV{MACOSX_DEPLOY_DIR}")
endif()
if (NOT OPENCSG_INCLUDE_DIR)
  message(STATUS "OPENCSG_DIR: " ${OPENCSG_DIR})
  find_path(OPENCSG_INCLUDE_DIR
            opencsg.h
            PATHS ${OPENCSG_DIR}/include)
  find_library(OPENCSG_LIBRARY
               opencsg
               PATHS ${OPENCSG_DIR}/lib)
  if (NOT OPENCSG_INCLUDE_DIR OR NOT OPENCSG_LIBRARY)
    message(FATAL_ERROR "OpenCSG not found")
  else()
    message(STATUS "OpenCSG include found in " ${OPENCSG_INCLUDE_DIR})
    message(STATUS "OpenCSG library found in " ${OPENCSG_LIBRARY})
  endif()
endif()
include_directories(${OPENCSG_INCLUDE_DIR})

# GLEW

if (NOT $ENV{MACOSX_DEPLOY_DIR} STREQUAL "")
  set(GLEW_DIR "$ENV{MACOSX_DEPLOY_DIR}")
endif()

find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_PATH})

if(WIN32_STATIC_BUILD)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGLEW_STATIC")
endif()

# Flex/Bison
find_package(BISON)

if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
  # FreeBSD has an old flex in /usr/bin and a new flex in /usr/local/bin
  set(FLEX_EXECUTABLE /usr/local/bin/flex)
endif()

find_package(FLEX)
# The COMPILE_FLAGS and forced C++ compiler is just to be compatible with qmake
if (WIN32)
  set(FLEX_UNISTD_FLAG "-DYY_NO_UNISTD_H")
endif()
FLEX_TARGET(OpenSCADlexer ../src/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp COMPILE_FLAGS "-Plexer ${FLEX_UNISTD_FLAG}")
BISON_TARGET(OpenSCADparser ../src/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser_yacc.c COMPILE_FLAGS "-p parser")
ADD_FLEX_BISON_DEPENDENCY(OpenSCADlexer OpenSCADparser)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/parser_yacc.c PROPERTIES LANGUAGE "CXX")

if (NOT $ENV{MACOSX_DEPLOY_DIR} STREQUAL "")
  set(CGAL_DIR "$ENV{MACOSX_DEPLOY_DIR}/lib/CGAL")
  set(CMAKE_MODULE_PATH "${CGAL_DIR}")
endif()
find_package(CGAL REQUIRED)
if("${CGAL_MAJOR_VERSION}.${CGAL_MINOR_VERSION}" VERSION_LESS 3.6)
  message(FATAL_ERROR "CGAL >= 3.6 required")
endif()
include_directories(${CGAL_INCLUDE_DIRS})

# Internal includes
include_directories(../src)

add_definitions(-DOPENSCAD_VERSION=test -DOPENSCAD_YEAR=2011 -DOPENSCAD_MONTH=10)
add_definitions(-DOPENSCAD_TESTING)

set(CORE_SOURCES
  tests-common.cc 
  ../src/mathc99.cc
  ../src/handle_dep.cc 
  ../src/value.cc 
  ../src/expr.cc 
  ../src/func.cc 
  ../src/module.cc 
  ../src/node.cc 
  ../src/context.cc 
  ../src/csgterm.cc 
  ../src/polyset.cc 
  ../src/csgops.cc 
  ../src/transform.cc 
  ../src/color.cc 
  ../src/primitives.cc 
  ../src/projection.cc 
  ../src/cgaladv.cc 
  ../src/surface.cc 
  ../src/control.cc 
  ../src/render.cc 
  ../src/dxfdata.cc 
  ../src/dxfdim.cc 
  ../src/linearextrude.cc 
  ../src/rotateextrude.cc 
  ../src/printutils.cc 
  ../src/progress.cc 
  ${FLEX_OpenSCADlexer_OUTPUTS}
  ${BISON_OpenSCADparser_OUTPUTS})

set(NOCGAL_SOURCES
  ../src/builtin.cc 
  ../src/dxftess.cc 
  ../src/import.cc
  ../src/export.cc) 

set(CGAL_SOURCES
  ${NOCGAL_SOURCES}
  ../src/CSGTermEvaluator.cc 
  ../src/CGAL_Nef_polyhedron.cc 
  ../src/cgalutils.cc 
  ../src/CGALEvaluator.cc
  ../src/CGALCache.cc
  ../src/PolySetCGALEvaluator.cc
  ../src/CGAL_Nef_polyhedron_DxfData.cc
  ../src/cgaladv_minkowski2.cc)

set(COMMON_SOURCES
  ../src/nodedumper.cc 
  ../src/traverser.cc 
  ../src/PolySetEvaluator.cc 
  ../src/PolySetCache.cc 
  ../src/Tree.cc
  lodepng.cpp)

#
# Offscreen OpenGL context source code
#
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	message(STATUS "Offscreen OpenGL Context - using Apple CGL")
	set(OFFSCREEN_CTX_SOURCE "OffscreenContext.mm")
elseif(UNIX)
	message(STATUS "Offscreen OpenGL Context - using Unix GLX")
	set(OFFSCREEN_CTX_SOURCE "OffscreenContext.cc")
elseif(WIN32)
	message(STATUS "Offscreen OpenGL Context - using Microsoft WGL")
	set(OFFSCREEN_CTX_SOURCE "OffscreenContextWGL.cc")
endif()

set(OFFSCREEN_SOURCES
  OffscreenView.cc
  ${OFFSCREEN_CTX_SOURCE}
  imageutils.cc
  fbo.cc
  system-gl.cc)

add_library(tests-core STATIC ${CORE_SOURCES})
add_library(tests-common STATIC ${COMMON_SOURCES})
target_link_libraries(tests-common tests-core)
add_library(tests-cgal STATIC ${CGAL_SOURCES})
set_target_properties(tests-cgal PROPERTIES COMPILE_FLAGS "-DENABLE_CGAL ${CGAL_CXX_FLAGS_INIT}")
target_link_libraries(tests-cgal tests-common)
add_library(tests-nocgal STATIC ${NOCGAL_SOURCES})
target_link_libraries(tests-nocgal tests-common)
add_library(tests-offscreen STATIC ${OFFSCREEN_SOURCES})

#
# echotest
#
add_executable(echotest echotest.cc)
target_link_libraries(echotest tests-nocgal tests-core ${QT_LIBRARIES} ${OPENGL_LIBRARY} ${Boost_LIBRARIES})

#
# Yangli Hector Yee's PerceptualDiff code
# FIXME: Disabled since we use ImageMagick now. Eventually remove this and the files.
# add_executable(yee_compare yee_compare.cpp lodepng.cpp)

#
# dumptest
#
add_executable(dumptest dumptest.cc)
target_link_libraries(dumptest tests-common tests-nocgal ${QT_LIBRARIES} ${OPENGL_LIBRARY} ${Boost_LIBRARIES})

#
# csgtexttest
#
add_executable(csgtexttest csgtexttest.cc CSGTextRenderer.cc CSGTextCache.cc)
target_link_libraries(csgtexttest tests-common tests-nocgal ${QT_LIBRARIES} ${OPENGL_LIBRARY} ${Boost_LIBRARIES})

#
# csgtermtest
#
add_executable(csgtermtest csgtermtest.cc ../src/CSGTermEvaluator.cc)
target_link_libraries(csgtermtest tests-common tests-nocgal ${QT_LIBRARIES} ${OPENGL_LIBRARY} ${Boost_LIBRARIES})

#
# cgaltest
#
add_executable(cgaltest cgaltest.cc)
set_target_properties(cgaltest PROPERTIES COMPILE_FLAGS "-DENABLE_CGAL ${CGAL_CXX_FLAGS_INIT}")
target_link_libraries(cgaltest tests-cgal ${CGAL_LIBRARY} ${CGAL_3RD_PARTY_LIBRARIES} ${QT_LIBRARIES} ${OPENGL_LIBRARY} ${Boost_LIBRARIES})

#
# cgalpngtest
#
add_executable(cgalpngtest cgalpngtest.cc bboxhelp.cc ../src/CGALRenderer.cc)
set_target_properties(cgalpngtest PROPERTIES COMPILE_FLAGS "-DENABLE_CGAL ${CGAL_CXX_FLAGS_INIT}")
target_link_libraries(cgalpngtest tests-offscreen tests-cgal ${CGAL_LIBRARY} ${CGAL_3RD_PARTY_LIBRARIES} ${QT_LIBRARIES}  ${GLEW_LIBRARY} ${COCOA_LIBRARY} ${OPENGL_LIBRARY} ${Boost_LIBRARIES})

#
# opencsgtest
#

add_executable(opencsgtest opencsgtest.cc csgtestcore.cc ../src/OpenCSGRenderer.cc ../src/ThrownTogetherRenderer.cc)
set_target_properties(opencsgtest PROPERTIES COMPILE_FLAGS "-DENABLE_OPENCSG -DENABLE_CGAL ${CGAL_CXX_FLAGS_INIT}")
target_link_libraries(opencsgtest tests-offscreen tests-cgal ${CGAL_LIBRARY} ${CGAL_3RD_PARTY_LIBRARIES} ${QT_LIBRARIES} ${OPENCSG_LIBRARY} ${GLEW_LIBRARY} ${COCOA_LIBRARY} ${OPENGL_LIBRARY} ${Boost_LIBRARIES})

#
# throwntogethertest
#

add_executable(throwntogethertest throwntogethertest.cc csgtestcore.cc ../src/OpenCSGRenderer.cc ../src/ThrownTogetherRenderer.cc)
set_target_properties(throwntogethertest PROPERTIES COMPILE_FLAGS "-DENABLE_OPENCSG -DENABLE_CGAL ${CGAL_CXX_FLAGS_INIT}")
target_link_libraries(throwntogethertest tests-offscreen tests-cgal ${CGAL_LIBRARY} ${CGAL_3RD_PARTY_LIBRARIES} ${QT_LIBRARIES} ${OPENCSG_LIBRARY} ${GLEW_LIBRARY} ${COCOA_LIBRARY} ${OPENGL_LIBRARY} ${Boost_LIBRARIES})

#
# Tags tests as disabled. This is more convenient than removing them manually
# from the lists of filenames
#
macro(disable_tests)
  foreach (TESTNAME ${ARGN})
#    message("Disabling ${TESTNAME}")
    list(APPEND DISABLED_TESTS ${TESTNAME})
  endforeach()
endmacro()

#
# Tags the given tests as belonging to the given CONFIG, i.e. will
# only be executed when run using ctest -C <CONFIG>
#
# Usage example: set_test_config(Heavy dumptest_testname opencsgtest_testname2)
#
function(set_test_config CONFIG)
  list(APPEND ${CONFIG}_TEST_CONFIG ${ARGN})
  list(FIND TEST_CONFIGS ${CONFIG} FOUND)
  if (FOUND EQUAL -1)
    list(APPEND TEST_CONFIGS ${CONFIG})
    # Export to parent scope
    set(TEST_CONFIGS ${TEST_CONFIGS} PARENT_SCOPE)
  endif()
  # Export to parent scope
  set(${CONFIG}_TEST_CONFIG ${${CONFIG}_TEST_CONFIG} PARENT_SCOPE)
endfunction()

#
# Returns a list of test configs 
#
function(get_test_config TESTNAME CONFIGS)
  foreach(CONFIG ${TEST_CONFIGS})
    list(FIND ${CONFIG}_TEST_CONFIG ${TESTNAME} IDX)
    if (${IDX} GREATER -1)
      list(APPEND ${CONFIGS} ${CONFIG})
    endif()
  endforeach()
  if (${CONFIGS})
    # Convert to a format understood by add_test()
    string(REPLACE ";" "|" ${${CONFIGS}} ${CONFIGS})
    # Export to parent scope
    set(${CONFIGS} ${${CONFIGS}} PARENT_SCOPE)
  endif()
endfunction()

#
# Returns into the FULLNAME variable the global full test name (identifier) 
# given a test command and source filename
#
function(get_test_fullname TESTCMD FILENAME FULLNAME)
  get_filename_component(TESTCMD_NAME ${TESTCMD} NAME_WE)
  get_filename_component(TESTNAME ${FILENAME} NAME_WE)
  string(REPLACE " " "_" TESTNAME ${TESTNAME}) # Test names cannot include spaces
  set(${FULLNAME} ${TESTCMD_NAME}_${TESTNAME})
  # Export to parent scope
  set(${FULLNAME} ${${FULLNAME}} PARENT_SCOPE)
endfunction()

#
# This functions adds cmd-line tests given files.
# Files are sent as the parameters following TESTSUFFIX
#
# Usage add_cmdline_test(TESTCMD TESTSUFFIX <test files>)
#
find_package(PythonInterp)
macro(add_cmdline_test TESTCMD TESTSUFFIX)
  get_filename_component(TESTCMD_NAME ${TESTCMD} NAME_WE)

  # Add tests from args
  foreach (SCADFILE ${ARGN})
    get_filename_component(TESTNAME ${SCADFILE} NAME_WE)
    string(REPLACE " " "_" TESTNAME ${TESTNAME}) # Test names cannot include spaces
    set(TEST_FULLNAME "${TESTCMD_NAME}_${TESTNAME}")
    list(FIND DISABLED_TESTS ${TEST_FULLNAME} DISABLED)
    if (${DISABLED} EQUAL -1)

      # Handle configurations
      unset(FOUNDCONFIGS)
      get_test_config(${TEST_FULLNAME} FOUNDCONFIGS)
      if (NOT FOUNDCONFIGS)
        set_test_config(Default ${TEST_FULLNAME})
      endif()
      set_test_config(All ${TEST_FULLNAME})
      unset(FOUNDCONFIGS)
      get_test_config(${TEST_FULLNAME} FOUNDCONFIGS)
      set(CONFARG CONFIGURATIONS)
      set(CONFVAL ${FOUNDCONFIGS})

      add_test(NAME ${TEST_FULLNAME} ${CONFARG} ${CONFVAL} COMMAND ${PYTHON_EXECUTABLE} ${tests_SOURCE_DIR}/test_cmdline_tool.py -s ${TESTSUFFIX} ${CMAKE_BINARY_DIR}/${TESTCMD} "${SCADFILE}")
    endif()
  endforeach()
endmacro()

enable_testing()

set_directory_properties(PROPERTIES TEST_INCLUDE_FILE "${CMAKE_SOURCE_DIR}/EnforceConfig.cmake")

# Find all scad files
file(GLOB MINIMAL_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/minimal/*.scad)
file(GLOB FEATURES_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/features/*.scad)
# Remove included files not to be used as tests
list(REMOVE_ITEM FEATURES_FILES
     ${CMAKE_SOURCE_DIR}/../testdata/scad/features/include\ test6.scad
     ${CMAKE_SOURCE_DIR}/../testdata/scad/features/include-test5.scad)
file(GLOB BUGS_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/*.scad)
file(GLOB SCAD_DXF_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/dxf/*.scad)
file(GLOB FUNCTION_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/functions/*.scad)
file(GLOB EXAMPLE_FILES ${CMAKE_SOURCE_DIR}/../examples/*.scad)

list(APPEND ECHO_FILES ${FUNCTION_FILES}
            ${CMAKE_SOURCE_DIR}/../testdata/scad/minimal/echo.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/echo-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/escape-test.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/parser-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/builtin-tests.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/dim-all.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/string-test.scad
            ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/string-indexing.scad)

list(APPEND DUMPTEST_FILES ${MINIMAL_FILES} ${FEATURES_FILES} ${EXAMPLE_FILES})
list(APPEND DUMPTEST_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/misc/escape-test.scad)

list(APPEND CGALPNGTEST_FILES ${FEATURES_FILES} ${SCAD_DXF_FILES} ${EXAMPLE_FILES})
list(APPEND OPENCSGTEST_FILES ${CGALPNGTEST_FILES})
list(APPEND THROWNTOGETHERTEST_FILES ${CGALPNGTEST_FILES})

# Disable tests which are known to cause floating point comparison issues
# Once we're capable of comparing these across platforms, we can put these back in
disable_tests(dumptest_transform-tests
              dumptest_render-tests
              dumptest_example001
              dumptest_example005
              dumptest_example006
              dumptest_example007
              dumptest_example008
              dumptest_example012
              dumptest_example016
              dumptest_example020
              dumptest_example021)

# FIXME: This test illustrates a weakness in child() combined with modifiers.
# Reenable it when this is improved
disable_tests(opencsgtest_child-background)

# These tests only makes sense in OpenCSG mode
disable_tests(cgalpngtest_child-background
              cgalpngtest_highlight-and-background-modifier
              cgalpngtest_testcolornames
              throwntogethertest_child-background
              throwntogethertest_highlight-and-background-modifier
              throwntogethertest_testcolornames)

# Test config handling

set_test_config(Heavy opencsgtest_minkowski3-tests
                      opencsgtest_projection-tests
                      throwntogethertest_minkowski3-tests
                      throwntogethertest_projection-tests
                      cgalpngtest_projection-tests
                      cgalpngtest_rotate_extrude-tests
                      cgalpngtest_surface-tests
                      cgalpngtest_sphere-tests
                      cgalpngtest_minkowski3-tests
                      cgalpngtest_for-tests
                      cgalpngtest_for-nested-tests
                      cgalpngtest_difference-tests
                      cgalpngtest_intersection-tests)

foreach(FILE ${EXAMPLE_FILES})
  get_test_fullname(cgalpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Examples ${TEST_FULLNAME})
  get_test_fullname(opencsgtest ${FILE} TEST_FULLNAME)
  set_test_config(Examples ${TEST_FULLNAME})
  get_test_fullname(throwntogethertest ${FILE} TEST_FULLNAME)
  set_test_config(Examples ${TEST_FULLNAME})
endforeach()

# Add tests

add_cmdline_test(echotest txt ${ECHO_FILES})
add_cmdline_test(dumptest txt ${DUMPTEST_FILES})
add_cmdline_test(csgtexttest txt ${MINIMAL_FILES})
add_cmdline_test(csgtermtest txt ${MINIMAL_FILES})
add_cmdline_test(cgalpngtest png ${CGALPNGTEST_FILES})
add_cmdline_test(opencsgtest png ${OPENCSGTEST_FILES})
add_cmdline_test(throwntogethertest png ${THROWNTOGETHERTEST_FILES})

message("Available test configurations: ${TEST_CONFIGS}")
#foreach(CONF ${TEST_CONFIGS})
#  message("${CONF}: ${${CONF}_TEST_CONFIG}")
#endforeach()
