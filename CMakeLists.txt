cmake_minimum_required(VERSION 3.29)

# Project name and language.
project(modbus_slave VERSION 0.0.1 LANGUAGES CXX)

# Access the project name.
message(STATUS "System:                                [${CMAKE_SYSTEM_NAME}]")
message(STATUS "Generator:                             [${CMAKE_GENERATOR}]")
message(STATUS "Shared libs supported:                 [${CMAKE_SHARED_LIBRARY_SUPPORTED}]")
message(STATUS "CMake Build Type:                      [${CMAKE_BUILD_TYPE}]")
message(STATUS "CMake Source Directory is:             [${CMAKE_SOURCE_DIR}]")
message(STATUS "CMake Current Source Directory is:     [${CMAKE_CURRENT_SOURCE_DIR}]")
message(STATUS "CMake Build Directory is:              [${CMAKE_CURRENT_BINARY_DIR}]")
message(STATUS "Project Source Directory is:           [${PROJECT_SOURCE_DIR}]")
message(STATUS "The current project name is:           [${PROJECT_NAME}]") 
message(STATUS "The top-level project name is:         [${CMAKE_PROJECT_NAME}]")
message(STATUS "CMAKE_RUNTIME_OUTPUT_DIRECTORY:        [${CMAKE_RUNTIME_OUTPUT_DIRECTORY}]")

# Set C++ standard.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/assets/cmake")
message(STATUS "CMAKE_MODULE_PATH:        [${CMAKE_MODULE_PATH}]")

# # Find libmodbus
# find_path(MODBUS_INCLUDE_DIR
#     NAMES modbus.h
#     PATHS D:/proj/libmodbus_git/libmodbus/src
# )
# find_library(MODBUS_LIBRARY
#     NAMES modbus
#     PATHS D:/proj/libmodbus_git/libmodbus/src/win32/x64/Release
# )
# message(STATUS "MODBUS_INCLUDE_DIR:         [${MODBUS_INCLUDE_DIR}]")
# message(STATUS "MODBUS_LIBRARY:             [${MODBUS_LIBRARY}]")
# if(NOT MODBUS_INCLUDE_DIR OR NOT MODBUS_LIBRARY)
#     message(FATAL_ERROR "libmodbus not found")

# endif()

# Manually search in Homebrew's common paths
find_path(MODBUS_INCLUDE_DIR NAMES modbus.h
          PATHS /opt/homebrew/include /opt/homebrew/include/modbus /usr/local/include/modbus)

find_library(MODBUS_LIBRARY NAMES modbus
             PATHS /opt/homebrew/lib /usr/local/lib)

 if(NOT MODBUS_INCLUDE_DIR OR NOT MODBUS_LIBRARY)
     message(FATAL_ERROR "libmodbus not found")
 endif()

 message(INFO "Modbus include Dirs: " ${MODBUS_INCLUDE_DIR})

 # Include directories
set(BUILD_SHARED_LIBS OFF)
message(STATUS "Pointer size: ${CMAKE_SIZEOF_VOID_P}")

string(TIMESTAMP BUILD_TIME "%Y-%m-%d %H:%M:%S")
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Configure a header file to pass version info to C++.
configure_file(
    "${PROJECT_SOURCE_DIR}/include/version.h.in"
    "${PROJECT_SOURCE_DIR}/include/version.h"
    @ONLY
)

set(src_files
    ${PROJECT_SOURCE_DIR}/main.cpp
    ${PROJECT_SOURCE_DIR}/include/version.h

    ${PROJECT_SOURCE_DIR}/include/modbus_config.hpp
    ${PROJECT_SOURCE_DIR}/src/modbus_config.cpp

    ${PROJECT_SOURCE_DIR}/include/modbus_factory.hpp
    ${PROJECT_SOURCE_DIR}/src/modbus_factory.cpp

    ${PROJECT_SOURCE_DIR}/include/modbus_simulator.hpp
    ${PROJECT_SOURCE_DIR}/src/modbus_simulator.cpp
)

message(STATUS "List [Source Files]: ${src_files}")

# Add executable.
add_executable(${PROJECT_NAME} ${src_files})

# Add include directories.
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${MODBUS_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/../../deps/
)

# Link Lib Modbus libraries
target_link_directories(${PROJECT_NAME} PRIVATE

)
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${MODBUS_LIBRARY}
)

# Install the executable and its runtime dependencies automatically
# include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    RUNTIME DEPENDENCIES # Automatically finds and installs associated DLLs/SOs
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "ronaktheshinigami@gmail.com")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})

# Output file name
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")

# Generator options (choose one or more)
set(CPACK_GENERATOR "ZIP;NSIS")

# NSIS-specific (installer)
set(CPACK_NSIS_DISPLAY_NAME "${PROJECT_NAME}")
set(CPACK_NSIS_PACKAGE_NAME "${PROJECT_NAME} Installer")
set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")

include(CPack)

