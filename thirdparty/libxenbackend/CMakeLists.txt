cmake_minimum_required(VERSION 3.0)

################################################################################
# Options
################################################################################

OPTION(WITH_TEST "build with test" ON)
OPTION(WITH_DOC "build with documenation" OFF)

message(STATUS)
message(STATUS "${PROJECT_NAME} Configuration:")
message(STATUS "CMAKE_BUILD_TYPE              = ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_INSTALL_PREFIX          = ${CMAKE_INSTALL_PREFIX}")
message(STATUS)
message(STATUS "WITH_DOC                      = ${WITH_DOC}")
message(STATUS "WITH_TEST                     = ${WITH_TEST}")
message(STATUS)
message(STATUS "XEN_INCLUDE_PATH              = ${XEN_INCLUDE_PATH}")
message(STATUS "XEN_LIB_PATH                  = ${XEN_LIB_PATH}")
message(STATUS)

################################################################################
# Autodetected options
################################################################################

include(CheckSymbolExists)

# Test if version 2 of the xengnttab_dmabuf_imp_to_refs and
# xengnttab_dmabuf_exp_from_refs functions are supported:
# these functions have an additional data_ofs parameter
# to set/get the offset of the data in the dma-buf's scatter-gather
# table
set(CMAKE_REQUIRED_LIBRARIES xengnttab)
check_symbol_exists("xengnttab_dmabuf_imp_to_refs_v2" "xengnttab.h" GNTTAB_HAS_DMABUF_OFFSET)

if(GNTTAB_HAS_DMABUF_OFFSET)
	add_definitions(-DGNTTAB_HAS_DMABUF_OFFSET)
endif()

################################################################################
# Compiler flags
################################################################################

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=gnu++11 -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

################################################################################
# Includes
################################################################################

include_directories(
	include
	include/xen/be
	${XEN_INCLUDE_PATH}
	${CMAKE_CURRENT_BINARY_DIR}
)

link_directories(${XEN_LIB_PATH})

################################################################################
# Sources
################################################################################

add_subdirectory(src)

if(WITH_DOC)
	add_subdirectory(example)
endif()

if(WITH_TEST)
	enable_testing()
	add_subdirectory(tests)
	add_dependencies(xenbemock Version)
	if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
		set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)
		include(CodeCoverage)
		setup_target_for_coverage(coverage unitTests coverage)
	endif()
endif()

################################################################################
# Install
################################################################################

install(
	DIRECTORY include/
	DESTINATION include
	FILES_MATCHING PATTERN "*.hpp"
)

################################################################################
# Versioning
################################################################################

find_package(Git)

if (NOT GIT_FOUND)
	set(GIT_EXECUTABLE git)
endif()

add_custom_target(Version
	COMMAND ${CMAKE_COMMAND}
	-D GIT_EXECUTABLE=${GIT_EXECUTABLE}
	-D INPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/Version.hpp.in
	-D OUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/Version.hpp
	-D GIT_SOURCE_DIR=${CMAKE_SOURCE_DIR}
	-P ${CMAKE_SOURCE_DIR}/generateVersion.cmake
)

add_dependencies(xenbe Version)

################################################################################
# Doc
################################################################################

find_package(Doxygen)

if(WITH_DOC AND NOT DOXYGEN_FOUND)
	message(FATAL_ERROR "Doxygen not found. Required to generate docs.")
else()
	configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/doxygen.cfg
		${CMAKE_CURRENT_BINARY_DIR}/doxygen.cfg @ONLY
	)
	add_custom_target(
		doc ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doxygen.cfg
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating API documentation with Doxygen" VERBATIM
	)
endif()
