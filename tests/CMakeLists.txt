cmake_minimum_required(VERSION 2.8)
project(tests)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")

# Build debug build as default
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

#
# Build test apps
#

# Qt4
find_package(OpenGL)
find_package(Qt4 COMPONENTS QtCore QtGui QtOpenGL REQUIRED)
include(${QT_USE_FILE})

# Eigen2
if (NOT EIGEN2_INCLUDE_DIR)
  find_path(EIGEN2_INCLUDE_DIR
            Eigen/Core 
            PATHS ENV EIGEN2DIR /opt/local/include/eigen2 /usr/include/eigen2)
  if (NOT EIGEN2_INCLUDE_DIR)
    message(FATAL_ERROR "Eigen2 not found")
  else()
    message(STATUS "Eigen2 found in " ${EIGEN2_INCLUDE_DIR})
  endif()
endif()
include_directories(${EIGEN2_INCLUDE_DIR})

# OpenCSG
if (NOT $ENV{MACOSX_DEPLOY_DIR} STREQUAL "")
  set(OPENCSG_DIR "$ENV{MACOSX_DEPLOY_DIR}")
endif()
if (NOT OPENCSG_INCLUDE_DIR)
  find_path(OPENCSG_INCLUDE_DIR
            opencsg.h
            PATHS $ENV{MACOSX_DEPLOY_DIR}/include)
  find_library(OPENCSG_LIBRARY
               opencsg
               PATHS $ENV{MACOSX_DEPLOY_DIR}/lib)
  if (NOT OPENCSG_INCLUDE_DIR OR NOT OPENCSG_LIBRARY)
    message(FATAL_ERROR "OpenCSG not found")
  else()
    message(STATUS "OpenCSG found in " ${OPENCSG_LIBRARY})
  endif()
endif()
include_directories(${OPENCSG_INCLUDE_DIR})

if (NOT $ENV{MACOSX_DEPLOY_DIR} STREQUAL "")
  set(GLEW_DIR "$ENV{MACOSX_DEPLOY_DIR}")
endif()
find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_PATH})

# Flex/Bison
find_package(BISON)
find_package(FLEX)
# The COMPILE_FLAGS and forced C++ compiler is just to be compatible with qmake
FLEX_TARGET(OpenSCADlexer ../src/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp COMPILE_FLAGS "-Plexer")
BISON_TARGET(OpenSCADparser ../src/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser_yacc.c COMPILE_FLAGS "-p parser")
ADD_FLEX_BISON_DEPENDENCY(OpenSCADlexer OpenSCADparser)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/parser_yacc.c PROPERTIES LANGUAGE "CXX")


# Internal includes
include_directories(../src)

add_definitions(-DOPENSCAD_VERSION=test)

set(COMMON_SOURCES
  ../src/export.cc 
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
  ../src/primitives.cc 
  ../src/projection.cc 
  ../src/cgaladv.cc 
  ../src/surface.cc 
  ../src/control.cc 
  ../src/render.cc 
  ../src/import.cc 
  ../src/dxfdata.cc 
  ../src/dxftess.cc 
  ../src/dxfdim.cc 
  ../src/dxflinextrude.cc 
  ../src/dxfrotextrude.cc 
  ../src/printutils.cc 
  ../src/progress.cc 
  ../src/nodedumper.cc 
  ../src/traverser.cc 
  ../src/PolySetRenderer.cc 
  ../src/Tree.cc
  ${FLEX_OpenSCADlexer_OUTPUTS}
  ${BISON_OpenSCADparser_OUTPUTS})

#
# dumptest
#
add_executable(dumptest dumptest.cc ${COMMON_SOURCES})
target_link_libraries(dumptest ${QT_LIBRARIES} ${OPENGL_LIBRARY})

#
# csgtexttest
#
add_executable(csgtexttest csgtexttest.cc CSGTextRenderer.cc CSGTextCache.cc ../src/qhash.cc ${COMMON_SOURCES})
target_link_libraries(csgtexttest ${QT_LIBRARIES} ${OPENGL_LIBRARY})

#
# csgtermtest
#
add_executable(csgtermtest csgtermtest.cc ../src/CSGTermRenderer.cc ${COMMON_SOURCES})
target_link_libraries(csgtermtest ${QT_LIBRARIES} ${OPENGL_LIBRARY})

