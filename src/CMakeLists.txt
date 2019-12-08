if (IMTUI_ALL_WARNINGS)
    if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    else()
        # todo : windows
    endif()
endif()

add_executable(test0 test0.cpp)

target_link_libraries(test0 PRIVATE
    imgui
    curses
    ${CMAKE_DL_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
    )

add_executable(example_ncurses0
    example_ncurses0.cpp
    imtui_demo.cpp
    imgui_impl_text.cpp
    imgui_impl_ncurses.cpp
    )

target_link_libraries(example_ncurses0 PRIVATE
    imgui
    curses
    ${CMAKE_DL_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
    )

add_executable(example_ncurses1
    example_ncurses1.cpp
    imgui_impl_text.cpp
    imgui_impl_ncurses.cpp
    )

target_link_libraries(example_ncurses1 PRIVATE
    imgui
    curses
    ${CMAKE_DL_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
    )
