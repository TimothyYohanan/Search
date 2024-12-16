# https://github.com/aimotive-legacy/imgui-glfw/blob/master/CMakeLists.txt
cmake_minimum_required(VERSION 3.0)

######################### Compile time options ##########################
project(imgui)

option(IMGUI_ENABLE_ANDROID   "enable android backend"    OFF)
option(IMGUI_ENABLE_DIRECTX12 "enable directx-12 backend" OFF)
option(IMGUI_ENABLE_DIRECTX11 "enable directx-11 backend" OFF)
option(IMGUI_ENABLE_GLFW      "enable glfw backend"       OFF)                       # glfw is the only option that currently works (only option with code included for it below)
option(IMGUI_ENABLE_OPENGL3   "enable opengl-3 backend"   OFF)                       # the rest of these options will be implimented in the future, as needed.
option(IMGUI_ENABLE_OPENGL2   "enable opengl-2 backend"   OFF)                       # their presence here is just a reminder that that can be implimented if needed.
option(IMGUI_ENABLE_OSX       "enable OSX backend"        OFF)
option(IMGUI_ENABLE_VULKAN    "enable vulkan backend"     OFF)

set(PACKAGE_NAME              "${PROJECT_NAME}"                                      CACHE PATH "Name of the package")
set(PACKAGE_NAMESPACE         "ImGui::"                                              CACHE PATH "Namespace to use when using the package")              # find_package(<PACKAGE_NAMESPACE><PACKAGE_NAME>)
set(PACKAGE_SOURCE_DIRECTORY  "${CMAKE_CURRENT_LIST_DIR}"                            CACHE PATH "Source directory of the package")
set(PACKAGE_TARGETS           "${PROJECT_NAME}-targets"                              CACHE PATH "Name of targets.cmake file for the package")
set(PACKAGE_CONFIG            "${PROJECT_NAME}-config"                               CACHE PATH "Name of config.cmake file for the package")
set(PACKAGE_CONFIG_VERSION    "${PROJECT_NAME}-config-version"                       CACHE PATH "Name of config-version.cmake file for the package")
set(PACKAGE_VERSION           "1.89.7"                                               CACHE PATH "Version number for the package")
set(INSTALL_LIB_DIR           "${CMAKE_INSTALL_PREFIX}/lib"                          CACHE PATH "Location of libs")
set(INSTALL_ARCHIVE_DIR       "${CMAKE_INSTALL_PREFIX}/lib"                          CACHE PATH "Location of archive directory")                         # where lib.a, lib.dylib, etc.. files will be installed
set(INSTALL_RUNTIME_DIR       "${CMAKE_INSTALL_PREFIX}/bin"                          CACHE PATH "Location of runtime directory")                         # where executables will be stalled
set(INSTALL_INCLUDE_DIR       "${CMAKE_INSTALL_PREFIX}/include"                      CACHE PATH "Location of include directory")                         # where headers will be installed
set(SYSCONFIG_INSTALL_DIR     "${CMAKE_INSTALL_PREFIX}/lib/cmake/${PACKAGE_NAME}"    CACHE PATH "Location of cmake configuration files")
####################### end Compile time options ########################

file(GLOB PACKAGE_SOURCE_FILES "${PACKAGE_SOURCE_DIRECTORY}/*.c" "${PACKAGE_SOURCE_DIRECTORY}/*.cpp")
file(GLOB PACKAGE_HEADER_FILES "${PACKAGE_SOURCE_DIRECTORY}/*.h" "${PACKAGE_SOURCE_DIRECTORY}/*.hpp")
file(GLOB PACKAGE_ADDITIONAL_HEADER_FILES "${PACKAGE_SOURCE_DIRECTORY}/backends/*.h" "${PACKAGE_SOURCE_DIRECTORY}/backends/*.hpp")
set(PACKAGE_PUBLIC_HEADERS ${PACKAGE_HEADER_FILES} ${PACKAGE_ADDITIONAL_HEADER_FILES})

if (${BUILD_SHARED_LIBS})                                     # this does not work currently. make sure BUILD_SHARED_LIBS=OFF
    add_library(${PROJECT_NAME} SHARED
        ${PACKAGE_SOURCE_FILES}
        )
else()
    add_library(${PROJECT_NAME} STATIC
        ${PACKAGE_SOURCE_FILES}
        )
    set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
if (NOT DEFINED CMAKE_CXX_STANDARD)
  set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 11)
endif()

target_include_directories(${PROJECT_NAME}
    PRIVATE 
        ${PACKAGE_SOURCE_DIRECTORY}                           # where the library itself will look for its internal headers
    PUBLIC 
        $<BUILD_INTERFACE:${PACKAGE_SOURCE_DIRECTORY}>        # where top-level project will look for the library's public headers, relative to the current source directory
        $<INSTALL_INTERFACE:include>                          # where external projects will look for the library's public headers, relative to the installation directory
    )

####### Find and link (optional) backends #######
if(IMGUI_ENABLE_GLFW)
    find_package(glfw3 REQUIRED)
    target_link_libraries(${PROJECT_NAME} glfw)
endif()

if(IMGUI_ENABLE_VULKAN)
    find_package(Vulkan REQUIRED)
    target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)
endif()
####### end Find and link (optional) backends #######


#######               Copy Fonts              #######
set(fonts_source_dir "${PACKAGE_SOURCE_DIRECTORY}/misc/fonts")
set(fonts_destination_dir "${INSTALL_INCLUDE_DIR}/imgui/fonts")
set(file_extension ".ttf")
file(COPY "${fonts_source_dir}" DESTINATION "${fonts_destination_dir}" FILES_MATCHING PATTERN "*${file_extension}")
#######             End copy fonts            #######

# set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX d)
target_compile_definitions(${PROJECT_NAME} PUBLIC -DImTextureID=ImU64)   # Define ImTextureID as ImU64 (necessary for rendering images with Vulkan)

install(
    TARGETS               ${PACKAGE_NAME} 
    EXPORT                ${PACKAGE_TARGETS}
    LIBRARY DESTINATION   ${INSTALL_LIB_DIR}
    ARCHIVE DESTINATION   ${INSTALL_ARCHIVE_DIR}
    RUNTIME DESTINATION   ${INSTALL_RUNTIME_DIR}
    INCLUDES DESTINATION  ${INSTALL_INCLUDE_DIR}
    )
configure_file(${PACKAGE_CONFIG}.cmake.in ${PACKAGE_CONFIG}.cmake @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/${PACKAGE_CONFIG}.cmake DESTINATION ${SYSCONFIG_INSTALL_DIR})
install(FILES ${PACKAGE_PUBLIC_HEADERS} DESTINATION ${INSTALL_INCLUDE_DIR}/${PACKAGE_NAME})
install(EXPORT ${PACKAGE_TARGETS} NAMESPACE ${PACKAGE_NAMESPACE} DESTINATION ${SYSCONFIG_INSTALL_DIR})



### Unset Cache Variables Used to Configure & Generate the Project ###
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
unset(PACKAGE_PUBLIC_HEADERS                                         )
unset(fonts_source_dir                                               )
unset(fonts_destination_dir                                          )
################################ End #################################
