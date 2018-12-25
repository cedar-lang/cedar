CXX = clang++
WARNINGS = -Wall -Wformat -Wno-unused-command-line-argument -Wno-deprecated-declarations -Wno-unused
CFLAGS = -I../include -g -O3
CXXLDLIBS =  -std=c++17 -O3 -pthread -lffi -lc -lgc

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
	SHAREDARGS = -dynamiclib -flat_namespace
endif
ifeq ($(yUNAME), Linux)
	SHAREDARGS = -fPIC -shared
	CXXLDLIBS += -ldl
endif


exe = test
objs = $(srcs:.cc=.o)
includes = $(wildcard include/*)

SRCDIR = .
OBJDIR = build

SOURCEFILES := $(addsuffix /*,$(shell find $(SRCDIR) -type d))
CODEFILES := $(wildcard $(SOURCEFILES))

CXXSRCFILES := $(filter %.cc,$(CODEFILES))
CXXOBJFILES := $(CXXSRCFILES:%.cc=%.o)


.PHONY: all clean gen lib install

default:
	@$(MAKE) -j $(shell getconf _NPROCESSORS_ONLN) all

compile: $(exe)

all: $(exe) compile

%.o: $(addprefix $(SRCDIR)/,%.cc) ${includes}
	@printf " CXX\t$@\n"
	@mkdir -p $(dir $@)
	@$(CXX) $(WARNINGS) $(CFLAGS) -c $< -o $@

$(exe): $(CXXOBJFILES) main.cc
	@printf " LD\t$@\n"
	@$(CXX) $(CXXLDLIBS) $(WARNINGS) -g -L../build -lcedar $(CXXOBJFILES) -o $@
