cmake_minimum_required(VERSION 3.0)

######################### Compile time options ##########################
project(sqlite3)

# Options to enable features. SQLite3 generally has minimal dependencies,
# but we can still add options for features like static or shared builds.
option(SQLITE3_ENABLE_SHARED  "Enable shared SQLite3 library"  OFF)
option(SQLITE3_ENABLE_STATIC  "Enable static SQLite3 library"  ON)

set(PACKAGE_NAME              "${PROJECT_NAME}"                                      CACHE PATH "Name of the package")
set(PACKAGE_NAMESPACE         "SQLite3::"                                               CACHE PATH "Namespace to use when using the package")
set(PACKAGE_SOURCE_DIRECTORY  "${CMAKE_CURRENT_LIST_DIR}"                            CACHE PATH "Source directory of the package")
set(PACKAGE_TARGETS           "${PROJECT_NAME}-targets"                              CACHE PATH "Name of targets.cmake file for the package")
set(PACKAGE_CONFIG            "${PROJECT_NAME}-config"                               CACHE PATH "Name of config.cmake file for the package")
set(PACKAGE_CONFIG_VERSION    "${PROJECT_NAME}-config-version"                       CACHE PATH "Name of config-version.cmake file for the package")
set(PACKAGE_VERSION           "3.39.3"                                                CACHE PATH "Version number for the package")
set(INSTALL_LIB_DIR           "${CMAKE_INSTALL_PREFIX}/lib"                          CACHE PATH "Location of libs")
set(INSTALL_ARCHIVE_DIR       "${CMAKE_INSTALL_PREFIX}/lib"                          CACHE PATH "Location of archive directory")                         
set(INSTALL_RUNTIME_DIR       "${CMAKE_INSTALL_PREFIX}/bin"                          CACHE PATH "Location of runtime directory")                         
set(INSTALL_INCLUDE_DIR       "${CMAKE_INSTALL_PREFIX}/include"                      CACHE PATH "Location of include directory")                         
set(SYSCONFIG_INSTALL_DIR     "${CMAKE_INSTALL_PREFIX}/lib/cmake/${PACKAGE_NAME}"    CACHE PATH "Location of cmake configuration files")
####################### end Compile time options ########################

# SQLite amalgamation source files (single file amalgamation .c)
set(SOURCE_FILE "${PACKAGE_SOURCE_DIRECTORY}/sqlite3.c")

# Create the static or shared library based on user settings
if (SQLITE3_ENABLE_SHARED)
    add_library(${PROJECT_NAME} SHARED ${SOURCE_FILE})
elseif (SQLITE3_ENABLE_STATIC)
    add_library(${PROJECT_NAME} STATIC ${SOURCE_FILE})
    set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
else()
    message(FATAL_ERROR "You must enable either STATIC or SHARED builds for SQLite3.")
endif()

# Set C++ standard to at least C++11 (can be adjusted as needed)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
if (NOT DEFINED CMAKE_CXX_STANDARD)
  set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 11)
endif()

# Include directories for SQLite3 headers
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${PACKAGE_SOURCE_DIRECTORY}>  # For building projects using SQLite3
        $<INSTALL_INTERFACE:include>                    # For external projects looking for public headers
)

# Export target to be used in other CMake projects
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Installation rules
install(
    TARGETS               ${PACKAGE_NAME} 
    EXPORT                ${PACKAGE_TARGETS}
    LIBRARY DESTINATION   ${INSTALL_LIB_DIR}
    ARCHIVE DESTINATION   ${INSTALL_ARCHIVE_DIR}
    RUNTIME DESTINATION   ${INSTALL_RUNTIME_DIR}
    INCLUDES DESTINATION  ${INSTALL_INCLUDE_DIR}
)

# Configure and install cmake files for external projects
configure_file(${PACKAGE_CONFIG}.cmake.in ${PACKAGE_CONFIG}.cmake @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/${PACKAGE_CONFIG}.cmake DESTINATION ${SYSCONFIG_INSTALL_DIR})
install(FILES "${PACKAGE_SOURCE_DIRECTORY}/sqlite3.h" DESTINATION ${INSTALL_INCLUDE_DIR}/${PACKAGE_NAME})
install(EXPORT ${PACKAGE_TARGETS} NAMESPACE ${PACKAGE_NAMESPACE} DESTINATION ${SYSCONFIG_INSTALL_DIR})

# Cleanup variables used during configuration
unset(PACKAGE_NAME                                              CACHE)
unset(PACKAGE_NAMESPACE                                         CACHE)  
unset(PACKAGE_SOURCE_DIRECTORY                                  CACHE)         
unset(PACKAGE_TARGETS                                           CACHE)
unset(PACKAGE_CONFIG                                            CACHE)
unset(PACKAGE_CONFIG_VERSION                                    CACHE)
unset(PACKAGE_VERSION                                           CACHE)
unset(INSTALL_LIB_DIR                                           CACHE)
unset(INSTALL_ARCHIVE_DIR                                       CACHE)                     
unset(INSTALL_RUNTIME_DIR                                       CACHE)                  
unset(INSTALL_INCLUDE_DIR                                       CACHE)                 
unset(SYSCONFIG_INSTALL_DIR                                     CACHE)
