add_executable(hnterm main.cpp)

target_include_directories(hnterm PRIVATE
    ..
    ${CURL_INCLUDE_DIR}
    )

target_link_libraries(hnterm PRIVATE
    imtui-ncurses
    imtui-examples-common
    ${CURL_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    )
