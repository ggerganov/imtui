set(TARGET hnterm)

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

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/index-tmpl.html ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET}/index.html @ONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/style.css ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET}/style.css @ONLY)
else()
    if (APPLE AND NOT OPENSSL_ROOT_DIR)
        # usual OpenSSL location on Mac OS
        set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
    endif()

    find_package(OpenSSL REQUIRED)

    add_executable(${TARGET}
        main.cpp
        impl-ncurses.cpp
        )

    target_include_directories(${TARGET} PRIVATE
        ..
        ${CURL_INCLUDE_DIR}
        ${OPENSSL_INCLUDE_DIR}
        )

    target_link_libraries(${TARGET} PRIVATE
        imtui-ncurses
        imtui-examples-common
        ${CURL_LIBRARIES}
        )
endif()

