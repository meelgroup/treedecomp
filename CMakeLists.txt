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

cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

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
foreach(build_config ${build_types})
    string(TOUPPER ${build_config} upper_case_build_config)
    foreach(language CXX C)
        set(VAR_TO_MODIFY "CMAKE_${language}_FLAGS_${upper_case_build_config}")
        string(REGEX REPLACE "(^| )[/-]D *NDEBUG($| )"
                             " "
                             replacement
                             "${${VAR_TO_MODIFY}}")
        set(${VAR_TO_MODIFY} "${replacement}" CACHE STRING "Default flags for ${build_config} configuration" FORCE)
    endforeach()
endforeach()

set(CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY ON)
set(CMAKE_EXPORT_PACKAGE_REGISTRY OFF)
project(treedecomp)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
include(GenerateExportHeader)
include(GNUInstallDirs)

message(STATUS "LIB directory is '${CMAKE_INSTALL_LIBDIR}'")
message(STATUS "BIN directory is '${CMAKE_INSTALL_BINDIR}'")

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# static compilation
option(BUILD_SHARED_LIBS "Build the shared library" ON)

if((${CMAKE_SYSTEM_NAME} MATCHES "Linux") OR (${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
    if(NOT BUILD_SHARED_LIBS)
        message(STATUS "Compiling statically")
        if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
        endif()
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    else()
        message(STATUS "Compiling for dynamic library use")
    endif()
endif()

include(CheckCXXCompilerFlag)
macro(add_cxx_flag_if_supported flagname)
  check_cxx_compiler_flag("${flagname}" HAVE_FLAG_${flagname})
  if(HAVE_FLAG_${flagname})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flagname}")
  endif()
endmacro()

if(NOT MSVC)
    add_compile_options(-g)
    add_compile_options("$<$<CONFIG:RelWithDebInfo>:-O3>")
    add_compile_options("$<$<CONFIG:Release>:-O3>")
    add_compile_options("$<$<CONFIG:Release>:-g0>")
    add_compile_options("$<$<CONFIG:Debug>:-O0>")

    foreach(flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG)
        if(${flag_var} MATCHES "-O3")
            string(REGEX REPLACE "-O3" "-O0" ${flag_var} "${${flag_var}}")
        endif()
        if(${flag_var} MATCHES "-O2")
            string(REGEX REPLACE "-O2" "-O0" ${flag_var} "${${flag_var}}")
        endif()
    endforeach()
endif()

option(ENABLE_ASSERTIONS "Build with assertions enabled" ON)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(ENABLE_ASSERTIONS OFF)
endif()
if(ENABLE_ASSERTIONS)
    # NDEBUG was already removed.
else()
    add_compile_definitions(NDEBUG)
    add_cxx_flag_if_supported("-fno-stack-protector")
    add_compile_definitions(_FORTIFY_SOURCE=0)
endif()

if(NOT WIN32)
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

# -----------------------------------------------------------------------------
# Version
# -----------------------------------------------------------------------------
set(TREEDECOMP_FULL_VERSION "1.0.0")
string(REPLACE "." ";" TREEDECOMP_FULL_VERSION_LIST ${TREEDECOMP_FULL_VERSION})
list(GET TREEDECOMP_FULL_VERSION_LIST 0 PROJECT_VERSION_MAJOR)
list(GET TREEDECOMP_FULL_VERSION_LIST 1 PROJECT_VERSION_MINOR)
list(GET TREEDECOMP_FULL_VERSION_LIST 2 PROJECT_VERSION_PATCH)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
message(STATUS "PROJECT_VERSION: ${PROJECT_VERSION}")

# -----------------------------------------------------------------------------
# Provide an export name to be used by targets that wish to export themselves.
# -----------------------------------------------------------------------------
set(TREEDECOMP_EXPORT_NAME "treedecompTargets")
set(TREEDECOMP_TARGETS_FILENAME "treedecompTargets.cmake")
set(TREEDECOMP_CONFIG_FILENAME "treedecompConfig.cmake")

add_subdirectory(src)

# Export targets
export(
    TARGETS treedecomp
    FILE "${CMAKE_CURRENT_BINARY_DIR}/${TREEDECOMP_TARGETS_FILENAME}"
)

# Build-tree config (absolute paths, for use without installation)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/treedecompConfig-buildtree.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${TREEDECOMP_CONFIG_FILENAME}"
    @ONLY
)

# Export this package to the CMake user package registry
# Now the user can just use find_package(treedecomp) on their system
export(PACKAGE treedecomp)

# Install-tree config and version files (relocatable via CMakePackageConfigHelpers)
set(DEF_INSTALL_CMAKE_DIR lib/cmake/treedecomp)
set(TREEDECOMP_INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH
    "Installation directory for treedecomp CMake files")
set(TREEDECOMP_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}")
set(TREEDECOMP_VERSION_FILENAME "treedecompConfigVersion.cmake")

include(CMakePackageConfigHelpers)
configure_package_config_file(treedecompConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/${TREEDECOMP_CONFIG_FILENAME}"
    INSTALL_DESTINATION "${TREEDECOMP_INSTALL_CMAKE_DIR}"
    PATH_VARS TREEDECOMP_INSTALL_INCLUDEDIR
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${TREEDECOMP_VERSION_FILENAME}"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/${TREEDECOMP_CONFIG_FILENAME}"
    "${CMAKE_CURRENT_BINARY_DIR}/${TREEDECOMP_VERSION_FILENAME}"
    DESTINATION "${TREEDECOMP_INSTALL_CMAKE_DIR}"
)

install(
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
