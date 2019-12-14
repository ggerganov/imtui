find_package(Threads REQUIRED)

add_library(imtui-examples-common ${IMTUI_LIBRARY_TYPE}
    imtui-demo.cpp
    )

target_link_libraries(imtui-examples-common PRIVATE
    imtui-core
    )

if (EMSCRIPTEN)
    add_subdirectory(emscripten0)
    return()
endif()

if (IMTUI_SUPPORT_NCURSES)
    add_subdirectory(ncurses0)

    if (IMTUI_SUPPORT_CURL)
        add_subdirectory(hnterm)
    endif()
endif()
