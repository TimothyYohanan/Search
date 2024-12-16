
########### Build External Libraries ################

##### sqlite3 #####
set(sqlite3_cmakelists "${CMAKE_SOURCE_DIR}/extern/sqlite3")              # PATH to source directory containing the CMakeLists.txt for SQLite3
set(sqlite3_build_location "${PROJECT_BINARY_DIR}/extern/sqlite3")        # PATH to build-folder sub-directory for SQLite3
file(MAKE_DIRECTORY ${sqlite3_build_location})                             # Creates the sub-directory if it doesn't already exist

execute_process(                                                            # Configure SQLite3
    COMMAND ${CMAKE_COMMAND} -S ${sqlite3_cmakelists} -B ${sqlite3_build_location} 
    -D CMAKE_INSTALL_PREFIX=${CMAKE_LIBRARY_OUTPUT_DIRECTORY} 
    -D SQLITE3_ENABLE_SHARED=OFF -D SQLITE3_ENABLE_STATIC=ON 
    WORKING_DIRECTORY ${sqlite3_build_location}
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Failed to configure sqlite3")
endif()

execute_process(                                                            # Generate SQLite3
    COMMAND make -C ${sqlite3_build_location} -j${CPU_THREADS}
    WORKING_DIRECTORY ${sqlite3_build_location}
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Failed to generate sqlite3")
endif()

execute_process(                                                            # Install SQLite3
    COMMAND make install -C ${sqlite3_build_location} -j${CPU_THREADS}
    WORKING_DIRECTORY ${sqlite3_build_location}
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Failed to install sqlite3")
endif()
##### end sqlite3 #####

##### yaml-cpp #####
set(yaml-cpp_cmakelists "${CMAKE_SOURCE_DIR}/extern/yaml-cpp")              # PATH to source directory containing CMakeLists.txt for library
set(yaml-cpp_build_location "${PROJECT_BINARY_DIR}/extern/yaml-cpp")          # PATH to build-folder sub-directory
file(MAKE_DIRECTORY ${yaml-cpp_build_location})                                      # Creates the sub-directory if it doesn't already exist

execute_process(                                                                     # Configure yaml-cpp
    COMMAND ${CMAKE_COMMAND} -S ${yaml-cpp_cmakelists} -B ${yaml-cpp_build_location} 
    -D CMAKE_INSTALL_PREFIX=${CMAKE_LIBRARY_OUTPUT_DIRECTORY} 
    -D BUILD_SHARED_LIBS=OFF
    WORKING_DIRECTORY ${yaml-cpp_build_location}
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Failed to configure yaml-cpp")
endif()

execute_process(                                                                     # Generate yaml-cpp
    COMMAND make -C ${yaml-cpp_build_location} -j${CPU_THREADS}
    WORKING_DIRECTORY ${yaml-cpp_build_location}
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Failed to generate yaml-cpp")
endif()

execute_process(                                                                     # Install yaml-cpp
    COMMAND make install -C ${yaml-cpp_build_location} -j${CPU_THREADS}
    WORKING_DIRECTORY ${yaml-cpp_build_location}
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Failed to install yaml-cpp")
endif()
##### end yaml-cpp #####

##### imgui #####
set(imgui_cmakelists "${CMAKE_SOURCE_DIR}/extern/imgui")              # PATH to source directory containing CMakeLists.txt for library
set(imgui_build_location "${PROJECT_BINARY_DIR}/extern/imgui")          # PATH to build-folder sub-directory
file(MAKE_DIRECTORY ${imgui_build_location})                                      # Creates the sub-directory if it doesn't already exist

execute_process(                                                                     # Configure imgui
    COMMAND ${CMAKE_COMMAND} -S ${imgui_cmakelists} -B ${imgui_build_location} -D CMAKE_INSTALL_PREFIX=${CMAKE_LIBRARY_OUTPUT_DIRECTORY} -D BUILD_SHARED_LIBS=OFF 
    -D IMGUI_ENABLE_ANDROID=OFF -D IMGUI_ENABLE_DIRECTX12=OFF -D IMGUI_ENABLE_DIRECTX11=OFF -D IMGUI_ENABLE_GLFW=ON -D IMGUI_ENABLE_OPENGL3=OFF 
    -D IMGUI_ENABLE_OPENGL2=OFF -D IMGUI_ENABLE_OSX=OFF -D IMGUI_ENABLE_VULKAN=ON 
    WORKING_DIRECTORY ${imgui_build_location} 
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Failed to configure imgui")
endif()

execute_process(                                                                     # Generate imgui
    COMMAND make -C ${imgui_build_location} -j${CPU_THREADS}
    WORKING_DIRECTORY ${imgui_build_location}
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Failed to generate imgui")
endif()

execute_process(                                                                     # Install imgui
    COMMAND make install -C ${imgui_build_location} -j${CPU_THREADS}
    WORKING_DIRECTORY ${imgui_build_location}
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Failed to install imgui")
endif()
##### end imgui #####

if(NOT DEBUG_BUILD AND CLEANUP_EXTERN) 
    file(REMOVE_RECURSE "${PROJECT_BINARY_DIR}/external")                                # [cleanup] remove external-build directory now that external libraries are installed
endif()

########### End Build External Libraries ################