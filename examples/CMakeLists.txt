set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_library(imtui-examples-common ${IMTUI_LIBRARY_TYPE}
    imtui-demo.cpp
    )

target_link_libraries(imtui-examples-common PRIVATE
    imtui
    )

if (EMSCRIPTEN)
    add_subdirectory(emscripten0)
    add_subdirectory(hnterm)
    add_subdirectory(slack)
    return()
endif()

if (IMTUI_SUPPORT_NCURSES)
    add_subdirectory(ncurses0)
    add_subdirectory(slack)

    if (IMTUI_SUPPORT_CURL)
        if (IMTUI_SUPPORT_CURL)
            find_package(CURL REQUIRED)
            if (MINGW)
                set(CURL_LIBRARIES curl)
            endif()
        endif()

        add_subdirectory(hnterm)
    endif()
endif()
