cmake_minimum_required(VERSION 3.0)

set(TARGET wt_go)
set(SRCS web.go bridge.go)
set(LIB web_go.a)

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${LIB}
  DEPENDS ${SRCS}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND protoc ./EQMessage.proto --go_out=. --go_opt=paths=source_relative
  COMMAND go build -buildmode=c-archive
  -o "${CMAKE_CURRENT_SOURCE_DIR}/${LIB}"
  ${CMAKE_GO_FLAGS} ${SRCS} EQMessage.pb.go
  COMMAND openssl req -new -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 -x509 -nodes -days 10 -out ./certificate.pem -keyout ./certificate.key -subj '/CN=Test Certificate' -addext "subjectAltName = DNS:localhost"
  COMMAND openssl x509 -in certificate.pem | openssl dgst -sha256 -binary | openssl enc -base64 | tee hash.txt
  COMMENT "Building Go library")

add_custom_target(${TARGET} DEPENDS ${LIB} ${HEADER})

# add_custom_command(
#   TARGET ${TARGET} POST_BUILD
#   COMMAND ${CMAKE_COMMAND} -E copy
#           ${CMAKE_CURRENT_SOURCE_DIR}/certificate.key
#           ${PROJECT_BINARY_DIR}/bin/certificate.key
#   COMMAND ${CMAKE_COMMAND} -E copy
#           ${CMAKE_CURRENT_SOURCE_DIR}/certificate.pem
#           ${PROJECT_BINARY_DIR}/bin/certificate.pem        
#   COMMAND ${CMAKE_COMMAND} -E copy
#           ${CMAKE_CURRENT_SOURCE_DIR}/hash.txt
#           ${PROJECT_BINARY_DIR}/bin/hash.txt      
#           )

add_library(webtransport_go STATIC IMPORTED GLOBAL)
add_dependencies(webtransport_go ${TARGET})
set_target_properties(webtransport_go
  PROPERTIES
  IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/${LIB}
  INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR})