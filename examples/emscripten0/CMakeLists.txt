set(TARGET imtui-example-emscripten0)

add_executable(${TARGET}
    main.cpp
    )

target_include_directories(${TARGET} PRIVATE
    ..
    )

target_link_libraries(${TARGET} PRIVATE
    imtui
    imtui-examples-common
    imtui-emscripten
    )

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/index-tmpl.html ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET}/index.html @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/style.css ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET}/style.css @ONLY)
