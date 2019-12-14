set(TARGET imtui-example-emscripten0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${TARGET})

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

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/index-tmpl.html ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/index.html @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/style.css ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/style.css @ONLY)
