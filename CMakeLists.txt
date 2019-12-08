cmake_minimum_required (VERSION 3.10)
project(imtui)

set(CMAKE_EXPORT_COMPILE_COMMANDS "on")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(IMTUI_STANDALONE ON)
else()
    set(IMTUI_STANDALONE OFF)
endif()

# options

option(IMTUI_ALL_WARNINGS            "imtui: enable all compiler warnings" ${IMTUI_STANDALONE})
option(IMTUI_ALL_WARNINGS_3RD_PARTY  "imtui: enable all compiler warnings in 3rd party libs" OFF)

option(IMTUI_SANITIZE_THREAD         "imtui: enable thread sanitizer" OFF)
option(IMTUI_SANITIZE_ADDRESS        "imtui: enable address sanitizer" OFF)
option(IMTUI_SANITIZE_UNDEFINED      "imtui: enable undefined sanitizer" OFF)

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

# main

find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
endif ()

add_subdirectory(third-party)
add_subdirectory(src)

if (IMTUI_STANDALONE)
endif()
