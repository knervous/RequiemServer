cmake_minimum_required(VERSION 3.0)

set(TARGET wt_go)
set(SRCS web.go bridge.go cert.go)
set(LIB web_go.a)
set(DEPS ${SRCS} ../../common/patches/web_structs.h EQMessage.proto)

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${LIB}
  DEPENDS ${DEPS}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND protoc ./EQMessage.proto --go_out=. --go_opt=paths=source_relative
  COMMAND go build -buildmode=c-archive
  -o "${CMAKE_CURRENT_SOURCE_DIR}/${LIB}"
  ${CMAKE_GO_FLAGS} ${SRCS} EQMessage.pb.go
  COMMENT "Building Go library")

add_custom_target(${TARGET} DEPENDS ${LIB} ${HEADER})
add_library(webtransport_go STATIC IMPORTED GLOBAL)
add_dependencies(webtransport_go ${TARGET})
set_target_properties(webtransport_go
  PROPERTIES
  IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/${LIB}
  INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR})