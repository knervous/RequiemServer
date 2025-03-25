cmake_minimum_required(VERSION 3.12)

# Define the local bin directory relative to the current CMakeLists.txt
set(LOCAL_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin")
file(MAKE_DIRECTORY "${LOCAL_BIN_DIR}")

# Check for curl and unzip dependencies
find_program(CURL_EXECUTABLE curl)
if(NOT CURL_EXECUTABLE)
    message(FATAL_ERROR "curl not found. Please install curl.")
endif()
find_program(UNZIP_EXECUTABLE unzip)
if(NOT UNZIP_EXECUTABLE)
    message(FATAL_ERROR "unzip not found. Please install unzip.")
endif()

# Install Go if not already in local bin/go
set(GO_INSTALL_DIR "${LOCAL_BIN_DIR}/go")
set(GO_BIN "${GO_INSTALL_DIR}/bin/go")
if(NOT EXISTS "${GO_INSTALL_DIR}" OR NOT EXISTS "${GO_BIN}")
    message(STATUS "Go not found in ${GO_INSTALL_DIR} or binary missing. Downloading Go 1.24.1...")

    if(UNIX)
        set(GO_URL "https://go.dev/dl/go1.24.1.linux-amd64.tar.gz")
        set(GO_TARBALL "${CMAKE_BINARY_DIR}/go1.24.1.linux-amd64.tar.gz")
        set(GO_TEMP_DIR "${CMAKE_BINARY_DIR}/go_temp")

        # Download Go tarball
        execute_process(
            COMMAND ${CURL_EXECUTABLE} -fL -o "${GO_TARBALL}" "${GO_URL}"
            RESULT_VARIABLE DOWNLOAD_RESULT
        )
        if(NOT DOWNLOAD_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to download Go from ${GO_URL}")
        endif()

        # Extract to temporary directory
        file(MAKE_DIRECTORY "${GO_TEMP_DIR}")
        execute_process(
            COMMAND tar -C "${GO_TEMP_DIR}" -xzf "${GO_TARBALL}"
            RESULT_VARIABLE EXTRACT_RESULT
        )
        if(NOT EXTRACT_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to extract Go tarball")
        endif()

        # Remove existing go file or directory if it exists
        if(EXISTS "${GO_INSTALL_DIR}")
            execute_process(
                COMMAND rm -rf "${GO_INSTALL_DIR}"
                RESULT_VARIABLE REMOVE_RESULT
            )
            if(NOT REMOVE_RESULT EQUAL 0)
                message(FATAL_ERROR "Failed to remove existing ${GO_INSTALL_DIR}")
            endif()
        endif()

        # Move the entire go directory to local bin/go
        execute_process(
            COMMAND mv "${GO_TEMP_DIR}/go" "${GO_INSTALL_DIR}"
            RESULT_VARIABLE MOVE_RESULT
        )
        if(NOT MOVE_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to move Go to ${GO_INSTALL_DIR}")
        endif()

        # Clean up temporary files
        file(REMOVE_RECURSE "${GO_TEMP_DIR}")
        file(REMOVE "${GO_TARBALL}")

        message(STATUS "Go 1.24.1 installed in ${GO_INSTALL_DIR}")
    else()
        message(FATAL_ERROR "Go installation only supported on UNIX")
    endif()
else()
    message(STATUS "Go already exists in ${GO_INSTALL_DIR}")
endif()

# Install protoc if not already in local bin
set(PROTOC_BIN "${LOCAL_BIN_DIR}/protoc")
if(NOT EXISTS "${PROTOC_BIN}")
    message(STATUS "protoc not found in ${LOCAL_BIN_DIR}. Downloading protoc 29.4...")

    if(UNIX)
        set(PROTOC_URL "https://github.com/protocolbuffers/protobuf/releases/download/v29.4/protoc-29.4-linux-x86_64.zip")
        set(PROTOC_ZIP "${CMAKE_BINARY_DIR}/protoc-29.4-linux-x86_64.zip")
        set(PROTOC_TEMP_DIR "${CMAKE_BINARY_DIR}/protoc_temp")

        # Download protoc zip
        execute_process(
            COMMAND ${CURL_EXECUTABLE} -fL -o "${PROTOC_ZIP}" "${PROTOC_URL}"
            RESULT_VARIABLE DOWNLOAD_RESULT
        )
        if(NOT DOWNLOAD_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to download protoc from ${PROTOC_URL}")
        endif()

        # Extract to temporary directory
        file(MAKE_DIRECTORY "${PROTOC_TEMP_DIR}")
        execute_process(
            COMMAND ${UNZIP_EXECUTABLE} -o "${PROTOC_ZIP}" -d "${PROTOC_TEMP_DIR}"
            RESULT_VARIABLE UNZIP_RESULT
        )
        if(NOT UNZIP_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to unzip protoc archive")
        endif()

        # Move the protoc binary to local bin
        execute_process(
            COMMAND mv "${PROTOC_TEMP_DIR}/bin/protoc" "${PROTOC_BIN}"
            RESULT_VARIABLE MOVE_RESULT
        )
        if(NOT MOVE_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to move protoc binary to ${LOCAL_BIN_DIR}")
        endif()

        # Clean up temporary files
        file(REMOVE_RECURSE "${PROTOC_TEMP_DIR}")
        file(REMOVE "${PROTOC_ZIP}")

        message(STATUS "protoc 29.4 binary placed in ${LOCAL_BIN_DIR}")
    else()
        message(FATAL_ERROR "protoc installation only supported on UNIX")
    endif()
else()
    message(STATUS "protoc already exists in ${LOCAL_BIN_DIR}")
endif()

# Ensure binaries are executable
execute_process(COMMAND chmod +x "${GO_BIN}")
execute_process(COMMAND chmod +x "${PROTOC_BIN}")

# Set variables for use in CMake
set(GO_EXECUTABLE "${GO_BIN}" CACHE PATH "Path to Go binary")
set(PROTOC_EXECUTABLE "${PROTOC_BIN}" CACHE PATH "Path to protoc binary")

# Rest of your script
add_subdirectory(go)

set(TARGET webtransport)
set(SRCS web.cpp)

source_group(Files FILES web.h)
include_directories(Files)

add_library(${TARGET} ${SRCS})
add_dependencies(${TARGET} wt_go)

install(TARGETS webtransport RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
target_link_libraries(${TARGET} common)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)