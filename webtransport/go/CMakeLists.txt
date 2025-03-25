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

# Ensure protoc-gen-go is executable
execute_process(
    COMMAND chmod +x "${PROTOC_GEN_GO_BIN}"
    RESULT_VARIABLE CHMOD_RESULT
)
if(NOT CHMOD_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to make protoc-gen-go executable at ${PROTOC_GEN_GO_BIN}")
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

add_custom_target(${TARGET} DEPENDS ${LIB})