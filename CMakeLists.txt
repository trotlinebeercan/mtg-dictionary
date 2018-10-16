#! ----------------------------------------------------------------------------
#! CMakeLists.txt - DownloadCards
#!
#! MTGDictionary is licensed under a
#! Creative Commons Attribution-NonCommercial 4.0 International License.
#! You should have received a copy of the license along with this
#! work. If not, see http://creativecommons.org/licenses/by-nc-sa/4.0/.
#!
#! (c) Copyright Dustin Hopper 2015 - hopper.dustin@gmail.com
#! ----------------------------------------------------------------------------

cmake_minimum_required (VERSION 3.0)

project (MTGDictionary)

# build options
set (EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake-build)
set (LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake-build)
set (CMAKE_BUILD_TYPE DEBUG CACHE String "Build type defers to Debug, options are: Debug Release RelWithDebInfo")
set (LIBRARY_BUILD_TYPE SHARED CACHE String "Library type defers to SHARED, options are: SHARED STATIC")

# custom options based on operating system
set (OS ${CMAKE_SYSTEM_NAME})
if (${OS} MATCHES "Darwin" OR ${OS} MATCHES "Linux")
    set (CMAKE_FIND_FRAMEWORK FIRST)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra -std=c++11")
    set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -Wextra -O2 -std=c++11")
    add_definitions(-DGLM_FORCE_RADIANS)
elseif (${OS} MATCHES "Windows")
    message (FATAL_ERROR "Visual Studio is preferred.")
    add_definitions(-DGLM_FORCE_RADIANS -DGLEW_STATIC -D_USE_MATH_DEFINES)
endif()

# third party dependencies
find_package (OpenCV REQUIRED)
find_package (OpenGL REQUIRED)

set (CMAKE_AUTOMOC ON)
find_package (Qt4 REQUIRED COMPONENTS QtCore QtGui QtNetwork)

# set and link dependencies
set (DEPENDENCIES ${OPENGL_LIBRARY} ${OpenCV_LIBS} ${TESSERACT_LIBS} pthread m)
set (DEPENDENCIES ${DEPENDENCIES} Qt4::QtCore Qt4::QtGui Qt4::QtNetwork)
include_directories (${OPENGL_INCLUDE_DIR})
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/include)

# print out some information, for the pretties
message ("------------------------------------------------------------")
message ("--------------------Information Report----------------------")
message ("------------------------------------------------------------")
message (STATUS "Info for project:\t\t${CMAKE_PROJECT_NAME}")
message (STATUS "Operating system:\t\t${OS}")
message (STATUS "CMake build type:\t\t${CMAKE_BUILD_TYPE}")
message (STATUS "Dependencies report:")
message (STATUS "\tOPENCV_INCLUDE_DIR = ${OpenCV_INCLUDE_DIRS}")
message (STATUS "\tOPENGL_INCLUDE_DIR = ${OPENGL_INCLUDE_DIR}")
message ("------------------------------------------------------------")

# build source
file (GLOB_RECURSE ALL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/source/*.cpp")

add_executable (app ${ALL_SOURCES})
target_link_libraries (app ${DEPENDENCIES})

# build applications
add_subdirectory ("${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/apps")
