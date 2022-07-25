set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TARGET slack)

option(SLACK_ENABLE_API_CACHE "slack: enable API caching" OFF)

if (EMSCRIPTEN)
    set (CMAKE_CXX_FLAGS "-s ALLOW_MEMORY_GROWTH=1 -s FETCH=1 -s ASSERTIONS=1 -s DISABLE_EXCEPTION_CATCHING=0")

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
else()
    add_executable(${TARGET}
        main.cpp
        )

    target_include_directories(${TARGET} PRIVATE
        ..
        )

    target_link_libraries(${TARGET} PRIVATE
        imtui-ncurses
        imtui-examples-common
        )
endif()

if (SLACK_ENABLE_API_CACHE)
    target_compile_definitions(${TARGET} PRIVATE ENABLE_API_CACHE)
endif()
