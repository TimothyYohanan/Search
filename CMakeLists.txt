cmake_minimum_required(VERSION 3.26.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)

project(Search C CXX)


################## Compile-time options ##################
set(CLEANUP_EXTERN         ON                           )
set(DEBUG_BUILD            ON                           )
set(REBUILD_LIBS_EXTERNAL  TRUE                         )
set(PROJECT_BINARY_DIR     "${CMAKE_SOURCE_DIR}/build"  )
set(CPU_THREADS            18                           )
################ end Compile-time options ################


set(CMAKE_CONFIGURATION_TYPES "DEBUG;RELEASE" CACHE STRING "Available build configurations" FORCE)

if(DEBUG_BUILD) 
    set(CMAKE_BUILD_TYPE DEBUG CACHE STRING "" FORCE)
    add_compile_options(-Wall)                # "Enable All Warnings," including those that are less frequently enabled by default. eg. uninitialized variables, unused variables, narrowing conversions, etc..
    add_compile_options(-Wextra)              # "Enable Additional Warnings" Includes extra warnings. eg. implicit conversion from a signed to an unsigned type, unused parameters or variables, missing or mismatched function prototypes, unused function return values, redundant or unnecessary code constructs
    # add_compile_options(-Wformat-security)    # warning: format string is not a string literal (potentially insecure)
    # add_compile_options(-Wconversion)         # "Enable Warnings for Implicit Type Conversions" that may result in loss of data or unintended behavior. eg. narrowing conversions, where a value is being converted to a smaller type that cannot hold the full range of values, conversions between signed and unsigned types that may change the sign or cause overflow.
    # add_compile_options(-fsanitize=address)   # "Enable AddressSanitizer" [runtime memory error detector] Helps identify memory-related issues: buffer overflows, use-after-free, memory leaks, etc..
    add_compile_options(-O0)                  # "Disable Optimization" Optimizations performed by the compiler can sometimes interfere with debugging, as they can change the order of execution or eliminate code that appears to be unused.
    add_compile_options(-pedantic)            # Enforces strict compliance with the C++ standard and enables additional warnings and errors for non-standard or potentially problematic code constructs.
else()
    set(CMAKE_BUILD_TYPE RELEASE CACHE STRING "" FORCE)
    add_compile_options(-O3)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/library")      # Path to directory where the project's archive is built
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/library")      # Path to directory where the project's external libraries are built
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")          # Path to directory where the project's executables are built
set(CMAKE_RUNTIME_DATA_DIRECTORY "${PROJECT_BINARY_DIR}/data")
set(CMAKE_PREFIX_PATH "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib/cmake;${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib64/cmake")  # define search path(s) for find_package()

if(REBUILD_LIBS_EXTERNAL)
    include(${CMAKE_SOURCE_DIR}/BuildExternalLibs.cmake)   # Build external dependencies (from the 'extern' folder) before the rest of the project        
endif()

file(REMOVE_RECURSE "${PROJECT_BINARY_DIR}/config")
file(COPY config DESTINATION ${PROJECT_BINARY_DIR}) # Copy config files to the build directory

find_package(sqlite3 REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(imgui REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(Search main.cpp)

file(GLOB_RECURSE SRC_FILES ${PROJECT_SOURCE_DIR}/src/**/*.cpp)

target_sources(Search
    PRIVATE 
        ${PROJECT_SOURCE_DIR}/extern/imgui/backends/imgui_impl_glfw.cpp 
        ${PROJECT_SOURCE_DIR}/extern/imgui/backends/imgui_impl_vulkan.cpp 
        ${PROJECT_SOURCE_DIR}/src/database.cpp
        ${PROJECT_SOURCE_DIR}/src/search.cpp
        ${SRC_FILES}
    )
target_include_directories(Search 
    PUBLIC 
        # ${PROJECT_SOURCE_DIR} 
        # ${PROJECT_BINARY_DIR} 
        ${PROJECT_SOURCE_DIR}/extern/imgui
        # ${PROJECT_BINARY_DIR}/library/include 
    )
target_link_libraries(Search PUBLIC ${YAML_CPP_LIBRARIES} ${imgui_LIBRARIES} ${sqlite3_LIBRARIES} glfw Vulkan::Vulkan)

if(DEBUG_BUILD) 
    # target_compile_definitions(Search PRIVATE VULKAN_DEBUG_REPORT)
endif()

# Create the 'data' directory at the same level as 'bin'
file(MAKE_DIRECTORY "${CMAKE_RUNTIME_DATA_DIRECTORY}")


install(
    TARGETS               Search 
    EXPORT                ${PACKAGE_TARGETS}
    LIBRARY DESTINATION   ${INSTALL_LIB_DIR}
    ARCHIVE DESTINATION   ${INSTALL_ARCHIVE_DIR}
    RUNTIME DESTINATION   ${INSTALL_RUNTIME_DIR}
    INCLUDES DESTINATION  ${INSTALL_INCLUDE_DIR}
)
