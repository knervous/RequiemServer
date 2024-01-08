cmake_minimum_required(VERSION 3.0)

set(TARGET wt_go)
set(SRCS web.go bridge.go cert.go constants.go client_interface.go server_interface.go)
if (WIN32)
  set(LIB web_go.dll)
else(WIN32)
  set(LIB web_go.so)
endif(WIN32)
set(DEPS ${SRCS} ../../common/patches/web_structs.h EQMessage.proto)

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${LIB}
  DEPENDS ${DEPS}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND protoc ./EQMessage.proto --go_out=. --go_opt=paths=source_relative
  COMMAND go build -gcflags "all=-N -l" -buildmode=c-shared
  -o "${PROJECT_BINARY_DIR}/bin/${LIB}"
  ${CMAKE_GO_FLAGS} ${SRCS} EQMessage.pb.go
  COMMENT "Building Go library and output to ${PROJECT_BINARY_DIR}/bin/${LIB}")

add_custom_target(${TARGET} DEPENDS ${LIB} ${HEADER})