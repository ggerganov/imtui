if (IMTUI_ALL_WARNINGS)
    if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    else()
        # todo : windows
    endif()
endif()

# core

add_library(imtui-core ${IMTUI_LIBRARY_TYPE}
    imtui-impl-text.cpp
    )

target_include_directories(imtui-core PUBLIC
    ../include
    )

target_link_libraries(imtui-core PUBLIC
    imgui
    )

target_link_libraries(imtui-core PRIVATE
    ${CMAKE_DL_LIBS}
    )

# ncurses

if (IMTUI_SUPPORT_NCURSES)
    add_library(imtui-ncurses ${IMTUI_LIBRARY_TYPE}
        imtui-impl-ncurses.cpp
        )

    target_include_directories(imtui-core PUBLIC
        ${CURSES_INCLUDE_DIR}
        )

    target_link_libraries(imtui-ncurses PUBLIC
        imtui-core
        ${CURSES_LIBRARIES}
        )
endif()

# emscripten

if (EMSCRIPTEN)
    add_library(imtui-emscripten ${IMTUI_LIBRARY_TYPE}
        imtui-impl-emscripten.cpp
        )

    target_link_libraries(imtui-emscripten PUBLIC
        imtui-core
        )
endif()
