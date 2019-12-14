set(TARGET imtui-example-emscripten0)

add_executable(${TARGET}
    main.cpp
    )

target_include_directories(${TARGET} PRIVATE
    ..
    )

target_link_libraries(${TARGET} PRIVATE
    imtui-core
    imtui-examples-common
    imtui-emscripten
    )

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/index-tmpl.html ${CMAKE_BINARY_DIR}/bin/${TARGET}/index.html @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/style.css ${CMAKE_BINARY_DIR}/bin/${TARGET}/style.css @ONLY)
