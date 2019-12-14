set(TARGET hnterm)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${TARGET})

if (EMSCRIPTEN)
    add_executable(${TARGET}
        main.cpp
        impl-emscripten.cpp
        )

    target_include_directories(${TARGET} PRIVATE
        ..
        )

    target_link_libraries(${TARGET} PRIVATE
        imtui-core
        imtui-examples-common
        imtui-emscripten
        )

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/index-tmpl.html ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/index.html @ONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/style.css ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/style.css @ONLY)
else()
    add_executable(${TARGET}
        main.cpp
        impl-ncurses.cpp
        )

    target_include_directories(${TARGET} PRIVATE
        ..
        ${CURL_INCLUDE_DIR}
        )

    target_link_libraries(${TARGET} PRIVATE
        imtui-ncurses
        imtui-examples-common
        ${CURL_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        )
endif()

