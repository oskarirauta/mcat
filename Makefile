all: world

CXX?=g++
CXXFLAGS?=--std=c++17 -Wall -fPIC -g
INCLUDES?=-I./include

OBJS:= \
	objs/util.o \
	objs/detect.o \
	objs/render.o \
	objs/highlight_json.o \
	objs/highlight_config.o \
	objs/highlight_markdown.o \
	objs/highlight_cpp.o \
	objs/highlight_shell.o \
	objs/highlight_diff.o \
	objs/pager.o \
	objs/main.o

USAGECPP_DIR:=usage
JSON_DIR:=json
include $(USAGECPP_DIR)/Makefile.inc
include $(JSON_DIR)/Makefile.inc

world: mcat

$(shell mkdir -p objs)

objs/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<;

mcat: $(USAGE_OBJS) $(JSON_OBJS) $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@;

.PHONY: clean
clean:
	@rm -rf objs mcat
