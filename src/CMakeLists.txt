include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if (NOT WIN32)
    add_cxx_flag_if_supported("-Wno-class-memaccess")
endif()

SET(SOURCES
    TreeDecomposition.cpp
    IFlowCutter.cpp
    graph.cpp
    flow-cutter-pace17/src/tree_decomposition.cpp
    flow-cutter-pace17/src/cell.cpp
    flow-cutter-pace17/src/greedy_order.cpp
)

ADD_LIBRARY(treedecomp ${SOURCES})

set_target_properties(treedecomp PROPERTIES
  PUBLIC_HEADER "${treedecomp_public_headers}"
  VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
  SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
  INSTALL_RPATH_USE_LINK_PATH TRUE
)

treedecomp_add_public_header(treedecomp ${CMAKE_CURRENT_SOURCE_DIR}/TreeDecomposition.hpp)
treedecomp_add_public_header(treedecomp ${CMAKE_CURRENT_SOURCE_DIR}/IFlowCutter.hpp)
treedecomp_add_public_header(treedecomp ${CMAKE_CURRENT_SOURCE_DIR}/utils.hpp)
treedecomp_add_public_header(treedecomp ${CMAKE_CURRENT_SOURCE_DIR}/bitset.hpp)
treedecomp_add_public_header(treedecomp ${CMAKE_CURRENT_SOURCE_DIR}/treedecomp_defs.hpp)
treedecomp_add_public_header(treedecomp ${CMAKE_CURRENT_SOURCE_DIR}/graph.hpp)

# -----------------------------------------------------------------------------
# Copy public headers into build directory include directory.
# The treedecompConfig.cmake we generate in the build directory depends on this.
# -----------------------------------------------------------------------------
set(HEADER_DEST "${PROJECT_BINARY_DIR}/include/treedecomp")
add_custom_target(CopyPublicHeaders ALL)
get_target_property(treedecomp_public_headers treedecomp PUBLIC_HEADER)
foreach(public_header ${treedecomp_public_headers})
    get_filename_component(HEADER_NAME ${public_header} NAME)
    add_custom_command(TARGET CopyPublicHeaders PRE_BUILD
                       COMMAND ${CMAKE_COMMAND} -E make_directory
                               "${HEADER_DEST}"
                       COMMAND ${CMAKE_COMMAND} -E echo
                       "Copying ${HEADER_NAME} to ${HEADER_DEST}"
                       COMMAND ${CMAKE_COMMAND} -E
                           copy_if_different
                           ${public_header}
                           "${HEADER_DEST}"
                      )
endforeach()

# Also copy the flow-cutter headers (needed at compile time by consumers)
set(FC_HEADER_DEST "${PROJECT_BINARY_DIR}/include/treedecomp/flow-cutter-pace17/src")
file(GLOB FC_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/flow-cutter-pace17/src/*.hpp")
foreach(fc_header ${FC_HEADERS})
    get_filename_component(HEADER_NAME ${fc_header} NAME)
    add_custom_command(TARGET CopyPublicHeaders PRE_BUILD
                       COMMAND ${CMAKE_COMMAND} -E make_directory
                               "${FC_HEADER_DEST}"
                       COMMAND ${CMAKE_COMMAND} -E
                           copy_if_different
                           ${fc_header}
                           "${FC_HEADER_DEST}"
                      )
endforeach()

install(TARGETS treedecomp
    EXPORT ${TREEDECOMP_EXPORT_NAME}
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/treedecomp"
)

# Install flow-cutter headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/flow-cutter-pace17/src/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/treedecomp/flow-cutter-pace17/src"
    FILES_MATCHING PATTERN "*.hpp"
)
