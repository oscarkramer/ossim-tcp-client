cmake_minimum_required(VERSION 2.8)
project(ossim-tcp-client)

set(CMAKE_CXX_STANDARD 11)
set(SOURCE_FILES)

include_directories(src)
include_directories(/home/okramer/dev/ossim/ossimlabs/ossim/include)

add_subdirectory (src)

add_subdirectory (app/client-app)
add_subdirectory (app/stream-image)

