cmake_minimum_required (VERSION 3.10)
project(imtui)

set(CMAKE_EXPORT_COMPILE_COMMANDS "on")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(IMTUI_STANDALONE ON)
    include(cmake/GitVars.cmake)
else()
    set(IMTUI_STANDALONE OFF)
endif()

if (EMSCRIPTEN)
    set (CMAKE_CXX_FLAGS "-s ALLOW_MEMORY_GROWTH=1 -s FETCH=1 -s ASSERTIONS=1 -s DISABLE_EXCEPTION_CATCHING=0")

    set(IMTUI_SUPPORT_NCURSES_DEFAULT OFF)
    set(IMTUI_SUPPORT_CURL_DEFAULT OFF)
    set(IMTUI_LIBRARY_TYPE STATIC)
else()
    set(IMTUI_SUPPORT_NCURSES_DEFAULT ON)
    set(IMTUI_SUPPORT_CURL_DEFAULT ON)
    if (BUILD_SHARED_LIBS)
        set(IMTUI_LIBRARY_TYPE SHARED)
    else ()
        set(IMTUI_LIBRARY_TYPE STATIC)
    endif()
endif()

# options

option(IMTUI_ALL_WARNINGS            "imtui: enable all compiler warnings" ${IMTUI_STANDALONE})
option(IMTUI_ALL_WARNINGS_3RD_PARTY  "imtui: enable all compiler warnings in 3rd party libs" OFF)

option(IMTUI_SANITIZE_THREAD         "imtui: enable thread sanitizer" OFF)
option(IMTUI_SANITIZE_ADDRESS        "imtui: enable address sanitizer" OFF)
option(IMTUI_SANITIZE_UNDEFINED      "imtui: enable undefined sanitizer" OFF)

option(IMTUI_SUPPORT_NCURSES         "imtui: support for libncurses" ${IMTUI_SUPPORT_NCURSES_DEFAULT})
option(IMTUI_SUPPORT_CURL            "imtui: support for libcurl" ${IMTUI_SUPPORT_CURL_DEFAULT})

option(IMTUI_BUILD_EXAMPLES          "imtui: build examples" ${IMTUI_STANDALONE})

# sanitizers

if (IMTUI_SANITIZE_THREAD)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
endif()

if (IMTUI_SANITIZE_ADDRESS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
endif()

if (IMTUI_SANITIZE_UNDEFINED)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
endif()

# dependencies

if (IMTUI_SUPPORT_NCURSES)
    find_package(Curses REQUIRED)
endif()

if (IMTUI_SUPPORT_CURL)
    find_package(CURL REQUIRED)
endif()

# main

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
endif ()

add_subdirectory(third-party)

if (IMTUI_ALL_WARNINGS)
    if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    else()
        # todo : windows
    endif()
endif()

add_subdirectory(src)

if (IMTUI_STANDALONE AND IMTUI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
