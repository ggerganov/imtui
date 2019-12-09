add_executable(imtui-example-ncurses0 main.cpp)
target_include_directories(imtui-example-ncurses0 PRIVATE ..)
target_link_libraries(imtui-example-ncurses0 PRIVATE imtui-ncurses imtui-examples-common)
