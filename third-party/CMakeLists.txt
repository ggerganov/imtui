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

add_library(imgui-for-imtui STATIC
    imgui/imgui/imgui.cpp
    imgui/imgui/imgui_draw.cpp
    imgui/imgui/imgui_demo.cpp
    imgui/imgui/imgui_widgets.cpp
    imgui/imgui/imgui_tables.cpp
    )

target_include_directories(imgui-for-imtui INTERFACE
    imgui
    )

target_include_directories(imgui-for-imtui PRIVATE
    imgui/imgui
    )

target_link_libraries(imgui-for-imtui PRIVATE
    ${ADDITIONAL_LIBRARIES}
    )

set_property(TARGET imgui-for-imtui PROPERTY POSITION_INDEPENDENT_CODE ON)

set_target_properties(imgui-for-imtui PROPERTIES PUBLIC_HEADER "imgui/imgui/imgui.h;imgui/imgui/imconfig.h")

if (MINGW)
    set_target_properties(imgui-for-imtui PROPERTIES COMPILE_FLAGS -fno-threadsafe-statics)
endif()

if (IMTUI_STANDALONE AND NOT EMSCRIPTEN)
    install(TARGETS imgui-for-imtui
        EXPORT imgui-for-imtui
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/imgui-for-imtui/imgui
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/imgui-for-imtui/imgui
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
endif()
