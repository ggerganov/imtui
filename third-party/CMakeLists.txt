if (IMTUI_ALL_WARNINGS_3RD_PARTY)
    if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    else()
        # todo : windows
    endif()
endif()

if (APPLE)
    set(ADDITIONAL_LIBRARIES "-framework Cocoa")
else (APPLE)
    unset(ADDITIONAL_LIBRARIES)
endif (APPLE)

add_library(imgui SHARED
    imgui/imgui/imgui.cpp
    imgui/imgui/imgui_draw.cpp
    imgui/imgui/imgui_demo.cpp
    imgui/imgui/imgui_widgets.cpp
    )

target_include_directories(imgui INTERFACE
    imgui
    )

target_include_directories(imgui PRIVATE
    imgui/imgui
    )

target_link_libraries(imgui PRIVATE
    ${ADDITIONAL_LIBRARIES}
    )
