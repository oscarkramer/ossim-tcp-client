cmake_minimum_required(VERSION 2.8)

get_filename_component(APP_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" APP_NAME ${APP_NAME})

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project(${APP_NAME})

if(NOT MSVC)
    add_definitions( -std=c++11 )
endif(NOT MSVC)

############################################################
# Create a library
############################################################

set(tcp_library_dir ../src/tcpclient ../src/ossim)

# Source files to be used in the library
message("SOURCE_FILES = ${SOURCE_FILES}")

#Generate the static library from the library sources
set( OSSIM_LIBRARIES /home/okramer/dev/ossim/ossimlabs/build/lib/libossim.so )

############################################################
# Create an executable
############################################################

# Add an executable with the above sources
add_executable(${APP_NAME} main.cpp)

# link the new tcpclient target with the app target
if(WIN32)
    target_link_libraries(${APP_NAME} PRIVATE ossim-tcp-client ws2_32)
else()
    add_custom_command(TARGET ${APP_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${APP_NAME} test_app
        WORKING_DIRECTORY ${TARGET_DIR}
    )
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        # require at least gcc 4.8
        if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
	    message(FATAL_ERROR "GCC version must be at least 4.8!")
        endif()
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        # require at least clang 3.2
        if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.2)
    	    message(FATAL_ERROR "Clang version must be at least 3.2!")
        endif()
    else()
        message(WARNING "You are using an unsupported compiler! Compilation has only been tested with Clang and GCC.")
    endif()
    find_package(Threads)
    target_link_libraries(${APP_NAME} PRIVATE tcpclient pthread ${OSSIM_LIBRARIES})
endif()
