add_executable (stream-image main.cpp ${SOURCE_FILES} )
set( OSSIM_LIBRARIES /home/okramer/dev/ossim/ossimlabs/build/lib/libossim.so )
target_link_libraries(stream-image ${OSSIM_LIBRARIES} ${tcpclient})
