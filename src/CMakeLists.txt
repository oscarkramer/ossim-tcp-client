cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 11)
set(SOURCE_FILES PARENT_SCOPE)

file(GLOB SOURCE_FILES PARENT_SCOPE "*.cpp" )
file(GLOB HEADER_FILES PARENT_SCOPE "*.h" "*.hxx" )

#Generate the static library from the library sources
add_library(tcpclient STATIC ${SOURCE_FILES} ${HEADER_FILES})
target_include_directories(tcpclient PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
