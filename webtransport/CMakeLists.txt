cmake_minimum_required(VERSION 3.12)

add_subdirectory(go)

set(TARGET webtransport)
set(SRCS web.cpp)

SOURCE_GROUP(Files FILES
    web.h
)

INCLUDE_DIRECTORIES(Files)

ADD_LIBRARY(${TARGET} ${SRCS})

install(TARGETS webtransport RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
target_link_libraries(${TARGET} common webtransport_go pthread)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)