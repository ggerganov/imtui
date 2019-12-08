#
# Makefile to use with emscripten
# See https://emscripten.org/docs/getting_started/downloads.html
# for installation instructions. This Makefile assumes you have
# loaded emscripten's environment.
#
# Running `make` will produce three files:
#  - example_emscripten.html
#  - example_emscripten.js
#  - example_emscripten.wasm
#
# All three are needed to run the demo.

CC = emcc
CXX = em++

EXE = example_emscripten.html
SOURCES = example_emscripten.cpp
SOURCES += imgui_impl_text.cpp imtui_demo.cpp
SOURCES += ../third-party/imgui/imgui.cpp ../third-party/imgui/imgui/imgui_demo.cpp ../third-party/imgui/imgui/imgui_draw.cpp ../third-party/imgui/imgui/imgui_widgets.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
UNAME_S := $(shell uname -s)

EMS = -std=c++17 -s USE_SDL=2 -s WASM=1
EMS += -s ALLOW_MEMORY_GROWTH=1
EMS += -s DISABLE_EXCEPTION_CATCHING=1 -s NO_EXIT_RUNTIME=1
EMS += -s ASSERTIONS=1
EMS += -s NO_FILESYSTEM=1 -DIMGUI_DISABLE_FILE_FUNCTIONS
#EMS += -s EXPORTED_FUNCTIONS='["_main", "_render_frame"]'
EMS += -s EXTRA_EXPORTED_RUNTIME_METHODS='["cwrap"]'
# Uncomment next line to fix possible rendering bugs with emscripten version older then 1.39.0 (https://github.com/ocornut/imgui/issues/2877)
#EMS += -s BINARYEN_TRAP_MODE=clamp
#EMS += -s SAFE_HEAP=1    ## Adds overhead

CPPFLAGS = -I./ -I../third-party/imgui/ -I../third-party/imgui/imgui
#CPPFLAGS += -g
CPPFLAGS += -Wall -Wformat -O3
CPPFLAGS += $(EMS)
LIBS = $(EMS)
LDFLAGS = --shell-file shell_minimal.html

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

%.o:%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

%.o:../%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

%.o:../third-party/imgui/imgui/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

%.o:../libs/gl3w/GL/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(EXE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(LIBS) $(LDFLAGS)

clean:
	rm -f $(EXE) $(OBJS) example_emscripten.js *.wasm *.wasm.pre
