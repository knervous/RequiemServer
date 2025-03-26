cmake_minimum_required(VERSION 3.0)

set(TARGET wt_go)
set(SRCS web.go bridge.go cert.go constants.go client_interface.go server_interface.go)
if(WIN32)
    set(LIB web_go.dll)
else()
    set(LIB web_go.so)
endif()
set(DEPS ${SRCS} ../../common/patches/web_structs.h EQMessage.proto)

# Define paths to go and protoc binaries from the parent directory's bin folder
set(GO_BIN "${CMAKE_CURRENT_SOURCE_DIR}/../bin/go/bin/go")
set(PROTOC_BIN "${CMAKE_CURRENT_SOURCE_DIR}/../bin/protoc")
set(PROTOC_GEN_GO_BIN "${CMAKE_CURRENT_SOURCE_DIR}/../bin/protoc-gen-go")

# Install protoc-gen-go if not already in local bin
if(NOT EXISTS "${PROTOC_GEN_GO_BIN}")
    message(STATUS "protoc-gen-go not found in ${CMAKE_CURRENT_SOURCE_DIR}/../bin. Installing with Go...")
    # Set GOBIN to our bin directory to install directly there
    set(ENV{GOBIN} "${CMAKE_CURRENT_SOURCE_DIR}/../bin")
    execute_process(
        COMMAND ${GO_BIN} install google.golang.org/protobuf/cmd/protoc-gen-go@latest
        RESULT_VARIABLE INSTALL_RESULT
        OUTPUT_VARIABLE INSTALL_OUTPUT
        ERROR_VARIABLE INSTALL_ERROR
    )
    if(NOT INSTALL_RESULT EQUAL 0)
        message(STATUS "Install output: ${INSTALL_OUTPUT}")
        message(STATUS "Install error: ${INSTALL_ERROR}")
        message(FATAL_ERROR "Failed to install protoc-gen-go. Ensure internet access and Go module support.")
    endif()

    if(NOT EXISTS "${PROTOC_GEN_GO_BIN}")
        message(FATAL_ERROR "protoc-gen-go not found at ${PROTOC_GEN_GO_BIN} after installation. Check Go environment.")
    endif()
    message(STATUS "protoc-gen-go installed to ${CMAKE_CURRENT_SOURCE_DIR}/../bin")
endif()

# Ensure protoc-gen-go is executable (Unix only)
if(NOT WIN32)
    execute_process(
        COMMAND chmod +x "${PROTOC_GEN_GO_BIN}"
        RESULT_VARIABLE CHMOD_RESULT
    )
    if(NOT CHMOD_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to make protoc-gen-go executable at ${PROTOC_GEN_GO_BIN}")
    endif()
endif()

# Define source and target paths for the library
set(SOURCE_SO "${PROJECT_BINARY_DIR}/bin/${LIB}")
set(TARGET_SO "/home/eqemu/server/bin/${LIB}")
if(WIN32)
    string(REPLACE "/" "\\" SOURCE_SO "${SOURCE_SO}") # Convert to Windows-style paths
    set(TARGET_SO "C:\\eqemu\\server\\bin\\${LIB}")   # Example Windows path; adjust as needed
endif()

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${LIB}
    DEPENDS ${DEPS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${PROTOC_BIN} --plugin=protoc-gen-go=${PROTOC_GEN_GO_BIN} ./EQMessage.proto --go_out=. --go_opt=paths=source_relative
    COMMAND ${GO_BIN} build -gcflags "all=-N -l" -buildmode=c-shared
            -o "${PROJECT_BINARY_DIR}/bin/${LIB}"
            ${CMAKE_GO_FLAGS} ${SRCS} EQMessage.pb.go
    COMMENT "Building Go library and output to ${PROJECT_BINARY_DIR}/bin/${LIB}"
)

# Add custom target for building the library
add_custom_target(${TARGET} DEPENDS ${LIB})

# Set the executable output path
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

# Add custom command to handle the symlink or copy based on platform
if(WIN32)
    # On Windows, copy the file
    add_custom_command(
        TARGET ${TARGET}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying ${SOURCE_SO} to ${TARGET_SO}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SOURCE_SO}" "${TARGET_SO}"
        COMMENT "Copying ${LIB} to target directory on Windows"
    )
else()
    # On Unix, use CMake's file(CREATE_LINK) for symlink creation
    add_custom_command(
        TARGET ${TARGET}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Checking symlink for ${SOURCE_SO} at ${TARGET_SO}"
        COMMAND ${CMAKE_COMMAND} -E remove "${TARGET_SO}" # Remove existing file/symlink if not a symlink
        COMMAND ${CMAKE_COMMAND} -E create_symlink "${SOURCE_SO}" "${TARGET_SO}"
        COMMAND ${CMAKE_COMMAND} -E echo "Symlink created for ${SOURCE_SO} at ${TARGET_SO}"
        COMMENT "Creating or updating symlink for ${LIB} on Unix"
    )
endif()