# Copyright (c) 2023, Authors of treedecomp
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

if(POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif()

if(POLICY CMP0022)
    cmake_policy(SET CMP0022 NEW)
endif()

if(POLICY CMP0046)
    cmake_policy(SET CMP0046 NEW)
endif()

# -----------------------------------------------------------------------------
# Make RelWithDebInfo the default build type if otherwise not set
# -----------------------------------------------------------------------------
set(build_types Debug Release RelWithDebInfo MinSizeRel)
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "You can choose the type of build, options are:${build_types}")
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
        "Options are ${build_types}"
        FORCE
    )
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${build_types})
endif()
message(STATUS "Doing a ${CMAKE_BUILD_TYPE} build")

# -----------------------------------------------------------------------------
# Option to enable/disable assertions
# -----------------------------------------------------------------------------
foreach (build_config ${build_types})
    string(TOUPPER ${build_config} upper_case_build_config)
    foreach (language CXX C)
        set(VAR_TO_MODIFY "CMAKE_${language}_FLAGS_${upper_case_build_config}")
        string(REGEX REPLACE "(^| )[/-]D *NDEBUG($| )"
                             " "
                             replacement
                             "${${VAR_TO_MODIFY}}")
        set(${VAR_TO_MODIFY} "${replacement}" CACHE STRING "Default flags for ${build_config} configuration" FORCE)
    endforeach()
endforeach()

project(treedecomp)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
enable_language(CXX)
include(GenerateExportHeader)
include(GNUInstallDirs)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# static compilation
option(BUILD_SHARED_LIBS "Build the shared library" ON)
option(STATICCOMPILE "Compile to static executable" OFF)
if (STATICCOMPILE)
    set(BUILD_SHARED_LIBS OFF)
endif()

if ((${CMAKE_SYSTEM_NAME} MATCHES "Linux") OR (${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
    if(NOT BUILD_SHARED_LIBS)
        MESSAGE(STATUS "Compiling statically")
        if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
        endif()
        SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    else()
        MESSAGE(STATUS "Compiling for dynamic library use")
    endif()
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include(CheckCXXCompilerFlag)
macro(add_cxx_flag_if_supported flagname)
  check_cxx_compiler_flag("${flagname}" HAVE_FLAG_${flagname})
  if(HAVE_FLAG_${flagname})
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flagname}" )
  endif()
endmacro()

if (NOT MSVC)
    add_compile_options(-g)
    add_compile_options("$<$<CONFIG:RelWithDebInfo>:-O3>")
    add_compile_options("$<$<CONFIG:Release>:-O3>")
    add_compile_options("$<$<CONFIG:Release>:-g0>")
    add_compile_options("$<$<CONFIG:Release>:-DNDEBUG>")
    add_compile_options("$<$<CONFIG:Debug>:-O0>")

    foreach(flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG)
        if(${flag_var} MATCHES "-O3")
            string(REGEX REPLACE "-O3" "-O0" ${flag_var} "${${flag_var}}")
        endif(${flag_var} MATCHES "-O3")
        if(${flag_var} MATCHES "-O2")
            string(REGEX REPLACE "-O2" "-O0" ${flag_var} "${${flag_var}}")
        endif(${flag_var} MATCHES "-O2")
    endforeach(flag_var)
endif()

option(ENABLE_ASSERTIONS "Build with assertions enabled" ON)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(ENABLE_ASSERTIONS OFF)
endif()
if (ENABLE_ASSERTIONS)
    # NDEBUG was already removed.
else()
    add_definitions(-DNDEBUG)
    add_cxx_flag_if_supported("-fno-stack-protector")
    add_definitions(-D_FORTIFY_SOURCE=0)
endif()

if (NOT WIN32)
    add_cxx_flag_if_supported("-mpopcnt")
    add_cxx_flag_if_supported("-msse4.2")
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
        add_cxx_flag_if_supported("-Wall")
        add_cxx_flag_if_supported("-Wextra")
        add_cxx_flag_if_supported("-Wunused")
        add_cxx_flag_if_supported("-Wsign-compare")
        add_cxx_flag_if_supported("-fno-omit-frame-pointer")
        add_cxx_flag_if_supported("-ggdb3")
    endif()
endif()

# -----------------------------------------------------------------------------
# No external dependencies needed
# -----------------------------------------------------------------------------

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)

macro(treedecomp_add_public_header LIBTARGET HEADER)
    get_target_property(EXISTING_PUBLIC_HEADERS ${LIBTARGET} PUBLIC_HEADER)
    if(EXISTING_PUBLIC_HEADERS)
        list(APPEND EXISTING_PUBLIC_HEADERS "${HEADER}")
    else()
        set(EXISTING_PUBLIC_HEADERS "${HEADER}")
    endif()
    set_target_properties(
        ${LIBTARGET}
        PROPERTIES
        PUBLIC_HEADER "${EXISTING_PUBLIC_HEADERS}"
     )
endmacro()

set(DEF_INSTALL_CMAKE_DIR lib/cmake/treedecomp)
set(TREEDECOMP_INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH
    "Installation directory for treedecomp CMake files")

# -----------------------------------------------------------------------------
# Version
# -----------------------------------------------------------------------------
set(TREEDECOMP_FULL_VERSION "1.0.0")
string(REPLACE "." ";" TREEDECOMP_FULL_VERSION_LIST ${TREEDECOMP_FULL_VERSION})
list(GET TREEDECOMP_FULL_VERSION_LIST 0 PROJECT_VERSION_MAJOR)
list(GET TREEDECOMP_FULL_VERSION_LIST 1 PROJECT_VERSION_MINOR)
list(GET TREEDECOMP_FULL_VERSION_LIST 2 PROJECT_VERSION_PATCH)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

add_subdirectory(src)

# -----------------------------------------------------------------------------
# Export
# -----------------------------------------------------------------------------
set(TREEDECOMP_EXPORT_NAME "treedecompTargets")
set(TREEDECOMP_TARGETS_FILENAME "treedecompTargets.cmake")
set(TREEDECOMP_CONFIG_FILENAME "treedecompConfig.cmake")

# Export targets
export(
    TARGETS treedecomp
    FILE "${CMAKE_CURRENT_BINARY_DIR}/${TREEDECOMP_TARGETS_FILENAME}"
)

# Create config file for build tree
set(EXPORT_TYPE "Build-tree")
set(CONF_INCLUDE_DIRS "${CMAKE_CURRENT_BINARY_DIR}/include")
configure_file(treedecompConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/${TREEDECOMP_CONFIG_FILENAME}" @ONLY
)

export(PACKAGE treedecomp)

# Create config file for install tree
set(EXPORT_TYPE "installed")
set(CONF_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include")
configure_file(treedecompConfig.cmake.in
   "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/${TREEDECOMP_CONFIG_FILENAME}" @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/${TREEDECOMP_CONFIG_FILENAME}"
    DESTINATION "${TREEDECOMP_INSTALL_CMAKE_DIR}"
)

install(
    TARGETS treedecomp
    EXPORT ${TREEDECOMP_EXPORT_NAME}
    DESTINATION "${TREEDECOMP_INSTALL_CMAKE_DIR}"
)

# -----------------------------------------------------------------------------
# Uninstall
# -----------------------------------------------------------------------------
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY
)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)
