CC = clang
CXX = clang++
WARNINGS = -Wall -Wextra -Wformat -Wno-deprecated-declarations -Wno-unused -Weffc++
CFLAGS = -I./include -g
CXXLDLIBS = -std=c++17 -pthread -lffi -lc

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
	SHAREDARGS = -dynamiclib -flat_namespace
endif
ifeq ($(UNAME), Linux)
	SHAREDARGS = -fPIC -shared
	CXXLDLIBS += -ldl
	CFLAGS += -fPIC
	CC = gcc
	CXX = g++
endif

objs = $(srcs:.cc=.o)
includes = $(wildcard include/*)

exe = cedar

SRCDIR = src
OBJDIR = build

SOURCEFILES := $(addsuffix /*,$(shell find $(SRCDIR) -type d))
CODEFILES := $(wildcard $(SOURCEFILES))

CXXSRCFILES := $(filter %.cc,$(CODEFILES))
CXXOBJFILES := $(subst $(SRCDIR),$(OBJDIR)/cc,$(CXXSRCFILES:%.cc=%.o))


CSRCFILES := $(filter %.c,$(CODEFILES))
COBJFILES := $(subst $(SRCDIR),$(OBJDIR)/c,$(CSRCFILES:%.c=%.o))


.PHONY: all clean gen lib install test

default:
	@$(MAKE) -j $(shell getconf _NPROCESSORS_ONLN) all

compile: $(OBJDIR) $(exe)

all: compile

$(OBJDIR):
	echo $(SHAREDARGS)
	@mkdir -p $@

$(OBJDIR)/c/%.o: $(addprefix $(SRCDIR)/,%.c) ${includes}
	@printf " CC\t$@\n"
	@mkdir -p $(dir $@)
	@$(CC) $(WARNINGS) $(CFLAGS) -c $< -o $@


$(OBJDIR)/cc/%.o: $(addprefix $(SRCDIR)/,%.cc) ${includes}
	@printf " CXX\t$@\n"
	@mkdir -p $(dir $@)
	@$(CXX) $(WARNINGS) $(CFLAGS) -c $< -o $@

build/libcedar.so: $(CXXOBJFILES) $(COBJFILES)
	@printf " SO\t$@\n"
	@$(CXX) $(SHAREDARGS) $(CXXLDLIBS) $(WARNINGS) -o $@ $(CXXOBJFILES) $(COBJFILES)

$(exe): build/libcedar.so main.cc
	@printf " LD\t$@\n"
	@cp build/libcedar.so /usr/local/lib/libcedar.so
	@$(CXX) $(WARNINGS) $(CFLAGS) -g -c main.cc -o build/main.o
	@$(CXX) $(CXXLDLIBS) $(WARNINGS) -g -Lbuild -lcedar -o $@ build/main.o


clean:
	@rm -rf $(OBJDIR)
	@rm -rf $(exe)
	@rm -rf lib/stdbind.so*
	@rm -rf testbuild
	@rm -rf test/test
	@rm -rf test/build
	@rm -rf test/*.o

lib:
	@rm -rf /usr/local/lib/cedar
	@mkdir -p /usr/local/lib/cedar
	@cp -a lib/ /usr/local/lib/cedar
	@mkdir -p /usr/local/include/cedar
	@cp -a include/ /usr/local/include/cedar


install:
	cp -a ./include/. /usr/local/include/
	cp -a ./build/libcedar.so /usr/local/lib/
	cp -a ./cedar /usr/local/bin/


test: build/libcedar.so
	@cd test; $(MAKE)
	@test/test