if (NOT $ENV{MACOSX_DEPLOY_DIR} STREQUAL "")
  set(CGAL_DIR "$ENV{MACOSX_DEPLOY_DIR}/lib/CGAL")
  set(CMAKE_MODULE_PATH "${CGAL_DIR}")
endif()
find_package(CGAL REQUIRED)
include_directories(${CGAL_INCLUDE_DIRS})

#
# cgaltest
#
add_executable(cgaltest cgaltest.cc ../src/CSGTermRenderer.cc ../src/CGALRenderer.cc
                        ../src/PolySetCGALRenderer.cc ../src/qhash.cc ../src/nef2dxf.cc
                        ../src/cgaladv_minkowski2.cc ../src/cgaladv_minkowski3.cc ${COMMON_SOURCES})
set_target_properties(cgaltest PROPERTIES COMPILE_FLAGS "-DENABLE_CGAL ${CGAL_CXX_FLAGS_INIT}")
target_link_libraries(cgaltest ${CGAL_LIBRARY} ${CGAL_3RD_PARTY_LIBRARIES} ${QT_LIBRARIES} ${OPENGL_LIBRARY})

#
# opencsgtest
#
QT4_WRAP_CPP(MOC_SRC_opencsgtest ../src/GLView.h)
add_executable(opencsgtest opencsgtest.cc ${MOC_SRC_opencsgtest} ../src/glview.cc
                           ../src/render-opencsg.cc ../src/CSGTermRenderer.cc ../src/CGALRenderer.cc
                           ../src/PolySetCGALRenderer.cc ../src/qhash.cc ../src/nef2dxf.cc
                           ../src/cgaladv_minkowski2.cc ../src/cgaladv_minkowski3.cc
                           ${COMMON_SOURCES})
set_target_properties(opencsgtest PROPERTIES COMPILE_FLAGS "-DENABLE_OPENCSG -DENABLE_CGAL ${CGAL_CXX_FLAGS_INIT}")
target_link_libraries(opencsgtest ${CGAL_LIBRARY} ${CGAL_3RD_PARTY_LIBRARIES} ${QT_LIBRARIES} ${OPENCSG_LIBRARY} ${GLEW_LIBRARY} ${OPENGL_LIBRARY})

#
# This functions adds cmd-line tests given files.
# Files are sent as the parameters following TESTSUFFIX
#
function(add_cmdline_test TESTCMD TESTSUFFIX)
  get_filename_component(TESTCMD_NAME ${TESTCMD} NAME_WE)
  foreach (SCADFILE ${ARGN})
    get_filename_component(TESTNAME ${SCADFILE} NAME_WE)
    string(REPLACE " " "_" TESTNAME ${TESTNAME}) # Test names cannot include spaces
    add_test("${TESTCMD_NAME}_${TESTNAME}" ${tests_SOURCE_DIR}/test_cmdline_tool.py -s ${TESTSUFFIX} ${CMAKE_BINARY_DIR}/${TESTCMD} "${SCADFILE}")
  endforeach()
endfunction()

enable_testing()

# Find all scad files
file(GLOB MINIMAL_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/minimal/*.scad)
file(GLOB FEATURES_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/features/*.scad)
file(GLOB BUGS_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/bugs/*.scad)

# Add dumptest tests to CTest
add_cmdline_test(dumptest txt ${MINIMAL_FILES})
# Add csgtexttest tests to CTest
add_cmdline_test(csgtexttest txt ${SCAD_FILES})
# Add csgtermtest tests to CTest
add_cmdline_test(csgtermtest txt ${SCAD_FILES})
# Add cgaltest tests to CTest
LIST(APPEND CGALTEST_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/cube.scad)
LIST(APPEND CGALTEST_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/sphere.scad)
LIST(APPEND CGALTEST_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/cylinder.scad)
LIST(APPEND CGALTEST_FILES ${CMAKE_SOURCE_DIR}/../examples/example001.scad)
add_cmdline_test(cgaltest stl ${CGALTEST_FILES})

# Add dxfexport tests to CTest
#add_cmdline_test(${CMAKE_SOURCE_DIR}/../test-code/exportdxf dxf ${SCAD_FILES})